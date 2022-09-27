#ifndef WAVEMAP_RVIZ_PLUGIN_WAVEMAP_OCTREE_DISPLAY_H_
#define WAVEMAP_RVIZ_PLUGIN_WAVEMAP_OCTREE_DISPLAY_H_

#ifndef Q_MOC_RUN
#include <memory>

#include <rviz/message_filter_display.h>
#include <rviz/properties/color_property.h>
#include <rviz/properties/float_property.h>
#include <rviz/properties/int_property.h>
#include <wavemap_msgs/Octree.h>

#include "wavemap_rviz_plugin/common.h"
#include "wavemap_rviz_plugin/multi_resolution_grid_visual.h"
#include "wavemap_rviz_plugin/multi_resolution_slice_visual.h"
#endif

namespace wavemap::rviz_plugin {
// The WavemapOctreeDisplay class implements the editable parameters and Display
// subclass machinery. The visuals themselves are represented by a separate
// class, WavemapOctreeVisual. The idiom for the visuals is that when the
// objects exist, they appear in the scene, and when they are deleted, they
// disappear.
class WavemapOctreeDisplay
    : public rviz::MessageFilterDisplay<wavemap_msgs::Octree> {
  Q_OBJECT
 public:
  // Constructor. pluginlib::ClassLoader creates instances by calling
  // the default constructor, so make sure you have one.
  WavemapOctreeDisplay();
  ~WavemapOctreeDisplay() override = default;

 protected:
  void onInitialize() override;

  // A helper to clear this display back to the initial state.
  void reset() override;

 private Q_SLOTS:  // NOLINT
  // These Qt slots get connected to signals indicating changes in the
  // user-editable properties
  void updateOccupancyThresholdsOrOpacity();
  void updateMultiResolutionGridVisibility();
  void updateMultiResolutionSliceVisibility();
  void updateMultiResolutionSliceHeight();

 private:
  // Function to handle an incoming ROS message
  void processMessage(
      const wavemap_msgs::Octree::ConstPtr& octree_msg) override;

  // Storage and message parsers for the octree
  std::unique_ptr<Octree> octree_;
  static std::unique_ptr<Octree> octreeFromRosMsg(
      const wavemap_msgs::Octree& octree_msg);

  // Storage for the visuals
  // NOTE: Visuals are enabled when they are allocated, and automatically
  //       removed from the scene when destructed.
  std::unique_ptr<MultiResolutionGridVisual> multi_resolution_grid_visual_;
  std::unique_ptr<MultiResolutionSliceVisual> multi_resolution_slice_visual_;

  // User-editable property variables
  std::unique_ptr<rviz::FloatProperty> min_occupancy_threshold_property_;
  std::unique_ptr<rviz::FloatProperty> max_occupancy_threshold_property_;
  std::unique_ptr<rviz::BoolProperty>
      multi_resolution_grid_visibility_property_;
  std::unique_ptr<rviz::BoolProperty>
      multi_resolution_slice_visibility_property_;
  std::unique_ptr<rviz::FloatProperty> multi_resolution_slice_height_property_;
  std::unique_ptr<rviz::FloatProperty> opacity_property_;
};
}  // namespace wavemap::rviz_plugin

#endif  // WAVEMAP_RVIZ_PLUGIN_WAVEMAP_OCTREE_DISPLAY_H_
