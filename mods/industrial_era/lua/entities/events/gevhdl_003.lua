-- Industrial_Era_mod - Base game files for Symphony of Empires
-- Copyright (C) 2021, Symphony of Empires contributors
-- 
-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation; either version 2 of the License, or
-- (at your option) any later version.
-- 
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License along
-- with this program; if not, write to the Free Software Foundation, Inc.,
-- 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
--
-- ----------------------------------------------------------------------------
--	Name:
--  	lua/events/gevhdl_004.lua
--
-- 	Abstract:
--      Does important stuff
-- ----------------------------------------------------------------------------

generic_militancy_nationwide_evhdl = Event:new{
	ref_name = "generic_militancy_nationwide",
	conditions_fn = function()
		if math.random(0, 100) == 0 then
			return EVENT_CONDITIONS_MET
		end
		return EVENT_CONDITIONS_UNMET
	end,
	event_fn = function(ref_name)
		local texts = {
			title = "Stop the activists",
			text = "Some activists are trying to undermine us with false statments about the efficiency of our goverment - we can always crack down on those liars but we may upset free-speech supporters"	
		}
		decision = Decision:new{
			ref_name = "generic_militancy_nationwide_decision_0",
			name = "",
			effects = "Every POP gets +0.5 militancy",
			decision_fn = function(ref_name)
				local prov_list = Nation:get(ref_name):get_owned_provinces()
				for k, province in pairs(prov_list) do
					local pops = province:get_pops()
					for k, pop in pairs(pops) do
						pop.militancy = pop.militancy + 0.5
						province:update_pop(pop)
					end
					province:update_pops()
				end
			end,
			effects = "+0.50 militancy and +0.50 militancy nationwide"
		}
		generic_militancy_nationwide_evhdl:add_decision(decision)
		decision = Decision:new{
			ref_name = "generic_militancy_nationwide_decision_1",
			name = "Take them down",
			effects = "Every POP gets +0.75 militancy",
			decision_fn = function(ref_name)
				local prov_list = Nation:get(ref_name):get_owned_provinces()
				for k, province in pairs(prov_list) do
					local pops = province:get_pops()
					for k, pop in pairs(pops) do
						pop.militancy = pop.militancy + 0.75
						province:update_pop(pop)
					end
					province:update_pops()
				end
			end,
			effects = "+0.75 militancy nationwide"
		}
		generic_militancy_nationwide_evhdl:add_decision(decision)
		return EVENT_DO_MANY_TIMES
	end,
    title = "",
	text = ""
}
generic_militancy_nationwide_evhdl:register()
generic_militancy_nationwide_evhdl:add_receivers(table.unpack(Nation:get_all()))
