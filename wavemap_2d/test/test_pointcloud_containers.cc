#include <gtest/gtest.h>

#include "wavemap_2d/pointcloud.h"
#include "wavemap_2d/utils/random_number_generator.h"

namespace wavemap_2d {
class PointcloudTest : public ::testing::Test {
 protected:
  void SetUp() override {
    random_number_generator_ = std::make_unique<RandomNumberGenerator>();
  }

  static void compare(const std::vector<Point>& point_vector,
                      const Pointcloud& pointcloud) {
    ASSERT_EQ(point_vector.size(), pointcloud.size());
    size_t point_idx = 0u;
    for (const Point& point : point_vector) {
      EXPECT_EQ(point, pointcloud[point_idx++]);
    }
  }
  static void compare(const PointcloudData& point_matrix,
                      const Pointcloud& pointcloud) {
    ASSERT_EQ(point_matrix.cols(), pointcloud.size());
    for (Eigen::Index point_idx = 0; point_idx < point_matrix.cols();
         ++point_idx) {
      EXPECT_EQ(point_matrix.col(point_idx), pointcloud[point_idx]);
    }
  }
  static void compare(const Pointcloud& pointcloud_reference,
                      const Pointcloud& pointcloud_to_test) {
    ASSERT_EQ(pointcloud_reference.size(), pointcloud_to_test.size());
    for (size_t point_idx = 0u; point_idx < pointcloud_reference.size();
         ++point_idx) {
      EXPECT_EQ(pointcloud_reference[point_idx], pointcloud_to_test[point_idx]);
    }
  }

  unsigned int getRandomPointcloudLength() {
    constexpr unsigned int kMinSize = 1u;
    constexpr unsigned int kMaxSize = 10u;
    return random_number_generator_->getRandomInteger(kMinSize, kMaxSize);
  }
  FloatingPoint getRandomAngle() {
    constexpr FloatingPoint kMinAngle = -M_PI;
    constexpr FloatingPoint kMaxAngle = M_PI;
    return random_number_generator_->getRandomRealNumber(kMinAngle, kMaxAngle);
  }
  std::vector<Point> getRandomPointVector() {
    std::vector<Point> random_point_vector(getRandomPointcloudLength());
    for (Point& point : random_point_vector) {
      point.setRandom();
    }
    return random_point_vector;
  }
  PointcloudData getRandomPointMatrix() {
    const Eigen::Index random_length = getRandomPointcloudLength();
    PointcloudData random_point_matrix;
    random_point_matrix.resize(kPointcloudPointDim, random_length);
    random_point_matrix.setRandom();
    return random_point_matrix;
  }

 private:
  std::unique_ptr<RandomNumberGenerator> random_number_generator_;
};

TEST_F(PointcloudTest, DefaultInitialize) {
  Pointcloud default_pointcloud;
  EXPECT_EQ(default_pointcloud.size(), 0u);
  for (auto it = default_pointcloud.begin(); it != default_pointcloud.end();
       ++it) {
    ADD_FAILURE();
  }
}

TEST_F(PointcloudTest, InitializeFromStl) {
  std::vector<Point> empty_point_vector;
  Pointcloud empty_pointcloud(empty_point_vector);
  EXPECT_EQ(empty_pointcloud.size(), 0u);

  std::vector<Point> random_point_vector = getRandomPointVector();
  Pointcloud random_pointcloud(random_point_vector);
  compare(random_point_vector, random_pointcloud);
}

TEST_F(PointcloudTest, InitializeFromEigen) {
  PointcloudData empty_point_matrix;
  Pointcloud empty_pointcloud(empty_point_matrix);
  EXPECT_EQ(empty_pointcloud.size(), 0u);

  PointcloudData random_point_matrix = getRandomPointMatrix();
  Pointcloud random_pointcloud(random_point_matrix);
  compare(random_point_matrix, random_pointcloud);
}

TEST_F(PointcloudTest, CopyInitializationAndAssignment) {
  const Pointcloud random_pointcloud(getRandomPointMatrix());

  Pointcloud copy_initialized_random_pointcloud(random_pointcloud);
  compare(random_pointcloud, copy_initialized_random_pointcloud);

  Pointcloud copy_assigned_random_pointcloud;
  copy_assigned_random_pointcloud = random_pointcloud;
  compare(random_pointcloud, copy_assigned_random_pointcloud);
}

TEST_F(PointcloudTest, ResizeAndClear) {
  Pointcloud pointcloud;
  ASSERT_EQ(pointcloud.size(), 0u);

  unsigned int random_new_size = getRandomPointcloudLength();
  pointcloud.resize(random_new_size);
  EXPECT_EQ(pointcloud.size(), random_new_size);

  pointcloud.clear();
  EXPECT_EQ(pointcloud.size(), 0u);
}

TEST_F(PointcloudTest, Iterators) {
  std::vector<Point> random_point_vector = getRandomPointVector();

  Pointcloud random_pointcloud(random_point_vector);
  size_t point_idx = 0u;
  for (const auto& random_point : random_pointcloud) {
    EXPECT_EQ(random_point, random_point_vector[point_idx++]);
  }
  EXPECT_EQ(point_idx, random_pointcloud.size());

  const Pointcloud const_random_pointcloud(random_point_vector);
  point_idx = 0u;
  for (const auto& random_point : const_random_pointcloud) {
    EXPECT_EQ(random_point, random_point_vector[point_idx++]);
  }
  EXPECT_EQ(point_idx, const_random_pointcloud.size());

  // TODO(victorr): Add test that checks if modifying when iterating by
  //                reference changes the source data as expected
}

class PosedPointcloudTest : public PointcloudTest {
 protected:
  static void compare(const PosedPointcloud& pointcloud_reference,
                      const PosedPointcloud& pointcloud_to_test) {
    EXPECT_EQ(pointcloud_reference.getOrigin(), pointcloud_to_test.getOrigin());
    EXPECT_EQ(pointcloud_reference.getPose(), pointcloud_to_test.getPose());

    PointcloudTest::compare(pointcloud_reference.getPointsLocal(),
                            pointcloud_to_test.getPointsLocal());
    PointcloudTest::compare(pointcloud_reference.getPointsGlobal(),
                            pointcloud_to_test.getPointsGlobal());
  }

  Transformation getRandomTransformation() {
    const Rotation random_rotation(getRandomAngle());
    const Translation random_translation(Point().setRandom());
    return {random_rotation, random_translation};
  }
};

TEST_F(PosedPointcloudTest, InitializationAndCopying) {
  // Initialize
  const Transformation random_transformation = getRandomTransformation();
  const Pointcloud random_pointcloud(getRandomPointVector());
  PosedPointcloud random_posed_pointcloud(random_transformation,
                                          random_pointcloud);

  // Test initialization
  EXPECT_EQ(random_posed_pointcloud.getOrigin(),
            random_transformation.getPosition());
  EXPECT_EQ(random_posed_pointcloud.getPose(), random_transformation);
  PointcloudTest::compare(random_posed_pointcloud.getPointsLocal(),
                          random_pointcloud);

  // Test copying
  PosedPointcloud copy_initialized_posed_pointcloud(random_posed_pointcloud);
  compare(random_posed_pointcloud, copy_initialized_posed_pointcloud);

  PosedPointcloud copy_assigned_posed_pointcloud;
  copy_assigned_posed_pointcloud = random_posed_pointcloud;
  compare(random_posed_pointcloud, copy_assigned_posed_pointcloud);
}

TEST_F(PosedPointcloudTest, Transformation) {
  const std::vector<Point> random_points_C = getRandomPointVector();
  const Transformation random_T_W_C = getRandomTransformation();
  PosedPointcloud random_posed_pointcloud(random_T_W_C,
                                          Pointcloud(random_points_C));

  Pointcloud pointcloud_W = random_posed_pointcloud.getPointsGlobal();
  ASSERT_EQ(pointcloud_W.size(), random_points_C.size());
  size_t point_idx = 0u;
  for (const Point& point_C : random_points_C) {
    const Point point_W = random_T_W_C * point_C;
    const Point& posed_pointcloud_point_W = pointcloud_W[point_idx];

    for (int axis = 0; axis < Point::Base::RowsAtCompileTime; ++axis) {
      EXPECT_FLOAT_EQ(point_W[axis], posed_pointcloud_point_W[axis])
          << " mismatch for point " << point_idx << " along axis " << axis;
    }
    ++point_idx;
  }
}
}  // namespace wavemap_2d
