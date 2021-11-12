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
    std::queue<T> queue;

    void push(T&& item) {
      std::lock_guard lock(mutex);
      queue.push(std::forward<T>(item));
      fprintf(stderr, "Pushed one object %lu\n", queue.size());
    }

    std::optional<T> pop() {
      std::lock_guard lock(mutex);
      if (queue.empty())
        return std::nullopt;
      auto obj = std::make_optional<T>(queue.front());
      queue.pop();
      fprintf(stderr, "Poped one object %lu\n", queue.size());
      return obj;
    }
};