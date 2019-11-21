#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include <unordered_map>
#include <set>

#include <gears/StringManip.hpp>
#include <gears/OutputMemoryStream.hpp>
#include <gears/StreamLogger.hpp>
#include <gears/ActiveObjectCallback.hpp>

//#include <gears/Tokenizer.hpp>

#include <litemq/AcceptorDescriptorHandler.hpp>
#include <litemq/DescriptorHandlerPoller.hpp>
#include <litemq/MessageQueue.hpp>
#include <litemq/Connection.hpp>

namespace
{
  const bool CHECK_CONCURRENCY_ = true;
  const size_t MSG_MAXIOVLEN = 64;

  // HashAcceptorDescriptorHandler
  class HashAcceptorDescriptorHandler: public LiteMQ::AcceptorDescriptorHandler
  {
  public:
    HashAcceptorDescriptorHandler(
      unsigned long port,
      const LiteMQ::DescriptorHandlerOwner_var& poller_proxy)
      throw(Exception);

  protected:
    virtual LiteMQ::DescriptorHandler_var
    create_descriptor_handler(int fd)
      throw();
  };

  // HashConnectionDescriptorHandler
  class HashConnectionDescriptorHandler: public LiteMQ::DescriptorHandler
  {
  public:
    HashConnectionDescriptorHandler(int fd)
      throw();

    virtual
    ~HashConnectionDescriptorHandler() throw();

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

  protected:
    typedef std::vector<unsigned char> Buf;
    typedef std::list<Buf> BufList;

  protected:
    unsigned long
    error_() throw(Exception);

    void
    add_write_buf_(Buf& buf) throw();

    void
    reconfigure_write_bufs_(unsigned long sent_size)
      throw();

    void
    process_input_message_(const void* buf, unsigned long size)
      throw();

  protected:
    static const unsigned long READ_BLOCK_SIZE_ = 128 * 1024;
    static const unsigned long SHRINK_READ_SIZE_ = 64 * 1024;

    int fd_;

    unsigned long read_processed_buf_pos_;
    unsigned long read_buf_pos_;
    Buf read_buf_;

    // all next fields consistent and ready for sendmsg
    BufList write_bufs_;
    unsigned long first_write_buf_pos_; // optimization : allow to avoid buf restruct
    std::vector<iovec> write_iovec_buf_;
    struct msghdr write_msg_;

    std::atomic<int> concurrency_count_;
  };

  // HashAcceptorDescriptorHandler impl
  HashAcceptorDescriptorHandler::HashAcceptorDescriptorHandler(
    unsigned long port,
    const LiteMQ::DescriptorHandlerOwner_var& poller_proxy)
    throw(Exception)
    : AcceptorDescriptorHandler(port, poller_proxy)
  {}

  LiteMQ::DescriptorHandler_var
  HashAcceptorDescriptorHandler::create_descriptor_handler(int fd)
    throw()
  {
    return LiteMQ::DescriptorHandler_var(new HashConnectionDescriptorHandler(fd));
  }

  // HashConnectionDescriptorHandler impl
  HashConnectionDescriptorHandler::HashConnectionDescriptorHandler(int fd)
    throw()
    : fd_(fd),
      read_processed_buf_pos_(0),
      read_buf_pos_(0),
      first_write_buf_pos_(0),
      concurrency_count_(0)
  {
    read_buf_.reserve(SHRINK_READ_SIZE_ + READ_BLOCK_SIZE_);

    write_iovec_buf_.reserve(10);
    ::memset(&write_msg_, 0, sizeof(write_msg_));

    LiteMQ::DescriptorHandlerPoller::set_non_blocking(fd_);
  }

  HashConnectionDescriptorHandler::~HashConnectionDescriptorHandler()
    throw()
  {
    ::close(fd_);
  }

  int
  HashConnectionDescriptorHandler::fd() const throw()
  {
    return fd_;
  }

  unsigned long
  HashConnectionDescriptorHandler::read() throw(Exception)
  {
    ssize_t read_res;

    while(true)
    {
      // prepare buf for read READ_BLOCK_SIZE_
      if(read_buf_.size() < read_buf_pos_ + READ_BLOCK_SIZE_)
      {
        read_buf_.resize(read_buf_pos_ + READ_BLOCK_SIZE_);
      }

      // do read
      read_res = ::read(fd_, read_buf_.data() + read_buf_pos_, READ_BLOCK_SIZE_);

      if(read_res < 0)
      {
        if(errno == EAGAIN || errno == EWOULDBLOCK)
        {
          return !write_bufs_.empty() ? START_WRITE_HANDLE : STOP_WRITE_HANDLE;
        }

        return error_();
      }

      if(read_res == 0)
      {
        // connection closed
        //std::cerr << "connection closed" << std::endl;
        return static_cast<unsigned long>(STOP_READ_HANDLE) | (
          !write_bufs_.empty() ? START_WRITE_HANDLE : STOP_WRITE_HANDLE);
      }

      // some bytes got
      read_buf_pos_ += read_res;

      // check newline appearance
      unsigned char* nl_pos = std::find(
        read_buf_.data() + read_processed_buf_pos_,
        read_buf_.data() + read_buf_pos_,
        static_cast<unsigned char>('\n'));

      while(nl_pos != read_buf_.data() + read_buf_pos_)
      {
        process_input_message_(
          read_buf_.data() + read_processed_buf_pos_,
          nl_pos - (read_buf_.data() + read_processed_buf_pos_));

        read_processed_buf_pos_ = nl_pos + 1 - read_buf_.data();

        nl_pos = std::find(
          read_buf_.data() + read_processed_buf_pos_,
          read_buf_.data() + read_buf_pos_,
          static_cast<unsigned char>('\n'));
      }

      assert(read_processed_buf_pos_ <= read_buf_pos_);

      // shrink buf
      if(read_processed_buf_pos_ > SHRINK_READ_SIZE_)
      {
        auto tgt_it = read_buf_.begin();
        for(auto it = read_buf_.begin() + read_processed_buf_pos_;
          it != read_buf_.begin() + read_buf_pos_;
          ++it, ++tgt_it)
        {
          *tgt_it = *it;
        }

        read_buf_pos_ -= read_processed_buf_pos_;
        read_processed_buf_pos_ = 0;
      }
    }

    assert(false); // never reach
    return 0;
  }

  unsigned long
  HashConnectionDescriptorHandler::write() throw(Exception)
  {
    ssize_t sent_size;

    while(!write_bufs_.empty())
    {
      sent_size = ::sendmsg(fd_, &write_msg_, 0);

      if(sent_size <= 0)
      {
        if(errno == EAGAIN || errno == EWOULDBLOCK || sent_size == 0)
        {
          return CONTINUE_HANDLE;
        }

        // error on write
        return error_();
      }

      assert(sent_size > 0);

      reconfigure_write_bufs_(sent_size);
    }

    return write_bufs_.empty() ? STOP_WRITE_HANDLE : START_WRITE_HANDLE;
  }

  unsigned long
  HashConnectionDescriptorHandler::error_() throw(Exception)
  {
    return static_cast<unsigned long>(STOP_READ_HANDLE) | STOP_WRITE_HANDLE;
  }

  void
  HashConnectionDescriptorHandler::stopped() throw()
  {}

  void
  HashConnectionDescriptorHandler::add_write_buf_(Buf& buf) throw()
  {
    if(CHECK_CONCURRENCY_)
    {
      int p = concurrency_count_++;
      assert(p == 0);
    }

    write_bufs_.push_back(Buf());
    Buf& new_buf = write_bufs_.back();
    new_buf.swap(buf);

    iovec el;
    el.iov_base = new_buf.data();
    el.iov_len = new_buf.size();
    write_iovec_buf_.push_back(el);

    write_msg_.msg_iov = write_iovec_buf_.data();
    write_msg_.msg_iovlen = std::min(write_iovec_buf_.size(), MSG_MAXIOVLEN);

    if(CHECK_CONCURRENCY_)
    {
      int p = concurrency_count_--;
      assert(p == 1);
    }
  }

  void
  HashConnectionDescriptorHandler::reconfigure_write_bufs_(
    unsigned long sent_size)
    throw()
  {
    if(CHECK_CONCURRENCY_)
    {
      int p = concurrency_count_++;
      assert(p == 0);
    }

    if(sent_size > 0)
    {
      assert(!write_bufs_.empty());

      unsigned long prev_cur_size = 0;
      unsigned long cur_size = 0;
      auto sent_it = write_bufs_.begin();
      unsigned long sent_i = 0;

      cur_size += sent_it->size() - first_write_buf_pos_;

      while(cur_size <= sent_size)
      {
        ++sent_it;
        ++sent_i;

        if(sent_it == write_bufs_.end())
        {
          break;
        }

        prev_cur_size = cur_size;
        cur_size += sent_it->size();
      }

      // sent_it at pos before that we should remove elements and
      // sent_it need to shrink
      write_bufs_.erase(write_bufs_.begin(), sent_it);
      assert(sent_i <= write_iovec_buf_.size());
      write_iovec_buf_.erase(
        write_iovec_buf_.begin(), write_iovec_buf_.begin() + sent_i);

      // check last element
      if(cur_size > sent_size && prev_cur_size < sent_size)
      {
        assert(!write_bufs_.empty());
        assert(write_bufs_.size() == write_iovec_buf_.size());

        first_write_buf_pos_ = sent_size - prev_cur_size;
        Buf& buf = *write_bufs_.begin();
        iovec& io_vec = *write_iovec_buf_.begin();
        io_vec.iov_base = &buf[first_write_buf_pos_];
        io_vec.iov_len = buf.size() - first_write_buf_pos_;
      }
      else
      {
        first_write_buf_pos_ = 0;
      }

      write_msg_.msg_iov = write_iovec_buf_.data();
      write_msg_.msg_iovlen = std::min(write_iovec_buf_.size(), MSG_MAXIOVLEN);
    }

    if(CHECK_CONCURRENCY_)
    {
      int p = concurrency_count_--;
      assert(p == 1);
    }
  }

  void
  HashConnectionDescriptorHandler::process_input_message_(
    const void* buf,
    unsigned long size)
    throw()
  {
    std::vector<unsigned char> buf_vec;
    /*
    buf_vec.resize(size + 1);
    ::memcpy(&buf_vec[0], buf, size);
    buf_vec[size] = '\n';
    */
    buf_vec.resize(8, '0');
    buf_vec[7] = '\n';
    add_write_buf_(buf_vec);
  }
}

// the main function is called at program startup
int
main(int argc, char** argv)
{
  Gears::Logger_var logger(
    new Gears::OStream::Logger(
      Gears::OStream::Config(std::cerr)));

  Gears::ActiveObjectCallback_var callback(
    new Gears::ActiveObjectCallbackImpl(logger));

  LiteMQ::DescriptorHandlerPollerPool_var poller(new LiteMQ::DescriptorHandlerPollerPool(
    callback,
    4,
    16,
    Gears::Time::ONE_SECOND));

  LiteMQ::DescriptorHandler_var acceptor(new HashAcceptorDescriptorHandler(
    11111,
    poller->proxy()));

  poller->add(acceptor);

  poller->activate_object();
  poller->wait_object();

  return 0;
}
