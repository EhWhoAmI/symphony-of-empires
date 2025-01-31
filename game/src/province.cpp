// Symphony of Empires
// Copyright (C) 2021, Symphony of Empires contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------------
// Name:
//      province.cpp
//
// Abstract:
//      Does some important stuff.
// ----------------------------------------------------------------------------

#include "province.hpp"
#include "world.hpp"
#include "product.hpp"
#include "unit.hpp"
#include "building.hpp"
#include "pop.hpp"
#include <glm/vec3.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <glm/geometric.hpp>

#include "eng3d/utils.hpp"

//
// Province
//
// Calculates the total number of POPs in this province (total population)
float Province::get_attractiveness(const Pop& pop) const {
    float attractive = this->base_attractive;
    const auto& owner = g_world.nations[this->owner_id];
    // A social value between 0 and 1 is for poor people, the value for medium class
    // is between 1 and 2, for the rich is above 2
    if(World::get_instance().pop_types[pop.type_id].social_value >= 0.f && World::get_instance().pop_types[pop.type_id].social_value <= 1.f)
        // For the lower class, lower taxes is good, and so on for other POPs
        attractive += -(owner.current_policy.poor_flat_tax) * 100.f;
    else if(World::get_instance().pop_types[pop.type_id].social_value >= 1.f && World::get_instance().pop_types[pop.type_id].social_value <= 2.f)
        // For the medium class
        attractive += -(owner.current_policy.med_flat_tax) * 100.f;
    else if(World::get_instance().pop_types[pop.type_id].social_value >= 2.f)
        // For the high class
        attractive += -(owner.current_policy.rich_flat_tax) * 100.f;
    return attractive;
}

void Province::add_building(const BuildingType& building_type) {
    // Now build the building
    this->buildings[building_type.get_id()].level += 1;
    this->buildings[building_type.get_id()].req_goods = building_type.req_goods;
}

void Province::cancel_construction_project() {
    for(auto& building : buildings)
        building.working_unit_type_id = UnitTypeId(-1);
}

glm::vec3 Province::get_sphere_coord(glm::vec2 world_size, float radius) const {
    const auto normalized_pos = this->get_pos() / world_size;
    glm::vec2 radiance_pos;
    radiance_pos.x = normalized_pos.x * 2.f * glm::pi<float>();
    radiance_pos.y = normalized_pos.y * glm::pi<float>();

    glm::vec3 sphere_position;
    sphere_position.x = glm::cos(radiance_pos.x) * glm::sin(radiance_pos.y);
    sphere_position.y = glm::sin(radiance_pos.x) * glm::sin(radiance_pos.y);
    sphere_position.z = glm::cos(radiance_pos.y);
    sphere_position *= radius;
    return sphere_position;
}

float Province::euclidean_distance(const Province& other_province, glm::vec2 world_size, float radius) const {
    const auto sphere_coord1 = get_sphere_coord(world_size, radius);
    const auto sphere_coord2 = other_province.get_sphere_coord(world_size, radius);
    const auto cos_angle = glm::dot(sphere_coord1, sphere_coord2) / (radius * radius);
    const auto angle = glm::acos(cos_angle);
    const auto distance = angle * radius;
    return distance;
}

bool Province::is_neighbour(Province& province) const {
    return std::find(this->neighbour_ids.begin(), this->neighbour_ids.end(), province) != this->neighbour_ids.end();
}

void Province::clean_pops() {
    // Remove pops with 0 size and ones that are redundant/duplicated
    for(auto it = pops.begin(); it != pops.end(); ) {
        // Delete ill'formed or invalid pops
        if(!it->size) {
            // Overwrite with last one and remove last one
            *it = pops.back();
            pops.pop_back();
            it = pops.begin();
            continue;
        }

        // Merge duplicate pops
        auto dup_it = std::find(pops.begin(), pops.end(), *it);
        if(dup_it != pops.end() && dup_it != it) {
            it->budget += dup_it->budget;
            it->size += dup_it->size;
            // Overwrite with last one and remove last one
            *it = pops.back();
            pops.pop_back();
            it = pops.begin();
            continue;
        }

        it->size = glm::min<float>(it->size, 10000000.f); // Limit pop size
        it->literacy = glm::min<float>(it->literacy, 1.f); // Limit literacy
        it++;
    }
}

void Province::remove_pop(size_t idx) {
    // Remove pops with 0 size and ones that are redundant/duplicated
    for(auto it = pops.begin(); it != pops.end(); it++) {
        // Delete ill'formed or invalid pops
        if(it == pops.begin() + idx) {
            // Overwrite with last one and remove last one
            *it = pops.back();
            pops.pop_back();
            it = pops.begin();
            return;
        }
    }
}
