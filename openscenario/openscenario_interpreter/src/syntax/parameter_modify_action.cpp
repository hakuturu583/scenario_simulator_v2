// Copyright 2015-2021 Tier IV, Inc. All rights reserved.
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

#include <openscenario_interpreter/syntax/parameter_modify_action.hpp>

namespace openscenario_interpreter
{
inline namespace syntax
{
auto ParameterModifyAction::accomplished() noexcept -> bool { return true; }

auto ParameterModifyAction::run() -> void
{
  try {
    const auto target = localScope().findElement(parameter_ref);
    if (rule.is<ParameterAddValueRule>()) {
      rule.as<ParameterAddValueRule>()(target);
    } else {
      rule.as<ParameterMultiplyByValueRule>()(target);
    }
  } catch (const std::out_of_range &) {
    throw SemanticError("No such parameter ", std::quoted(parameter_ref));
  }
}

auto ParameterModifyAction::start() noexcept -> void {}
}  // namespace syntax
}  // namespace openscenario_interpreter
