#pragma once

#include "definitions.h"

namespace fastlin {

template <typename value_type>
std::ostream& operator<<(std::ostream& os, const operation_t<value_type>& op) {
  os << op.id << " " << methodtos(op.method) << " " << op.value << " "
     << op.startTime << " " << op.endTime;
  return os;
}

}  // namespace fastlin