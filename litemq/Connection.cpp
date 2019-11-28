#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>

#include "Connection.hpp"

namespace
{
  const bool CHECK_CONCURRENCY_ = false;
  const size_t MSG_MAXIOVLEN = 64;
}

namespace LiteMQ
{
  class Connection::SendDescriptorHandler: public LiteMQ::DescriptorHandler
  {
  public:
    SendDescriptorHandler(
      const Connection::State_var& state,
      uint32_t ip_addr,
      unsigned int port);

    virtual
    ~SendDescriptorHandler() throw();

    // DescriptorHandler interface
    virtual int
    fd() const throw();

    // return false if need stop loop
    virtual unsigned long
    read() throw(Exception);

    // return false if need stop loop
    virtual unsigned long
    write() throw(Exception);

    virtual void
    stopped() throw();

    void
    wakeup() throw();

  private:
    struct MessageHeader
    {
      
    } __attribute__((__packed__));
    
  private:
    void
    success_connect_() throw();

    unsigned long
    error_();

    void
    remove_sent_messages_(
      MessageHolderArray& messages,
      ssize_t sent_size)
      throw();
    
    bool
    fill_send_messages_(MessageHolderArray& messages)
      throw();

  private:
    static const unsigned long READ_BLOCK_SIZE_ = 128 * 1024;
    static const unsigned long SHRINK_READ_SIZE_ = 64 * 1024;

    State_var state_;
    bool connect_finished_;
    int fd_;

    unsigned int header_sent_size_;
    MessageHolder_var partly_sent_message_;
    unsigned long partly_sent_size_;

    // optimization : allow to avoid buf realloc
    std::vector<iovec> write_iovec_buf_;
    struct msghdr write_msg_;
    std::vector<uint32_t> write_size_bufs_;

    std::atomic<int> concurrency_count_;
  };

  Connection::SendDescriptorHandler::SendDescriptorHandler(
    const Connection::State_var& state,
    uint32_t ip_addr,
    unsigned int port)
    : state_(state),
      connect_finished_(false),
      header_sent_size_(0),
      partly_sent_size_(0)
  {
    static const char* FUN = "Connection::SendDescriptorHandler::SendDescriptorHandler()";

    // create socket
    fd_ = ::socket(AF_INET, SOCK_STREAM, 0);

    if(fd_ < 0)
    {
      Gears::throw_errno_exception<Exception>(errno, FUN, ": socket() failed");
    }

    DescriptorHandlerPoller::set_non_blocking(fd_);

    sockaddr_in addr;
    ::memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ip_addr;
    addr.sin_port = htons(port);

    int res = ::connect(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if(res < 0 && errno != EINPROGRESS)
    {
      Gears::throw_errno_exception<Exception>(errno, FUN, ": connect() failed");
    }

    if(res == 0)
    {
      success_connect_();
      connect_finished_ = true;
    }

    //read_buf_.reserve(SHRINK_READ_SIZE_ + READ_BLOCK_SIZE_);
    write_iovec_buf_.reserve(10);
    ::memset(&write_msg_, 0, sizeof(write_msg_));
  }

  Connection::SendDescriptorHandler::~SendDescriptorHandler() throw()
  {
    ::close(fd_);
  }

  int
  Connection::SendDescriptorHandler::fd() const throw()
  {
    return fd_;
  }

  unsigned long
  Connection::SendDescriptorHandler::read() throw(Exception)
  {
    if(!connect_finished_)
    {
      success_connect_();
      connect_finished_ = true;
    }

    return DescriptorHandler::CONTINUE_HANDLE;
  }

  unsigned long
  Connection::SendDescriptorHandler::write() throw(Exception)
  {
    if(!connect_finished_)
    {
      success_connect_();
      connect_finished_ = true;
    }

    ssize_t sent_size;

    while(true)
    {
      // get portion of messages
      MessageHolderArray result_messages;
      state_->message_queue.pop(result_messages, 16000);

      if(result_messages.empty())
      {
        return static_cast<unsigned long>(START_READ_HANDLE) | STOP_WRITE_HANDLE;
      }

      if(fill_send_messages_(result_messages))
      {
        // try send
        sent_size = ::sendmsg(fd_, &write_msg_, 0);

        // push not sent messages back
        remove_sent_messages_(result_messages, sent_size);

        state_->message_queue.push_front(result_messages);

        if(sent_size <= 0)
        {
          if(errno == EAGAIN || errno == EWOULDBLOCK || sent_size == 0)
          {
            return CONTINUE_HANDLE;
          }

          // error on write
          return error_();
        }
      }
    }

    return DescriptorHandler::CONTINUE_HANDLE;
  }

  void
  Connection::SendDescriptorHandler::wakeup() throw()
  {
    rearm(false, true);
  }

  void
  Connection::SendDescriptorHandler::stopped() throw()
  {
  }

  unsigned long
  Connection::SendDescriptorHandler::error_()
  {
    return static_cast<unsigned long>(CLOSE_READ_HANDLE) | CLOSE_WRITE_HANDLE;
  }

  void
  Connection::SendDescriptorHandler::success_connect_() throw()
  {
  }

  bool
  Connection::SendDescriptorHandler::fill_send_messages_(
    MessageHolderArray& result_messages)
    throw()
  {
    // prepare sendmsg struct for send
    write_iovec_buf_.reserve(result_messages.size() + 1);
    write_iovec_buf_.resize(result_messages.size() + 1);

    write_size_bufs_.reserve(result_messages.size() + 1);
    write_size_bufs_.resize(result_messages.size() + 1);

    unsigned long size_buf_i = 0;
    unsigned long io_vec_i = 0;

    if(partly_sent_message_.get())
    {
      // for partly sent message size
      if(header_sent_size_ < 4)
      {
        write_size_bufs_[size_buf_i] = partly_sent_message_->size();

        iovec& io_vec = write_iovec_buf_[io_vec_i++];
        io_vec.iov_base = reinterpret_cast<unsigned char*>(
          &write_size_bufs_[size_buf_i]) + header_sent_size_;
        io_vec.iov_len = 4 - header_sent_size_; // header size
        ++size_buf_i;
      }

      iovec& io_vec = write_iovec_buf_[io_vec_i++];
      io_vec.iov_base = static_cast<unsigned char*>(
        const_cast<void*>(partly_sent_message_->data())) + partly_sent_size_;
      io_vec.iov_len = partly_sent_message_->size() - partly_sent_size_;
    }

    for(auto it = result_messages.begin(); it != result_messages.end(); ++it)
    {
      if((*it)->need_to_send())
      {
        {
          write_size_bufs_[size_buf_i] = partly_sent_message_->size();
          iovec& io_vec = write_iovec_buf_[io_vec_i++];
          io_vec.iov_base = &write_size_bufs_[size_buf_i];
          io_vec.iov_len = 4;
          ++size_buf_i;
        }

        iovec& io_vec = write_iovec_buf_[io_vec_i++];
        io_vec.iov_base = const_cast<void*>((*it)->data());
        io_vec.iov_len = (*it)->size();
      }
    }

    write_iovec_buf_.resize(io_vec_i);

    write_msg_.msg_iov = write_iovec_buf_.data();
    write_msg_.msg_iovlen = std::min(write_iovec_buf_.size(), MSG_MAXIOVLEN);

    return io_vec_i != 0;
  }

  void
  Connection::SendDescriptorHandler::remove_sent_messages_(
    MessageHolderArray& messages,
    ssize_t sent_size)
    throw()
  {
    unsigned long unprocessed_sent_size = sent_size;

    MessageHolderArray result_messages;
    result_messages.reserve(messages.size());

    if(partly_sent_message_.get())
    {
      if(partly_sent_message_->size() - partly_sent_size_ >= unprocessed_sent_size)
      {
        unprocessed_sent_size -= partly_sent_message_->size() - partly_sent_size_;
        partly_sent_message_.reset();
        partly_sent_size_ = 0;
      }
      else
      {
        partly_sent_size_ += unprocessed_sent_size;
        unprocessed_sent_size = 0;
      }
    }

    for(auto it = messages.begin(); it != messages.end(); ++it)
    {
      if(unprocessed_sent_size > 0)
      {
        if((*it)->size() <= unprocessed_sent_size)
        {
          unprocessed_sent_size -= (*it)->size();
        }
        else // unprocessed_sent_size > (*it)->size()
        {
          partly_sent_message_ = std::move(*it);
          partly_sent_size_ = unprocessed_sent_size;
          unprocessed_sent_size = 0;
        }
      }
      else if((*it)->need_to_send())
      {
        result_messages.emplace_back(std::move(*it));
      }
    }

    messages.swap(result_messages);
  }

  void
  Connection::success_connect_()
  {
  }

  // Connection impl
  bool
  Connection::wakeup_waiting_connection_()
  {
    SendDescriptorHandler_var handler;

    {
      Gears::Mutex::WriteGuard guard(waiting_handlers_lock_);
      if(!waiting_handlers_.empty())
      {
        handler = std::move(*waiting_handlers_.begin());
        waiting_handlers_.pop_front();
      }
    }

    if(handler)
    {
      handler->wakeup();
      return true;
    }
    else
    {
      return false;
    }
  }

  bool
  Connection::extend_connections_available_() const
  {
    return opened_connections_.load() <= static_cast<int>(max_out_sockets_);
  }

  void
  Connection::try_extend_connections_()
  {
    if(++opened_connections_ < static_cast<int>(max_out_sockets_))
    {
      // open socket
      SendDescriptorHandler_var new_handler(
        new SendDescriptorHandler(state_, ip_address_, port_));

      context_->add_handler(new_handler);

      Gears::Mutex::WriteGuard guard(waiting_handlers_lock_);
      waiting_handlers_.push_back(new_handler);
    }
    else
    {
      --opened_connections_;
    }
  }
}
