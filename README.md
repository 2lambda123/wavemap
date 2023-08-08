# Wavemap
![wavemap_cover](https://github.com/ethz-asl/wavemap/assets/6238939/a51bef62-01f3-40f5-a302-cabc59b0eed8)

Wavemap is a hierarchical, multi-resolution occupancy mapping framework. It achieves state-of-the-art memory and computational efficiency by combining Haar wavelet compression and a hierarchical measurement integration scheme. These efficiency improvements make demanding uncertainty-aware sensor models tractable, allowing wavemap to attain exceptionally high recall rates on challenging obstacles such as thin objects.

The framework is very flexible and supports several data structures, integration schemes, measurement models and projection models out of the box. Any number of inputs with potentially different settings and weights can simultaneously be fused into a single map.

We test the code both on Intel and ARM. At the moment, only ROS1 is supported, but we would be interested in adding ROS2 support. Please [reach out to us](https://github.com/ethz-asl/wavemap/issues) if you are interested in collaborating.

## Paper
When using wavemap for research, please cite the following paper [[preprint](https://www.research-collection.ethz.ch/bitstream/handle/20.500.11850/614632/RSS22_WavemapFinalPreprintCompressed.pdf?sequence=1&isAllowed=y)]:


```
@INPROCEEDINGS{reijgwart2023wavemap,
    author = {Reijgwart, Victor and Cadena, Cesar and Siegwart, Roland and Ott, Lionel},
    journal = {Robotics: Science and Systems. Online Proceedings},
    title = {Efficient volumetric mapping of multi-scale environments using wavelet-based compression},
    year = {2023-07},
}
```

<details>
<summary>Abstract</summary>
<br>
Volumetric maps are widely used in robotics due to their desirable properties in applications such as path planning, exploration, and manipulation. Constant advances in mapping technologies are needed to keep up with the improvements in sensor technology, generating increasingly vast amounts of precise measurements. Handling this data in a computationally and memory-efficient manner is paramount to representing the environment at the desired scales and resolutions. In this work, we express the desirable properties of a volumetric mapping framework through the lens of multi-resolution analysis. This shows that wavelets are a natural foundation for hierarchical and multi-resolution volumetric mapping. Based on this insight we design an efficient mapping system that uses wavelet decomposition. The efficiency of the system enables the use of uncertainty-aware sensor models, improving the quality of the maps. Experiments on both synthetic and real-world data provide mapping accuracy and runtime performance comparisons with state-of-the-art methods on both RGB-D and 3D LiDAR data. The framework is open-sourced to allow the robotics community at large to explore this approach.
</details>

Note that the code has significantly been improved since the paper was written. In terms of performance, wavemap now includes multi-threaded measurement integrators and faster, more memory efficient data structures inspired by [OpenVDB](https://github.com/AcademySoftwareFoundation/openvdb).

## Install
To use wavemap with ROS, we recommend using ROS Noetic as installed following the [standard instructions](http://wiki.ros.org/noetic/Installation). Other ROS1 distributions should also work, but have not yet been tested.

Start by installing the necessary system dependencies

```shell script
sudo apt update
sudo apt install git python3-catkin-tools python3-vcstool -y
```

Create a catkin workspace with

```shell script
mkdir -p ~/catkin_ws/src && cd ~/catkin_ws/
source /opt/ros/noetic/setup.sh
catkin init
catkin config --cmake-args -DCMAKE_BUILD_TYPE=Release
```

Clone the code for wavemap and its catkin dependencies

```shell script
# With SSH keys
cd ~/catkin_ws/src
git clone git@github.com:ethz-asl/wavemap.git
vcs import --recursive . --input wavemap/tooling/vcstool/wavemap_ssh.yml
```

<details>
<summary>No ssh keys?</summary>
<br>

```shell
cd ~/catkin_ws/src
git clone https://github.com/ethz-asl/wavemap.git
vcs import --recursive . --input wavemap/tooling/vcstool/wavemap_https.yml
```

</details>

Then install the remaining system dependencies using

```shell script
cd ~/catkin_ws/src
rosdep update
rosdep install --from-paths . --skip-keys="numpy_eigen catkin_boost_python_buildtool" --ignore-src -y
```

Build wavemap's ROS interface and the Rviz plugin used to visualize its maps with

```shell script
cd ~/catkin_ws/
catkin build wavemap_all
```

## Run
### Newer College dataset
The Newer College dataset is available [here](https://ori-drs.github.io/newer-college-dataset/download/). To get the
sharpest maps, we recommend supplying wavemap with a high-rate odometry estimate and turning on its built-in pointcloud
motion undistortion. In our experiments, we got these estimates by modifying FastLIO2 to publish its forward-integrated
IMU poses. If you would like to run FastLIO2 yourself, our public fork
is [available here](https://github.com/ethz-asl/fast_lio). Alternatively, we provide rosbags with pre-recorded odometry
for the Multi-Cam Cloister, Park, Math-easy and Mine-easy
sequences [here](https://drive.google.com/drive/folders/1sTmDBUt97wwE220gVFwCq88JT5IOQlk5).

To run wavemap on the Cloister sequence used in the paper, run

```shell script
roslaunch wavemap_ros newer_college_os0_cloister.launch rosbag_dir:=<path_to_downloaded_dataset_directory>
```

For additional options, please refer to the launch file's documented arguments
[here](ros/wavemap_ros/launch/datasets/newer_college/newer_college_os0_cloister.launch). To experiment with wavemap's configuration, modify [this config file](ros/wavemap_ros/config/ouster_os0.yaml).

### Panoptic mapping dataset
The Panoptic Mapping flat dataset is available [here](https://projects.asl.ethz.ch/datasets/doku.php?id=panoptic_mapping). You can automatically download it using
```shell script
export FLAT_DATA_DIR="/home/$USER/data/panoptic_mapping" # Set to path of your preference
bash <(curl -s https://raw.githubusercontent.com/ethz-asl/panoptic_mapping/3926396d92f6e3255748ced61f5519c9b102570f/panoptic_mapping_utils/scripts/download_flat_dataset.sh)
```

To process it with wavemap, run
```shell script
roslaunch wavemap_ros panoptic_mapping_rgbd_flat.launch base_path:="${FLAT_DATA_DIR}"/flat_dataset/run1
```
To experiment with different wavemap settings, modify [this config file](ros/wavemap_ros/config/panoptic_mapping_rgbd.yaml).

### Your own data
The basic requirements for running wavemap are:
1. an odometry source, and
2. a source of depth camera or 3D LiDAR data, as either depth images or point clouds.

*Instructions coming soon.*

## Contributing
We are extending wavemap's API and invite you to share requests for specific interfaces by opening a [GitHub issue](https://github.com/ethz-asl/wavemap/issues). Additionally, we encourage code merge requests and would be happy to review and help optimize contributed code.

To maintain code quality, we use the pre-commit framework to automatically format, lint and perform basic code checks. You can install pre-commit together with the dependencies required to run all of wavemap's checks with
```shell script
rosrun wavemap_utils install_pre_commit.sh
```

After running the above script, pre-commit will automatically check changed code when it is committed to git. All the checks can also be run manually at any time by calling `pre-commit run --all`.

Wavemap's codebase includes a broad suite of tests. These are run in our Continuous Integration pipeline for active merge requests, [see here](https://github.com/ethz-asl/wavemap/actions/workflows/ci.yml). You can also run the tests locally with
```shell script
rosrun wavemap_utils build_and_test_all.sh
```
