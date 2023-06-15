#ifndef WAVEMAP_FILE_CONVERSIONS_PROTO_CONVERSIONS_H_
#define WAVEMAP_FILE_CONVERSIONS_PROTO_CONVERSIONS_H_

#include <wavemap/common.h>
#include <wavemap/data_structure/volumetric/cell_types/haar_coefficients.h>
#include <wavemap/data_structure/volumetric/hashed_chunked_wavelet_octree.h>
#include <wavemap/data_structure/volumetric/hashed_wavelet_octree.h>
#include <wavemap/data_structure/volumetric/wavelet_octree.h>

#include "wavemap_file_conversions/Map.pb.h"

namespace wavemap::convert {
void indexToProto(const Index3D& index, proto::Index* index_proto);
void protoToIndex(const proto::Index& index_proto, Index3D& index);

void detailsToProto(
    const HaarCoefficients<FloatingPoint, 3>::Details& coefficients,
    google::protobuf::RepeatedField<float>* coefficients_proto);
void protoToDetails(
    const google::protobuf::RepeatedField<float>& coefficients_proto,
    HaarCoefficients<FloatingPoint, 3>::Details& coefficients);

bool mapToProto(const VolumetricDataStructureBase& map, proto::Map* map_proto);
void protoToMap(const proto::Map& map_proto,
                VolumetricDataStructureBase::Ptr& map);

void mapToProto(const WaveletOctree& map, proto::Map* map_proto);
void protoToMap(const proto::Map& map_proto, WaveletOctree::Ptr& map);

void mapToProto(const HashedWaveletOctree& map, proto::Map* map_proto);
void protoToMap(const proto::Map& map_proto, HashedWaveletOctree::Ptr& map);

void mapToProto(const HashedChunkedWaveletOctree& map, proto::Map* map_proto);
}  // namespace wavemap::convert

#endif  // WAVEMAP_FILE_CONVERSIONS_PROTO_CONVERSIONS_H_
