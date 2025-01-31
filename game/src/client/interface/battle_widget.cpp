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
//      eng3d/ui/battle_widget.cpp
//
// Abstract:
//      Does some important stuff.
// ----------------------------------------------------------------------------

#include "eng3d/ui/image.hpp"
#include "eng3d/ui/progress_bar.hpp"
#include "eng3d/camera.hpp"
#include "eng3d/color.hpp"

#include "client/interface/battle_widget.hpp"
#include "client/map.hpp"
#include "client/game_state.hpp"
#include "nation.hpp"
#include "diplomacy.hpp"
#include "unit.hpp"
#include "nation.hpp"
#include "world.hpp"

using namespace Interface;

BattleWidget::BattleWidget(Map& _map, UI::Widget* parent)
    : UI::Div(0, 0, 188, 30, parent),
    map{ _map }
{
    this->background_color = Eng3D::Color(1.f, 1.f, 1.f, 1.f);
    /// @todo On click display information about the battle

    auto& gs = static_cast<GameState&>(Eng3D::State::get_instance());

    this->left_flag_img = new UI::Image(1, 1, 38, 28, gs.tex_man.get_white(), this);
    this->left_size_label = new UI::Div(41, 1, 48, 28, this);
    this->left_size_label->text_align_x = UI::Align::END;
    this->left_size_label->background_color = Eng3D::Color(0.8f, 0.f, 0.f, 1.f);
    this->left_size_label->set_on_each_tick([this](UI::Widget&) {
        if(this->province == nullptr) return;
        if(this->idx >= this->province->battles.size()) return;
        const auto& battle = this->province->battles[this->idx];
        auto unit_size = 0;
        for(const auto unit_id : battle.attackers_ids) {
            const auto& unit = g_world.unit_manager.units[unit_id];
            unit_size += (int)unit.size;
        }
        this->left_size_label->text(std::to_string(unit_size));
    });

    this->right_flag_img = new UI::Image(139, 1, 38, 28, gs.tex_man.get_white(), this);
    this->right_size_label = new UI::Div(90, 1, 48, 28, this);
    this->right_size_label->text_align_x = UI::Align::END;
    this->right_size_label->background_color = Eng3D::Color(0.f, 0.f, 0.8f, 1.f);
    this->right_size_label->set_on_each_tick([this](UI::Widget&) {
        if(this->province == nullptr) return;
        if(this->idx >= this->province->battles.size()) return;
        const auto& battle = this->province->battles[this->idx];
        auto unit_size = 0;
        for(const auto unit_id : battle.defenders_ids) {
            const auto& unit = g_world.unit_manager.units[unit_id];
            unit_size += (int)unit.size;
        }
        this->right_size_label->text(std::to_string(unit_size));
    });
}

void BattleWidget::set_battle(Province& _province, size_t _idx) {
    this->province = &_province;
    this->idx = _idx;
    const auto& battle = this->province->battles[this->idx];
    const auto& camera = *map.camera;
    const auto battle_pos = province->get_pos();
    const auto screen_pos = camera.get_tile_screen_pos(battle_pos);

    this->x = screen_pos.x - this->width / 2;
    this->y = screen_pos.y - this->height / 2;

    auto& gs = static_cast<GameState&>(Eng3D::State::get_instance());
    if(!battle.attackers_ids.empty()) {
        auto left_nation_flag = gs.get_nation_flag(gs.world->nations[gs.world->unit_manager.units[battle.attackers_ids[0]].owner_id]);
        this->left_flag_img->current_texture = left_nation_flag;
        this->left_size_label->on_each_tick(*this->left_size_label);
    }

    if(!battle.defenders_ids.empty()) {
        auto right_nation_flag = gs.get_nation_flag(gs.world->nations[gs.world->unit_manager.units[battle.defenders_ids[0]].owner_id]);
        this->right_flag_img->current_texture = right_nation_flag;
        this->right_size_label->on_each_tick(*this->right_size_label);
    }
}
