#ifndef WAVEMAP_2D_INTEGRATOR_POINT_INTEGRATOR_BEAM_INTEGRATOR_H_
#define WAVEMAP_2D_INTEGRATOR_POINT_INTEGRATOR_BEAM_INTEGRATOR_H_

#include <utility>

#include <wavemap_common/iterator/grid_iterator.h>

#include "wavemap_2d/integrator/pointcloud_integrator.h"

namespace wavemap {
class BeamIntegrator : public PointcloudIntegrator {
 public:
  using PointcloudIntegrator::PointcloudIntegrator;

  void integratePointcloud(
      const PosedPointcloud<Point2D, Transformation2D>& pointcloud) override {
    if (!isPointcloudValid(pointcloud)) {
      return;
    }

    MeasurementModelType measurement_model(occupancy_map_->getMinCellWidth());
    measurement_model.setStartPoint(pointcloud.getOrigin());

    for (const auto& end_point : pointcloud.getPointsGlobal()) {
      measurement_model.setEndPoint(end_point);
      if (!measurement_model.isMeasurementValid()) {
        continue;
      }

      const Grid grid(measurement_model.getBottomLeftUpdateIndex(),
                      measurement_model.getTopRightUpdateIndex());
      for (const auto& index : grid) {
        const FloatingPoint update = measurement_model.computeUpdateAt(index);
        if (kEpsilon < std::abs(update)) {
          occupancy_map_->addToCellValue(index, update);
        }
      }
    }
  }

 private:
  using MeasurementModelType = BeamModel;
};
}  // namespace wavemap

#endif  // WAVEMAP_2D_INTEGRATOR_POINT_INTEGRATOR_BEAM_INTEGRATOR_H_
