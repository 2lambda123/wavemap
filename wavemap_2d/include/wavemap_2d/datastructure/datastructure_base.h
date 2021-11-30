#ifndef WAVEMAP_2D_DATASTRUCTURE_DATASTRUCTURE_BASE_H_
#define WAVEMAP_2D_DATASTRUCTURE_DATASTRUCTURE_BASE_H_

#include <algorithm>
#include <memory>
#include <string>

#include <Eigen/Eigen>
#include <opencv2/core/eigen.hpp>
#include <opencv2/opencv.hpp>

#include "wavemap_2d/common.h"

namespace wavemap_2d {
class DataStructureBase {
 public:
  using Ptr = std::shared_ptr<DataStructureBase>;

  explicit DataStructureBase(const FloatingPoint resolution)
      : resolution_(resolution) {}

  virtual bool empty() const = 0;
  virtual Index size() const = 0;
  virtual void clear() = 0;

  virtual Index getMinIndex() const = 0;
  virtual Index getMaxIndex() const = 0;
  virtual bool containsIndex(const Index& index) const = 0;
  FloatingPoint getResolution() const { return resolution_; }

  virtual void updateCell(const Index& index, FloatingPoint update) = 0;
  virtual FloatingPoint getCellValue(const Index& index) const = 0;

  void printSize() const { LOG(INFO) << "Size:\n" << size(); }

  virtual cv::Mat getImage(bool use_color) const = 0;
  void showImage(bool use_color = false) const;
  void saveImage(const std::string& file_path, bool use_color = false) const;
  virtual bool save(const std::string& file_path_prefix,
                    bool use_floating_precision) const = 0;
  // TODO(victorr): Automatically determine whether floating or fixed precision
  //                was used from the file format once it has been designed
  virtual bool load(const std::string& file_path_prefix,
                    bool used_floating_precision) = 0;

 protected:
  const FloatingPoint resolution_;

  static std::string getHeaderFilePath(const std::string& file_path_prefix) {
    return file_path_prefix + "_header";
  }
  static std::string getDataFilePath(const std::string& file_path_prefix,
                                     const bool use_floating_precision) {
    return file_path_prefix + "_data." +
           (use_floating_precision ? "exr" : "jp2");
  }
};
}  // namespace wavemap_2d

#endif  // WAVEMAP_2D_DATASTRUCTURE_DATASTRUCTURE_BASE_H_
