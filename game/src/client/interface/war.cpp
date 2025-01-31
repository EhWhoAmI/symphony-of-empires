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
//      client/interface/war.cpp
//
// Abstract:
//      Does some important stuff.
// ----------------------------------------------------------------------------

#include "client/interface/war.hpp"
#include "nation.hpp"
#include "world.hpp"
#include "client/game_state.hpp"
#include "client/client_network.hpp"
#include "action.hpp"
#include "eng3d/ui/components.hpp"

using namespace Interface;

WarDeclarePrompt::WarDeclarePrompt(GameState& _gs, const Nation& _nation)
    : UI::Window(0, 0, 256, 512),
    gs{ _gs },
    nation{ _nation }
{
    this->is_scroll = false;
    this->set_close_btn_function([this](Widget&) {
        this->kill();
    });

    this->body_txt = new UI::Text(0, 0, this->width, 24, this);
    this->body_txt->text("TODO: add cassus bellis");

    this->approve_btn = new UI::Button(0, 24, this->width, 24, this);
    this->approve_btn->text("Declare war");
    this->approve_btn->set_on_click([this](UI::Widget&) {
        this->gs.client->send(Action::DiploDeclareWar::form_packet(this->nation));
        this->kill();
    });

    this->deny_btn = new UI::Button(0, 48, this->width, 24, this);
    this->deny_btn->text("Cancel");
    this->deny_btn->set_on_click([this](UI::Widget&) {
        this->kill();
    });
}