#ifndef WAVEMAP_2D_ITERATOR_RAY_ITERATOR_H_
#define WAVEMAP_2D_ITERATOR_RAY_ITERATOR_H_

#include <wavemap_common/common.h>
#include <wavemap_common/indexing/index_conversions.h>

namespace wavemap {
// NOTE: The ray casting code in this class is largely based on voxblox's
//       RayCaster class, please see:
//       https://github.com/ethz-asl/voxblox/blob/master/voxblox/include/voxblox/integrator/integrator_utils.h#L100
class Ray {
 public:
  Ray(const Point2D& start_point, const Point2D& end_point,
      FloatingPoint min_cell_width) {
    const Point2D start_point_scaled = start_point / min_cell_width;
    const Point2D end_point_scaled = end_point / min_cell_width;
    if (start_point_scaled.hasNaN() || end_point_scaled.hasNaN()) {
      ray_length_in_steps_ = 0u;
      return;
    }

    start_index_ = convert::scaledPointToNearestIndex(start_point_scaled);
    end_index_ = convert::scaledPointToNearestIndex(end_point_scaled);
    const Index2D diff_index = end_index_ - start_index_;
    ray_length_in_steps_ = diff_index.cwiseAbs().sum() + 1u;

    const Vector2D ray_scaled = end_point_scaled - start_point_scaled;
    ray_step_signs_ = ray_scaled.cwiseSign().cast<IndexElement>();

    const Index2D corrected_step = ray_step_signs_.cwiseMax(0);
    const Point2D start_scaled_shifted =
        start_point_scaled - start_index_.cast<FloatingPoint>();
    const Vector2D distance_to_boundaries =
        corrected_step.cast<FloatingPoint>() - start_scaled_shifted;

    t_to_next_boundary_init_ =
        (ray_scaled.array().abs() <= 0.f)
            .select(2.f, distance_to_boundaries.array() / ray_scaled.array());

    // Distance to cross one grid cell along the ray in t
    t_step_size_ =
        (ray_scaled.array().abs() <= 0.f)
            .select(2.f, ray_step_signs_.cast<FloatingPoint>().array() /
                             ray_scaled.array());
  }

  class Iterator {
   public:
    explicit Iterator(const Ray& ray)
        : ray_(ray),
          current_index_(ray_.start_index_),
          t_to_next_boundary_(ray_.t_to_next_boundary_init_) {}
    Iterator(const Ray& ray, bool end) : Iterator(ray) {
      if (end) {
        current_step_ = ray_.ray_length_in_steps_;
        current_index_ = ray_.end_index_;
      }
    }

    Index2D operator*() const { return current_index_; }
    Iterator& operator++() {  // prefix ++
      int t_min_idx;
      t_to_next_boundary_.minCoeff(&t_min_idx);

      current_index_[t_min_idx] += ray_.ray_step_signs_[t_min_idx];
      t_to_next_boundary_[t_min_idx] += ray_.t_step_size_[t_min_idx];
      ++current_step_;

      return *this;
    }
    Iterator operator++(int) {  // postfix ++
      Iterator retval = *this;
      ++(*this);  // call the above prefix incrementer
      return retval;
    }
    friend bool operator==(const Iterator& lhs, const Iterator& rhs) {
      return (lhs.current_step_ == rhs.current_step_) &&
             (&lhs.ray_ == &rhs.ray_);
    }
    friend bool operator!=(const Iterator& lhs, const Iterator& rhs) {
      return !(lhs == rhs);  // NOLINT
    }

   private:
    const Ray& ray_;
    unsigned int current_step_ = 0u;
    Index2D current_index_;
    Vector2D t_to_next_boundary_;
  };

  Iterator begin() const { return Iterator{*this}; }
  Iterator end() const { return Iterator{*this, /*end*/ true}; }

 private:
  Index2D start_index_;
  Index2D end_index_;
  unsigned int ray_length_in_steps_;

  Index2D ray_step_signs_;
  Vector2D t_step_size_;
  Vector2D t_to_next_boundary_init_;
};
}  // namespace wavemap

#endif  // WAVEMAP_2D_ITERATOR_RAY_ITERATOR_H_
