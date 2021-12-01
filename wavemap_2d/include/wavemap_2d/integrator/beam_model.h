#ifndef WAVEMAP_2D_INTEGRATOR_BEAM_MODEL_H_
#define WAVEMAP_2D_INTEGRATOR_BEAM_MODEL_H_

#include "wavemap_2d/common.h"

namespace wavemap_2d {
class BeamModel {
 public:
  explicit BeamModel(FloatingPoint resolution)
      : resolution_(resolution),
        resolution_inv_(1.f / resolution),
        measured_distance_(0.f),
        max_lateral_component_(0.f) {}

  void setStartPoint(const Point& start_point) {
    W_start_point_ = start_point;
    updateCachedVariables();
  }
  void setEndPoint(const Point& end_point) {
    W_end_point_ = end_point;
    updateCachedVariables();
  }

  FloatingPoint getLength() const { return measured_distance_; }
  bool exceedsMaxRange() const { return kRangeMax < measured_distance_; }
  Index getBottomLeftUpdateIndex();
  Index getTopRightUpdateIndex();

  FloatingPoint computeUpdateAt(const Index& index);

  static constexpr FloatingPoint kAngleThresh = 0.0174533f;
  static constexpr FloatingPoint kRangeMax = 40.f;
  static constexpr FloatingPoint kRangeDeltaThresh = 0.1f;

 protected:
  static constexpr FloatingPoint kAngleSigma = kAngleThresh / 3.f;
  static constexpr FloatingPoint kRangeSigma = kRangeDeltaThresh / 3.f;
  static constexpr FloatingPoint kFreeScaling = 0.2f;
  static constexpr FloatingPoint kOccScaling = 0.4f;

  const FloatingPoint resolution_;
  const FloatingPoint resolution_inv_;

  Point W_start_point_;
  Point W_end_point_;

  FloatingPoint measured_distance_;
  Point C_end_point_;
  Point C_end_point_normalized_;
  // TODO(victorr): Consider calculating the true min/max lateral components,
  //                or using a better approximation
  FloatingPoint max_lateral_component_;

  void updateCachedVariables() {
    C_end_point_ = W_end_point_ - W_start_point_;
    measured_distance_ = C_end_point_.norm();
    C_end_point_normalized_ = C_end_point_ / measured_distance_;
    max_lateral_component_ =
        std::sin(kAngleThresh) * (measured_distance_ + kRangeDeltaThresh);
  }

  static FloatingPoint q_cdf(const FloatingPoint t);
};
}  // namespace wavemap_2d

#endif  // WAVEMAP_2D_INTEGRATOR_BEAM_MODEL_H_
