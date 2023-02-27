#include <gtest/gtest.h>

#include "wavemap/common.h"
#include "wavemap/data_structure/volumetric/volumetric_octree.h"
#include "wavemap/test/fixture_base.h"

namespace wavemap {
using VolumetricOctreeTest = FixtureBase;

// NOTE: Insertion tests are performed as part of the test suite for the
//       VolumetricDataStructure interface.
// TODO(victorr): Test whether out of bounds accesses/insertions are handled
//                correctly (e.g. throw error or do nothing and print error).

TEST_F(VolumetricOctreeTest, Initialization) {
  const FloatingPoint random_min_cell_width = getRandomMinCellWidth();
  VolumetricOctree map(VolumetricDataStructureConfig{random_min_cell_width});
  EXPECT_EQ(map.getMinCellWidth(), random_min_cell_width);
  EXPECT_TRUE(map.empty());
  EXPECT_EQ(map.size(), 1u);  // Contains exactly 1 node (the root)
}

TEST_F(VolumetricOctreeTest, IndexConversions) {
  VolumetricOctree map(VolumetricDataStructureConfig{getRandomMinCellWidth()});
  std::vector<Index3D> random_indices = getRandomIndexVector(
      map.getMinPossibleIndex(), map.getMaxPossibleIndex());
  random_indices.emplace_back(map.getMinPossibleIndex());
  random_indices.emplace_back(map.getMaxPossibleIndex());
  for (const Index3D& index : random_indices) {
    // NOTE: Since the Index is converted into an internal NodeIndex at height
    //       0, the NodeIndex's min and max corner Indexes should be the same.
    EXPECT_EQ(map.toExternalIndex(
                  convert::nodeIndexToMinCornerIndex(map.toInternal(index))),
              index);
    EXPECT_EQ(map.toExternalIndex(
                  convert::nodeIndexToMaxCornerIndex(map.toInternal(index))),
              index);
  }
}

TEST_F(VolumetricOctreeTest, Resizing) {
  VolumetricOctree map(VolumetricDataStructureConfig{getRandomMinCellWidth()});
  ASSERT_TRUE(map.empty());
  ASSERT_EQ(map.size(), 1u);

  const Index3D kMinIndex = Index3D::Constant(-2e3);
  const Index3D kMaxIndex = Index3D::Constant(2e3);
  const std::vector<Index3D> random_indices =
      getRandomIndexVector<3>(kMinIndex, kMaxIndex);

  const Index3D& first_random_index = random_indices[0];
  map.addToCellValue(first_random_index, 0.f);
  EXPECT_FALSE(map.empty());
  EXPECT_EQ(map.size(), map.kMaxHeight + 1);

  Index3D min_index = first_random_index;
  Index3D max_index = first_random_index;
  for (auto index_it = ++random_indices.cbegin();
       index_it != random_indices.cend(); ++index_it) {
    min_index = min_index.cwiseMin(*index_it);
    max_index = max_index.cwiseMax(*index_it);
    map.addToCellValue(*index_it, 0.f);
  }
  EXPECT_GE(map.size(), map.kMaxHeight);
  size_t max_unique_nodes = 0u;
  const size_t num_inserted_nodes = random_indices.size();
  for (QuadtreeIndex::Element depth = 0u; depth <= map.kMaxHeight; ++depth) {
    constexpr int kMapDimension = 2;
    const size_t max_unique_nodes_at_depth =
        int_math::exp2(kMapDimension * depth);
    if (max_unique_nodes_at_depth < num_inserted_nodes) {
      max_unique_nodes += max_unique_nodes_at_depth;
    } else {
      max_unique_nodes += num_inserted_nodes;
    }
  }
  EXPECT_LE(map.size(), max_unique_nodes);

  for (Index3D index = min_index; index.x() <= max_index.x(); ++index.x()) {
    for (index.y() = min_index.y(); index.y() <= max_index.y(); ++index.y()) {
      EXPECT_FLOAT_EQ(map.getCellValue(index), 0.f);
    }
  }

  map.clear();
  EXPECT_TRUE(map.empty());
  EXPECT_EQ(map.size(), 1u);
}

TEST_F(VolumetricOctreeTest, Pruning) {
  constexpr int kNumRepetitions = 10;
  for (int i = 0; i < kNumRepetitions; ++i) {
    VolumetricOctree map(
        VolumetricDataStructureConfig{getRandomMinCellWidth()});
    const std::vector<Index3D> random_indices = getRandomIndexVector<3>(
        map.getMinPossibleIndex(), map.getMaxPossibleIndex());

    // Check that zero values are inserted but removed after pruning
    ASSERT_TRUE(map.empty());
    for (const Index3D& random_index : random_indices) {
      map.setCellValue(random_index, 0.f);
    }
    EXPECT_FALSE(map.empty());
    map.prune();
    EXPECT_TRUE(map.empty());

    // Check that non-zero values are inserted but not removed after pruning
    ASSERT_TRUE(map.empty());
    for (const Index3D& random_index : random_indices) {
      map.setCellValue(random_index, 1.f);
    }
    EXPECT_FALSE(map.empty());
    map.prune();
    EXPECT_FALSE(map.empty());
  }
}
}  // namespace wavemap
