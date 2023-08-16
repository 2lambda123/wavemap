#include "wavemap_rviz_plugin/visuals/slice_visual.h"

#include <ros/console.h>
#include <wavemap/indexing/index_conversions.h>

namespace wavemap::rviz_plugin {
SliceVisual::SliceVisual(Ogre::SceneManager* scene_manager,
                         Ogre::SceneNode* parent_node,
                         rviz::Property* submenu_root_property,
                         const std::shared_ptr<MapAndMutex> map_and_mutex)
    : map_and_mutex_(map_and_mutex),
      scene_manager_(CHECK_NOTNULL(scene_manager)),
      frame_node_(CHECK_NOTNULL(parent_node)->createChildSceneNode()),
      visibility_property_(
          "Show", false,
          "Whether to show the octree as a multi-resolution grid.",
          CHECK_NOTNULL(submenu_root_property), SLOT(generalUpdateCallback()),
          this),
      min_occupancy_threshold_property_(
          "Min log odds", -1e6, "Ranges from -Inf to Inf.",
          submenu_root_property, SLOT(generalUpdateCallback()), this),
      max_occupancy_threshold_property_(
          "Max log odds", 1e6, "Ranges from -Inf to Inf.",
          submenu_root_property, SLOT(generalUpdateCallback()), this),
      slice_height_property_(
          "Slice height", 0.0, "Z-coordinate of the map slice to display.",
          submenu_root_property, SLOT(generalUpdateCallback()), this),
      opacity_property_("Alpha", 1.0, "Opacity of the displayed visuals.",
                        submenu_root_property, SLOT(opacityUpdateCallback()),
                        this) {
  // Initialize the slice cell material
  static int instance_count = 0;
  ++instance_count;
  slice_cell_material_ =
      Ogre::MaterialManager::getSingleton().getByName("rviz/PointCloudBox");
  slice_cell_material_ =
      Ogre::MaterialPtr(slice_cell_material_)
          ->clone("WavemapSliceMaterial_" + std::to_string(instance_count));
  slice_cell_material_->load();
}

SliceVisual::~SliceVisual() {
  // Destroy the frame node
  scene_manager_->destroySceneNode(frame_node_);
}

void SliceVisual::update() {
  if (!visibility_property_.getBool()) {
    clear();
    return;
  }

  // Get a shared-access lock to the map,
  // to ensure it doesn't get written to while we read it
  std::scoped_lock lock(map_and_mutex_->mutex);
  const VolumetricDataStructureBase::ConstPtr map = map_and_mutex_->map;
  if (!map) {
    ROS_INFO("Map is empty. Nothing to draw.");
    return;
  }

  // Constants
  const FloatingPoint min_cell_width = map->getMinCellWidth();
  const FloatingPoint min_occupancy_log_odds =
      min_occupancy_threshold_property_.getFloat();
  const FloatingPoint max_occupancy_log_odds =
      max_occupancy_threshold_property_.getFloat();
  const FloatingPoint slice_height = slice_height_property_.getFloat();
  const FloatingPoint alpha = opacity_property_.getFloat();
  const int max_height = map->getTreeHeight();

  // Cache the intersecting node z-indices in function of node height
  const NdtreeIndexElement num_levels = max_height + 1;
  std::vector<IndexElement> intersecting_indices(num_levels);
  std::generate(intersecting_indices.begin(), intersecting_indices.end(),
                [=, height = 0]() mutable {
                  return static_cast<IndexElement>(
                      std::floor(slice_height / convert::heightToCellWidth(
                                                    min_cell_width, height++)));
                });

  // Add a colored square for each leaf
  std::vector<std::vector<GridLayer::Cell>> cells_per_level(num_levels);
  map->forEachLeaf([=, &cells_per_level](const OctreeIndex& cell_index,
                                         FloatingPoint cell_log_odds) {
    // Skip cells that don't intersect the slice
    if (cell_index.position.z() != intersecting_indices[cell_index.height]) {
      return;
    }

    // Skip cells that don't meet the occupancy threshold
    if (cell_log_odds < min_occupancy_log_odds ||
        max_occupancy_log_odds < cell_log_odds) {
      return;
    }

    // Determine the cell's position
    CHECK_GE(cell_index.height, 0);
    CHECK_LE(cell_index.height, max_height);
    const Point3D cell_center =
        convert::nodeIndexToCenterPoint(cell_index, min_cell_width);

    // Create the cube at the right scale
    auto& cell = cells_per_level[cell_index.height].emplace_back();
    cell.center.x = cell_center[0];
    cell.center.y = cell_center[1];
    cell.center.z = slice_height;

    // Set the cube's color
    const FloatingPoint cell_odds = std::exp(cell_log_odds);
    const FloatingPoint cell_prob = cell_odds / (1.f + cell_odds);
    const FloatingPoint cell_free_prob = 1.f - cell_prob;
    cell.color.a = 1.f;
    cell.color.r = cell_free_prob;
    cell.color.g = cell_free_prob;
    cell.color.b = cell_free_prob;
  });

  // Add a grid layer for each scale level
  for (int height = 0; height <= max_height; ++height) {
    // Allocate the pointcloud representing this grid level if needed
    if (static_cast<int>(grid_levels_.size()) <= height) {
      const Ogre::String name = "multi_res_slice_" + std::to_string(height);
      const FloatingPoint cell_width =
          convert::heightToCellWidth(min_cell_width, height);
      auto& grid_level = grid_levels_.emplace_back(
          std::make_unique<GridLayer>(slice_cell_material_));
      grid_level->setName(name);
      grid_level->setCellDimensions(cell_width, cell_width, 0.0);
      grid_level->setAlpha(alpha, false);
      frame_node_->attachObject(grid_level.get());
    }
    // Update the points
    auto& grid_level = grid_levels_[height];
    grid_level->clear();
    auto& cells_at_level = cells_per_level[height];
    grid_level->addCells(&cells_at_level.front(), cells_at_level.size());
  }
}

// Position and orientation are passed through to the SceneNode
void SliceVisual::setFramePosition(const Ogre::Vector3& position) {
  frame_node_->setPosition(position);
}

void SliceVisual::setFrameOrientation(const Ogre::Quaternion& orientation) {
  frame_node_->setOrientation(orientation);
}

void SliceVisual::opacityUpdateCallback() {
  for (auto& grid_level : grid_levels_) {
    grid_level->setAlpha(opacity_property_.getFloat());
  }
}
}  // namespace wavemap::rviz_plugin
