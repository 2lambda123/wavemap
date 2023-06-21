#ifndef WAVEMAP_RVIZ_PLUGIN_VISUALS_GRID_VISUAL_H_
#define WAVEMAP_RVIZ_PLUGIN_VISUALS_GRID_VISUAL_H_

#ifndef Q_MOC_RUN
#include <memory>
#include <unordered_map>
#include <vector>

#include <OGRE/OgreQuaternion.h>
#include <OGRE/OgreSceneManager.h>
#include <OGRE/OgreSceneNode.h>
#include <OGRE/OgreVector3.h>
#include <rviz/ogre_helpers/point_cloud.h>
#include <rviz/properties/bool_property.h>
#include <rviz/properties/color_property.h>
#include <rviz/properties/float_property.h>
#include <rviz/properties/int_property.h>
#include <rviz/properties/property.h>
#include <wavemap/data_structure/volumetric/volumetric_data_structure_base.h>
#include <wavemap/indexing/index_hashes.h>
#endif

namespace wavemap::rviz_plugin {
// Each instance of MultiResolutionGridVisual represents the visualization of a
// map's leaves as cubes whose sizes match their height in the tree.
class GridVisual : public QObject {
  Q_OBJECT
 public:  // NOLINT
  // Constructor. Creates the visual elements and puts them into the
  // scene, in an unconfigured state.
  GridVisual(Ogre::SceneManager* scene_manager, Ogre::SceneNode* parent_node,
             rviz::Property* submenu_root_property,
             const std::shared_ptr<std::mutex> map_mutex,
             const std::shared_ptr<VolumetricDataStructureBase::Ptr> map);

  // Destructor. Removes the visual elements from the scene.
  virtual ~GridVisual();

  void updateMap(bool redraw_all = false);

  void clear() { block_grids_.clear(); }

  // Set the pose of the coordinate frame the message refers to
  void setFramePosition(const Ogre::Vector3& position);

  void setFrameOrientation(const Ogre::Quaternion& orientation);

 private Q_SLOTS:  // NOLINT
  // These Qt slots get connected to signals indicating changes in the
  // user-editable properties
  void thresholdUpdateCallback() { updateMap(true); }
  void visibilityUpdateCallback();
  void opacityUpdateCallback();

 private:
  enum class ColorBy { kProbability, kPosition } kColorBy = ColorBy::kPosition;

  // Read only shared pointer to the map, owned by WavemapMapDisplay
  const std::shared_ptr<std::mutex> map_mutex_;
  const std::shared_ptr<VolumetricDataStructureBase::Ptr> map_ptr_;

  // The SceneManager, kept here only so the destructor can ask it to
  // destroy the `frame_node_`.
  Ogre::SceneManager* scene_manager_;

  // A SceneNode whose pose is set to match the coordinate frame of
  // the WavemapOctree message header.
  Ogre::SceneNode* frame_node_;

  // User-editable property variables, contained in the visual's submenu
  rviz::BoolProperty visibility_property_;
  rviz::FloatProperty min_occupancy_threshold_property_;
  rviz::FloatProperty max_occupancy_threshold_property_;
  rviz::FloatProperty opacity_property_;

  // The object implementing the grid visuals
  using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
  TimePoint last_update_time_{};
  std::unordered_map<Index3D, IndexElement, Index3DHash> block_update_queue_;
  using MultiResGrid = std::vector<std::unique_ptr<rviz::PointCloud>>;
  std::unordered_map<Index3D, MultiResGrid, Index3DHash> block_grids_;

  bool force_lod_update_ = true;
  float lod_update_distance_threshold_{0.1f};
  Ogre::Vector3 last_lod_update_position_{};
  void updateLOD(Ogre::Camera* cam);

  static constexpr int kMaxDrawsPerCycle = 50;
  void processBlockUpdateQueue();

  using PointcloudList = std::vector<std::vector<rviz::PointCloud::Point>>;
  void getLeafCentersAndColors(int tree_height, FloatingPoint min_cell_width,
                               FloatingPoint min_occupancy_log_odds,
                               FloatingPoint max_occupancy_log_odds,
                               const OctreeIndex& cell_index,
                               FloatingPoint cell_log_odds,
                               PointcloudList& cells_per_level);
  void drawMultiResGrid(IndexElement tree_height, FloatingPoint min_cell_width,
                        const Index3D& block_index, FloatingPoint alpha,
                        PointcloudList& cells_per_level,
                        MultiResGrid& multi_res_grid);

  // Map a voxel's log-odds value to a color (grey value)
  static Ogre::ColourValue logOddsToColor(FloatingPoint log_odds);

  // Map a voxel's position to a color (HSV color map)
  static Ogre::ColourValue positionToColor(const Point3D& center_point);
};

template <typename CallbackT>
class ViewportCamChangedListener : public Ogre::Viewport::Listener {
 public:
  explicit ViewportCamChangedListener(CallbackT callback)
      : callback_(callback) {}

  void viewportCameraChanged(Ogre::Viewport* viewport) override {
    std::invoke(callback_, viewport);
  }

 private:
  CallbackT callback_;
};

template <typename CallbackT>
class CamPrerenderListener : public Ogre::Camera::Listener {
 public:
  explicit CamPrerenderListener(CallbackT callback) : callback_(callback) {}
  void cameraPreRenderScene(Ogre::Camera* cam) override {
    std::invoke(callback_, cam);
  }

 private:
  CallbackT callback_;
};
}  // namespace wavemap::rviz_plugin

#endif  // WAVEMAP_RVIZ_PLUGIN_VISUALS_GRID_VISUAL_H_
