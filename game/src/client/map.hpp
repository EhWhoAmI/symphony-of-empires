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
//      client/map.hpp
//
// Abstract:
//      Does some important stuff.
// ----------------------------------------------------------------------------

#pragma once

#include <vector>
#include <memory>
#include <utility>
#include <functional>
#include <cstddef>
#include "eng3d/color.hpp"
#include "eng3d/primitive.hpp"
#include "eng3d/shader.hpp"
#include "eng3d/font_sdf.hpp"
#include "eng3d/event.hpp"

namespace Eng3D {
    class Texture;
    class Model;
    class TextureArray;
    class Model;
    class Camera;

    namespace OpenGL {
        class Square;
        class Sphere;
        class Program;
        class Quad2D;
        class Framebuffer;
    }
}

namespace UI {
    class Context;
    class Group;
    class Tooltip;
}
namespace Interface {
    class UnitWidget;
    class BattleWidget;
}

#include "province.hpp"
#include "terrain.hpp"

enum class MapView {
    SPHERE_VIEW,
    PLANE_VIEW,
};

class World;
class Nation;
class MapRender;
class GameState;
class Input;
class Rivers;
class Borders;

class ProvinceColor {
public:
    ProvinceId id;
    Eng3D::Color color;
    ProvinceColor(ProvinceId _id, Eng3D::Color _color): id{ _id }, color{ _color } {}
};
typedef std::function<std::string(const World& world, const ProvinceId id)> mapmode_tooltip;
typedef std::function<std::vector<ProvinceColor>(const World& world)> mapmode_generator;
class Map;
typedef std::function<void(const World& world, Map& map, const Province& province)> selector_func;
std::vector<ProvinceColor> political_map_mode(const World& world);
std::string political_province_tooltip(const World& world, const ProvinceId id);
std::string empty_province_tooltip(const World& world, const ProvinceId id);

class Map {
    mapmode_generator mapmode_func = nullptr;
    mapmode_tooltip mapmode_tooltip_func = nullptr;
    selector_func selector = nullptr;
    GameState& gs;
public:
    Map(GameState& gs, const World& world, UI::Group* map_ui_layer,  int screen_width, int screen_height);
    ~Map();
    void update_mapmode();
    void draw_flag(const Eng3D::OpenGL::Program& shader, const Nation& nation);
    void draw();
    void set_map_mode(mapmode_generator mapmode_func, mapmode_tooltip tooltip_func);
    void set_selected_province(bool selected, ProvinceId id);
    void set_view(MapView view);
    void reload_shaders();
    void update_nation_label(const Nation& nation);
    void create_labels();

    void handle_resize();
    void handle_mouse_button(const Eng3D::Event::MouseButton& e);
    void handle_mouse_motions(const Eng3D::Event::MouseMotion& e);

    bool province_selected = false;
    ProvinceId selected_province_id;
    MapView view_mode = MapView::PLANE_VIEW;

    std::vector<std::shared_ptr<Eng3D::Model>> building_type_models;
    std::vector<std::shared_ptr<Eng3D::Model>> unit_type_models;
    std::vector<std::shared_ptr<Eng3D::Model>> tree_type_models;
    std::vector<glm::vec2> tree_spawn_pos; // Location of where to place trees
    std::vector<std::shared_ptr<Eng3D::Texture>> building_type_icons;
    std::vector<std::shared_ptr<Eng3D::Texture>> unit_type_icons;
    std::vector<std::shared_ptr<Eng3D::Texture>> nation_flags;

#ifndef E3D_TARGET_SWITCH
    std::vector<std::unique_ptr<Eng3D::Label3D>> province_labels;
#endif
    std::vector<std::unique_ptr<Eng3D::Label3D>> nation_labels;
    bool gen_labels = true;

    /// @brief Wind oscillator (for flags)
    float wind_osc = 0.f;
    /// @brief Input states
    bool is_drag = false;
    glm::vec2 last_camera_drag_pos;

    void set_selection(selector_func selector);

    const World& world;
    UI::Tooltip* tooltip = nullptr;
    UI::Group* map_ui_layer = nullptr;
private:
    Eng3D::Sphere skybox;
public:
    std::unique_ptr<Eng3D::Camera> camera;
private:
    std::unique_ptr<Eng3D::FontSDF> map_font;
public:
    std::unique_ptr<MapRender> map_render;
private:
    std::shared_ptr<Eng3D::Texture> line_tex;
    std::shared_ptr<Eng3D::Texture> skybox_tex;
    std::unique_ptr<Eng3D::OpenGL::Program> tree_shder;
public:
    std::vector<Interface::UnitWidget*> unit_widgets;
    std::vector<Interface::BattleWidget*> battle_widgets;
    std::unique_ptr<Eng3D::OpenGL::Program> obj_shader;
};

