// Copyright 2015 TIER IV, Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <ament_index_cpp/get_package_share_directory.hpp>
#include <cpp_mock_scenarios/catalogs.hpp>
#include <cpp_mock_scenarios/cpp_scenario_node.hpp>
#include <rclcpp/rclcpp.hpp>
#include <traffic_simulator/api/api.hpp>
#include <traffic_simulator/utils/distance.hpp>

#include <traffic_simulator_msgs/msg/behavior_parameter.hpp>

#include <quaternion_operation/quaternion_operation.h>

#include <memory>
#include <string>
#include <vector>

namespace cpp_mock_scenarios
{
class GetDistanceInLaneCoordinateScenario : public cpp_mock_scenarios::CppScenarioNode
{
public:
  explicit GetDistanceInLaneCoordinateScenario(const rclcpp::NodeOptions & option)
  : cpp_mock_scenarios::CppScenarioNode(
      "get_longitudinal_distance",
      ament_index_cpp::get_package_share_directory("kashiwanoha_map") + "/map", "lanelet2_map.osm",
      __FILE__, false, option)
  {
    start();
  }

private:
  bool requested = false;

  auto getLateralDistance(const std::string & from_entity_name, const std::string & to_entity_name)
    -> std::optional<double>
  {
    const auto from_lanelet_pose_opt = api_.getEntity(from_entity_name)->getLaneletPose();
    const auto to_lanelet_pose_opt = api_.getEntity(to_entity_name)->getLaneletPose();
    if (from_lanelet_pose_opt && to_lanelet_pose_opt) {
      return traffic_simulator::distance::getLateralDistance(
        *from_lanelet_pose_opt, *to_lanelet_pose_opt, false, api_.getHdmapUtils());
    } else {
      return std::nullopt;
    }
  };

  auto getLateralDistance(
    const std::string & from_entity_name, const std::string & to_entity_name,
    const double matching_distance) -> std::optional<double>
  {
    const auto from_lanelet_pose_opt =
      api_.getEntity(from_entity_name)->getLaneletPose(matching_distance);
    const auto to_lanelet_pose_opt =
      api_.getEntity(to_entity_name)->getLaneletPose(matching_distance);
    if (from_lanelet_pose_opt && to_lanelet_pose_opt) {
      return traffic_simulator::distance::getLateralDistance(
        *from_lanelet_pose_opt, *to_lanelet_pose_opt, false, api_.getHdmapUtils());
    } else {
      return std::nullopt;
    }
  };

  auto getLongitudinalDistance(
    const std::string & from_entity_name, const std::string & to_entity_name)
    -> std::optional<double>
  {
    const auto from_lanelet_pose_opt = api_.getEntity(from_entity_name)->getLaneletPose();
    const auto to_lanelet_pose_opt = api_.getEntity(to_entity_name)->getLaneletPose();
    if (from_lanelet_pose_opt && to_lanelet_pose_opt) {
      return traffic_simulator::distance::getLongitudinalDistance(
        *from_lanelet_pose_opt, *to_lanelet_pose_opt, false, true, false, api_.getHdmapUtils());

    } else {
      return std::nullopt;
    }
  }

  void onUpdate() override
  {
    if (api_.getCurrentTime() >= 10.0) {
      stop(cpp_mock_scenarios::Result::SUCCESS);
    }
    const auto distance_to_front = getLongitudinalDistance("ego", "front");
    const auto distance_to_behind = getLongitudinalDistance("ego", "behind");
    const auto lateral_none = getLateralDistance("ego", "front", 0.1);
    const auto lateral_about_one = getLateralDistance("ego", "front", 1.5);
    const auto lateral_to_front = getLateralDistance("ego", "front");
    const auto lateral_to_behind = getLateralDistance("ego", "behind");
    // LCOV_EXCL_START
    if (lateral_none) {
      return stop(cpp_mock_scenarios::Result::FAILURE);
    }
    if (!lateral_about_one) {
      return stop(cpp_mock_scenarios::Result::FAILURE);
    }
    if (lateral_about_one && !equals(lateral_about_one.value(), 1.0, 0.001)) {
      return stop(cpp_mock_scenarios::Result::FAILURE);
    }
    if (lateral_to_front && !equals(lateral_to_front.value(), 1.0)) {
      return stop(cpp_mock_scenarios::Result::FAILURE);
    }
    if (lateral_to_behind && !equals(lateral_to_behind.value(), -1.0)) {
      return stop(cpp_mock_scenarios::Result::FAILURE);
    }
    if (!distance_to_front) {
      stop(cpp_mock_scenarios::Result::FAILURE);
      return;
    }
    if (!lateral_to_behind) {
      stop(cpp_mock_scenarios::Result::FAILURE);
      return;
    }
    if (distance_to_front.value() >= 5.1 || 4.9 >= distance_to_front.value()) {
      stop(cpp_mock_scenarios::Result::FAILURE);
      return;
    }
    if (!distance_to_behind) {
      stop(cpp_mock_scenarios::Result::FAILURE);
      return;
    }
    if (distance_to_behind.value() >= -4.9 || -5.1 >= distance_to_behind.value()) {
      stop(cpp_mock_scenarios::Result::FAILURE);
      return;
    }
    // LCOV_EXCL_STOP
  }
  void onInitialize() override
  {
    api_.spawn(
      "ego",
      api_.canonicalize(traffic_simulator::helper::constructLaneletPose(34513, 5, 0, 0, 0, 0)),
      getVehicleParameters());
    api_.setLinearVelocity("ego", 10);
    api_.requestSpeedChange("ego", 3, true);

    api_.spawn(
      "front",
      api_.canonicalize(traffic_simulator::helper::constructLaneletPose(34513, 10, 1, 0, 0, 0)),
      getVehicleParameters());
    api_.setLinearVelocity("front", 10);
    api_.requestSpeedChange("front", 3, true);

    api_.spawn(
      "behind",
      api_.canonicalize(traffic_simulator::helper::constructLaneletPose(34513, 0, -1, 0, 0, 0)),
      getVehicleParameters());
    api_.setLinearVelocity("behind", 10);
    api_.requestSpeedChange("behind", 3, true);
  }
};
}  // namespace cpp_mock_scenarios

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::NodeOptions options;
  auto component =
    std::make_shared<cpp_mock_scenarios::GetDistanceInLaneCoordinateScenario>(options);
  rclcpp::spin(component);
  rclcpp::shutdown();
  return 0;
}
