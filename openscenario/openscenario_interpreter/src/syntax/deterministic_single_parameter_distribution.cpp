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

#include <openscenario_interpreter/reader/element.hpp>
#include <openscenario_interpreter/syntax/deterministic_single_parameter_distribution.hpp>

namespace openscenario_interpreter
{
inline namespace syntax
{
DeterministicSingleParameterDistribution::DeterministicSingleParameterDistribution(
  const pugi::xml_node & node, Scope & scope)
: DeterministicSingleParameterDistributionType(node, scope),
  parameter_name(readAttribute<String>("parameterName", node, scope))
{
}

ParameterDistribution DeterministicSingleParameterDistribution::derive()
{
  ParameterDistribution distribution;
  return apply<ParameterDistribution>(
    [&](auto & unnamed_distribution) {
      ParameterDistribution distribution;
      for (const auto & unnamed_parameter : unnamed_distribution.derive()) {
        distribution.emplace_back(
          std::make_shared<ParameterList>(ParameterList{{parameter_name, unnamed_parameter}}));
      }
      return distribution;
    },
    *this);
}

auto DeterministicSingleParameterDistribution::getNumberOfDeriveScenarios() const -> size_t
{
  return apply<size_t>(
    [](auto & unnamed_distribution) { return unnamed_distribution.getNumberOfDeriveScenarios(); },
    (DeterministicSingleParameterDistributionType &)*this);
}
}  // namespace syntax
}  // namespace openscenario_interpreter
