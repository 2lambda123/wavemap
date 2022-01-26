#ifndef WAVEMAP_2D_ROS_WAVEMAP_2D_SERVER_H_
#define WAVEMAP_2D_ROS_WAVEMAP_2D_SERVER_H_

#include <memory>
#include <queue>
#include <string>

#include <glog/logging.h>
#include <ros/ros.h>
#include <sensor_msgs/LaserScan.h>
#include <wavemap_2d/common.h>
#include <wavemap_2d/datastructure/cell.h>
#include <wavemap_2d/datastructure/dense_grid/dense_grid.h>
#include <wavemap_2d/integrator/pointcloud_integrator.h>
#include <wavemap_2d_msgs/FilePath.h>

#include "wavemap_2d_ros/tf_transformer.h"

namespace wavemap_2d {

class Wavemap2DServer {
 public:
  struct Config {
    float map_resolution = 0.f;

    std::string world_frame = "odom";

    std::string pointcloud_topic_name = "scan";
    int pointcloud_topic_queue_length = 10;

    float map_visualization_period_s = 10.f;

    float map_autosave_period_s = -1.f;
    std::string map_autosave_path;

    float pointcloud_queue_processing_period_s = 0.1f;
    float pointcloud_queue_max_wait_for_transform_s = 1.f;

    static Config fromRosParams(ros::NodeHandle nh);
    bool isValid(bool verbose = true);
  };

  Wavemap2DServer(ros::NodeHandle nh, ros::NodeHandle nh_private)
      : Wavemap2DServer(nh, nh_private, Config::fromRosParams(nh_private)) {}
  Wavemap2DServer(ros::NodeHandle nh, ros::NodeHandle nh_private,
                  Config config);

  void pointcloudCallback(const sensor_msgs::LaserScan& scan_msg);
  bool saveMapCallback(wavemap_2d_msgs::FilePath::Request& request,
                       wavemap_2d_msgs::FilePath::Response& response) {
    response.success = saveMap(request.file_path);
    return true;
  }
  bool loadMapCallback(wavemap_2d_msgs::FilePath::Request& request,
                       wavemap_2d_msgs::FilePath::Response& response) {
    response.success = loadMap(request.file_path);
    return true;
  }
  bool evaluateMapCallback(wavemap_2d_msgs::FilePath::Request& request,
                           wavemap_2d_msgs::FilePath::Response& response) {
    response.success = evaluateMap(request.file_path);
    return true;
  }

  void visualizeMap();
  bool saveMap(const std::string& file_path) {
    return !occupancy_map_->empty() &&
           occupancy_map_->save(file_path, kSaveWithFloatingPointPrecision);
  }
  bool loadMap(const std::string& file_path) {
    return !occupancy_map_->empty() &&
           occupancy_map_->save(file_path, kSaveWithFloatingPointPrecision);
  }
  bool evaluateMap(const std::string& file_path);

 protected:
  using DataStructureType = DenseGrid<SaturatingCell<>>;
  using MeasurementModelType = BeamModel;
  static constexpr bool kSaveWithFloatingPointPrecision = true;

  Config config_;

  std::shared_ptr<DataStructureType> occupancy_map_;
  std::shared_ptr<MeasurementModelType> measurement_model_;
  std::shared_ptr<PointcloudIntegrator> pointcloud_integrator_;
  TfTransformer transformer_;

  std::queue<sensor_msgs::LaserScan> pointcloud_queue_;
  void processPointcloudQueue();

  ros::Timer pointcloud_queue_processing_timer_;
  ros::Timer map_visualization_timer_;
  ros::Timer map_autosave_timer_;
  ros::Subscriber pointcloud_sub_;
  ros::ServiceServer save_map_srv_;
  ros::ServiceServer load_map_srv_;
  ros::ServiceServer evaluate_map_srv_;
  void subscribeToTimers(ros::NodeHandle nh);
  void subscribeToTopics(ros::NodeHandle nh);
  void advertiseTopics(ros::NodeHandle nh_private);
  void advertiseServices(ros::NodeHandle nh_private);
};
}  // namespace wavemap_2d

#endif  // WAVEMAP_2D_ROS_WAVEMAP_2D_SERVER_H_
