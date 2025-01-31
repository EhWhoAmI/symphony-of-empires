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
//      client/interface/lobby.cpp
//
// Abstract:
//      Does some important stuff.
// ----------------------------------------------------------------------------

#include <filesystem>
#include "eng3d/ui/ui.hpp"
#include "eng3d/ui/button.hpp"
#include "eng3d/ui/label.hpp"
#include "eng3d/ui/group.hpp"
#include "eng3d/ui/image.hpp"
#include "eng3d/ui/window.hpp"
#include "eng3d/camera.hpp"

#include "client/interface/lobby.hpp"
#include "client/interface/main_menu.hpp"
#include "client/client_network.hpp"
#include "server/server_network.hpp"
#include "client/game_state.hpp"
#include "world.hpp"
#include "client/map.hpp"

using namespace Interface;

// Start screen
LobbySelectView::LobbySelectView(GameState& _gs)
    : UI::Window(-576, 0, 576, 128),
    gs{ _gs },
    curr_selected_nation{ 0 }
{
    this->origin = UI::Origin::UPPER_RIGHT_SCREEN;

    // Flag with shadow
    this->curr_country_flag_img = new UI::Image(0, 0, 38, 28, gs.tex_man.get_white(), this);
    new UI::Image(0, 0, 38, 28, "gfx/drop_shadow.png", this);

    // Country button for selection
    this->curr_country_btn = new UI::Button(0, 0, 320, 28, this);
    curr_country_btn->right_side_of(*this->curr_country_flag_img);
    curr_country_btn->text("Select a country");
    curr_country_btn->set_on_click([this](UI::Widget&) {
        if(this->gs.curr_nation != nullptr) {
            this->gs.ui_ctx.clear();
            if(this->gs.host_mode) {
                this->gs.server = std::make_unique<Server>(gs, 1836);
                this->gs.client = std::make_unique<Client>(gs, "127.0.0.1", 1836);
                this->gs.client->username = this->gs.editor ? "Editor" : "Guest";
                this->gs.client->username += "-";
                this->gs.client->username += this->gs.host_mode ? "Host" : "Player";
            }
            this->gs.in_game = true;
            this->gs.play_nation();
        }
    });

    auto* back_btn = new UI::Button(0, 0, 128, 24, this);
    back_btn->text("Back");
    back_btn->below_of(*curr_country_btn);
    back_btn->set_on_click([this](UI::Widget&) {
        this->kill();
        new Interface::MainMenu(gs);
    });

    auto* next_country_btn = new UI::Button(0, 0, 128, 24, this);
    next_country_btn->text("Next");
    next_country_btn->below_of(*curr_country_btn);
    next_country_btn->right_side_of(*back_btn);
    next_country_btn->set_on_click([this](UI::Widget&) {
        this->change_nation(this->curr_selected_nation + 1);
    });

    auto* prev_country_btn = new UI::Button(0, 0, 128, 24, this);
    prev_country_btn->text("Previous");
    prev_country_btn->below_of(*curr_country_btn);
    prev_country_btn->right_side_of(*next_country_btn);
    prev_country_btn->set_on_click([this](UI::Widget&) {
        this->change_nation(this->curr_selected_nation - 1);
    });

    const auto path = std::filesystem::current_path().string();
    auto* game_group = new UI::Group(0, 0, 128, gs.height, this);
    game_group->below_of(*prev_country_btn);
    game_group->flex = UI::Flex::COLUMN;
    game_group->is_scroll = true;
    for(const auto& entry : std::filesystem::directory_iterator(path)) {
        if(!entry.is_directory() && entry.path().extension() == ".sc4") {
            auto savefile_path = entry.path().lexically_relative(path).string();
            auto* ldgame_btn = new UI::Button(0, 0, 128, 24, game_group);
            ldgame_btn->text(savefile_path);
            ldgame_btn->set_on_click([this, savefile_path](UI::Widget&) {
                if(this->gs.curr_nation == nullptr)
                    this->change_nation(0);
                const auto nation_id = this->gs.curr_nation->get_id();
                this->gs.paused = true;

                Archive ar = Archive();
                ar.from_file(savefile_path);
                ::deserialize(ar, *this->gs.world);
                /// @todo Events aren't properly saved yet
                this->gs.world->events.clear();
                this->gs.world->taken_decisions.clear();
                for(auto& nation : this->gs.world->nations)
                    nation.inbox.clear();
                this->gs.world->load_mod();
                this->gs.curr_nation = &this->gs.world->nations[nation_id];
                this->gs.ui_ctx.prompt("Loaded", "Loaded savefile");
            });
        }
    }
}

// Change nation in start screen
void LobbySelectView::change_nation(size_t id) {
    size_t old_id = curr_selected_nation;
    if(g_world.nations.empty()) {
        gs.ui_ctx.prompt("Error", "No nations to select");
        return;
    }

    if(id >= g_world.nations.size())
        id = 0;

    gs.curr_nation = &gs.world->nations[id];
    if(id > old_id) {
        while(gs.curr_nation->exists() == false) {
            id++;
            if(id >= gs.world->nations.size())
                id = 0;
            gs.curr_nation = &gs.world->nations[id];
        }
    } else {
        while(gs.curr_nation->exists() == false) {
            id--;
            if(id >= gs.world->nations.size())
                id = gs.world->nations.size() - 1;
            gs.curr_nation = &gs.world->nations[id];
        }
    }

    curr_selected_nation = id;
    this->curr_country_flag_img->current_texture = this->gs.get_nation_flag(*this->gs.curr_nation);
    this->curr_country_btn->text(gs.curr_nation->name.c_str());
    const auto& capital = gs.world->provinces[gs.curr_nation->capital_id];
    gs.map->camera->set_pos(capital.box_area.right, capital.box_area.bottom);
}
