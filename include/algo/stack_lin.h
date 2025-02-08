#pragma once

#include <algorithm>
#include <ranges>
#include <unordered_map>
#include <vector>

#include "commons/interval_tree.h"
#include "commons/segment_tree.h"
#include "fastlinutils.h"

namespace fastlin {

namespace stack {

using add_methods = method_group<Method::PUSH>;
using remove_methods = method_group<Method::POP>;

template <typename value_type>
struct stack_perm_segtree {
  typedef int pos_t;

  stack_perm_segtree(const history_t<value_type>& hist, size_t n)
      : n{n}, segTree1{2 * n - 1}, segTree2{2 * n - 1} {
    for (const operation_t<value_type>& o : hist) {
      if (o.method == POP) critIntervals[o.value].end = o.startTime;
      if (o.method == PUSH) critIntervals[o.value].start = o.endTime;
    }
    for (const auto& [value, intvl] : critIntervals) {
      segTree1.update_range(intvl.start, intvl.end - 1, 1);
      segTree2.update_range(intvl.start, intvl.end - 1, value);
    }
  }

  void remove_subhistory(const value_type& v) {
    auto& [b, e] = critIntervals[v];
    segTree1.update_range(b, e - 1, -1);
    segTree2.update_range(b, e - 1, -v);
    for (const time_type& t : waitingReturns[v]) pendingReturns.push_back(t);
  }

  std::pair<pos_t, std::optional<value_type>> get_permissive() {
    if (!pendingReturns.empty()) {
      pos_t t = pendingReturns.back();
      pendingReturns.pop_back();
      return {t, std::nullopt};
    }

    auto [layers, pos] = segTree1.query_min();
    segTree1.update_range(pos, pos, 2 * n);
    if (layers == 0) return {pos, std::nullopt};
    if (layers == 1) {
      value_type val = segTree2.query_val(pos);
      waitingReturns[val].push_back(pos);
      return {pos, val};
    }
    return {-1, std::nullopt};
  }

 private:
  std::unordered_map<value_type, std::vector<time_type>> waitingReturns;
  std::vector<time_type> pendingReturns;
  segment_tree segTree1;
  segment_tree segTree2;
  size_t n;
  std::unordered_map<value_type, interval> critIntervals;
};

template <typename value_type>
std::unordered_set<value_type> get_concurrent(events_t<value_type>& events) {
  std::sort(events.begin(), events.end());
  std::unordered_set<value_type> removableVals;
  for (const auto& [_, isInv, optr] : events) {
    if (isInv) {
      // add padding to values so that min value is 1 (for segment tree use)
      ++optr->value;
      if (optr->method == POP) removableVals.erase(optr->value);
    } else if (optr->method == PUSH)
      removableVals.insert(optr->value);
  }
  return removableVals;
}

template <typename value_type>
void remove_concurrent(history_t<value_type>& hist,
                       events_t<value_type>& events) {
  std::unordered_set<value_type> concVals = get_concurrent(events);
  hist.erase(std::remove_if(hist.begin(), hist.end(),
                            [&concVals](const auto& o) {
                              return concVals.count(o.value);
                            }),
             hist.end());
  events = get_events(hist);  // pointers can be invalid
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
  remove_concurrent(hist, events);

  interval_tree ops;
  std::unordered_map<value_type, interval_tree> opByVal;
  time_type maxTime =
      std::get<0>(*std::ranges::max_element(events.begin(), events.end()));
  std::vector<value_type> startTimeToVal(maxTime + 1);
  stack_perm_segtree<value_type> sst{hist, static_cast<size_t>(maxTime)};

  for (const auto& o : hist) {
    interval itr{static_cast<int>(o.startTime), static_cast<int>(o.endTime)};
    ops.insert(itr);
    startTimeToVal[o.startTime] = o.value;
    opByVal[o.value].insert(itr);
  }

  while (!ops.empty()) {
    auto [pos, optVal] = sst.get_permissive();
    if (pos == -1) return false;

    std::vector<interval> overlaps =
        optVal ? opByVal[*optVal].query(pos) : ops.query(pos);
    for (const interval& itr : overlaps) {
      value_type val = startTimeToVal[itr.start];
      opByVal[val].remove(itr);
      ops.remove(itr);
      if (opByVal[val].empty()) sst.remove_subhistory(val);
    }
  }

  return true;
}

};  // namespace stack

}  // namespace fastlin