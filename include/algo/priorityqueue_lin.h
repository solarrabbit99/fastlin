#pragma once

#include <map>
#include <set>
#include <unordered_set>
#include <vector>

#include "fastlinutils.h"

namespace fastlin {

namespace priorityqueue {

using add_methods = method_group<Method::INSERT>;
using remove_methods = method_group<Method::POLL>;

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

  std::map<value_type, std::unordered_set<id_type>> runningOp;
  std::set<value_type> critVal;
  std::unordered_set<value_type> ignoreVal;

  for (const auto& [_, isInv, optr] : events) {
    const value_type& val = optr->value;

    if (isInv) {
      if (optr->method != Method::INSERT) runningOp[val].emplace(optr->id);

      if (optr->method == Method::POLL) {
        critVal.erase(val);
        ignoreVal.insert(val);
      }
    } else {
      if (runningOp[val].count(optr->id)) return false;

      if (optr->method == Method::INSERT && !ignoreVal.count(val))
        critVal.insert(val);
    }

    if (critVal.empty())
      runningOp.clear();
    else {
      const value_type& maxPriorityVal = *critVal.rbegin();
      auto runningIter = runningOp.rbegin();
      while (runningIter != runningOp.rend()) {
        if (runningIter->first < maxPriorityVal) break;
        std::advance(runningIter, 1);
        runningOp.erase(runningIter.base());
      }
    }
  }

  return true;
}

};  // namespace priorityqueue

}  // namespace fastlin