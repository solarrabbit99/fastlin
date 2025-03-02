#pragma once

#include <unordered_map>
#include <unordered_set>

#include "commons/queue_set.h"
#include "definitions.h"

namespace fastlin {

/**
 * - checks for duplicated adds/removes of the same value
 * - extends history using first remove method
 * - O(n)
 */
template <typename value_type, typename add_group, typename remove_group>
bool extend_dist_history(history_t<value_type>& hist,
                         const value_type& emptyVal) {
  time_type maxTime = MIN_TIME;
  id_type maxId = 0;
  std::unordered_map<value_type, std::pair<int, int>> hasAddRemove;

  for (const auto& o : hist) {
    maxId = std::max(maxId, o.id);
    if (o.value == emptyVal) continue;

    auto [map_iter, _] = hasAddRemove.try_emplace(o.value, 0, 0);
    auto& [hasAdd, hasRemove] = map_iter->second;
    if (add_group::contains(o.method) && hasAdd++) return false;
    if (remove_group::contains(o.method) && hasRemove++) return false;

    maxTime = std::max(maxTime, o.endTime);
  }

  for (const auto& [key, value] : hasAddRemove) {
    auto& [hasAdd, hasRemove] = value;
    if (!hasAdd) return false;
    if (!hasRemove)
      hist.emplace_back(++maxId, remove_group::first, key, maxTime + 1,
                        maxTime + 2);
  }

  return true;
};

/**
 * retrieves events, O(n)
 */
template <typename value_type>
events_t<value_type> get_events(history_t<value_type>& hist) {
  events_t<value_type> events;
  events.reserve(hist.size() << 1);
  for (operation_t<value_type>& o : hist) {
    events.emplace_back(o.startTime, true, &o);
    events.emplace_back(o.endTime, false, &o);
  }
  return events;
}

/**
 * tune events so that add responds first and remove invokes last, O(n log n)
 * important: resulting events might not be sorted
 */
template <typename value_type, typename add_group, typename remove_group>
bool tune_events(events_t<value_type>& events, const value_type& emptyVal) {
  std::sort(events.begin(), events.end());

  using oper_ptr = operation_t<value_type>*;
  struct value_event_data {
    oper_ptr add_op = nullptr;
    oper_ptr remove_op = nullptr;
    bool add_ended = false;
    bool remove_ended = false;
    queue_set<oper_ptr> others;
  };
  std::unordered_map<value_type, value_event_data> ongoings;

  time_type time = MIN_TIME;
  for (const auto& [_, isInv, o] : events) {
    const value_type& value = o->value;
    value_event_data& data = ongoings[value];

    if (value == emptyVal) {
      time_type& ts = isInv ? o->startTime : o->endTime;
      ts = ++time;
      continue;
    }

    if (isInv) {
      o->startTime = ++time;
      if (add_group::contains(o->method)) {
        data.add_op = o;
        for (oper_ptr op : data.others) op->startTime = ++time;
        if (data.remove_op) data.remove_op->startTime = ++time;
      } else if (remove_group::contains(o->method)) {
        data.remove_op = o;
      } else {
        data.others.push(o);
        if (data.remove_op) {
          if (data.remove_ended) return false;
          data.remove_op->startTime = ++time;
        }
      }
    } else {
      if (add_group::contains(o->method)) {
        o->endTime = ++time;
        data.add_ended = true;
      } else if (remove_group::contains(o->method)) {
        if (data.add_op == NULL) return false;
        if (!data.add_ended) data.add_op->endTime = ++time;
        while (!data.others.empty()) {
          data.others.front()->endTime = ++time;
          data.others.pop();
        }
        data.remove_op->endTime = ++time;
        data.remove_ended = true;
      } else {
        if (!data.others.contains(o)) continue;
        if (data.add_op == NULL) return false;
        if (!data.add_ended) {
          data.add_op->endTime = ++time;
          data.add_ended = true;
        }
        data.others.erase(o);
        o->endTime = ++time;
      }
    }
  }

  for (auto& [t, isInv, o] : events) t = isInv ? o->startTime : o->endTime;
  return true;
}

// only works on tuned events
// empty operations can be of any method
// O(n log n)
template <typename value_type, typename add_group, typename remove_group>
bool verify_empty(events_t<value_type>& events, const value_type& emptyVal) {
  std::sort(events.begin(), events.end());

  std::unordered_set<id_type> runningEmptyOp;
  std::unordered_set<value_type> critVal, endedVal;

  for (const auto& [_, isInv, op] : events) {
    if (op->value != emptyVal) {
      if (isInv && remove_group::contains(op->method)) {
        critVal.erase(op->value);
        endedVal.insert(op->value);
      } else if (!isInv && add_group::contains(op->method) &&
                 !endedVal.count(op->value)) {
        critVal.insert(op->value);
      }
    } else {
      if (isInv)
        runningEmptyOp.insert(op->id);
      else if (runningEmptyOp.count(op->id))
        return false;
    }

    if (critVal.empty()) runningEmptyOp.clear();
  }

  return true;
}

template <typename value_type>
void remove_empty(history_t<value_type>& hist, events_t<value_type>& events,
                  const value_type& emptyVal) {
  hist.erase(std::remove_if(
                 hist.begin(), hist.end(),
                 [&emptyVal](const auto& o) { return o.value == emptyVal; }),
             hist.end());
  events = get_events(hist);  // pointers can be invalid
}

template <typename value_type>
void remove_empty(history_t<value_type>& hist, const value_type& emptyVal) {
  hist.erase(std::remove_if(
                 hist.begin(), hist.end(),
                 [&emptyVal](const auto& o) { return o.value == emptyVal; }),
             hist.end());
}

}  // namespace fastlin
