#ifndef WAVEMAP_2D_DATA_STRUCTURE_VOLUMETRIC_IMPL_HASHED_BLOCKS_INL_H_
#define WAVEMAP_2D_DATA_STRUCTURE_VOLUMETRIC_IMPL_HASHED_BLOCKS_INL_H_

#include <limits>
#include <string>

#include "wavemap_2d/data_structure/volumetric/dense_grid.h"
#include "wavemap_2d/iterator/grid_iterator.h"

namespace wavemap_2d {
template <typename CellT>
Index HashedBlocks<CellT>::getMinIndex() const {
  if (!empty()) {
    Index min_block_index =
        Index::Constant(std::numeric_limits<IndexElement>::max());
    for (const auto& [block_index, block] : blocks_) {
      min_block_index = min_block_index.cwiseMin(block_index);
    }
    return kCellsPerSide * min_block_index;
  }
  return Index::Zero();
}

template <typename CellT>
Index HashedBlocks<CellT>::getMaxIndex() const {
  if (!empty()) {
    Index max_block_index =
        Index::Constant(std::numeric_limits<IndexElement>::lowest());
    for (const auto& [block_index, block] : blocks_) {
      max_block_index = max_block_index.cwiseMax(block_index);
    }
    return kCellsPerSide * (max_block_index + Index::Ones());
  }
  return Index::Zero();
}

template <typename CellT>
bool HashedBlocks<CellT>::hasCell(const Index& index) const {
  return blocks_.count(computeBlockIndexFromIndex(index));
}

template <typename CellT>
FloatingPoint HashedBlocks<CellT>::getCellValue(const Index& index) const {
  const CellDataSpecialized* cell_data = accessCellData(index);
  if (cell_data) {
    return static_cast<FloatingPoint>(*cell_data);
  }
  return 0.f;
}

template <typename CellT>
void HashedBlocks<CellT>::setCellValue(const Index& index,
                                       FloatingPoint new_value) {
  constexpr bool kAutoAllocate = true;
  CellDataSpecialized* cell_data = accessCellData(index, kAutoAllocate);
  if (cell_data) {
    // TODO(victorr): Decide whether truncation should be applied here as well
    *cell_data = new_value;
  } else {
    LOG(ERROR) << "Failed to allocate cell at index: " << index;
  }
}

template <typename CellT>
void HashedBlocks<CellT>::addToCellValue(const Index& index,
                                         FloatingPoint update) {
  constexpr bool kAutoAllocate = true;
  CellDataSpecialized* cell_data = accessCellData(index, kAutoAllocate);
  if (cell_data) {
    *cell_data = CellT::add(*cell_data, update);
  } else {
    LOG(ERROR) << "Failed to allocate cell at index: " << index;
  }
}

template <typename CellT>
void HashedBlocks<CellT>::forEachLeaf(
    VolumetricDataStructure::IndexedLeafVisitorFunction visitor_fn) const {
  // TODO(victorr): Remove these hard-coded values and change the quadtree
  //                classes to expose offset free indexes, instead of
  //                compensating here.
  constexpr QuadtreeIndex::Element kMaxDepth = 14;
  const Index root_node_offset = Index::Constant(int_math::exp2(kMaxDepth - 1));

  const Index min_local_cell_index = Index{};
  const Index max_local_cell_index = Index{kCellsPerSide, kCellsPerSide};

  for (const auto& block_kv : blocks_) {
    const BlockIndex& block_index = block_kv.first;
    const Block& block_data = block_kv.second;

    for (const Index& local_cell_index :
         Grid(min_local_cell_index, max_local_cell_index)) {
      const FloatingPoint cell_data =
          block_data(local_cell_index.x(), local_cell_index.y());
      const Index cell_index =
          computeCellIndexFromBlockIndexAndIndex(block_index, local_cell_index);
      const QuadtreeIndex hierarchical_cell_index =
          convert::indexAndDepthToNodeIndex(cell_index + root_node_offset,
                                            kMaxDepth, kMaxDepth);
      visitor_fn(hierarchical_cell_index, cell_data);
    }
  }
}

template <typename CellT>
cv::Mat HashedBlocks<CellT>::getImage(bool use_color) const {
  DenseGrid<CellT> dense_grid(resolution_);
  for (const Index& index : Grid(getMinIndex(), getMaxIndex())) {
    const FloatingPoint cell_value = getCellValue(index);
    dense_grid.setCellValue(index, cell_value);
  }
  return dense_grid.getImage(use_color);
}

template <typename CellT>
bool HashedBlocks<CellT>::save(const std::string& /* file_path_prefix */,
                               bool /* use_floating_precision */) const {
  // TODO(victorr): Implement this
  return false;
}

template <typename CellT>
bool HashedBlocks<CellT>::load(const std::string& /* file_path_prefix */,
                               bool /* used_floating_precision */) {
  // TODO(victorr): Implement this
  return false;
}

template <typename CellT>
typename CellT::Specialized* HashedBlocks<CellT>::accessCellData(
    const Index& index, bool auto_allocate) {
  BlockIndex block_index = computeBlockIndexFromIndex(index);
  auto it = blocks_.find(block_index);
  if (it == blocks_.end()) {
    if (auto_allocate) {
      it = blocks_.template emplace(block_index, Block::Zero()).first;
    } else {
      return nullptr;
    }
  }
  CellIndex cell_index =
      computeCellIndexFromBlockIndexAndIndex(block_index, index);
  return &it->second.coeffRef(cell_index.x(), cell_index.y());
}

template <typename CellT>
const typename CellT::Specialized* HashedBlocks<CellT>::accessCellData(
    const Index& index) const {
  BlockIndex block_index = computeBlockIndexFromIndex(index);
  const auto& it = blocks_.find(block_index);
  if (it != blocks_.end()) {
    CellIndex cell_index =
        computeCellIndexFromBlockIndexAndIndex(block_index, index);
    return &it->second.coeff(cell_index.x(), cell_index.y());
  }
  return nullptr;
}

template <typename CellT>
typename HashedBlocks<CellT>::BlockIndex
HashedBlocks<CellT>::computeBlockIndexFromIndex(const Index& index) const {
  return {
      std::floor(kCellsPerSideInv * static_cast<FloatingPoint>(index.x())),
      std::floor(kCellsPerSideInv * static_cast<FloatingPoint>(index.y())),
  };
}

template <typename CellT>
typename HashedBlocks<CellT>::CellIndex
HashedBlocks<CellT>::computeCellIndexFromBlockIndexAndIndex(
    const HashedBlocks::BlockIndex& block_index, const Index& index) const {
  Index origin = kCellsPerSide * block_index;
  Index cell_index = index - origin;

  DCHECK((0 <= cell_index.array() && cell_index.array() < kCellsPerSide).all())
      << "(Local) cell indices should always be within range [0, "
         "kCellsPerSide[, but computed index equals "
      << cell_index << " instead.";

  return cell_index;
}
}  // namespace wavemap_2d

#endif  // WAVEMAP_2D_DATA_STRUCTURE_VOLUMETRIC_IMPL_HASHED_BLOCKS_INL_H_
