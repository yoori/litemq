#ifndef LITEMQ_MESSAGEQUEUE_HPP_
#define LITEMQ_MESSAGEQUEUE_HPP_

#if __GNUC__ > 4 || \
  (__GNUC__ == 4 && (__GNUC_MINOR__ > 4))
#  include <atomic>
#else
#  include <cstdatomic>
#endif

#include <memory>
#include <deque>
#include <vector>
#include <map>
#include <gears/Time.hpp>
#include <gears/Lock.hpp>

namespace LiteMQ
{
  void null_timeout_callback()
  {}

  // MessageHolder
  struct MessageHolder
  {
    MessageHolder(
      const Gears::Time& push_time_val,
      const Gears::Time& expire_time_val,
      uint64_t request_id_val)
      : push_time(push_time_val),
        expire_time(expire_time_val),
        request_id(request_id_val),
        expired_(0)
    {}

    virtual const void*
    data() const = 0;

    virtual unsigned long
    size() const = 0;

    virtual unsigned long
    plain_size() = 0;

    virtual bool
    expire() = 0;

    bool
    need_to_send() const;

    const Gears::Time push_time;
    const Gears::Time expire_time;
    const uint64_t request_id;

  protected:
    mutable std::atomic<int> expired_;
  };

  typedef std::shared_ptr<MessageHolder> MessageHolder_var;

  typedef std::vector<MessageHolder_var> MessageHolderArray;

  // MessageQueue
  class MessageQueue
  {
  public:
    MessageQueue(unsigned long max_size);

    template<typename MessageType>
    bool
    add(MessageType message);

    template<
      typename MessageType,
      typename TimeoutCallbackType>
    bool
    add(
      MessageType message,
      TimeoutCallbackType timeout_callback,
      const Gears::Time& timeout,
      unsigned long request_id);

    void
    push_front(const MessageHolderArray& messages);

    // return messages
    void
    pop(MessageHolderArray& result_messages, unsigned long send_size);

    void
    clear_timeouted(const Gears::Time& time);

    Gears::Time
    eldest_time() const;

  protected:
    template<typename MessageType, typename TimeoutCallbackType>
    struct MessageHolderImpl: public MessageHolder
    {
      MessageHolderImpl(
        MessageType message,
        TimeoutCallbackType timeout_callback,
        const Gears::Time& timeout_val,
        uint64_t request_id_val);

      virtual const void*
      data() const ;

      virtual unsigned long
      size() const;

      virtual bool
      expire();

      virtual unsigned long
      plain_size();

    protected:
      MessageType message_;
      TimeoutCallbackType timeout_callback_;
    };

  protected:
    const unsigned long max_size_;

    mutable Gears::Mutex lock_;
    // queue in push order
    std::deque<MessageHolder_var> messages_;
    // messages with timeout order for clear timeouted
    std::multimap<Gears::Time, MessageHolder_var> timed_messages_;

    std::atomic<int> message_count_;

    //std::unordered_map<std::string, RecvCallbackHolder_var> recv_callbacks_;
  };

}

namespace LiteMQ
{
  inline
  bool
  MessageHolder::need_to_send() const
  {
    return expired_.load();
  }

  // MessageQueue::MessageHolderImpl impl
  template<typename MessageType, typename TimeoutCallbackType>
  MessageQueue::
  MessageHolderImpl<MessageType, TimeoutCallbackType>::MessageHolderImpl(
    MessageType message,
    TimeoutCallbackType timeout_callback,
    const Gears::Time& expire_time_val,
    uint64_t request_id_val)
    : MessageHolder(Gears::Time::get_time_of_day(), expire_time_val, request_id_val),
      message_(std::move(message)),
      timeout_callback_(timeout_callback)
  {}

  template<typename MessageType, typename TimeoutCallbackType>
  const void*
  MessageQueue::
  MessageHolderImpl<MessageType, TimeoutCallbackType>::data() const
  {
    return message_.data();
  }

  template<typename MessageType, typename TimeoutCallbackType>
  unsigned long
  MessageQueue::
  MessageHolderImpl<MessageType, TimeoutCallbackType>::size() const
  {
    return message_.size();
  }

  template<typename MessageType, typename TimeoutCallbackType>
  bool
  MessageQueue::
  MessageHolderImpl<MessageType, TimeoutCallbackType>::expire()
  {
    if(++expired_ == 1)
    {
      timeout_callback_();
      return true;
    }

    return false;
  }

  template<typename MessageType, typename TimeoutCallbackType>
  unsigned long
  MessageQueue::
  MessageHolderImpl<MessageType, TimeoutCallbackType>::plain_size()
  {
    return size() + 4;
  }

  // MessageQueue impl
  MessageQueue::MessageQueue(unsigned long max_size)
    : max_size_(max_size)
  {}

  template<typename MessageType>
  bool
  MessageQueue::add(MessageType message)
  {
    if(++message_count_ < static_cast<int>(max_size_))
    {
      // generate request_id
      MessageHolder_var new_msg_holder(
        new MessageHolderImpl<MessageType, void()>(
          message, &null_timeout_callback, Gears::Time::ZERO, 0));

      {
        Gears::Mutex::WriteGuard lock(lock_);
        messages_.push_back(new_msg_holder);
      }

      return true;
    }
    else
    {
      --message_count_;
      return false;
    }
  }

  template<
    typename MessageType,
    typename TimeoutCallbackType>
  bool
  MessageQueue::add(
    MessageType message,
    TimeoutCallbackType timeout_callback,
    const Gears::Time& timeout,
    unsigned long request_id)
  {
    if(++message_count_ < max_size_)
    {
      // generate request_id
      MessageHolder_var new_msg_holder(
        new MessageHolderImpl<MessageType, TimeoutCallbackType>(
          message, timeout, request_id));

      {
        Gears::Mutex::WriteGuard lock(lock_);
        messages_.push_back(new_msg_holder);
        timed_messages_.insert(std::make_pair(timeout, new_msg_holder));
      }

      return true;
    }
    else
    {
      --message_count_;
      return false;
    }
  }

  void
  MessageQueue::push_front(const MessageHolderArray& messages)
  {
    message_count_ += messages.size();
    MessageHolderArray erase_messages;

    if(message_count_ > static_cast<int>(max_size_))
    {
      erase_messages.reserve(messages.size());
    }

    {
      Gears::Mutex::WriteGuard lock(lock_);
      messages_.insert(messages_.begin(), messages.begin(), messages.end());
      if(messages_.size() > max_size_)
      {
        for(auto it = messages_.end() - (messages_.size() - max_size_);
          it != messages_.end(); ++it)
        {
          erase_messages.emplace_back(std::move(*it));
        }
        
        messages_.erase(
          messages_.end() - (messages_.size() - max_size_),
          messages_.end());
      }
    }

    for(auto it = erase_messages.begin(); it != erase_messages.end(); ++it)
    {
      (*it)->expire();
    }
  }

  void
  MessageQueue::clear_timeouted(const Gears::Time& time)
  {
    std::vector<MessageHolder_var> expire_messages;
    expire_messages.reserve(max_size_);

    {
      Gears::Mutex::WriteGuard lock(lock_);
      auto end_it = timed_messages_.upper_bound(time);
      for(auto it = timed_messages_.begin(); it != end_it; ++it)
      {
        expire_messages.emplace_back(std::move(it->second));
      }
      timed_messages_.erase(timed_messages_.begin(), end_it);
    }

    for(auto it = expire_messages.begin(); it != expire_messages.end(); ++it)
    {
      if((*it)->expire())
      {
        --message_count_;
      }
    }
  }

  void
  MessageQueue::pop(
    MessageHolderArray& result_messages,
    unsigned long send_size)
  {
    // result_messages.reserve();
    unsigned long cur_size = 0;

    Gears::Mutex::WriteGuard guard(lock_);
    auto it = messages_.begin();
    for(; it != messages_.end(); ++it)
    {
      cur_size += (*it)->plain_size();
      result_messages.emplace_back(std::move(*it));
      if(cur_size >= send_size)
      {
        break;
      }
    }

    messages_.erase(messages_.begin(), ++it);
  }

  Gears::Time
  MessageQueue::eldest_time() const
  {
    Gears::Mutex::WriteGuard guard(lock_);

    if(messages_.empty())
    {
      return Gears::Time::ZERO;
    }
    else
    {
      return (*messages_.begin())->push_time;
    }
  }
};

#endif /*LITEMQ_MESSAGEQUEUE_HPP_*/
