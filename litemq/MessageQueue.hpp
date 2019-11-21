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
    MessageHolder(const Gears::Time& timeout_val, uint64_t request_id_val)
      : timeout(timeout_val),
        request_id(request_id_val)
    {}

    virtual
    std::pair<const void*, unsigned long>
    capture() const = 0;

    virtual bool
    expire() = 0;

    virtual bool
    plain_size() const = 0;

    const Gears::Time timeout;
    const uint64_t request_id;
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

    // return messages
    void
    pop(MessageHolderArray& result_messages, unsigned long send_size);

    void
    clear_timeouted(const Gears::Time& time);

  protected:
    template<typename MessageType, typename TimeoutCallbackType>
    struct MessageHolderImpl: public MessageHolder
    {
      MessageHolderImpl(
        MessageType message,
        TimeoutCallbackType timeout_callback,
        const Gears::Time& timeout_val,
        uint64_t request_id_val);

      virtual
      std::pair<const void*, unsigned long>
      capture() const;

      virtual bool
      expire();

      virtual unsigned long
      plain_size();

    protected:
      MessageType message_;
      TimeoutCallbackType timeout_callback_;
      mutable std::atomic<int> captured_;
      volatile unsigned long plain_size_;
    };

  protected:
    const unsigned long max_size_;

    Gears::Mutex lock_;
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
  // MessageQueue::MessageHolderImpl impl
  template<typename MessageType, typename TimeoutCallbackType>
  MessageQueue::
  MessageHolderImpl<MessageType, TimeoutCallbackType>::MessageHolderImpl(
    MessageType message,
    TimeoutCallbackType timeout_callback,
    const Gears::Time& timeout_val,
    uint64_t request_id_val)
    : MessageHolder(timeout_val, request_id_val),
      message_(std::move(message)),
      timeout_callback_(timeout_callback),
      captured_(0),
      plain_size_(4 + 8 + message.size())
  {}

  template<typename MessageType, typename TimeoutCallbackType>
  std::pair<const void*, unsigned long>
  MessageQueue::
  MessageHolderImpl<MessageType, TimeoutCallbackType>::capture() const
  {
    if(++captured_ == 1)
    {
      return std::make_pair(message_.data(), message_.size());
    }

    return std::make_pair(static_cast<const void*>(0), 0ul);
  }

  template<typename MessageType, typename TimeoutCallbackType>
  bool
  MessageQueue::
  MessageHolderImpl<MessageType, TimeoutCallbackType>::expire()
  {
    if(++captured_ == 1)
    {
      timeout_callback_();
      message_ = MessageType();
      return true;
    }

    return false;
  }

  template<typename MessageType, typename TimeoutCallbackType>
  unsigned long
  MessageQueue::
  MessageHolderImpl<MessageType, TimeoutCallbackType>::plain_size()
  {
    if(++captured_ == 1)
    {
      timeout_callback_();
      message_ = MessageType();
      return true;
    }

    return false;
  }

  // MessageQueue impl
  MessageQueue::MessageQueue(unsigned long max_size)
    : max_size_(max_size)
  {}

  template<typename MessageType>
  bool
  MessageQueue::add(MessageType message)
  {
    if(++message_count_ < max_size_)
    {
      // generate request_id
      MessageHolder_var new_msg_holder(
        new MessageHolderImpl<MessageType, void()>(
          message, null_timeout_callback, Gears::Time::ZERO, 0));

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

    Gears::Mutex::WriteGuard lock(lock_);
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
};

#endif /*LITEMQ_MESSAGEQUEUE_HPP_*/
