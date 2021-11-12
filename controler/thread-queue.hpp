#pragma once

#include <mutex>
#include <queue>
#include <condition_variable>
#include <optional>
#include <chrono>

#include <stdio.h>

template <typename T>
class ThreadSafeQueue {

public:
    std::mutex mutex;
    std::condition_variable cond_var;
    std::queue<T> queue;

    void push(T&& item) {
      {
        std::lock_guard lock(mutex);
        queue.push(std::forward<T>(item));
      }
      cond_var.notify_all();
      fprintf(stderr, "Pushed one object %lu\n", queue.size());
    }

    std::optional<T> front(int utimeout) {
      std::unique_lock lock(mutex);
      if(cond_var.wait_for(lock, std::chrono::microseconds(utimeout), [&]{ return !queue.empty(); }))
        return std::make_optional<T>(queue.front());
      else
        return std::nullopt;
    }

    void pop() {
      std::lock_guard lock(mutex);
      queue.pop();
      fprintf(stderr, "Poped one object %lu\n", queue.size());
    }
};