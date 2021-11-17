#!/bin/bash

# ========== Functions ==========
function run_all_combinations() {
  for map_resolution in "${map_resolutions[@]}"; do
    # Get the git commit ID (included in the log dir name for future reference)
    git_dir=$(rospack find wavemap_2d)/../.git
    git_commit_id=$(git --git-dir=${git_dir} rev-parse --short --verify HEAD)

    for carmen_log_file_name in "${carmen_log_file_names[@]}"; do
      # Ensure that at most max_num_jobs jobs run in parallel
      while ((${num_jobs@P} >= max_num_jobs)); do
        wait -n
      done

      # Create the log dir and set the log file path
      run_log_dir="${output_log_dir}/${carmen_log_file_name}"
      mkdir -p "${run_log_dir}"
      run_log_file_prefix="${run_log_dir}/${experiment_date}_commit_${git_commit_id}_res_${map_resolution}_"

      # Run the experiments
      carmen_log_file_path="${carmen_log_dir}/${carmen_log_file_name}"
      "${executable}" -alsologtostderr -map_resolution ${map_resolution} -carmen-log-file-path "${carmen_log_file_path}" -output_log_dir "${run_log_file_prefix}" &
    done
  done
  # Wait for everything to finish before proceding
  wait -n
}

# ========== Settings ==========
home_dir="/home/victor"
catkin_ws_dir="${home_dir}/catkin_ws"
carmen_log_dir="${home_dir}/data/2d_carmen_datasets"
output_log_dir="${home_dir}/data/2d_carmen_datasets/output"

executable="${catkin_ws_dir}/devel/lib/wavemap_2d/wavemap_carmen_processor"

declare -a carmen_log_file_names=(
  "aces_publicb.gfs"
  "belgioioso.gfs"
  "csail.corrected"
  "edmonton.gfs"
  "fr079-complete.gfs"
  "fr101.carmen.gfs"
  "fr-campus-20040714.carmen.gfs"
  "intel.gfs"
  "mexico.gfs"
  "MIT_Infinite_Corridor_2002_09_11_same_floor.gfs"
  "orebro.gfs"
  "seattle-r.gfs"
)

# ============ Vars =============
experiment_date=$(date '+%Y-%m-%d-%H-%M-%S')
num_jobs="\j" # The prompt escape to get the number of currently running jobs

# ============ Run =============
declare -a map_resolutions=(0.20 0.1 0.05 0.02)
max_num_jobs=8
run_all_combinations

declare -a map_resolutions=(0.01)
max_num_jobs=2
run_all_combinations
