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
//      client/map.cpp
//
// Abstract:
//      Does some important stuff.
// ----------------------------------------------------------------------------

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <mutex>
#include <memory>
#include <unordered_set>
#include <glm/gtc/matrix_transform.hpp>

#ifdef E3D_BACKEND_OPENGL
// Required before GL/gl.h
#   include <GL/glew.h>
#   include <GL/gl.h>
#elif defined E3D_BACKEND_GLES
#   include <GLES3/gl3.h>
#endif

#include "eng3d/state.hpp"
#include "eng3d/ui/tooltip.hpp"
#include "eng3d/font_sdf.hpp"
#include "eng3d/texture.hpp"
#include "eng3d/primitive.hpp"
#include "eng3d/framebuffer.hpp"
#include "eng3d/model.hpp"
#include "eng3d/serializer.hpp"
#include "eng3d/string.hpp"
#include "eng3d/log.hpp"
#include "eng3d/orbit_camera.hpp"
#include "eng3d/flat_camera.hpp"
#include "eng3d/camera.hpp"
#include "eng3d/rivers.hpp"
#include "eng3d/borders.hpp"

#include "client/map.hpp"
#include "client/map_render.hpp"
#include "client/game_state.hpp"
#include "client/interface/province_view.hpp"
#include "client/interface/unit_widget.hpp"
#include "client/interface/battle_widget.hpp"
#include "client/interface/lobby.hpp"
#include "world.hpp"
#include "province.hpp"
#include "client/client_network.hpp"
#include "action.hpp"

template<>
struct std::hash<ProvinceId> {
    std::size_t operator()(const ProvinceId& id) const noexcept {
        return std::hash<size_t>{}(static_cast<size_t>(id));
    }
};

template<>
struct std::equal_to<ProvinceId> {
    constexpr bool operator()(const ProvinceId& a, const ProvinceId& b) const {
        return a == b;
    }
};

static inline void get_blob_bounds(std::vector<bool>& visited_provinces, const Nation& nation, const Province& province, glm::vec2* min_x, glm::vec2* min_y, glm::vec2* max_x, glm::vec2* max_y) {
    visited_provinces[province] = true;
    // Iterate over all neighbours
    for(const auto neighbour_id : province.neighbour_ids) {
        auto& neighbour = g_world.provinces[neighbour_id];
        if(visited_provinces[neighbour_id]) continue;
        visited_provinces[neighbour_id] = true;
        if(neighbour.controller_id != nation) continue;
        
        // Big provinces not taken in account
        if(glm::abs(neighbour.box_area.left - neighbour.box_area.right) >= g_world.width / 3.f) continue;
        if(glm::abs(neighbour.box_area.top - neighbour.box_area.bottom) >= g_world.height / 3.f) continue;

        if(neighbour.box_area.left < min_x->x) {
            min_x->x = neighbour.box_area.left;
            min_x->y = neighbour.box_area.top;
        }
        if(neighbour.box_area.top < min_y->y) {
            min_y->x = neighbour.box_area.left;
            min_y->y = neighbour.box_area.top;
        }
        if(neighbour.box_area.right > max_x->x) {
            max_x->x = neighbour.box_area.right;
            max_x->y = neighbour.box_area.bottom;
        }
        if(neighbour.box_area.bottom > max_y->y) {
            max_y->x = neighbour.box_area.right;
            max_y->y = neighbour.box_area.bottom;
        }
        get_blob_bounds(visited_provinces, nation, neighbour, min_x, min_y, max_x, max_y);
    }
}

Map::Map(GameState& _gs, const World& _world, UI::Group* _map_ui_layer, int screen_width, int screen_height)
    : gs{ _gs },
    world{ _world },
    map_ui_layer{ _map_ui_layer },
    skybox(0.f, 0.f, 0.f, 255.f * 10.f, 40, false),
    camera{ std::make_unique<Eng3D::FlatCamera>(glm::vec2(screen_width, screen_height), glm::vec2(this->gs.world->width, this->gs.world->height)) },
    map_font{ std::make_unique<Eng3D::FontSDF>("fonts/cinzel_sdf/cinzel") }
{
    if(this->gen_labels)
        this->create_labels();
    map_render = std::make_unique<MapRender>(this->gs, *this);

    // Set the mapmode
    extern std::vector<ProvinceColor> terrain_map_mode(const World& world);
    this->set_map_mode(terrain_map_mode, empty_province_tooltip);
    Eng3D::Log::debug("game", "Preloading-important stuff");

    line_tex = gs.tex_man.load(gs.package_man.get_unique("gfx/line_target.png"));
    skybox_tex = gs.tex_man.load(gs.package_man.get_unique("gfx/space.png"));

    // Query the initial nation flags
    nation_flags.resize(this->gs.world->nations.size(), gs.tex_man.get_white());
    for(const auto& building_type : this->gs.world->building_types) {
        const auto path = string_format("models/building_types/%s.obj", building_type.ref_name.c_str());
        building_type_models.push_back(gs.model_man.load(gs.package_man.get_unique(path)));
        building_type_icons.push_back(gs.tex_man.get_white());
    }
    for(const auto& unit_type : this->gs.world->unit_types) {
        const auto path = string_format("models/unit_types/%s.obj", unit_type.ref_name.c_str());
        unit_type_models.push_back(gs.model_man.load(gs.package_man.get_unique(path)));
        unit_type_icons.push_back(gs.tex_man.get_white());
    }

    // Create tooltip for hovering on the map
    this->tooltip = new UI::Tooltip();
    this->reload_shaders();

    this->unit_widgets.resize(this->gs.world->provinces.size());
    this->battle_widgets.resize(this->gs.world->provinces.size());
    for(size_t i = 0; i < this->gs.world->provinces.size(); i++) {
        this->unit_widgets[i] = new Interface::UnitWidget(*this, this->gs, this->map_ui_layer);
        this->battle_widgets[i] = new Interface::BattleWidget(*this, this->map_ui_layer);
    }
}

Map::~Map() {
    if(this->tooltip != nullptr)
        delete this->tooltip;
}

void Map::update_nation_label(const Nation& nation) {
    // No need to update if no labels are displayed!
    if(!this->gen_labels) return;

    glm::vec2 min_point_x(this->gs.world->width - 1.f, this->gs.world->height - 1.f), min_point_y(this->gs.world->width - 1.f, this->gs.world->height - 1.f);
    glm::vec2 max_point_x(0.f, 0.f), max_point_y(0.f, 0.f);
    if(nation.owned_provinces.empty()) return;
    auto province_id = *nation.owned_provinces.begin();
    if(Province::is_valid(nation.capital_id))
        province_id = nation.capital_id;

    const auto& province = this->gs.world->provinces[province_id];
    max_point_x = province.box_area.position() + province.box_area.size();
    max_point_y = province.box_area.position() + province.box_area.size();
    min_point_x = province.box_area.position();
    min_point_y = province.box_area.position();
    std::vector<bool> visited_provinces(this->world.provinces.size(), false);
    get_blob_bounds(visited_provinces, nation, province, &min_point_x, &min_point_y, &max_point_x, &max_point_y);
    glm::vec2 lab_min = (max_point_x + max_point_y + min_point_y) / 3.f;
    glm::vec2 lab_max = (max_point_y + min_point_x + min_point_y) / 3.f;
    glm::vec2 mid_point = (lab_min + lab_max) / 2.f;
    glm::vec3 center(mid_point, 0.f);
    glm::vec2 x_step(lab_max.x - mid_point.x, 0.f);
    glm::vec3 left(mid_point - x_step, 0.f);
    glm::vec3 right(mid_point + x_step, 0.f);
    auto width = glm::length(left - right) * 1.2f;
    auto right_dir = glm::vec3(mid_point.x + 1.f, mid_point.y, 0.f) - center;
    auto top_dir = glm::vec3(mid_point.x, mid_point.y - 1.f, 0.f) - center;
    glm::vec3 normal = glm::cross(top_dir, right_dir);
    normal = glm::normalize(normal);
    auto angle = glm::atan(lab_max.y - lab_min.y, lab_max.x - lab_min.x);
    if(angle > (glm::pi<float>() / 2.0f)) angle -= glm::pi<float>();
    else if(angle < -(glm::pi<float>() / 2.0f)) angle += glm::pi<float>();
    auto rot = glm::rotate(glm::mat4(1.), angle, normal);
    top_dir = rot * glm::vec4(top_dir, 1.);
    right_dir = rot * glm::vec4(right_dir, 1.);
    
    // Replace old label
    assert(this->nation_labels.size() > nation);
    auto label = this->map_font->gen_text(nation.get_client_hint().alt_name, top_dir, right_dir, width, center);
    this->nation_labels[nation] = std::move(label);
}

void Map::create_labels() {
#ifndef E3D_TARGET_SWITCH
    // Provinces
    this->province_labels.clear();
    for(const auto& province : this->gs.world->provinces) {
        glm::vec2 min_point(province.box_area.left, province.box_area.top);
        glm::vec2 max_point(province.box_area.right, province.box_area.bottom);
        glm::vec2 mid_point = 0.5f * (min_point + max_point);
        glm::vec2 x_step(glm::min<float>(max_point.x - mid_point.x, 250.f), 0);
        glm::vec3 center(mid_point, 0.f);
        glm::vec3 left(mid_point - x_step, 0.f);
        glm::vec3 right(mid_point + x_step, 0.f);
        float width = glm::length(left - right);
        width *= 0.2f;

        auto right_dir = glm::vec3(mid_point.x + 1.f, mid_point.y, 0.) - center;
        auto top_dir = glm::vec3(mid_point.x, mid_point.y - 1.f, 0.) - center;
        center.z -= 0.1f;
        auto label = this->map_font->gen_text(province.name, top_dir, right_dir, width, center);
        this->province_labels.push_back(std::move(label));
    }
#endif
    this->nation_labels.resize(this->gs.world->nations.size()); // Nations
    for(const auto& nation : this->gs.world->nations)
        this->update_nation_label(nation);
}

void Map::set_view(MapView view) {
    view_mode = view;
    if(view == MapView::PLANE_VIEW)
        camera.reset(new Eng3D::FlatCamera(*camera));
    else if(view == MapView::SPHERE_VIEW)
        camera.reset(new Eng3D::OrbitCamera(*camera, Eng3D::GLOBE_RADIUS));
}

std::string political_province_tooltip(const World& world, const ProvinceId id) {
    std::string end_str;
    if(Nation::is_valid(world.provinces[id].controller_id))
        end_str += world.nations[world.provinces[id].controller_id].client_username;
    if(((GameState&)Eng3D::State::get_instance()).editor)
        end_str += string_format("(%s)", world.provinces[id].ref_name.c_str());
    return end_str;
}

// The standard map mode with each province color = country color
std::vector<ProvinceColor> political_map_mode(const World& world) {
    std::vector<ProvinceColor> province_color;
    for(const auto& province : world.provinces) {
        if(Nation::is_invalid(province.controller_id))
            province_color.emplace_back(province, Eng3D::Color::rgba32(0xffdddddd));
        else
            province_color.emplace_back(province, Eng3D::Color::rgba32(world.nations[province.controller_id].get_client_hint().color));
    }
    return province_color;
}

std::string empty_province_tooltip(const World&, const ProvinceId) {
    return "";
}

void Map::reload_shaders() {
    // Shader used for drawing the models using custom model render
    obj_shader = std::make_unique<Eng3D::OpenGL::Program>();
    {
        obj_shader->attach_shader(*gs.builtin_shaders["vs_3d"].get());
        obj_shader->attach_shader(*gs.builtin_shaders["fs_3d"].get());
        obj_shader->attach_shader(*gs.builtin_shaders["fs_lib"].get());
        obj_shader->link();
    }

    tree_shder = std::make_unique<Eng3D::OpenGL::Program>();
    {
        tree_shder->attach_shader(*gs.builtin_shaders["vs_tree"].get());
        tree_shder->attach_shader(*gs.builtin_shaders["fs_tree"].get());
        tree_shder->attach_shader(*gs.builtin_shaders["fs_lib"].get());
        tree_shder->link();
    }

    map_render->reload_shaders();
    if(this->map_render->options.trees.used) {
        for(const auto& terrain_type : world.terrain_types) {
            const auto path = string_format("models/trees/%s.fbx", terrain_type.ref_name.c_str());
            tree_type_models.push_back(gs.model_man.load(gs.package_man.get_unique(path)));
        }

        size_t n_provinces = 0;
        tree_spawn_pos.reserve(world.provinces.size());
        for(const auto& province : world.provinces) {
            if(!world.terrain_types[province.terrain_type_id].is_water_body) {
                tree_spawn_pos.push_back(province.get_pos());
                n_provinces++;
            }
        }

        for(auto& tree_type_model : tree_type_models)
            for(auto& simple_model : tree_type_model->simple_models)
                simple_model.instancing(*tree_spawn_pos.data(), n_provinces);
    }
}

void Map::set_selection(selector_func _selector) {
    selector = _selector;
}

void Map::set_map_mode(mapmode_generator mapmode_generator, mapmode_tooltip tooltip_generator) {
    mapmode_func = mapmode_generator;
    mapmode_tooltip_func = tooltip_generator;
    update_mapmode();
}

void Map::set_selected_province(bool selected, ProvinceId id) {
    this->province_selected = selected;
    this->selected_province_id = id;
    map_render->request_update_visibility();
}

void Map::draw_flag(const Eng3D::OpenGL::Program& shader, const Nation& nation) {
    // Draw a flag that "waves" with some cheap wind effects it
    // looks nice and it's super cheap to make - only using sine
    const float n_steps = 8.f; // Resolution of flag in one side (in vertices)
    const float step = 90.f; // Steps per vertice
    auto flag = Eng3D::Mesh<glm::vec3, glm::vec2>(Eng3D::MeshMode::TRIANGLE_STRIP);
    for(float r = 0.f; r <= (n_steps * step); r += step) {
        float sin_r = (sin(r + wind_osc) / 24.f);
        sin_r = sin(r + wind_osc) / 24.f;
        flag.buffer.push_back(Eng3D::MeshData<glm::vec3, glm::vec2>(
            glm::vec3(((r / step) / n_steps) * 1.5f, sin_r, -2.f),
            glm::vec2((r / step) / n_steps, 0.f)
        ));
        sin_r = sin(r + wind_osc + 160.f) / 24.f;
        flag.buffer.push_back(Eng3D::MeshData<glm::vec3, glm::vec2>(
            glm::vec3(((r / step) / n_steps) * 1.5f, sin_r, -1.f),
            glm::vec2((r / step) / n_steps, 1.f)
        ));
    }
    flag.upload();
    shader.set_texture(0, "diffuse_map", *nation_flags[this->gs.world->get_id(nation)]);
    flag.draw();
}

#if 0
void Map::update(const SDL_Event& event, Input& input, UI::Context* ui_ctx) {
    auto& mouse_pos = input.mouse_pos;
    switch(event.type) {
    case SDL_JOYBUTTONUP:
        is_drag = false;
    case SDL_JOYBUTTONDOWN:
        if(event.jbutton.button == gs.zoomin_button_num) {
            camera->move(0.f, 0.f, -1.f * gs.delta_time * 120.f);
        } else if(event.jbutton.button == gs.zoomout_button_num) {
            camera->move(0.f, 0.f, 1.f * gs.delta_time * 120.f);
        }
        break;
    case SDL_JOYAXISMOTION: {
        if(event.jaxis.which == gs.map_movement_axis_num) {
            if(event.jaxis.axis == 0) {
                const float x_force = event.jaxis.value / Eng3D::State::JOYSTICK_DEAD_ZONE;
                camera->move(x_force, 0.f, 0.f);
            } else if(event.jaxis.axis == 1) {
                const float y_force = event.jaxis.value / Eng3D::State::JOYSTICK_DEAD_ZONE;
                camera->move(0.f, y_force, 0.f);
            }
        }
    } break;
    }
}
#endif

// Updates the province color texture with the changed provinces
void Map::update_mapmode() {
    std::vector<ProvinceColor> province_colors = mapmode_func(*this->gs.world);
    map_render->update_mapmode(province_colors);
}

void Map::draw() {
    const auto projection = camera->get_projection();
    const auto view = camera->get_view();
    glm::mat4 base_model(1.f);

    map_render->draw(*camera, view_mode);
    // rivers->draw(*camera);
    // borders->draw(*camera);
    
    /// @todo We need to better this
    obj_shader->use();
    obj_shader->set_uniform("projection", projection);
    obj_shader->set_uniform("view", view);

    const auto map_pos = camera->get_map_pos();
    const auto distance_to_map = map_pos.z / this->gs.world->width;
    constexpr auto small_zoom_factor = 0.07f;
    if(distance_to_map < small_zoom_factor) {
        // Properly display textures :)
        std::vector<float> province_units_y(this->gs.world->provinces.size(), 0.f);
        // Display units that aren't on battles
        for(auto& province : this->gs.world->provinces) {
            this->unit_widgets[province]->is_render = false;
            this->battle_widgets[province]->is_render = false;
            if((this->map_render->get_province_opt(province) & 0x000000ff) != 0x000000ff)
                continue;

            const auto prov_pos = province.get_pos();
            if(this->view_mode == MapView::SPHERE_VIEW) {
                const auto& orbit_camera = static_cast<const Eng3D::OrbitCamera&>(*camera);
                const auto cam_pos = camera->get_world_pos();
                const auto world_pos = camera->get_tile_world_pos(prov_pos);
                const auto world_to_camera = glm::normalize(cam_pos - world_pos) * orbit_camera.radius * 0.001f;
                // If outside our range of view we just don't evaluate this province
                if(glm::length(world_pos + world_to_camera) < orbit_camera.radius)
                    continue;
            }

            // Units
            const auto& province_units = this->gs.world->unit_manager.get_province_units(province);
            if(!province_units.empty()) {
                size_t total_stack_size = 0; // Calculate the total size of our stack
                for(const auto unit_id : province_units) {
                    const auto& unit = this->gs.world->unit_manager.units[unit_id];
                    total_stack_size += unit.size;
                }

                // Get first/topmost unit
                auto& unit = gs.world->unit_manager.units[province_units[0]];
                // Display unit only if not on a battle
                if(!unit.on_battle) {
                    this->unit_widgets[province]->set_unit(unit);
                    this->unit_widgets[province]->set_size(total_stack_size);
                    this->unit_widgets[province]->is_render = true;

                }
            }

            // Battles
            if(!province.battles.empty()) {
                this->battle_widgets[province]->set_battle(province, 0);
                this->battle_widgets[province]->is_render = true;
                /// @todo Display two opposing units
            } else {
                // Display a single standing unit
                if(this->map_render->options.units.used && !province_units.empty()) {
                    const auto& unit = this->gs.world->unit_manager.units[province_units[0]];
                    auto model = glm::translate(base_model, glm::vec3(prov_pos.x, prov_pos.y, 0.f));
                    if(Province::is_valid(unit.get_target_province_id())) {
                        const auto& unit_target = this->gs.world->provinces[unit.get_target_province_id()];
                        const auto target_pos = unit_target.get_pos();
                        const auto dist = glm::sqrt(glm::pow(glm::abs(prov_pos.x - target_pos.x), 2.f) + glm::pow(glm::abs(prov_pos.y - target_pos.y), 2.f));
                        const auto line_model = glm::rotate(model, glm::atan(target_pos.y - prov_pos.y, target_pos.x - prov_pos.x), glm::vec3(0.f, 0.f, 1.f));
                        obj_shader->set_texture(0, "diffuse_map", *line_tex);
                        obj_shader->set_uniform("model", line_model);
                        Eng3D::Square(0.f, 0.f, dist, 0.5f).draw();
                    }
                    model = glm::rotate(model, glm::radians(-90.f), glm::vec3(1.f, 0.f, 0.f));
                    obj_shader->set_uniform("model", model);
                    unit_type_models[unit.type_id]->draw(*obj_shader);
                }
            }
        }

        // Unit movement lines
        gs.world->unit_manager.for_each_unit([this](Unit& unit) {
            if(this->gs.curr_nation && unit.owner_id != this->gs.curr_nation->get_id())  return;
            const auto& path = unit.get_path();
        });

        // Buildings
        if(this->map_render->options.buildings.used) {
            for(const auto& province : this->gs.world->provinces) {
                province_units_y[this->gs.world->get_id(province)] += 2.5f;
                const auto prov_pos = province.get_pos();
                for(const auto& building_type : this->gs.world->building_types) {
                    if(!province.buildings[building_type].level) continue;
                    glm::mat4 model = glm::translate(base_model, glm::vec3(prov_pos.x, prov_pos.y, 0.f));
                    model = glm::rotate(model, glm::radians(-90.f), glm::vec3(1.f, 0.f, 0.f));
                    obj_shader->set_uniform("model", model);
                    building_type_models[building_type]->draw(*obj_shader);
                    break;
                }
            }
        }
    } else {
        for(auto& province : this->gs.world->provinces) {
            this->unit_widgets[province]->is_render = false;
            this->battle_widgets[province]->is_render = false;
        }
    }

    // Draw the "drag area" box
    if(is_drag) {
        auto model = base_model;
        obj_shader->set_uniform("model", model);
        obj_shader->set_texture(0, "diffuse_map", *line_tex);
        obj_shader->set_texture(1, "ambient_map", *gs.tex_man.get_white());
        obj_shader->set_texture(2, "occlussion_map", *gs.tex_man.get_white());
        obj_shader->set_texture(3, "height_map", *gs.tex_man.get_white());
        obj_shader->set_texture(4, "specular_map", *gs.tex_man.get_white());
        obj_shader->set_texture(5, "normal_map", *gs.tex_man.get_white());
        auto dragbox_square = Eng3D::Square(gs.input.drag_coord.x, gs.input.drag_coord.y, gs.input.select_pos.x, gs.input.select_pos.y);
        dragbox_square.draw();
    }

    if(this->gen_labels) {
#ifndef E3D_TARGET_SWITCH
        if(distance_to_map < small_zoom_factor) map_font->draw(province_labels, *camera, view_mode == MapView::SPHERE_VIEW);
        else map_font->draw(nation_labels, *camera, view_mode == MapView::SPHERE_VIEW);
#else
        if(distance_to_map > small_zoom_factor) map_font->draw(nation_labels, *camera, view_mode == MapView::SPHERE_VIEW);
#endif
    }

    if(this->map_render->options.trees.used) {
        // Drawing trees
        tree_shder->use();
        tree_shder->set_uniform("projection", projection);
        tree_shder->set_uniform("view", view);
        tree_shder->set_uniform("model", base_model);
        tree_type_models[1]->draw(*tree_shder, tree_spawn_pos.size());
    }

    if(view_mode == MapView::SPHERE_VIEW) {
        // Universe skybox
        const auto model = base_model;
        obj_shader->use();
        obj_shader->set_uniform("model", model);
        obj_shader->set_texture(0, "diffuse_map", *skybox_tex);
        obj_shader->set_texture(1, "ambient_map", *gs.tex_man.get_white());
        obj_shader->set_texture(2, "occlussion_map", *gs.tex_man.get_white());
        obj_shader->set_texture(3, "height_map", *gs.tex_man.get_white());
        obj_shader->set_texture(4, "specular_map", *gs.tex_man.get_white());
        obj_shader->set_texture(5, "normal_map", *gs.tex_man.get_white());
        skybox.draw();
    }

    wind_osc += 0.1f;
}

void Map::handle_resize() {
    this->camera->set_screen(this->gs.width, this->gs.height);
}

void Map::handle_mouse_button(const Eng3D::Event::MouseButton& e) {
    if(e.hold) {
        gs.mouse_pos = Eng3D::Event::get_mouse_pos();
        this->is_drag = false;
        if(e.type == Eng3D::Event::MouseButton::Type::LEFT) {
            gs.input.drag_coord = gs.input.select_pos;
            this->is_drag = true;
        } else if(e.type == Eng3D::Event::MouseButton::Type::MIDDLE) {
            glm::ivec2 map_pos;
            if(this->camera->get_cursor_map_pos(gs.mouse_pos, map_pos))
                this->last_camera_drag_pos = map_pos;
        }
    } else {
        if(gs.input.select_pos.x < 0 || gs.input.select_pos.x >= gs.world->width || gs.input.select_pos.y < 0 || gs.input.select_pos.y >= gs.world->height)
            return;

        const auto province_id = this->map_render->get_tile_province_id(gs.input.select_pos.x, gs.input.select_pos.y);
        if(e.type == Eng3D::Event::MouseButton::Type::LEFT) {
            this->is_drag = false;
            switch(gs.current_mode) {
                case MapMode::NORMAL:
                    if(this->selector) {
                        /// @todo Good selector function
                        this->selector(*gs.world, *this, gs.world->provinces[province_id]);
                        break;
                    }

                    // Check if we selected an unit
                    this->is_drag = false;
                    if(gs.input.get_selected_units().empty()) {
                        // Show province information when clicking on a province
                        if(Province::is_valid(province_id)) {
                            new Interface::ProvinceView(gs, gs.world->provinces[province_id]);
                            return;
                        }
                    }
                    break;
                case MapMode::COUNTRY_SELECT:
                    if(Province::is_valid(province_id)) {
                        const auto& province = gs.world->provinces[province_id];
                        if(Nation::is_valid(province.controller_id))
                            gs.select_nation->change_nation(province.controller_id);
                    }
                    break;
                default: break;
            }
        } else if(e.type == Eng3D::Event::MouseButton::Type::RIGHT) {
            if(Province::is_invalid(province_id)) return;
            auto& province = gs.world->provinces[province_id];
            if(gs.editor && gs.current_mode == MapMode::NORMAL) {
                if(world.terrain_types[province.terrain_type_id].is_water_body) {
                    province.terrain_type_id = (TerrainTypeId)1;
                }
                gs.curr_nation->control_province(province);
                gs.curr_nation->give_province(province);
                province.nuclei.push_back(gs.world->get_id(*gs.curr_nation));
                auto last = std::unique(province.nuclei.begin(), province.nuclei.end());
                province.nuclei.erase(last, province.nuclei.end());
                this->update_mapmode();
                this->map_render->request_update_visibility();
                this->map_render->update(this->gs);
            }

            /// @todo Handle the case where an unit is deleted
            for(const auto unit_id : gs.input.get_selected_units()) {
                auto& unit = gs.world->unit_manager.units[unit_id];
                auto province_id = gs.world->unit_manager.unit_province[unit_id];
                if(!unit.can_move()) continue;
                // Don't change target if ID is the same...
                if(province_id == gs.world->get_id(province) || unit.get_target_province_id() == gs.world->get_id(province))
                    continue;
                if(province.controller_id != gs.curr_nation->get_id()) {
                    // Must either be our ally, have military access with them or be at war
                    const auto& relation = gs.world->get_relation(gs.world->get_id(*gs.curr_nation), province.controller_id);
                    if(!relation.has_landpass()) continue;
                }

                Eng3D::Networking::Packet packet{};
                Archive ar{};
                ::serialize<ActionType>(ar, ActionType::UNIT_CHANGE_TARGET);
                ::serialize(ar, unit_id);
                ::serialize(ar, &province);
                packet.data(ar.get_buffer(), ar.size());
                gs.client->send(packet);

                const std::scoped_lock lock2(gs.audio_man.sound_lock);
                auto entries = gs.package_man.get_multiple_prefix("sfx/land_move");
                if(!entries.empty()) {
                    auto audio = gs.audio_man.load(entries[rand() % entries.size()]->get_abs_path());
                    gs.audio_man.sound_queue.push_back(audio);
                }
            }
            gs.input.clear_selected_units();
        }
    }
}

void Map::handle_mouse_motions(const Eng3D::Event::MouseMotion& e) {
    glm::ivec2 map_pos;
    if(gs.input.middle_mouse_down) {  // Drag the map with middlemouse
        if(gs.map->camera->get_cursor_map_pos(e.pos, map_pos)) {
            glm::vec2 current_pos = glm::make_vec2(gs.map->camera->get_map_pos());
            const glm::vec2 pos = current_pos + last_camera_drag_pos - glm::vec2(map_pos);
            gs.map->camera->set_pos(pos.x, pos.y);
        }
    }

    if(gs.map->camera->get_cursor_map_pos(e.pos, map_pos)) {
        if(map_pos.x < 0 || map_pos.x >(int)gs.world->width || map_pos.y < 0 || map_pos.y >(int)gs.world->height) return;
        gs.input.select_pos = map_pos;
        auto prov_id = map_render->get_tile_province_id(map_pos.x, map_pos.y);
        const std::string text = mapmode_tooltip_func != nullptr ? mapmode_tooltip_func(*gs.world, prov_id) : "";
        if(!text.empty()) {
            gs.map->tooltip->text(text);
            gs.ui_ctx.use_tooltip(gs.map->tooltip, e.pos);
        }
    }
}
