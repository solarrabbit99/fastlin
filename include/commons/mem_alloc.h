#pragma once

#include <vector>

namespace fastlin {

// Represents static sized memory_allocator for fast alloc and free
template <typename T>
struct memory_allocator {
  memory_allocator(size_t size)
      : data{(T*)malloc(sizeof(T) * size)}, free_list{size} {
    for (int i = 0; i < size; ++i) free_list[i] = data + i;
  }

  ~memory_allocator() { std::free(data); }

  T* alloc() {
    T* top = free_list.back();
    free_list.pop_back();
    return top;
  }

  void free(T* slot) {
    slot->~T();
    free_list.push_back(slot);
  }

 private:
  T* data;
  std::vector<T*> free_list;
};

};  // namespace fastlin