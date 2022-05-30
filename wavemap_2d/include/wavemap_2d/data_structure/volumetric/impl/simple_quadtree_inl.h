#ifndef WAVEMAP_2D_DATA_STRUCTURE_VOLUMETRIC_IMPL_SIMPLE_QUADTREE_INL_H_
#define WAVEMAP_2D_DATA_STRUCTURE_VOLUMETRIC_IMPL_SIMPLE_QUADTREE_INL_H_

#include <limits>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "wavemap_2d/data_structure/volumetric/cell_types/occupancy_state.h"

namespace wavemap_2d {
template <typename CellT>
void SimpleQuadtree<CellT>::prune() {
  std::function<void(FloatingPoint, Node<CellDataSpecialized>&)> recursive_fn =
      [&recursive_fn](FloatingPoint parent_value,
                      Node<CellDataSpecialized>& node) {
        // Process the children first
        const FloatingPoint node_value = parent_value + node.data();
        if (node.hasChildrenArray()) {
          for (QuadtreeIndex::RelativeChild child_idx = 0;
               child_idx < QuadtreeIndex::kNumChildren; ++child_idx) {
            if (node.hasChild(child_idx)) {
              recursive_fn(node_value, *node.getChild(child_idx));
            } else if (kEpsilon < std::abs(node_value)) {
              // Always propagate non-zero internal node value down to leaves
              recursive_fn(node_value, *node.template allocateChild(child_idx));
            }
          }
        }

        if (node.hasChildrenArray()) {
          // Check whether the node's children are all identical leaves
          bool all_children_are_identical_leaves = true;
          const FloatingPoint first_child_value =
              node.getChild(0) ? node.getChild(0)->data() : 0.f;
          for (QuadtreeIndex::RelativeChild child_idx = 0;
               child_idx < QuadtreeIndex::kNumChildren; ++child_idx) {
            // Check whether the child has children (i.e. is not a leaf)
            const Node<CellDataSpecialized>* child = node.getChild(child_idx);
            if (child && child->hasChildrenArray()) {
              all_children_are_identical_leaves = false;
              break;
            }
            // Check whether this child's value differs from the first child
            const FloatingPoint child_value = child ? child->data() : 0.f;
            if (kEpsilon < std::abs(child_value - first_child_value)) {
              all_children_are_identical_leaves = false;
              break;
            }
          }

          if (all_children_are_identical_leaves) {
            // Prune the node's children if they're all identical leaves
            node.data() = first_child_value;
            node.deleteChildrenArray();
          } else {
            // After pruning, all internal node values are zero
            node.data() = 0.f;
          }
        } else {
          // Threshold the leaf values
          node.data() = CellT::threshold(node_value);
        }
      };

  recursive_fn(0.f, quadtree_.getRootNode());
}

template <typename CellT>
Index SimpleQuadtree<CellT>::getMinPossibleIndex() const {
  return nodeIndexToIndex(QuadtreeIndex{max_depth_, Index::Zero()});
}

template <typename CellT>
Index SimpleQuadtree<CellT>::getMaxPossibleIndex() const {
  return nodeIndexToIndex(
      QuadtreeIndex{max_depth_, Index::Constant(int_math::exp2(max_depth_))});
}

// TODO(victorr): Replace this with an implementation that only expands
//                potential min candidates
template <typename CellT>
Index SimpleQuadtree<CellT>::getMinIndex() const {
  Index min_index = Index::Constant(std::numeric_limits<IndexElement>::max());

  std::stack<std::pair<QuadtreeIndex, const Node<CellDataSpecialized>&>> stack;
  stack.template emplace(QuadtreeIndex{}, quadtree_.getRootNode());
  while (!stack.empty()) {
    const QuadtreeIndex node_index = stack.top().first;
    const Node<CellDataSpecialized>& node = stack.top().second;
    stack.pop();

    if (node.hasChildrenArray()) {
      for (QuadtreeIndex::RelativeChild child_idx = 0;
           child_idx < QuadtreeIndex::kNumChildren; ++child_idx) {
        if (node.hasChild(child_idx)) {
          const QuadtreeIndex child_node_index =
              node_index.computeChildIndex(child_idx);
          const Node<CellDataSpecialized>& child_node =
              *node.getChild(child_idx);
          stack.template emplace(child_node_index, child_node);
        }
      }
    } else if (OccupancyState::isObserved(node.data())) {
      const Index index = nodeIndexToIndex(node_index);
      min_index = min_index.cwiseMin(index);
    }
  }

  return min_index;
}

// TODO(victorr): Replace this with an implementation that only expands
//                potential max candidates
template <typename CellT>
Index SimpleQuadtree<CellT>::getMaxIndex() const {
  Index max_index =
      Index::Constant(std::numeric_limits<IndexElement>::lowest());

  std::stack<std::pair<QuadtreeIndex, const Node<CellDataSpecialized>&>> stack;
  stack.template emplace(QuadtreeIndex{}, quadtree_.getRootNode());
  while (!stack.empty()) {
    const QuadtreeIndex node_index = stack.top().first;
    const Node<CellDataSpecialized>& node = stack.top().second;
    stack.pop();

    if (node.hasChildrenArray()) {
      for (QuadtreeIndex::RelativeChild child_idx = 0;
           child_idx < QuadtreeIndex::kNumChildren; ++child_idx) {
        if (node.hasChild(child_idx)) {
          const QuadtreeIndex child_node_index =
              node_index.computeChildIndex(child_idx);
          const Node<CellDataSpecialized>& child_node =
              *node.getChild(child_idx);
          stack.template emplace(child_node_index, child_node);
        }
      }
    } else if (OccupancyState::isObserved(node.data())) {
      const Index index = nodeIndexToIndex(node_index);
      max_index = max_index.cwiseMax(index);
    }
  }

  return max_index;
}

template <typename CellT>
bool SimpleQuadtree<CellT>::hasCell(const Index& index) const {
  const Node<CellDataSpecialized>* deepest_node_at_index =
      getDeepestNodeAtIndex(index);
  return deepest_node_at_index;
}

template <typename CellT>
QuadtreeIndex::Element SimpleQuadtree<CellT>::getDepthAtIndex(
    const Index& index) {
  QuadtreeIndex::Element depth = 0;
  const QuadtreeIndex deepest_possible_node_index = indexToNodeIndex(index);
  const Node<CellDataSpecialized>* node = &quadtree_.getRootNode();
  const std::vector<QuadtreeIndex::RelativeChild> child_indices =
      deepest_possible_node_index.computeRelativeChildIndices();
  for (const QuadtreeIndex::RelativeChild child_index : child_indices) {
    if (!node->hasChild(child_index)) {
      break;
    }
    node = node->getChild(child_index);
    ++depth;
  }
  return depth;
}

template <typename CellT>
FloatingPoint SimpleQuadtree<CellT>::getCellValue(const Index& index) const {
  const NodeType* node = getDeepestNodeAtIndex(index);
  if (node) {
    return node->data();
  }
  return 0.f;
}

template <typename CellT>
void SimpleQuadtree<CellT>::setCellValue(const Index& index,
                                         FloatingPoint new_value) {
  constexpr bool kAutoAllocate = true;
  const QuadtreeIndex node_index = indexToNodeIndex(index);
  Node<CellDataSpecialized>* node =
      quadtree_.getNode(node_index, kAutoAllocate);
  if (node) {
    node->data() = new_value;
  } else {
    LOG(ERROR) << "Failed to allocate cell at index: " << index;
  }
}

template <typename CellT>
void SimpleQuadtree<CellT>::setCellValue(const QuadtreeIndex& node_index,
                                         FloatingPoint new_value) {
  constexpr bool kAutoAllocate = true;
  Node<CellDataSpecialized>* node =
      quadtree_.getNode(node_index, kAutoAllocate);
  if (node) {
    node->data() = new_value;
  } else {
    LOG(ERROR) << "Failed to allocate cell at index: " << node_index.toString();
  }
}

template <typename CellT>
void SimpleQuadtree<CellT>::addToCellValue(const Index& index,
                                           FloatingPoint update) {
  constexpr bool kAutoAllocate = true;
  const QuadtreeIndex node_index = indexToNodeIndex(index);
  Node<CellDataSpecialized>* node =
      quadtree_.getNode(node_index, kAutoAllocate);
  if (node) {
    node->data() += update;
  } else {
    LOG(ERROR) << "Failed to allocate cell at index: " << index;
  }
}

template <typename CellT>
void SimpleQuadtree<CellT>::addToCellValue(const QuadtreeIndex& node_index,
                                           FloatingPoint update) {
  constexpr bool kAutoAllocate = true;
  Node<CellDataSpecialized>* node =
      quadtree_.getNode(node_index, kAutoAllocate);
  if (node) {
    node->data() += update;
  } else {
    LOG(ERROR) << "Failed to allocate cell at index: " << node_index.toString();
  }
}

template <typename CellT>
cv::Mat SimpleQuadtree<CellT>::getImage(bool /*use_color*/) const {
  // TODO(victorr): Implement this
  return {};
}

template <typename CellT>
bool SimpleQuadtree<CellT>::save(const std::string& /*file_path_prefix*/,
                                 bool /*use_floating_precision*/) const {
  // TODO(victorr): Implement this
  return false;
}

template <typename CellT>
bool SimpleQuadtree<CellT>::load(const std::string& /*file_path_prefix*/,
                                 bool /*used_floating_precision*/) {
  // TODO(victorr): Implement this
  return false;
}

template <typename CellT>
FloatingPoint SimpleQuadtree<CellT>::computeNodeWidthAtDepth(
    QuadtreeIndex::Element depth) {
  return root_node_width_ / std::exp2f(static_cast<FloatingPoint>(depth));
}

template <typename CellT>
Vector SimpleQuadtree<CellT>::computeNodeHalvedDiagonalAtDepth(
    QuadtreeIndex::Element depth) {
  return Vector::Constant(0.5f) * computeNodeWidthAtDepth(depth);
}

template <typename CellT>
const Node<typename CellT::Specialized>*
SimpleQuadtree<CellT>::getDeepestNodeAtIndex(const Index& index) const {
  const QuadtreeIndex deepest_possible_node_index = indexToNodeIndex(index);
  const Node<CellDataSpecialized>* node = &quadtree_.getRootNode();
  const std::vector<QuadtreeIndex::RelativeChild> child_indices =
      deepest_possible_node_index.computeRelativeChildIndices();
  for (const QuadtreeIndex::RelativeChild child_index : child_indices) {
    if (!node->hasChild(child_index)) {
      break;
    }
    node = node->getChild(child_index);
  }
  return node;
}
}  // namespace wavemap_2d

#endif  // WAVEMAP_2D_DATA_STRUCTURE_VOLUMETRIC_IMPL_SIMPLE_QUADTREE_INL_H_
