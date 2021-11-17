#ifndef WAVEMAP_2D_POINTCLOUD_INTEGRATOR_H_
#define WAVEMAP_2D_POINTCLOUD_INTEGRATOR_H_

#include <limits>
#include <utility>

#include "wavemap_2d/beam_model.h"
#include "wavemap_2d/common.h"
#include "wavemap_2d/occupancy_map.h"
#include "wavemap_2d/pointcloud.h"

namespace wavemap_2d {
class PointcloudIntegrator {
 public:
  PointcloudIntegrator() = delete;
  explicit PointcloudIntegrator(OccupancyMap::Ptr occupancy_map)
      : occupancy_map_(CHECK_NOTNULL(occupancy_map)),
        beam_model_(occupancy_map->getResolution()) {}

  void integratePointcloud(const PosedPointcloud& pointcloud) {
    beam_model_.setStartPoint(pointcloud.getOrigin());
    for (const auto& point : pointcloud.getPointsGlobal()) {
      beam_model_.setEndPoint(point);
      if (40.f < beam_model_.getLength()) {
        continue;
      }

      aabb_min_ = aabb_min_.cwiseMin(point);
      aabb_max_ = aabb_max_.cwiseMax(point);

      const Index bottom_left_idx = beam_model_.getBottomLeftUpdateIndex();
      const Index top_right_idx = beam_model_.getTopRightUpdateIndex();
      for (Index index = bottom_left_idx; index.x() <= top_right_idx.x();
           ++index.x()) {
        for (index.y() = bottom_left_idx.y(); index.y() <= top_right_idx.y();
             ++index.y()) {
          const FloatingPoint update = beam_model_.computeUpdateAt(index);
          occupancy_map_->updateCell(index, update);
        }
      }
    }
  }

  void printAabbBounds() const {
    LOG(INFO) << "AABB min:\n" << aabb_min_ << "\nmax:\n" << aabb_max_;
  }

 protected:
  Point aabb_min_ = Point::Constant(std::numeric_limits<FloatingPoint>::max());
  Point aabb_max_ =
      Point::Constant(std::numeric_limits<FloatingPoint>::lowest());

  OccupancyMap::Ptr occupancy_map_;

  BeamModel beam_model_;
};
}  // namespace wavemap_2d

#endif  // WAVEMAP_2D_POINTCLOUD_INTEGRATOR_H_
