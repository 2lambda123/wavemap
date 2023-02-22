#ifndef WAVEMAP_INTEGRATOR_MEASUREMENT_MODEL_CONTINUOUS_BEAM_H_
#define WAVEMAP_INTEGRATOR_MEASUREMENT_MODEL_CONTINUOUS_BEAM_H_

#include <algorithm>
#include <memory>
#include <utility>

#include "wavemap/common.h"
#include "wavemap/config/config_base.h"
#include "wavemap/indexing/index_conversions.h"
#include "wavemap/integrator/measurement_model/measurement_model_base.h"
#include "wavemap/integrator/projective/update_type.h"
#include "wavemap/utils/eigen_format.h"

namespace wavemap {
struct ContinuousBeamConfig : ConfigBase<ContinuousBeamConfig> {
  FloatingPoint angle_sigma = 0.f;
  FloatingPoint range_sigma = 0.f;
  FloatingPoint scaling_free = 0.5f;
  FloatingPoint scaling_occupied = 0.5f;

  BeamSelectorType beam_selector_type = BeamSelectorType::kAllNeighbors;

  // Constructors
  ContinuousBeamConfig() = default;
  ContinuousBeamConfig(FloatingPoint angle_sigma, FloatingPoint range_sigma,
                       FloatingPoint scaling_free,
                       FloatingPoint scaling_occupied)
      : angle_sigma(angle_sigma),
        range_sigma(range_sigma),
        scaling_free(scaling_free),
        scaling_occupied(scaling_occupied) {}

  bool isValid(bool verbose) const override;
  static ContinuousBeamConfig from(const param::Map& params);
};

class ContinuousBeam : public MeasurementModelBase {
 public:
  explicit ContinuousBeam(
      const ContinuousBeamConfig& config,
      ProjectorBase::ConstPtr projection_model,
      std::shared_ptr<const Image<>> range_image,
      std::shared_ptr<const Image<Vector2D>> beam_offset_image)
      : config_(config.checkValid()),
        projection_model_(std::move(projection_model)),
        range_image_(std::move(range_image)),
        beam_offset_image_(std::move(beam_offset_image)) {}

  const ContinuousBeamConfig& getConfig() const { return config_; }
  FloatingPoint getPaddingAngle() const override { return angle_threshold_; }
  FloatingPoint getPaddingSurfaceFront() const override {
    return range_threshold_front;
  }
  FloatingPoint getPaddingSurfaceBack() const override {
    return range_threshold_back_;
  }

  FloatingPoint computeWorstCaseApproximationError(
      UpdateType update_type, FloatingPoint cell_to_sensor_distance,
      FloatingPoint cell_bounding_radius) const override;

  FloatingPoint computeUpdate(
      const Vector3D& sensor_coordinates) const override;

 private:
  const ContinuousBeamConfig config_;

  const ProjectorBase::ConstPtr projection_model_;
  const std::shared_ptr<const Image<>> range_image_;
  const std::shared_ptr<const Image<Vector2D>> beam_offset_image_;

  const FloatingPoint angle_threshold_ = 6.f * config_.angle_sigma;
  const FloatingPoint range_threshold_front = 3.f * config_.range_sigma;
  const FloatingPoint range_threshold_back_ = 6.f * config_.range_sigma;
  // NOTE: The angle and upper range thresholds have a width of 6 sigmas because
  //       the assumed 'ground truth' surface thickness is 3 sigma, and the
  //       angular/range uncertainty extends the non-zero regions with another 3
  //       sigma.

  FloatingPoint computeBeamUpdate(const Vector3D& sensor_coordinates,
                                  const Index2D& image_index,
                                  const Vector2D& cell_offset) const;

  // Compute the full measurement update, i.e. valid anywhere
  FloatingPoint computeFullBeamUpdate(
      FloatingPoint cell_to_sensor_distance,
      FloatingPoint cell_to_beam_image_error_norm,
      FloatingPoint measured_distance) const;

  // Compute the measurement update given that we're fully in free space, i.e.
  // only in the interval [ 0, measured_distance - range_threshold_in_front [
  // NOTE: Using this method is optional. It's slightly cheaper and more
  // accurate, but the difference is minor.
  FloatingPoint computeFreeSpaceBeamUpdate(
      FloatingPoint cell_to_beam_image_error_norm) const;
};
}  // namespace wavemap

#include "wavemap/integrator/measurement_model/impl/continuous_beam_inl.h"

#endif  // WAVEMAP_INTEGRATOR_MEASUREMENT_MODEL_CONTINUOUS_BEAM_H_
