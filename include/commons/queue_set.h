#pragma once

#include <queue>
#include <unordered_set>
#include <utility>

namespace fastlin {

// amortized O(1) insertion to queue and
// removal from anywhere from the queue
template <typename T>
struct queue_set {
 public:
  template <typename U>
  void push(U&& item) {
    q.emplace(std::forward<U>(item));
    s.emplace(std::forward<U>(item));
  }

  void pop() {
    s.erase(front());
    q.pop();
  }

  T& front() {
    while (!s.count(q.front())) q.pop();
    return q.front();
  }

  const T& get(const T& item) const { return *s.find(item); }

  void erase(const T& item) { s.erase(item); }

  bool contains(const T& item) const { return s.count(item); }

  bool empty() const { return s.empty(); }

  void swap(queue_set<T>& rhs) {
    std::swap(q, rhs.q);
    std::swap(s, rhs.s);
  }

  auto begin() { return s.begin(); }

  auto end() { return s.end(); }

 private:
  std::queue<T> q;
  std::unordered_set<T> s;
};

}  // namespace fastlin