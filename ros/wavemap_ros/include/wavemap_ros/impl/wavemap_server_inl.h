#ifndef WAVEMAP_ROS_IMPL_WAVEMAP_SERVER_INL_H_
#define WAVEMAP_ROS_IMPL_WAVEMAP_SERVER_INL_H_

#include <functional>
#include <unordered_set>
#include <utility>
#include <vector>

#include <tracy/Tracy.hpp>
#include <wavemap_msgs/Map.h>
#include <wavemap_ros_conversions/map_msg_conversions.h>

namespace wavemap {
template <typename HashedMapT>
void WavemapServer::publishHashedMap(HashedMapT* hashed_map,
                                     bool republish_whole_map) {
  // Add all blocks that changed since the last publication time to the
  // queue. Since the queue is stored as a set, there are no duplicates.
  const Timestamp start_time = Time::now();
  for (const auto& [block_idx, block] : hashed_map->getBlocks()) {
    if (republish_whole_map ||
        last_map_pub_time_ < block.getLastUpdatedStamp()) {
      block_publishing_queue_.insert(block_idx);
    }
  }
  last_map_pub_time_ = start_time;

  // Make sure that all the blocks in the queue still exist and
  // sort them by their modification time
  std::vector<std::pair<Timestamp, Index3D>> changed_blocks_stamped;
  for (auto block_it = block_publishing_queue_.cbegin();
       block_it != block_publishing_queue_.cend();) {
    const Index3D block_idx = *block_it;
    if (hashed_map->hasBlock(block_idx)) {
      const Timestamp& last_modified_time =
          hashed_map->getBlock(block_idx).getLastUpdatedStamp();
      changed_blocks_stamped.emplace_back(last_modified_time, block_idx);
      ++block_it;
    } else {
      block_it = block_publishing_queue_.erase(block_it);
    }
  }

  // If the number of changed blocks exceeds 'max_num_blocks_per_msg',
  // select the 'max_num_blocks_per_msg' oldest blocks and drop the rest
  if (static_cast<size_t>(config_.max_num_blocks_per_msg) <=
      changed_blocks_stamped.size()) {
    auto nth = changed_blocks_stamped.begin() + config_.max_num_blocks_per_msg;
    std::nth_element(
        changed_blocks_stamped.begin(), nth, changed_blocks_stamped.end(),
        [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });
    changed_blocks_stamped.resize(config_.max_num_blocks_per_msg);
  }

  // Prepare the blocks to publish in the current cycle
  // NOTE: We take the config_.max_num_blocks_per_msg most recently modified
  //       blocks.
  std::unordered_set<Index3D, Index3DHash> blocks_to_publish;
  for (const auto& [_, block_idx] : changed_blocks_stamped) {
    hashed_map->getBlock(block_idx).threshold();
    blocks_to_publish.insert(block_idx);
  }

  // If there are no blocks to publish, we're done
  if (blocks_to_publish.empty()) {
    return;
  }

  // Serialize and publish the selected blocks
  wavemap_msgs::Map map_msg;
  map_msg.header.frame_id = config_.world_frame;
  map_msg.header.stamp = ros::Time::now();
  convert::mapToRosMsg(*hashed_map,
                       map_msg.hashed_wavelet_octree.emplace_back(),
                       blocks_to_publish, thread_pool_);
  {
    ZoneScopedN("publishMapRosMsg");
    map_pub_.publish(map_msg);
  }

  // Remove the published blocks from the publication queue
  std::for_each(
      blocks_to_publish.begin(), blocks_to_publish.end(),
      [&queue = block_publishing_queue_](const auto& published_block) {
        queue.erase(published_block);
      });

  // Handle publishing of the remaining blocks
  if (!block_publishing_queue_.empty()) {
    ROS_WARN_STREAM("Could not publish all blocks at once. Published "
                    << blocks_to_publish.size() << " out of "
                    << config_.max_num_blocks_per_msg
                    << ". Remaining in queue: "
                    << block_publishing_queue_.size());

    // If the map is being published at a fixed rate, wait for the next cycle
    if (0.f < config_.publication_period) {
      return;
    }

    // Otherwise finish publishing it now
    if (republish_whole_map && config_.publication_period < 0.f) {
      do {
        publishHashedMap(hashed_map, false);
      } while (!block_publishing_queue_.empty());
    }
  }
}
}  // namespace wavemap

#endif  // WAVEMAP_ROS_IMPL_WAVEMAP_SERVER_INL_H_
