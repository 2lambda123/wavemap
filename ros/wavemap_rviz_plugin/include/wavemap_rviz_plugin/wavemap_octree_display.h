#ifndef ROS_WAVEMAP_RVIZ_PLUGIN_INCLUDE_WAVEMAP_RVIZ_PLUGIN_WAVEMAP_OCTREE_DISPLAY_H_
#define ROS_WAVEMAP_RVIZ_PLUGIN_INCLUDE_WAVEMAP_RVIZ_PLUGIN_WAVEMAP_OCTREE_DISPLAY_H_

#include <memory>

#include <rviz/message_filter_display.h>
#include <rviz/properties/color_property.h>
#include <rviz/properties/float_property.h>
#include <rviz/properties/int_property.h>
#include <wavemap_msgs/Octree.h>

#include "wavemap_rviz_plugin/wavemap_octree_visual.h"

namespace wavemap_rviz_plugin {
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
  void updateOccupancyThreshold();

 private:
  // Function to handle an incoming ROS message
  void processMessage(const wavemap_msgs::Octree::ConstPtr& msg) override;

  // Storage for the octree visual
  std::unique_ptr<WavemapOctreeVisual> visual_;

  // User-editable property variables
  rviz::FloatProperty* occupancy_threshold_property_;
};
}  // namespace wavemap_rviz_plugin

#endif  // ROS_WAVEMAP_RVIZ_PLUGIN_INCLUDE_WAVEMAP_RVIZ_PLUGIN_WAVEMAP_OCTREE_DISPLAY_H_
