#include <gtest/gtest.h>

#include "wavemap_2d/map.h"
#include "wavemap_2d/utils/random_number_generator.h"

namespace wavemap_2d {
class MapTest : public ::testing::Test {
 protected:
  void SetUp() override {
    random_number_generator_ = std::make_unique<RandomNumberGenerator>();
  }

  FloatingPoint getRandomResolution() const {
    constexpr FloatingPoint kMinResolution = 1e-3;
    constexpr FloatingPoint kMaxResolution = 1e2;
    return random_number_generator_->getRandomRealNumber(kMinResolution,
                                                         kMaxResolution);
  }
  IndexElement getRandomIndexElement() const {
    constexpr IndexElement kMinCoordinate = -1e3;
    constexpr IndexElement kMaxCoordinate = 1e3;
    return random_number_generator_->getRandomInteger(kMinCoordinate,
                                                      kMaxCoordinate);
  }
  Index getRandomIndex() const {
    return {getRandomIndexElement(), getRandomIndexElement()};
  }
  std::vector<Index> getRandomIndexVector() const {
    constexpr size_t kMinNumIndices = 2u;
    constexpr size_t kMaxNumIndices = 100u;
    const size_t num_indices = random_number_generator_->getRandomInteger(
        kMinNumIndices, kMaxNumIndices);
    std::vector<Index> random_indices;
    random_indices.reserve(num_indices);
    for (size_t idx = 0u; idx < num_indices; ++idx) {
      random_indices.emplace_back(getRandomIndex());
    }
    return random_indices;
  }
  FloatingPoint getRandomUpdate() const {
    constexpr FloatingPoint kMinUpdate = 1e-3;
    constexpr FloatingPoint kMaxUpdate = 1e3;
    return random_number_generator_->getRandomRealNumber(kMinUpdate,
                                                         kMaxUpdate);
  }
  std::vector<FloatingPoint> getRandomUpdateVector() const {
    constexpr size_t kMinNumUpdates = 0u;
    constexpr size_t kMaxNumUpdates = 100u;
    const size_t num_updates = random_number_generator_->getRandomInteger(
        kMinNumUpdates, kMaxNumUpdates);
    std::vector<FloatingPoint> random_updates(num_updates);
    std::generate(random_updates.begin(), random_updates.end(),
                  [this]() { return getRandomUpdate(); });
    return random_updates;
  }

 private:
  std::unique_ptr<RandomNumberGenerator> random_number_generator_;
};

TEST_F(MapTest, Initialization) {
  const FloatingPoint random_resolution = getRandomResolution();
  GridMap map(random_resolution);
  EXPECT_EQ(map.getResolution(), random_resolution);
  EXPECT_TRUE(map.empty());
  EXPECT_EQ(map.size(), Index(0, 0));
  EXPECT_EQ(map.getMinIndex(), Index::Constant(NAN));
  EXPECT_EQ(map.getMaxIndex(), Index::Constant(NAN));
}

TEST_F(MapTest, AutoResize) {
  GridMap map(getRandomResolution());
  ASSERT_TRUE(map.empty());
  ASSERT_EQ(map.size(), Index(0, 0));

  const std::vector<Index> random_indices = getRandomIndexVector();

  const Index& first_random_index = random_indices[0];
  map.updateCell(first_random_index, getRandomUpdate());
  EXPECT_FALSE(map.empty());
  EXPECT_EQ(map.size(), Index(1, 1));
  EXPECT_EQ(map.getMinIndex(), first_random_index);
  EXPECT_EQ(map.getMaxIndex(), first_random_index);

  Index min_index = first_random_index;
  Index max_index = first_random_index;
  for (auto index_it = ++random_indices.begin();
       index_it != random_indices.end(); ++index_it) {
    min_index = min_index.cwiseMin(*index_it);
    max_index = max_index.cwiseMax(*index_it);
    map.updateCell(*index_it, getRandomUpdate());
  }
  Index min_map_size = max_index - min_index + Index::Ones();
  EXPECT_EQ(map.size(), min_map_size);
  EXPECT_EQ(map.getMinIndex(), min_index);
  EXPECT_EQ(map.getMaxIndex(), max_index);
}

TEST_F(MapTest, InsertionTest) {
  GridMap map(getRandomResolution());
  const std::vector<Index> random_indices = getRandomIndexVector();
  for (const Index& random_index : random_indices) {
    FloatingPoint expected_value = 0.f;
    for (const FloatingPoint random_update : getRandomUpdateVector()) {
      map.updateCell(random_index, random_update);
      expected_value += random_update;
    }
    EXPECT_FLOAT_EQ(map.getCellValue(random_index), expected_value);
  }
}
}  // namespace wavemap_2d
