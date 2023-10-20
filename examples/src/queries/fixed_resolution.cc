#include <wavemap/map/volumetric_data_structure_base.h>

#include "wavemap_examples/common.h"

using namespace wavemap;
int main(int, char**) {
  // Declare a map pointer for illustration purposes
  // NOTE: See the other tutorials on how to load maps from files or ROS topics,
  //       such as the map topic published by the wavemap ROS server.
  VolumetricDataStructureBase::Ptr map;

  // Declare the index to query
  // NOTE: See wavemap/indexing/index_conversions.h for helper methods
  //       to compute and convert indices.
  const Index3D query_index = Index3D::Zero();

  // Query the map
  const FloatingPoint value = map->getCellValue(query_index);
  examples::doSomething(value);
}
