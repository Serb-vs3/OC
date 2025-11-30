#ifndef BUFFERED_CHANNEL_H_
#define BUFFERED_CHANNEL_H_

#include <mutex>
#include <condition_variable>
#include <queue>
#include <utility>

template<class T>
class BufferedChannel {
 public:
    explicit BufferedChannel(std::size_t size) : capacity_(size), closed_(false) {}

    bool Send(const T& value) {
        std::unique_lock<std::mutex> lock(m_);
        space_cv_.wait(lock, [this]() { return queue_.size() < capacity_ || closed_; });
        if (closed_) { return false; }
        queue_.push(value);
        data_cv_.notify_one();
        return true;
    }

    std::pair<T, bool> Recv() {
        std::unique_lock<std::mutex> lock(m_);
        data_cv_.wait(lock, [this]() { return !queue_.empty() || closed_; });
        if (queue_.empty()) { return std::make_pair(T{}, false); }
        T value = std::move(queue_.front());
        queue_.pop();
        space_cv_.notify_one();
        return std::make_pair(std::move(value), true);
    }

    void Close() {
        std::lock_guard<std::mutex> lock(m_);
        closed_ = true;
        data_cv_.notify_all();
        space_cv_.notify_all();
    }

 private:
    const std::size_t capacity_;
    std::queue<T> queue_;
    std::mutex m_;
    std::condition_variable data_cv_;
    std::condition_variable space_cv_;
    bool closed_;
};

#endif 
