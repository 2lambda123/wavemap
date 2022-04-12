#include "wavemap_2d/integrator/measurement_model/fixed_logodds_model.h"

#include "wavemap_2d/indexing/index_conversions.h"

namespace wavemap_2d {
Index FixedLogOddsModel::getBottomLeftUpdateIndex() const {
  const Point bottom_left_point = W_start_point_.cwiseMin(W_end_point_);
  return computeFloorIndexForPoint(bottom_left_point, resolution_inv_);
}

Index FixedLogOddsModel::getTopRightUpdateIndex() const {
  const Point top_right_point = W_start_point_.cwiseMax(W_end_point_);
  return computeCeilIndexForPoint(top_right_point, resolution_inv_);
}
}  // namespace wavemap_2d
