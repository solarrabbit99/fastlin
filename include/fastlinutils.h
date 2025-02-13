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
  std::unordered_set<value_type> addvals, removevals, allvals;

  for (const auto& o : hist) {
    maxId = std::max(maxId, o.id);
    if (o.value == emptyVal) continue;

    allvals.insert(o.value);
    if (add_group::contains(o.method) && !addvals.insert(o.value).second)
      return false;
    else if (remove_group::contains(o.method) &&
             !removevals.insert(o.value).second)
      return false;

    maxTime = std::max(maxTime, o.endTime);
  }

  for (const value_type& value : allvals) {
    if (!addvals.count(value)) return false;
    if (!removevals.count(value))
      hist.emplace_back(++maxId, remove_group::first, value, maxTime + 1,
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
  std::unordered_map<value_type, oper_ptr> addongoing, rmongoing;
  std::unordered_map<value_type, queue_set<oper_ptr>> otherongoing;

  time_type time = MIN_TIME;
  for (const auto& [_, isInv, o] : events) {
    const value_type& value = o->value;

    if (value == emptyVal) {
      time_type& ts = isInv ? o->startTime : o->endTime;
      ts = ++time;
      continue;
    }

    if (isInv) {
      o->startTime = ++time;
      if (add_group::contains(o->method)) {
        addongoing.emplace(value, o);
        for (oper_ptr op : otherongoing[value]) op->startTime = ++time;
        if (auto iter = rmongoing.find(value); iter != rmongoing.end())
          iter->second->startTime = ++time;
      } else if (remove_group::contains(o->method)) {
        rmongoing.emplace(value, o);
      } else {
        otherongoing[value].push(o);
        if (auto iter = rmongoing.find(value); iter != rmongoing.end()) {
          if (!iter->second) return false;
          iter->second->startTime = ++time;
        }
      }
    } else {
      if (add_group::contains(o->method)) {
        o->endTime = ++time;
        addongoing.at(value) = NULL;
      } else if (remove_group::contains(o->method)) {
        if (!addongoing.count(value)) return false;
        if (addongoing.at(value)) addongoing.at(value)->endTime = ++time;
        queue_set<oper_ptr>& opQueue = otherongoing[value];
        while (!opQueue.empty()) {
          opQueue.front()->endTime = ++time;
          opQueue.pop();
        }
        rmongoing.at(value)->endTime = ++time;
        rmongoing.at(value) = NULL;
      } else {
        if (!otherongoing[o->value].contains(o)) continue;
        if (!addongoing.count(o->value)) return false;
        if (addongoing.at(o->value)) {
          addongoing.at(o->value)->endTime = ++time;
          addongoing.at(o->value) = NULL;
        }
        otherongoing[value].erase(o);
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

}  // namespace fastlin
