#pragma once

#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace fastlin {

typedef unsigned long long time_type;
typedef unsigned int proc_type;
typedef unsigned int id_type;

#define MIN_TIME std::numeric_limits<time_type>::lowest()

#define FASTLIN_METHOD_EXPAND(MACRO) \
  MACRO(PUSH, "push")                \
  MACRO(POP, "pop")                  \
  MACRO(PEEK, "peek")                \
  MACRO(ENQ, "enq")                  \
  MACRO(DEQ, "deq")                  \
  MACRO(PUSH_FRONT, "push_front")    \
  MACRO(POP_FRONT, "pop_front")      \
  MACRO(PEEK_FRONT, "peek_front")    \
  MACRO(PUSH_BACK, "push_back")      \
  MACRO(POP_BACK, "pop_back")        \
  MACRO(PEEK_BACK, "peek_back")      \
  MACRO(INSERT, "insert")            \
  MACRO(POLL, "poll")                \
  MACRO(CONTAINS, "contains")        \
  MACRO(REMOVE, "remove")

enum Method {
#define FASTLIN_METHOD_LIST(ENUM, STR) ENUM,
  FASTLIN_METHOD_EXPAND(FASTLIN_METHOD_LIST)
#undef FASTLIN_METHOD_LIST
};

inline Method stomethod(const std::string& str) {
#define FASTLIN_METHOD_TRANSLATE(ENUM, STR) \
  if (str == STR) return Method::ENUM;
  FASTLIN_METHOD_EXPAND(FASTLIN_METHOD_TRANSLATE)
#undef FASTLIN_METHOD_TRANSLATE
  throw std::invalid_argument("Unknown method: " + str);
}

template <Method first_method, Method... method>
struct method_group {
  // check if the argument is one of the variadic values
  static bool contains(Method value) {
    return value == first_method || ((value == method) || ...);
  }
  static constexpr Method first = first_method;
};

// As operations are assumed to be complete, return values are known and can be
// embedded within value_type if desired
template <typename value_type>
struct operation_t {
  id_type id;
  Method method;
  value_type value;
  time_type startTime;
  time_type endTime;
};

template <typename value_type>
using history_t = std::vector<operation_t<value_type>>;

template <typename value_type>
using events_t =
    std::vector<std::tuple<time_type, bool, operation_t<value_type>*>>;

}  // namespace fastlin