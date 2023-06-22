#include "wavemap_rviz_plugin/visuals/grid_visual.h"

#include <ros/console.h>
#include <wavemap/data_structure/volumetric/hashed_wavelet_octree.h>
#include <wavemap/indexing/index_conversions.h>

namespace wavemap::rviz_plugin {
GridVisual::GridVisual(
    Ogre::SceneManager* scene_manager, Ogre::SceneNode* parent_node,
    rviz::Property* submenu_root_property,
    const std::shared_ptr<std::mutex> map_mutex,
    const std::shared_ptr<VolumetricDataStructureBase::Ptr> map)
    : map_mutex_(map_mutex),
      map_ptr_(map),
      scene_manager_(CHECK_NOTNULL(scene_manager)),
      frame_node_(CHECK_NOTNULL(parent_node)->createChildSceneNode()),
      visibility_property_(
          "Show", true,
          "Whether to show the octree as a multi-resolution grid.",
          CHECK_NOTNULL(submenu_root_property),
          SLOT(visibilityUpdateCallback()), this),
      min_occupancy_threshold_property_(
          "Min log odds", 1e-3, "Ranges from -Inf to Inf.",
          submenu_root_property, SLOT(thresholdUpdateCallback()), this),
      max_occupancy_threshold_property_(
          "Max log odds", 1e6, "Ranges from -Inf to Inf.",
          submenu_root_property, SLOT(thresholdUpdateCallback()), this),
      opacity_property_("Alpha", 1.0, "Opacity of the displayed visuals.",
                        submenu_root_property, SLOT(opacityUpdateCallback()),
                        this) {
  const std::string kDefaultRvizCamPrefix = "ViewControllerCamera";
  bool success = false;
  for (const auto& [cam_name, cam] : scene_manager_->getCameras()) {
    if (cam_name.find(kDefaultRvizCamPrefix) != std::string::npos) {
      cam->getViewport()->addListener(
          new ViewportCamChangedListener([this](Ogre::Viewport* viewport) {
            if (Ogre::Camera* new_cam = viewport->getCamera(); new_cam) {
              new_cam->addListener(
                  new CamPrerenderListener([this](Ogre::Camera* active_cam) {
                    if (force_lod_update_ ||
                        lod_update_distance_threshold_ <
                            active_cam->getPosition().distance(
                                last_lod_update_position_)) {
                      updateLOD(active_cam);
                      last_lod_update_position_ = active_cam->getPosition();
                      force_lod_update_ = false;
                    }
                    processBlockUpdateQueue();
                  }));
            }
          }));
      success = true;
      break;
    }
  }
  ROS_WARN_STREAM_COND(!success,
                       "Could not register wavemap_rviz_plugin LOD callback. "
                       "Found no camera whose name contains "
                           << kDefaultRvizCamPrefix);
}

GridVisual::~GridVisual() {
  // Destroy the frame node
  scene_manager_->destroySceneNode(frame_node_);
}

void GridVisual::updateMap(bool redraw_all) {
  if (!visibility_property_.getBool()) {
    return;
  }

  // Get a shared-access lock to the map,
  // to ensure it doesn't get written to while we read it
  {
    std::scoped_lock lock(*map_mutex_);
    VolumetricDataStructureBase::ConstPtr map = *map_ptr_;
    if (!map) {
      return;
    }

    // Constants
    const IndexElement tree_height = map->getTreeHeight();
    const FloatingPoint min_cell_width = map->getMinCellWidth();
    const FloatingPoint min_log_odds =
        min_occupancy_threshold_property_.getFloat();
    const FloatingPoint max_log_odds =
        max_occupancy_threshold_property_.getFloat();
    const FloatingPoint alpha = opacity_property_.getFloat();

    const TimePoint start_time = std::chrono::steady_clock::now();

    if (const auto* hashed_map =
            dynamic_cast<const HashedWaveletOctree*>(map.get());
        hashed_map) {
      for (const auto& [block_idx, block] : hashed_map->getBlocks()) {
        // Add all blocks that changed since the last publication time to the
        // queue. Since the queue is stored as a set, there are no duplicates.
        if (redraw_all || last_update_time_ < block.getLastUpdatedStamp()) {
          force_lod_update_ = true;
          constexpr IndexElement kDefaultTermHeight = 0;
          block_update_queue_[block_idx] = kDefaultTermHeight;
        }
      }
    } else {
      const IndexElement num_levels = tree_height + 1;
      PointcloudList cells_per_level(num_levels);
      map->forEachLeaf([&](const auto& cell_index, auto cell_log_odds) {
        getLeafCentersAndColors(tree_height, min_cell_width, min_log_odds,
                                max_log_odds, cell_index, cell_log_odds,
                                cells_per_level);
      });
      const Index3D root_idx = Index3D::Zero();
      drawMultiResGrid(tree_height, min_cell_width, root_idx, alpha,
                       cells_per_level, block_grids_[root_idx]);
    }

    last_update_time_ = start_time;
  }
}

void GridVisual::updateLOD(Ogre::Camera* cam) {
  if (!visibility_property_.getBool()) {
    return;
  }

  // Get a shared-access lock to the map,
  // to ensure it doesn't get written to while we read it
  std::scoped_lock lock(*map_mutex_);
  VolumetricDataStructureBase::ConstPtr map = *map_ptr_;
  if (!map) {
    return;
  }

  const Point3D cam_position = {cam->getPosition().x, cam->getPosition().y,
                                cam->getPosition().z};

  if (const auto* hashed_map =
          dynamic_cast<const HashedWaveletOctree*>(map.get());
      hashed_map) {
    for (const auto& [block_idx, block] : hashed_map->getBlocks()) {
      const IndexElement tree_height = map->getTreeHeight();
      const OctreeIndex block_node_idx =
          convert::indexAndHeightToNodeIndex(block_idx, tree_height);
      const AABB block_aabb =
          convert::nodeIndexToAABB(block_node_idx, map->getMinCellWidth());
      const FloatingPoint distance_to_cam =
          block_aabb.minDistanceTo(cam_position);
      constexpr FloatingPoint kFactor = 0.002f;
      const auto term_height_recommended = std::clamp(
          static_cast<IndexElement>(std::round(std::log2(
              kFactor * distance_to_cam / hashed_map->getMinCellWidth()))),
          0, tree_height - 1);

      if (block_update_queue_.count(block_idx)) {
        block_update_queue_[block_idx] = term_height_recommended;
      } else if (block_grids_.count(block_idx)) {
        const IndexElement term_height_current =
            tree_height - static_cast<int>(block_grids_[block_idx].size()) + 1;
        if (term_height_current != term_height_recommended) {
          block_update_queue_[block_idx] = term_height_recommended;
        }
      }
    }
  }
}

// Position and orientation are passed through to the SceneNode
void GridVisual::setFramePosition(const Ogre::Vector3& position) {
  frame_node_->setPosition(position);
}

void GridVisual::setFrameOrientation(const Ogre::Quaternion& orientation) {
  frame_node_->setOrientation(orientation);
}

void GridVisual::visibilityUpdateCallback() {
  if (visibility_property_.getBool()) {
    updateMap(true);
  } else {
    clear();
  }
}

void GridVisual::opacityUpdateCallback() {
  for (auto& [block_idx, block_grid] : block_grids_) {
    for (auto& grid_level : block_grid) {
      grid_level->setAlpha(opacity_property_.getFloat());
    }
  }
}

void GridVisual::processBlockUpdateQueue() {
  if (!visibility_property_.getBool()) {
    return;
  }

  // Get a shared-access lock to the map,
  // to ensure it doesn't get written to while we read it
  std::scoped_lock lock(*map_mutex_);
  VolumetricDataStructureBase::ConstPtr map = *map_ptr_;
  if (!map) {
    return;
  }

  if (const auto* hashed_map =
          dynamic_cast<const HashedWaveletOctree*>(map.get());
      hashed_map) {
    // Constants
    const FloatingPoint min_cell_width = map->getMinCellWidth();
    const FloatingPoint min_log_odds =
        min_occupancy_threshold_property_.getFloat();
    const FloatingPoint max_log_odds =
        max_occupancy_threshold_property_.getFloat();
    const FloatingPoint alpha = opacity_property_.getFloat();

    // Sort the blocks in the queue by their modification time
    std::map<TimePoint, Index3D> changed_blocks_sorted;
    for (const auto& [block_idx, term_height] : block_update_queue_) {
      const TimePoint& last_modified_time =
          hashed_map->getBlock(block_idx).getLastUpdatedStamp();
      changed_blocks_sorted[last_modified_time] = block_idx;
    }

    // Redraw blocks, starting with the oldest and
    // stopping after kMaxDrawsPerCycle
    int num_draws = 0;
    for (const auto& [_, block_idx] : changed_blocks_sorted) {
      const auto& block = hashed_map->getBlock(block_idx);
      const IndexElement tree_height = map->getTreeHeight();
      const IndexElement term_height = block_update_queue_[block_idx];
      const int num_levels = tree_height + 1 - term_height;
      PointcloudList cells_per_level(num_levels);
      block.forEachLeaf(
          block_idx,
          [&](const auto& cell_index, auto cell_log_odds) {
            getLeafCentersAndColors(tree_height, min_cell_width, min_log_odds,
                                    max_log_odds, cell_index, cell_log_odds,
                                    cells_per_level);
          },
          term_height);
      drawMultiResGrid(tree_height, min_cell_width, block_idx, alpha,
                       cells_per_level, block_grids_[block_idx]);
      block_update_queue_.erase(block_idx);
      if (kMaxDrawsPerCycle <= ++num_draws) {
        break;
      }
    }
  }
}

void GridVisual::getLeafCentersAndColors(int tree_height,
                                         FloatingPoint min_cell_width,
                                         FloatingPoint min_occupancy_log_odds,
                                         FloatingPoint max_occupancy_log_odds,
                                         const OctreeIndex& cell_index,
                                         FloatingPoint cell_log_odds,
                                         PointcloudList& cells_per_level) {
  // Skip cells that don't meet the occupancy threshold
  if (cell_log_odds < min_occupancy_log_odds ||
      max_occupancy_log_odds < cell_log_odds) {
    return;
  }

  // Determine the cell's position
  const IndexElement depth = tree_height - cell_index.height;
  CHECK_GE(depth, 0);
  CHECK_LT(depth, cells_per_level.size());
  const Point3D cell_center =
      convert::nodeIndexToCenterPoint(cell_index, min_cell_width);

  // Create the cube at the right scale
  auto& point = cells_per_level[depth].emplace_back();
  point.position.x = cell_center[0];
  point.position.y = cell_center[1];
  point.position.z = cell_center[2];

  // Set the cube's color
  switch (kColorBy) {
    case ColorBy::kProbability:
      point.color = logOddsToColor(cell_log_odds);
      break;
    case ColorBy::kPosition:
    default:
      point.color = positionToColor(cell_center);
      break;
  }
}

void GridVisual::drawMultiResGrid(IndexElement tree_height,
                                  FloatingPoint min_cell_width,
                                  const Index3D& block_index,
                                  FloatingPoint alpha,
                                  PointcloudList& cells_per_level,
                                  MultiResGrid& multi_res_grid) {
  // Add a grid layer for each scale level
  const std::string prefix =
      "grid_" + std::to_string(Index3DHash()(block_index)) + "_";
  for (size_t depth = 0; depth < cells_per_level.size(); ++depth) {
    // Allocate the pointcloud representing this grid level if needed
    if (multi_res_grid.size() <= depth) {
      const Ogre::String name = prefix + std::to_string(depth);
      const IndexElement height = tree_height - static_cast<int>(depth);
      const FloatingPoint cell_width =
          convert::heightToCellWidth(min_cell_width, height);
      auto& grid_level =
          multi_res_grid.emplace_back(std::make_unique<rviz::PointCloud>());
      grid_level->setName(name);
      grid_level->setRenderMode(rviz::PointCloud::RM_BOXES);
      grid_level->setDimensions(cell_width, cell_width, cell_width);
      grid_level->setAlpha(alpha, false);
      frame_node_->attachObject(grid_level.get());
    }
    // Update the points
    auto& grid_level = multi_res_grid[depth];
    grid_level->clear();
    auto& cells_at_level = cells_per_level[depth];
    grid_level->addPoints(&cells_at_level.front(), cells_at_level.size());
  }
  // Deallocate levels that are no longer needed
  for (size_t depth = multi_res_grid.size() - 1;
       cells_per_level.size() <= depth; --depth) {
    frame_node_->detachObject(multi_res_grid[depth].get());
    multi_res_grid.pop_back();
  }
}

Ogre::ColourValue GridVisual::logOddsToColor(FloatingPoint log_odds) {
  Ogre::ColourValue color;
  color.a = 1.f;

  const FloatingPoint cell_odds = std::exp(log_odds);
  const FloatingPoint cell_prob = cell_odds / (1.f + cell_odds);
  const FloatingPoint cell_free_prob = 1.f - cell_prob;
  color.r = cell_free_prob;
  color.g = cell_free_prob;
  color.b = cell_free_prob;
  return color;
}

// NOTE: This coloring code is based on octomap_mapping, see:
//       https://github.com/OctoMap/octomap_mapping/blob/kinetic-devel/
//       octomap_server/src/OctomapServer.cpp#L1234
Ogre::ColourValue GridVisual::positionToColor(const Point3D& center_point) {
  Ogre::ColourValue color;
  color.a = 1.0;

  // Blend over HSV-values (more colors)
  constexpr FloatingPoint kScaling = 0.2f;
  constexpr FloatingPoint kOffset = -2.f;
  FloatingPoint h = kScaling * center_point.z() + kOffset;
  h -= std::floor(h);
  h *= 6;
  const FloatingPoint s = 1.f;
  const FloatingPoint v = 1.f;

  const int band_idx = std::floor(h);
  FloatingPoint f = h - static_cast<FloatingPoint>(band_idx);
  // Flip f if the band index is even
  if (!(band_idx & 1)) {
    f = 1.f - f;
  }
  const FloatingPoint m = v * (1.f - s);
  const FloatingPoint n = v * (1.f - s * f);

  switch (band_idx) {
    case 6:
    case 0:
      color.r = v;
      color.g = n;
      color.b = m;
      break;
    case 1:
      color.r = n;
      color.g = v;
      color.b = m;
      break;
    case 2:
      color.r = m;
      color.g = v;
      color.b = n;
      break;
    case 3:
      color.r = m;
      color.g = n;
      color.b = v;
      break;
    case 4:
      color.r = n;
      color.g = m;
      color.b = v;
      break;
    case 5:
      color.r = v;
      color.g = m;
      color.b = n;
      break;
    default:
      color.r = 1;
      color.g = 0.5;
      color.b = 0.5;
      break;
  }

  return color;
}
}  // namespace wavemap::rviz_plugin
