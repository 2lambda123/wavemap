#ifndef WAVEMAP_3D_INTEGRATOR_PROJECTIVE_COARSE_TO_FINE_RANGE_IMAGE_2D_INTERSECTOR_H_
#define WAVEMAP_3D_INTEGRATOR_PROJECTIVE_COARSE_TO_FINE_RANGE_IMAGE_2D_INTERSECTOR_H_

#include <limits>
#include <memory>
#include <utility>

#include <wavemap_common/common.h>
#include <wavemap_common/data_structure/aabb.h>
#include <wavemap_common/integrator/projective/intersection_type.h>

#include "wavemap_3d/integrator/projective/coarse_to_fine/hierarchical_range_bounds_2d.h"
#include "wavemap_3d/integrator/projective/range_image_2d.h"

namespace wavemap {
class RangeImage2DIntersector {
 public:
  struct MinMaxAnglePair {
    static constexpr FloatingPoint kInitialMin =
        std::numeric_limits<FloatingPoint>::max();
    static constexpr FloatingPoint kInitialMax =
        std::numeric_limits<FloatingPoint>::lowest();

    Vector2D min_spherical_coordinates = Vector2D::Constant(kInitialMin);
    Vector2D max_spherical_coordinates = Vector2D::Constant(kInitialMax);
  };

  struct SphericalMinMaxCornerIndices {
    Eigen::Matrix<NdtreeIndexRelativeChild, 2, 1> min_corner_indices;
    Eigen::Matrix<NdtreeIndexRelativeChild, 2, 1> max_corner_indices;
  };
  using Cache = std::optional<SphericalMinMaxCornerIndices>;

  RangeImage2DIntersector(std::shared_ptr<RangeImage2D> range_image,
                          SphericalProjector projection_model,
                          FloatingPoint max_range,
                          FloatingPoint angle_threshold,
                          FloatingPoint range_threshold_in_front_of_surface,
                          FloatingPoint range_threshold_behind_surface)
      : hierarchical_range_image_(std::move(range_image)),
        projection_model_(std::move(projection_model)),
        max_range_(max_range),
        angle_threshold_(angle_threshold),
        range_threshold_in_front_(range_threshold_in_front_of_surface),
        range_threshold_behind_(range_threshold_behind_surface) {}

  // NOTE: When the AABB is right behind the sensor, the angle range will wrap
  //       around at +-PI and a min_angle >= max_angle will be returned.
  MinMaxAnglePair getAabbMinMaxProjectedAngle(
      const Transformation3D& T_W_C, const AABB<Point3D>& W_aabb) const;
  MinMaxAnglePair getAabbMinMaxProjectedAngle(const Transformation3D& T_W_C,
                                              const AABB<Point3D>& W_aabb,
                                              Cache& cache) const;

  IntersectionType determineIntersectionType(
      const Transformation3D& T_W_C, const AABB<Point3D>& W_cell_aabb) const;
  IntersectionType determineIntersectionType(const Transformation3D& T_W_C,
                                             const AABB<Point3D>& W_cell_aabb,
                                             Cache& cache) const;

 private:
  static constexpr bool kAzimuthAllowedToWrapAround = true;
  const HierarchicalRangeBounds2D<kAzimuthAllowedToWrapAround>
      hierarchical_range_image_;

  const SphericalProjector projection_model_;

  const FloatingPoint max_range_;
  const FloatingPoint angle_threshold_;
  const FloatingPoint range_threshold_in_front_;
  const FloatingPoint range_threshold_behind_;
};
}  // namespace wavemap

#include "wavemap_3d/integrator/projective/coarse_to_fine/impl/range_image_2d_intersector_inl.h"

#endif  // WAVEMAP_3D_INTEGRATOR_PROJECTIVE_COARSE_TO_FINE_RANGE_IMAGE_2D_INTERSECTOR_H_
