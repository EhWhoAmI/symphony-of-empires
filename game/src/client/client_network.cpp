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
//      client/client_network.cpp
//
// Abstract:
//      Does some important stuff.
// ----------------------------------------------------------------------------

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <chrono>
#include <thread>

#include "eng3d/log.hpp"

#include "action.hpp"
#include "unit.hpp"
#include "diplomacy.hpp"
#include "world.hpp"
#include "client/client_network.hpp"

#include "client/game_state.hpp"
#include "client/map.hpp"

Client* g_client = nullptr;
Client::Client(GameState& _gs, std::string host, const unsigned port)
    : Eng3D::Networking::Client(host, port),
    gs{_gs}
{
    g_client = this;
    
    // Launch the receive and send thread
    this->run = true;
    net_thread = std::thread(&Client::net_loop, this);
    has_snapshot = false;
}

// The server assumes all clients are able to handle all events regardless of anything
// if the client runs out of memory it needs to disconnect and then reconnect in order
// to establish a new connection; since the server won't hand out snapshots - wait...
// if you need snapshots for any reason (like desyncs) you can request with ActionType::SNAPSHOT
void Client::net_loop() {
    {
        Archive ar{};
        ::serialize<ActionType>(ar, ActionType::CONNECT);
        ::serialize(ar, username);

        Eng3D::Networking::Packet packet(fd, ar.get_buffer(), ar.size());
        packet.send();
    }
    
    has_snapshot = true;
    try {
        ActionType action;
        Eng3D::Networking::SocketStream stream(fd);
        Eng3D::Networking::Packet packet(fd);
        packet.pred = [this]() -> bool {
            return this->run == true;
        };
        while(this->run) {
			// Update packets with pending list (acquiring the lock has priority to be lenient
			// since the client takes most of it's time sending to the server anyways)
			if(!pending_packets.empty()) {
                const std::scoped_lock lock(packets_mutex, pending_packets_mutex);
                packets.insert(packets.end(), pending_packets.begin(), pending_packets.end());
                pending_packets.clear();
			}

            // Conditional of above statements
			// When we are on host_mode we discard all potential packets sent by the server
			// (because our data is already synchronized since WE ARE the server)
            if(stream.has_pending()) {
                Archive ar{};

                // Obtain the action from the server
                while(1) {
                    try {
                        packet.recv();
                        break;
                    } catch(Eng3D::Networking::SocketException& e) {
                        // Pass
                    }
                    if(!this->run) CXX_THROW(ClientException, "Server closed");
                }
                ar.set_buffer(packet.data(), packet.size());
                ar.rewind();
                ::deserialize(ar, action);

                Eng3D::Log::debug("client", "Receiving package of " + std::to_string(packet.size()));
                if(!gs.host_mode) {
                    const std::scoped_lock lock(gs.world->world_mutex);
                    // Ping from server, we should answer with a pong!
                    switch(action) {
                    // Update/Remove/Add Actions
                    // These actions all follow the same format they give a specialized ID for the index
                    // where the operated object is or should be; this allows for extreme-level fuckery
                    // like ref-name changes in the middle of a game in the case of updates.
                    //
                    // After the ID the object in question is given in a serialized form, in which the
                    // deserializer will deserialize onto the final object; after this the operation
                    // desired is done.
                    case ActionType::NATION_UPDATE: {
                        NationId size;
                        ::deserialize(ar, size);
                        for(size_t i = 0; i < static_cast<size_t>(size); i++) {
                            Nation* nation;
                            ::deserialize(ar, nation);
                            if(nation == nullptr)
                                CXX_THROW(ClientException, "Unknown nation");
                            ::deserialize(ar, *nation);
                        }
                    } break;
                    case ActionType::NATION_ENACT_POLICY: {
                        Nation* nation;
                        ::deserialize(ar, nation);
                        if(nation == nullptr)
                            CXX_THROW(ClientException, "Unknown nation");
                        Policies policy;
                        ::deserialize(ar, policy);
                        nation->current_policy = policy;
                    } break;
                    case ActionType::PROVINCE_UPDATE: {
                        ProvinceId size;
                        ::deserialize(ar, size);
                        for(size_t i = 0; i < static_cast<size_t>(size); i++) {
                            Province* province;
                            ::deserialize(ar, province);
                            if(province == nullptr)
                                CXX_THROW(ClientException, "Unknown province");
                            
                            auto old_owner_id = province->owner_id;
                            auto old_controller_id = province->controller_id;
                            ::deserialize(ar, *province);
                            if(province->owner_id != old_owner_id)
                                gs.world->province_manager.mark_province_owner_changed(province->get_id());
                            if(province->controller_id != old_controller_id)
                                gs.world->province_manager.mark_province_control_changed(province->get_id());
                        }
                    } break;
                    case ActionType::UNIT_UPDATE: {
                        UnitId size;
                        ::deserialize(ar, size);
                        for(size_t i = 0; i < static_cast<size_t>(size); i++) {
                            Unit unit;
                            ::deserialize(ar, unit);
                            assert(gs.world->unit_manager.units.size() > unit);
                            gs.world->unit_manager.units[unit] = unit;
                        }
                    } break;
                    case ActionType::UNIT_ADD: {
                        Unit unit;
                        ::deserialize(ar, unit);
                        ProvinceId province_id;
                        ::deserialize(ar, province_id);
                        gs.world->unit_manager.add_unit(unit, province_id);
                        Eng3D::Log::debug("client", translate_format("Adding new unit from nation %s", g_world.nations[unit.owner_id].ref_name.c_str()));
                    } break;
                    case ActionType::UNIT_REMOVE: {
                        UnitId unit_id;
                        ::deserialize(ar, unit_id);
                        gs.world->unit_manager.remove_unit(unit_id);
                    } break;
                    case ActionType::UNIT_MOVE: {
                        UnitId unit_id;
                        ::deserialize(ar, unit_id);
                        ProvinceId province_id;
                        ::deserialize(ar, province_id);
                        gs.world->unit_manager.move_unit(unit_id, province_id);
                    } break;
                    case ActionType::BUILDING_ADD: {
                        Province* province;
                        ::deserialize(ar, province);
                        BuildingType* building_type;
                        ::deserialize(ar, building_type);
                        province->buildings[gs.world->get_id(*building_type)].level++;
                    } break;
                    case ActionType::BUILDING_REMOVE: {
                        Province* province;
                        ::deserialize(ar, province);
                        BuildingType* building_type;
                        ::deserialize(ar, building_type);
                        province->buildings[gs.world->get_id(*building_type)].level--;
                    } break;
                    case ActionType::TREATY_ADD: {
                        Treaty treaty;
                        ::deserialize(ar, treaty);
                        gs.world->insert(treaty);
                        Eng3D::Log::debug("client", translate_format("Adding new treaty sent by %s", gs.world->nations[treaty.sender_id].ref_name.c_str()));
                        for(const auto& [nation_id, _] : treaty.approval_status)
                            Eng3D::Log::debug("client", gs.world->nations[nation_id].ref_name.c_str());
                    } break;
                    case ActionType::WORLD_TICK: {
                        // Give up the world mutex for now
                        gs.update_tick = true;
                        gs.world->time++;
                    } break;
                    case ActionType::PROVINCE_COLONIZE: {
                        Province* province;
                        ::deserialize(ar, province);
                        if(province == nullptr)
                            CXX_THROW(ClientException, "Unknown province");
                        ::deserialize(ar, *province);
                    } break;
                    case ActionType::SELECT_NATION: {
                        Nation* nation;
                        ::deserialize(ar, nation);
                        ::deserialize(ar, nation->client_username);
                    } break;
                    default:
                        break;
                    }
                }
            }

            // Client will also flush it's queue to the server
            while(!packets.empty()) {
                Eng3D::Networking::Packet packet{};
                { // Make clear the lifetime of the lock since send is an expensive operation
                    const std::scoped_lock lock(packets_mutex);
                    packet = packets.back();
                    packet.stream = Eng3D::Networking::SocketStream(fd);
                    packets.pop_back();
                }
                Eng3D::Log::debug("client", "Sending package of " + std::to_string(packet.size()));
                packet.send();
            }
        }
    } catch(ClientException& e) {
        Eng3D::Log::error("client", translate_format("Exception: %s", e.what()));
    }
}

/// @brief Waits to receive the server initial world snapshot
void Client::wait_for_snapshot() {
    while(!has_snapshot) {
        // Just wait...
    }
}

Client::~Client() {
    this->run = false;
    if(this->net_thread.joinable())
        this->net_thread.join();
}
