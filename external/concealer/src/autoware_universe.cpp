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

#include <random>
#include <concealer/autoware_universe.hpp>

namespace concealer
{
AutowareUniverse::AutowareUniverse()
: getCommand("/control/command/control_cmd", rclcpp::QoS(1), *this),
  getGearCommandImpl("/control/command/gear_cmd", rclcpp::QoS(1), *this),
  getTurnIndicatorsCommand("/control/command/turn_indicators_cmd", rclcpp::QoS(1), *this),
  getPathWithLaneId(
    "/planning/scenario_planning/lane_driving/behavior_planning/path_with_lane_id", rclcpp::QoS(1),
    *this),
  setAcceleration("/localization/acceleration", *this),
  setOdometry("/localization/kinematic_state", *this),
  setSteeringReport("/vehicle/status/steering_status", *this),
  setGearReport("/vehicle/status/gear_status", *this),
  setControlModeReport("/vehicle/status/control_mode", *this),
  setVelocityReport("/vehicle/status/velocity_status", *this),
  setTurnIndicatorsReport("/vehicle/status/turn_indicators_status", *this),
  // Autoware.Universe requires localization topics to send data at 50Hz
  localization_update_timer(rclcpp::create_timer(
    this, get_clock(), std::chrono::milliseconds(20), [this]() { updateLocalization(); })),
  // Autoware.Universe requires vehicle state topics to send data at 30Hz
  vehicle_state_update_timer(rclcpp::create_timer(
    this, get_clock(), std::chrono::milliseconds(33), [this]() { updateVehicleState(); })),
  localization_and_vehicle_state_update_thread(std::thread([this]() {
    try {
      while (rclcpp::ok() and not is_stop_requested.load()) {
        rclcpp::spin_some(get_node_base_interface());
      }
    } catch (...) {
      thrown = std::current_exception();
      is_thrown.store(true);
    }
  }))
{
}

AutowareUniverse::~AutowareUniverse() { stopAndJoin(); }

auto AutowareUniverse::rethrow() -> void
{
  if (is_thrown.load()) {
    throw thrown;
  }
}

auto AutowareUniverse::stopAndJoin() -> void
{
  is_stop_requested.store(true);
  localization_and_vehicle_state_update_thread.join();
}

auto AutowareUniverse::getAcceleration() const -> double
{
  return getCommand().longitudinal.acceleration;
}

auto AutowareUniverse::getVelocity() const -> double { return getCommand().longitudinal.velocity; }

auto AutowareUniverse::getSteeringAngle() const -> double
{
  return getCommand().lateral.steering_tire_angle;
}

auto AutowareUniverse::updateLocalization() -> void
{
    // Add noise to the current_pose with std_dev that are statistics based on the AWSIM vlp16.
    constexpr uint_fast32_t seed = 1;
    static std::mt19937 rand_engine(seed);
    // localization noise should not be updated every time, so A changing probability is set to 0.01.
    double noise_update_probability = 0.01;
    // yaw noise affacts detected_objects' dummy point cloud, set yaw noise to 0.0 temporarily.
    double std_dev_x = 0.03, std_dev_y = 0.008, std_dev_yaw = 0.0; //std_dev_yaw = 0.04
    
    std::uniform_real_distribution<double> distribution_update(0.0, 1.0);

    if (distribution_update(rand_engine) < noise_update_probability){
      std::normal_distribution<double> distribution_x(0.0, std_dev_x);
      noise_x.store(distribution_x(rand_engine));
    }

    if (distribution_update(rand_engine) < noise_update_probability){
      std::normal_distribution<double> distribution_y(0.0, std_dev_y);
      noise_y.store(distribution_y(rand_engine));
    }
    if (distribution_update(rand_engine) < noise_update_probability){
      std::normal_distribution<double> distribution_yaw(0.0, std_dev_yaw);
      noise_yaw.store(distribution_yaw(rand_engine));
    }

    auto noised_pose = current_pose.load();
    // Add noise to the position
    noised_pose.position.x += noise_x.load();
    noised_pose.position.y += noise_y.load();
    // Add noise to the orientation
    tf2::Quaternion current_orientation;
    tf2::convert(noised_pose.orientation, current_orientation);
    current_orientation *= tf2::Quaternion(tf2::Vector3(0, 0, 1), noise_yaw.load());
    tf2::convert(current_orientation, noised_pose.orientation);

  setAcceleration([this]() {
    geometry_msgs::msg::AccelWithCovarianceStamped message;
    message.header.stamp = get_clock()->now();
    message.header.frame_id = "/base_link";
    message.accel.accel = current_acceleration.load();
    message.accel.covariance.at(6 * 0 + 0) = 0.001;  // linear x
    message.accel.covariance.at(6 * 1 + 1) = 0.001;  // linear y
    message.accel.covariance.at(6 * 2 + 2) = 0.001;  // linear z
    message.accel.covariance.at(6 * 3 + 3) = 0.001;  // angular x
    message.accel.covariance.at(6 * 4 + 4) = 0.001;  // angular y
    message.accel.covariance.at(6 * 5 + 5) = 0.001;  // angular z
    return message;
  }());

  setOdometry([&]() {
    nav_msgs::msg::Odometry message;
    message.header.stamp = get_clock()->now();
    message.header.frame_id = "map";
    message.pose.pose = noised_pose;
    message.pose.covariance = {};
    message.twist.twist = current_twist.load();
    return message;
  }());

  setTransform(noised_pose);
}

auto AutowareUniverse::updateVehicleState() -> void
{
  setControlModeReport([this]() {
    autoware_vehicle_msgs::msg::ControlModeReport message;
    message.mode = current_control_mode.load();
    return message;
  }());

  setGearReport([this]() {
    autoware_vehicle_msgs::msg::GearReport message;
    message.stamp = get_clock()->now();
    message.report = getGearCommand().command;
    return message;
  }());

  setSteeringReport([this]() {
    autoware_vehicle_msgs::msg::SteeringReport message;
    message.stamp = get_clock()->now();
    message.steering_tire_angle = getSteeringAngle();
    return message;
  }());

  setVelocityReport([this]() {
    const auto twist = current_twist.load();
    autoware_vehicle_msgs::msg::VelocityReport message;
    message.header.stamp = get_clock()->now();
    message.header.frame_id = "base_link";
    message.longitudinal_velocity = twist.linear.x;
    message.lateral_velocity = twist.linear.y;
    message.heading_rate = twist.angular.z;
    return message;
  }());

  setTurnIndicatorsReport([this]() {
    autoware_vehicle_msgs::msg::TurnIndicatorsReport message;
    message.stamp = get_clock()->now();
    message.report = getTurnIndicatorsCommand().command;
    return message;
  }());
}

auto AutowareUniverse::getGearCommand() const -> autoware_vehicle_msgs::msg::GearCommand
{
  return getGearCommandImpl();
}

auto AutowareUniverse::getGearSign() const -> double
{
  using autoware_vehicle_msgs::msg::GearCommand;
  // @todo Add support for GearCommand::NONE to return 0.0
  // @sa https://github.com/autowarefoundation/autoware.universe/blob/main/simulator/simple_planning_simulator/src/simple_planning_simulator/simple_planning_simulator_core.cpp#L475
  return getGearCommand().command == GearCommand::REVERSE or
             getGearCommand().command == GearCommand::REVERSE_2
           ? -1.0
           : 1.0;
}

auto AutowareUniverse::getVehicleCommand() const
  -> std::tuple<autoware_control_msgs::msg::Control, autoware_vehicle_msgs::msg::GearCommand>
{
  return std::make_tuple(getCommand(), getGearCommand());
}

auto AutowareUniverse::getRouteLanelets() const -> std::vector<std::int64_t>
{
  std::vector<std::int64_t> ids{};
  for (const auto & point : getPathWithLaneId().points) {
    std::copy(point.lane_ids.begin(), point.lane_ids.end(), std::back_inserter(ids));
  }
  return ids;
}

auto AutowareUniverse::setManualMode() -> void
{
  current_control_mode.store(autoware_vehicle_msgs::msg::ControlModeReport::MANUAL);
}

auto AutowareUniverse::setAutonomousMode() -> void
{
  current_control_mode.store(autoware_vehicle_msgs::msg::ControlModeReport::AUTONOMOUS);
}
}  // namespace concealer
