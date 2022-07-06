#ifndef WAVEMAP_2D_DATA_STRUCTURE_CELL_TYPES_CELL_TRAITS_H_
#define WAVEMAP_2D_DATA_STRUCTURE_CELL_TYPES_CELL_TRAITS_H_

namespace wavemap {
template <typename SpecializedType, typename BaseFloatType,
          typename BaseIntType>
struct CellTraits {
  using Specialized = SpecializedType;
  using BaseFloat = BaseFloatType;
  using BaseInt = BaseIntType;
};
}  // namespace wavemap

#endif  // WAVEMAP_2D_DATA_STRUCTURE_CELL_TYPES_CELL_TRAITS_H_
