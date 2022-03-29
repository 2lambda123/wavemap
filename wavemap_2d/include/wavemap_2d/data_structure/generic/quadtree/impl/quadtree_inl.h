#ifndef WAVEMAP_2D_DATA_STRUCTURE_GENERIC_QUADTREE_IMPL_QUADTREE_INL_H_
#define WAVEMAP_2D_DATA_STRUCTURE_GENERIC_QUADTREE_IMPL_QUADTREE_INL_H_

#include <stack>
#include <vector>

#include "wavemap_2d/data_structure/generic/pointcloud.h"
#include "wavemap_2d/utils/eigen_format.h"

namespace wavemap_2d {
template <typename NodeDataType>
size_t Quadtree<NodeDataType>::size() const {
  auto subtree_iterator = getIterator<TraversalOrder::kDepthFirstPreorder>();
  // NOTE: 1 is subtracted from the count to account for the fact that the root
  //       node is even allocated when the quadtree is empty.
  return std::distance(subtree_iterator.begin(), subtree_iterator.end()) - 1u;
}

template <typename NodeDataType>
void Quadtree<NodeDataType>::prune() {
  for (NodeType& node : getIterator<TraversalOrder::kDepthFirstPostorder>()) {
    if (node.hasChildrenArray()) {
      bool has_non_empty_child = false;
      for (int child_idx = 0; child_idx < NodeIndex::kNumChildren;
           ++child_idx) {
        NodeType* child_ptr = node.getChild(child_idx);
        if (child_ptr) {
          if (child_ptr->empty()) {
            node.deleteChild(child_idx);
          } else {
            has_non_empty_child = true;
          }
        }
      }

      // Free up the children array if it only contains null pointers
      if (!has_non_empty_child) {
        node.deleteChildrenArray();
      }
    }
  }
}

template <typename NodeDataType>
size_t Quadtree<NodeDataType>::getMemoryUsage() const {
  size_t memory_usage = 0u;

  std::stack<const Node<NodeDataType>*> stack;
  stack.template emplace(&root_node_);
  while (!stack.empty()) {
    const Node<NodeDataType>* node = stack.top();
    stack.pop();
    memory_usage += node->getMemoryUsage();

    if (node->hasChildrenArray()) {
      for (NodeRelativeChildIndex child_idx = 0;
           child_idx < NodeIndex::kNumChildren; ++child_idx) {
        if (node->hasChild(child_idx)) {
          stack.template emplace(node->getChild(child_idx));
        }
      }
    }
  }

  return memory_usage;
}

template <typename NodeDataType>
bool Quadtree<NodeDataType>::removeNode(const NodeIndex& index) {
  NodeIndex parent_index = index.computeParentIndex();
  Node<NodeDataType>* parent_node =
      getNode(parent_index, /*auto_allocate*/ false);
  if (parent_node) {
    return parent_node->deleteChild(index.computeRelativeChildIndex());
  }
  return false;
}

template <typename NodeDataType>
Node<NodeDataType>* Quadtree<NodeDataType>::getNode(const NodeIndex& index,
                                                    bool auto_allocate) {
  Node<NodeDataType>* current_parent = &root_node_;
  const std::vector<NodeRelativeChildIndex> child_indices =
      index.computeRelativeChildIndices();
  for (const NodeRelativeChildIndex child_index : child_indices) {
    // Check if the child is allocated
    if (!current_parent->hasChild(child_index)) {
      if (auto_allocate) {
        current_parent->allocateChild(child_index);
      } else {
        return nullptr;
      }
    }

    current_parent = current_parent->getChild(child_index);
  }

  return current_parent;
}

template <typename NodeDataType>
const Node<NodeDataType>* Quadtree<NodeDataType>::getNode(
    const NodeIndex& index) const {
  const Node<NodeDataType>* current_parent = &root_node_;
  const std::vector<NodeRelativeChildIndex> child_indices =
      index.computeRelativeChildIndices();
  for (const NodeRelativeChildIndex child_index : child_indices) {
    // Check if the child is allocated
    if (!current_parent->hasChild(child_index)) {
      return nullptr;
    }

    current_parent = current_parent->getChild(child_index);
  }

  return current_parent;
}
}  // namespace wavemap_2d

#endif  // WAVEMAP_2D_DATA_STRUCTURE_GENERIC_QUADTREE_IMPL_QUADTREE_INL_H_
