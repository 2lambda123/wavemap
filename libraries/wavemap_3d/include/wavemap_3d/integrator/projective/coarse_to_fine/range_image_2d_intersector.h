#ifndef WAVEMAP_3D_INTEGRATOR_PROJECTIVE_COARSE_TO_FINE_RANGE_IMAGE_2D_INTERSECTOR_H_
#define WAVEMAP_3D_INTEGRATOR_PROJECTIVE_COARSE_TO_FINE_RANGE_IMAGE_2D_INTERSECTOR_H_

#include <limits>
#include <memory>
#include <utility>

#include <wavemap_common/common.h>
#include <wavemap_common/data_structure/aabb.h>
#include <wavemap_common/integrator/projective/intersection_type.h>

#include "wavemap_3d/integrator/projective/coarse_to_fine/hierarchical_range_image_2d.h"
#include "wavemap_3d/integrator/projective/range_image_2d.h"

namespace wavemap {
class RangeImage2DIntersector {
 public:
  struct MinMaxAnglePair {
    Vector2D min_spherical_coordinates =
        Vector2D::Constant(std::numeric_limits<FloatingPoint>::max());
    Vector2D max_spherical_coordinates =
        Vector2D::Constant(std::numeric_limits<FloatingPoint>::lowest());
  };

  explicit RangeImage2DIntersector(std::shared_ptr<RangeImage2D> range_image)
      : hierarchical_range_image_(std::move(range_image)) {}

  // NOTE: When the AABB is right behind the sensor, the angle range will wrap
  //       around at +-PI and a min_angle >= max_angle will be returned.
  static MinMaxAnglePair getAabbMinMaxProjectedAngle(
      const Transformation3D& T_W_C, const AABB<Point3D>& W_aabb);

  IntersectionType determineIntersectionType(
      const Transformation3D& T_W_C, const AABB<Point3D>& W_cell_aabb,
      const SphericalProjector& spherical_projector) const;

 private:
  HierarchicalRangeImage2D hierarchical_range_image_;
};
}  // namespace wavemap

#include "wavemap_3d/integrator/projective/coarse_to_fine/inl/range_image_2d_intersector_inl.h"

#endif  // WAVEMAP_3D_INTEGRATOR_PROJECTIVE_COARSE_TO_FINE_RANGE_IMAGE_2D_INTERSECTOR_H_
