
#pragma once
#include <condition_variable>
#include <mutex>
#include <queue>
#include <optional>

template <typename T>
class buffered_channel {
public:
    explicit buffered_channel(size_t capacity = 0) : _capacity(capacity), _closed(false) {}

    buffered_channel(const buffered_channel&) = delete;
    buffered_channel& operator=(const buffered_channel&) = delete;

    bool push(const T& item) {
        std::unique_lock<std::mutex> lock(_mtx);
        _cv_not_full.wait(lock, [this]() { return _closed || _capacity == 0 || _queue.size() < _capacity; });
        if (_closed) return false;
        _queue.push(item);
        _cv_not_empty.notify_one();
        return true;
    }

    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(_mtx);
        _cv_not_empty.wait(lock, [this]() { return _closed || !_queue.empty(); });
        if (_queue.empty())
            return std::nullopt;
        T item = _queue.front();
        _queue.pop();
        _cv_not_full.notify_one();
        return item;
    }

    void close() {
        std::lock_guard<std::mutex> lock(_mtx);
        _closed = true;
        _cv_not_empty.notify_all();
        _cv_not_full.notify_all();
    }

    bool closed() const {
        std::lock_guard<std::mutex> lock(_mtx);
        return _closed;
    }

private:
    std::mutex _mtx;
    std::condition_variable _cv_not_empty;
    std::condition_variable _cv_not_full;
    std::queue<T> _queue;
    size_t _capacity;
    bool _closed;
};
