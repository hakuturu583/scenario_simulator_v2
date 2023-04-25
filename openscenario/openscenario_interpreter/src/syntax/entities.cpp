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

#include <openscenario_interpreter/reader/attribute.hpp>
#include <openscenario_interpreter/reader/element.hpp>
#include <openscenario_interpreter/syntax/entities.hpp>
#include <openscenario_interpreter/syntax/entity_selection.hpp>
#include <openscenario_interpreter/syntax/scenario_object.hpp>

namespace openscenario_interpreter
{
inline namespace syntax
{
Entities::Entities(const pugi::xml_node & node, Scope & scope)
{
  traverse<0, unbounded>(node, "ScenarioObject", [&](auto && node) {
    entities.emplace(readAttribute<String>("name", node, scope), make<ScenarioObject>(node, scope));
  });

  traverse<0, unbounded>(node, "EntitySelection", [&](auto && node) {
    entities.emplace(readAttribute<String>("name", node, scope), make<EntitySelection>(node, scope));
  });

  scope.global().entities = this;
}

auto Entities::isAdded(const EntityRef & entity_ref) const -> bool
{
  return ref(entity_ref).template as<ScenarioObject>().is_added;
}

auto Entities::ref(const EntityRef & entity_ref) const -> Object
{
  if (auto entry = entities.find(entity_ref); entry == std::end(entities)) {
    throw Error("An undeclared entity ", std::quoted(entity_ref), " was specified in entityRef.");
  } else if (not entry->second.is<ScenarioObject>()) {
    THROW_SEMANTIC_ERROR(
      "For now, access to entities by `Entities::ref` is only allowed for `ScenarioObject`,"
      "while `", entity_ref, "` points a `", makeTypename(entry->second->type().name()),"`."
    );
  } else {
    return entry->second;
  }
}

auto Entities::flatten(const EntityRef & entity_ref) const -> std::list<EntityRef>
{
  auto entity_refs = std::list { entity_ref };
  for (auto iterator = std::begin(entity_refs); iterator != std::end(entity_refs);) {
    if (auto entry = entities.find(*iterator); entry == std::end(entities)) {
      throw Error("An undeclared entity ", std::quoted(entity_ref), " was specified in entityRef.");
    } else if (auto [ key, entity ] = *entry; entity.is<ScenarioObject>()) {
      entity_refs.emplace_back(key);
      iterator = std::next(iterator);
    } else if (entity.is<EntitySelection>()) {
      // TODO: walk selected entities and push them into `entity_refs`
      iterator = entity_refs.erase(iterator);
    } else {
      throw UNSUPPORTED_SETTING_DETECTED(TYPE, makeTypename(entity.type().name()));
    }
  }
  return entity_refs;
}
}  // namespace syntax
}  // namespace openscenario_interpreter
