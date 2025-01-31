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
// ----------------------------------------------------------------------------
// Name:
//      client/game_state.cpp
//
// Abstract:
//      Does some important stuff.
// ----------------------------------------------------------------------------

#include <cstdint>
#include <cstdio>
#include <string>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <filesystem>

#include "eng3d/ui/ui.hpp"
#include "eng3d/ui/input.hpp"
#include "eng3d/ui/image.hpp"
#include "eng3d/ui/label.hpp"
#include "eng3d/ui/text.hpp"
#include "eng3d/ui/progress_bar.hpp"
#include "eng3d/audio.hpp"
#include "eng3d/string.hpp"
#include "eng3d/event.hpp"
#include "eng3d/serializer.hpp"
#include "eng3d/material.hpp"
#include "eng3d/model.hpp"
#include "eng3d/texture.hpp"
#include "eng3d/log.hpp"
#include "eng3d/camera.hpp"
#include "eng3d/interface.hpp"

#include "client/game_state.hpp"
#include "product.hpp"
#include "world.hpp"
#include "action.hpp"
#include "client/client_network.hpp"
#include "client/interface/decision.hpp"
#include "client/interface/lobby.hpp"
#include "client/interface/top_window.hpp"
#include "client/interface/army.hpp"
#include "client/interface/minimap.hpp"
#include "client/map.hpp"
#include "client/map_render.hpp"
#include "server/server_network.hpp"
#include "server/export_util.hpp"
#include "client/interface/main_menu.hpp"

void GameState::play_nation() {
    current_mode = MapMode::NORMAL;

    const auto& capital = this->world->provinces[this->curr_nation->capital_id];
    map->camera->set_pos(capital.box_area.right, capital.box_area.bottom);
    map->map_render->request_update_visibility();
    map->map_render->update(*this);

    // Make topwindow
    top_win = static_cast<UI::Widget*>(new Interface::TopWindow(*this));
    minimap = static_cast<UI::Widget*>(new Interface::Minimap(*this, -400, -200, UI::Origin::LOWER_RIGHT_SCREEN));
    Eng3D::Log::debug("game", translate_format("Playing as nation %s", this->curr_nation->ref_name.c_str()));
    this->client->send(Action::SelectNation::form_packet(*this->curr_nation));
}

std::shared_ptr<Eng3D::Texture> GameState::get_nation_flag(const Nation& nation) {
    auto& ideology = this->world->ideologies[nation.ideology_id];
    std::string path = string_format("gfx/flags/%s_%s.png", nation.ref_name.c_str(), Ideology::is_valid(nation.ideology_id) ? ideology.ref_name.c_str() : "default");
    return this->tex_man.load(this->package_man.get_unique(path));
}

void handle_popups(std::vector<TreatyId>& displayed_treaties, GameState& gs) {
    // Put popups
    // Event + Decision popups
    for(auto& msg : gs.curr_nation->inbox) // Check that the event is not already displayed to the user
        new Interface::DecisionWindow(gs, msg);
    gs.curr_nation->inbox.clear(); // Clear the inbox

    for(auto& treaty : gs.world->treaties) {
        // Check that the treaty is not already displayed
        auto iter = std::find(displayed_treaties.begin(), displayed_treaties.end(), treaty.get_id());
        if(iter != displayed_treaties.end()) continue;
        if(!treaty.does_participate(*gs.curr_nation)) continue; // Must participate in treaty
        //new Interface::TreatyChooseWindow(gs, treaty.get_id());
        displayed_treaties.push_back(treaty.get_id());
    }
}

void GameState::update_on_tick() {
    world->profiler.start("UI_TICK");
    ui_ctx.do_tick();
    world->profiler.stop("UI_TICK");

    /// @todo This is inefficient and we should only update **when** needed
    if(current_mode != MapMode::NO_MAP)
        map->update_mapmode();
}

/// @todo Don't run this thread if not needed (i.e non-host-mode)
void GameState::world_thread() {
    this->paused = true;
    while(this->run) {
        // Gamestate thread hasn't acknowledged the updated tick just yet
        while(this->paused) {
            if(!this->run) {
                Eng3D::Log::debug("world_thread", "Unpaused game");
                break;
            }
        }
        if(!this->run) break;

        // Only run the economy simulation of host mode is ON, otherwise don't :-)
        const auto delta = std::chrono::milliseconds{ ms_delay_speed };
        if(host_mode) {
            const auto start_time = std::chrono::system_clock::now();
            Eng3D::Log::debug("world_thread", "World tick performed!");
            try {
                world->do_tick();
                update_tick = true;
            } catch(const std::exception& e) {
                std::scoped_lock lock(render_lock);
                ui_ctx.prompt("Runtime exception", e.what());
                Eng3D::Log::error("world_thread", e.what());
                paused = true;
            }

            // Wait until time delta is fullfilled and update_tick is false
            auto end_time = std::chrono::system_clock::now();
            while(end_time - start_time < delta)
                end_time = std::chrono::system_clock::now();
        }
    }
}

void GameState::music_enqueue() {
    const std::scoped_lock lock(this->audio_man.sound_lock);
    if(this->audio_man.music_queue.empty()) {
        auto entries = this->package_man.get_multiple_prefix("sfx/music/ambience");
        this->audio_man.music_fade_value = 0.f;
        // Search through all the music in 'music/ambience' and picks a random
        if(!entries.empty()) {
            const int music_index = rand() % entries.size();
            auto audio = this->audio_man.load(entries[music_index]->get_abs_path());
            this->audio_man.music_queue.push_back(audio);
            Eng3D::Log::debug("music", "Now playing music file " + entries[music_index]->get_abs_path());
        }
    }
}

void GameState::load_world_thread() {
    this->world = &World::get_instance();
    this->world->init_lua();
    this->world->load_initial();
    this->load_progress = 0.0f;
    /// @todo Events can't be properly reloaded right now :/
    this->world->events.clear();
    this->world->load_mod();
    this->load_progress = 0.1f;
    this->loaded_world = true;
}

void GameState::handle_resize() {
    if(map != nullptr) map->camera->set_screen(width, height);
}

void GameState::handle_mouse_btn(const Eng3D::Event::MouseButton& e) {
    if(e.hold) {
        if(show_ui) {
            if(ui_ctx.check_hover(this->mouse_pos)) {
                if(e.type == Eng3D::Event::MouseButton::Type::LEFT) {
                    this->mouse_pos = Eng3D::Event::get_mouse_pos();
                    ui_ctx.check_drag(this->mouse_pos);
                }
                return;
            }
        }

        if(current_mode != MapMode::NO_MAP)
            map->handle_mouse_button(e);
        
        if(e.type == Eng3D::Event::MouseButton::Type::MIDDLE)
            input.middle_mouse_down = true;
    } else {
        this->mouse_pos = Eng3D::Event::get_mouse_pos();
        if(e.type == Eng3D::Event::MouseButton::Type::LEFT || e.type == Eng3D::Event::MouseButton::Type::RIGHT) {
            if(show_ui) {
                if(ui_ctx.check_click(this->mouse_pos)) {
                    const std::scoped_lock lock(audio_man.sound_lock);
                    auto entries = package_man.get_multiple_prefix("sfx/click");
                    if(!entries.empty()) {
                        auto audio = audio_man.load(entries[rand() % entries.size()]->get_abs_path());
                        audio_man.sound_queue.push_back(audio);
                    }
                    return;
                }
            }

            if(current_mode != MapMode::NO_MAP) // Map
                map->handle_mouse_button(e);
        } else if(e.type == Eng3D::Event::MouseButton::Type::MIDDLE) {
            input.middle_mouse_down = false;
        }
    }
}

void GameState::handle_mouse_motion(const Eng3D::Event::MouseMotion& e) {
    this->mouse_pos = e.pos;
    if(this->show_ui && this->ui_ctx.check_hover(this->mouse_pos))
        return;

    if(current_mode != MapMode::NO_MAP)
        map->handle_mouse_motions(e);
}

void GameState::handle_mouse_wheel(const Eng3D::Event::MouseWheel& e) {
    this->mouse_pos = Eng3D::Event::get_mouse_pos();
    if(show_ui) {
        ui_ctx.check_hover(this->mouse_pos);
        if(ui_ctx.check_wheel(this->mouse_pos, e.wheel.y * 6)) return;
    }

    if(current_mode != MapMode::NO_MAP)
        map->camera->move(0.f, 0.f, -e.wheel.y * delta_time * 120.f);
}

void GameState::handle_key(const Eng3D::Event::Key& e) {
    if(e.hold) {
        switch(e.type) {
        case Eng3D::Event::Key::Type::F1:
            show_ui = !show_ui;
            break;
        case Eng3D::Event::Key::Type::F2:
            if(current_mode == MapMode::NORMAL) {
                if(profiler_view) {
                    profiler_view->kill();
                    profiler_view = nullptr;
                }
                else profiler_view = new Eng3D::Interface::ProfilerView(*this, this->world->profiler);
            }
            break;
        case Eng3D::Event::Key::Type::F3: {
            this->reload_shaders();
            this->map->reload_shaders();
            const std::scoped_lock lock(audio_man.sound_lock);
            audio_man.music_queue.clear();
        } break;
        case Eng3D::Event::Key::Type::F4:
            LuaAPI::invoke_registered_callback(world->lua, "ai_settings_window_invoke");
            break;
        case Eng3D::Event::Key::Type::F5:
            if(editor) break;
            if(current_mode == MapMode::NORMAL) {
                paused = !paused;
                if(paused)
                    ui_ctx.prompt("Control", "Unpaused");
                else
                    ui_ctx.prompt("Control", "Paused");
            }
            break;
        case Eng3D::Event::Key::Type::BACKSPACE:
            ui_ctx.check_text_input(nullptr);
            break;
        case Eng3D::Event::Key::Type::UP:
            if(map && map->camera)
                map->camera->move(0.f, -1.f, 0.f);
            break;
        case Eng3D::Event::Key::Type::DOWN:
            if(map && map->camera)
                map->camera->move(0.f, 1.f, 0.f);
            break;
        case Eng3D::Event::Key::Type::LEFT:
            if(map && map->camera)
                map->camera->move(-1.f, 0.f, 0.f);
            break;
        case Eng3D::Event::Key::Type::RIGHT:
            if(map && map->camera)
                map->camera->move(1.f, 0.f, 0.f);
            break;
        default: break;
        }
    }
}

extern "C" void game_main(int argc, char** argv) {
    std::vector<std::string> pkg_paths;
    for(int i = 1; i < argc; i++) {
        std::string arg = std::string(argv[i]);
        if(arg == "--mod") {
            i++;
            if(i >= argc)
                CXX_THROW(std::runtime_error, translate("Expected an absolute path after --mod"));
            arg = std::string(argv[i]);
            pkg_paths.push_back(arg);
        }
    }

    GameState gs(pkg_paths);

    // After loading everything initialize the gamestate initial properties
    // Call update_on_tick on start of the gamestate
    gs.update_tick = true;
    gs.in_game = false;
    gs.input = Input();
    gs.run = true;
    gs.loaded_world = false;
    gs.loaded_map = false;
    gs.load_progress = 0.f;
    std::thread load_world_th(&GameState::load_world_thread, &gs);

    auto map_layer = new UI::Group(0, 0);
    map_layer->managed = false;

    auto *bg_img = new UI::Image(-(gs.width / 2.f), -(gs.height / 2.f), gs.width, gs.height);
    bg_img->origin = UI::Origin::CENTER_SCREEN;
    auto load_screen_entries = gs.package_man.get_multiple_prefix("gfx/load_screen");
    if(!load_screen_entries.empty())
        bg_img->current_texture = gs.tex_man.load(load_screen_entries[rand() % load_screen_entries.size()]->get_abs_path());

    auto* load_pbar = new UI::ProgressBar(0, -24, gs.width, 24, 0.f, 1.f);
    load_pbar->text(translate("Initializing game resources"));
    load_pbar->origin = UI::Origin::LOWER_LEFT_SCREEN;
    load_pbar->text_color = Eng3D::Color(1.f, 1.f, 1.f);

    auto mod_logo_tex = gs.tex_man.load(gs.package_man.get_unique("gfx/mod_logo.png"));
    auto* mod_logo_img = new UI::Image(0, 0, mod_logo_tex->width, mod_logo_tex->height, mod_logo_tex);
    gs.do_run([&gs](){ return gs.loaded_map == false; },
        ([&gs]() {
            gs.music_enqueue();
            // Widgets here SHOULD NOT REQUEST UPON WORLD DATA
            // so no world lock is needed beforehand
            gs.do_event();
        }), ([&gs, &map_layer, load_pbar]() {
            std::scoped_lock lock(gs.render_lock);
            gs.clear();
            if(gs.loaded_world) {
                gs.map = new Map(gs, *gs.world, map_layer, gs.width, gs.height);
                gs.current_mode = MapMode::DISPLAY_ONLY;
                gs.map->set_view(MapView::SPHERE_VIEW);
                gs.map->camera->move(0.f, 50.f, 10.f);
                gs.loaded_map = true;
                gs.load_progress = 1.f;
            }

            load_pbar->set_value(gs.load_progress);
            gs.ui_ctx.render_all(gs.mouse_pos);
            gs.world->profiler.render_done();
        })
    );
    bg_img->kill();
    load_pbar->kill();
    mod_logo_img->kill();

    load_world_th.join();
    // LuaAPI::invoke_registered_callback(gs.world->lua, "map_dev_view_invoke");

    // Connect to server prompt
    new Interface::MainMenu(gs);
    // new Interface::MapDebugMenu(gs);
    Export::export_provinces(*gs.world);
    std::vector<TreatyId> displayed_treaties;
    // Start the world thread
    std::thread world_th(&GameState::world_thread, &gs);
    gs.do_run([&gs](){ return gs.run == true; },
        ([&displayed_treaties, &gs]() {
            gs.music_enqueue();
            // Locking is very expensive, so we condense everything into a big "if"
            if(gs.world->world_mutex.try_lock()) {
                // Required since events may request world data
                gs.do_event();
                if(gs.current_mode == MapMode::NORMAL)
                    handle_popups(displayed_treaties, gs);

                if(gs.update_tick) {
                    gs.update_on_tick();
                    gs.map->map_render->update(gs);
                    gs.update_tick = false;

                    if(gs.current_mode == MapMode::NORMAL) {
                        // Production queue
                        for(size_t i = 0; i < gs.production_queue.size(); i++) {
                            const auto* unit_type = gs.production_queue[i];

                            /// @todo Make a better queue AI
                            bool is_built = false;
                            for(auto& building_type : gs.world->building_types) {
                                for(const auto province_id : gs.curr_nation->controlled_provinces) {
                                    auto& province = gs.world->provinces[province_id];
                                    auto& building = province.get_buildings()[building_type];
                                    // Must not be working on something else
                                    if(UnitType::is_invalid(building.working_unit_type_id)) {
                                        is_built = true;
                                        gs.client->send(Action::BuildingStartProducingUnit::form_packet(province, building_type, *gs.curr_nation, *unit_type));
                                        break;
                                    }
                                }

                                if(!is_built) break;
                            }
                            if(!is_built) break;
                            gs.production_queue.erase(gs.production_queue.begin() + i);
                            i--;
                        }
                    }
                }

                if(gs.current_mode == MapMode::DISPLAY_ONLY)
                    gs.map->camera->move(0.05f, 0.f, 0.f);
                gs.world->world_mutex.unlock();
            }
        }), ([&gs]() {
            std::scoped_lock lock(gs.render_lock);
            if(gs.current_mode != MapMode::NO_MAP) {
                const std::scoped_lock lock(gs.world->world_mutex);
                gs.map->camera->update();
                gs.map->draw();
            }
            gs.world->profiler.render_done();
        })
    );
    world_th.join();
}

GameState::~GameState() {
    if(this->map != nullptr)
        delete this->map;
}
