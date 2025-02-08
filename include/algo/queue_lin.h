#pragma once

#include <algorithm>
#include <ranges>
#include <unordered_map>
#include <vector>

#include "commons/interval_tree.h"
#include "commons/segment_tree.h"
#include "fastlinutils.h"

namespace fastlin {

namespace queue {

using add_methods = method_group<Method::ENQ>;
using remove_methods = method_group<Method::DEQ>;

template <typename value_type>
inline std::unordered_set<value_type> pendingVals;
template <typename value_type>
inline std::unordered_set<value_type> ignoreVals;
template <typename value_type>
inline std::unordered_map<value_type, size_t> runningFront;
template <typename value_type>
std::unordered_map<value_type, size_t> cntByVal;

template <typename value_type>
void upgrade_val(const value_type& val) {
  if (pendingVals<value_type>.erase(val))
    ignoreVals<value_type>.insert(val);
  else
    pendingVals<value_type>.insert(val);
}

template <typename value_type>
bool limit_front(const value_type& val) {
  return runningFront<value_type>[val] + 1 == cntByVal<value_type>[val];
}

template <typename value_type>
void consume_front(const value_type& val) {
  ++runningFront<value_type>[val];
  if (limit_front(val)) upgrade_val(val);
}

template <typename value_type, typename event_iter>
bool reverse_scan_enq(event_iter& start, const event_iter& end) {
  event_iter temp = start;
  while (start != end) {
    const auto& [_, isInv, optr] = *start;
    const value_type& val = optr->value;

    if (ignoreVals<value_type>.count(val) || optr->method != Method::ENQ) {
      ++start;
      continue;
    }
    if (isInv) break;

    upgrade_val(val);
    ++start;
  }
  return temp != start;
}

template <typename value_type, typename event_iter>
bool reverse_scan_front(event_iter& start, const event_iter& end,
                        std::optional<value_type>& last) {
  event_iter temp = start;
  while (start != end) {
    const auto& [_, isInv, optr] = *start;
    const value_type& val = optr->value;

    if (ignoreVals<value_type>.count(val) || optr->method == Method::ENQ) {
      ++start;
      continue;
    }

    if (ignoreVals<value_type>.count(*last)) last.reset();

    if (isInv) {
      if (!last) last = val;
      if (last != val || limit_front(val)) break;
    } else
      consume_front(val);

    ++start;
    continue;
  }
  return temp != start;
}

template <typename value_type>
bool is_linearizable(history_t<value_type>& hist, const value_type& emptyVal) {
  if (!extend_dist_history<value_type, add_methods, remove_methods>(hist,
                                                                    emptyVal))
    return false;

  events_t<value_type> events{get_events(hist)};
  if (!tune_events<value_type, add_methods, remove_methods>(events, emptyVal) ||
      !verify_empty<value_type, add_methods, remove_methods>(events, emptyVal))
    return false;

  remove_empty(hist, events, emptyVal);

  std::sort(events.begin(), events.end());

  cntByVal<value_type>.clear();
  for (const auto& o : hist) ++cntByVal<value_type>[o.value];

  // initializations
  std::optional<value_type> lastFront;
  pendingVals<value_type>.clear();
  ignoreVals<value_type>.clear();
  runningFront<value_type>.clear();
  auto enqStart = events.rbegin();
  auto frontStart = events.rbegin();
  auto end = events.rend();

  while (reverse_scan_enq<value_type, decltype(enqStart)>(enqStart, end) ||
         reverse_scan_front<value_type, decltype(frontStart)>(frontStart, end,
                                                              lastFront));

  return enqStart == end && frontStart == end;
}

};  // namespace queue

}  // namespace fastlin