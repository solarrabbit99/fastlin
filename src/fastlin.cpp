#include <unistd.h>

#include <chrono>
#include <iostream>
#include <memory>

#include "algo/priorityqueue_lin.h"
#include "algo/queue_lin.h"
#include "algo/stack_lin.h"
#include "history_reader.h"

using namespace fastlin;

typedef int default_value_type;
const int defaultEmptyVal = -1;

template <typename value_type>
auto get_monitor(const std::string& type, bool exclude_peeks) {
#define SUPPORT_DS(TYPE)                                       \
  if (type == #TYPE)                                           \
    return exclude_peeks ? TYPE::is_linearizable_x<value_type> \
                         : TYPE::is_linearizable<value_type>;
  SUPPORT_DS(stack);
  SUPPORT_DS(queue);
  SUPPORT_DS(priorityqueue);
#undef SUPPORT_DS
  throw std::invalid_argument("Unknown data type");
}

typedef std::chrono::high_resolution_clock hr_clock;

int main(int argc, char* argv[]) {
  bool print_time = false;
  bool exclude_peeks = false;
  std::string input_file;

  int flag;
  while ((flag = getopt(argc, argv, "tx")) != -1) switch (flag) {
      case 't':
        print_time = true;
        break;
      case 'x':
        exclude_peeks = true;
        break;
      case '?':
        std::cerr << "Unknown option `" << optopt << "'.\n";
        return 1;
      default:
        abort();
    }
  if (optind < argc) input_file = argv[optind];

  history_reader<default_value_type> reader(input_file);
  std::string histType = reader.get_type_s();
  auto monitor = get_monitor<default_value_type>(histType, exclude_peeks);
  auto hist = reader.get_hist();

  hr_clock::time_point start = hr_clock::now();
  bool result = monitor(hist, defaultEmptyVal);
  hr_clock::time_point end = hr_clock::now();
  long long time_micros =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start)
          .count();

  std::cout << result;

  if (print_time) std::cout << " " << (time_micros / 1e6);

  std::cout << std::endl;

  return 0;
}