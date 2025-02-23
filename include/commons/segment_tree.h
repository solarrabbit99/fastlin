#pragma once

#include <vector>

namespace fastlin {

// `O(log n)` range update
// `O(log n)` point query
// `O(1)` minimum query segment tree
// minimum size 1

template <typename value_type>
struct default_segment_tree_zero {
  static constexpr value_type value{};
};

template <typename value_type>
struct default_segment_tree_updater {
  inline void operator()(value_type& a, const value_type& b) { a += b; }
};

template <typename value_type,
          typename zero_allocator = default_segment_tree_zero<value_type>,
          typename updater = default_segment_tree_updater<value_type>>
struct segment_tree {
 public:
  segment_tree(const size_t& size) : tree(size * 4), size(size) {
    build(1, 0, size - 1);
  }

  void update_range(int l, int r, value_type addend) {
    update_range(1, 0, size - 1, l, r, addend);
  }

  std::pair<value_type, int> query_min() {
    return {tree[1].min_value, tree[1].min_pos};
  }

  std::pair<value_type, int> query_min_range(int l, int r) {
    return query_min_range(1, 0, size - 1, l, r);
  }

  int query_val(int pos) { return query_val(1, 0, size - 1, pos); }

 private:
  struct segment_tree_node {
    value_type min_value;
    value_type weight;
    int min_pos;
  };

  void build(int v, int tl, int tr) {
    tree[v] = {zero_allocator::value, zero_allocator::value, tl};
    if (tl != tr) {
      int tm = (tl + tr) >> 1;
      build(v << 1, tl, tm);
      build((v << 1) + 1, tm + 1, tr);
    }
  }

  void merge(segment_tree_node& par, const segment_tree_node& a,
             const segment_tree_node& b) {
    bool take_a = (a.min_value <= b.min_value);
    par.min_value = take_a ? a.min_value : b.min_value;
    par.min_pos = take_a ? a.min_pos : b.min_pos;
  }

  void propagate(int v) {
    auto& node = tree[v];
    if (node.weight == zero_allocator::value) return;
    apply(v << 1, node.weight);
    apply((v << 1) + 1, node.weight);
    node.weight = zero_allocator::value;
  }

  void apply(int v, value_type addend) {
    updater()(tree[v].min_value, addend);
    updater()(tree[v].weight, addend);
  }

  void update_range(int v, int tl, int tr, int l, int r, value_type addend) {
    if (l > r) return;
    if (l == tl && r == tr) {
      apply(v, addend);
      return;
    }
    propagate(v);
    int tm = (tl + tr) >> 1;
    update_range(v << 1, tl, tm, l, std::min(r, tm), addend);
    update_range((v << 1) + 1, tm + 1, tr, std::max(l, tm + 1), r, addend);
    merge(tree[v], tree[v << 1], tree[(v << 1) + 1]);
  }

  int query_val(int v, int tl, int tr, int pos) {
    if (tl == tr) return tree[v].weight;
    int tm = (tl + tr) >> 1;
    return ((pos <= tm) ? query_val(v << 1, tl, tm, pos)
                        : query_val((v << 1) + 1, tm + 1, tr, pos)) +
           tree[v].weight;
  }

  std::pair<value_type, int> query_min_range(int v, int tl, int tr, int l,
                                             int r) {
    if (l > r) return {std::numeric_limits<int>::max(), -1};
    if (l == tl && r == tr) return {tree[v].min_value, tree[v].min_pos};
    propagate(v);
    int tm = (tl + tr) >> 1;
    auto left_res = query_min_range(v << 1, tl, tm, l, std::min(r, tm));
    auto right_res =
        query_min_range((v << 1) + 1, tm + 1, tr, std::max(l, tm + 1), r);
    return (left_res.first <= right_res.first) ? left_res : right_res;
  }

  // root at `1`, left child at `2*par`, right child at `2*par+1`
  // for odd size ranges, mid belongs to right child
  std::vector<segment_tree_node> tree;
  int size;
};

}  // namespace fastlin