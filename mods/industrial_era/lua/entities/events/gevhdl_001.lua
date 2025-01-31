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
--  	lua/events/gevhdl_001.lua
--
-- 	Abstract:
--      Does important stuff
-- ----------------------------------------------------------------------------

gevhdl001 = Event:new{
	ref_name = "gevhdl001",
	conditions_fn = function()
		if math.random(0, 100) == 0 then
			return EVENT_CONDITIONS_MET
		end
		return EVENT_CONDITIONS_UNMET
	end,
	event_fn = function(ref_name)
		local r = math.random(0, 10)
		-- "So how many cholera dynamic-events?", "YES"
		if r == 0 then
			gevhdl001.title = "Cholera spreads"
			gevhdl001.text = "Due to the inappropiate usage of our sewage system it has made their way into the central waters of many of our cities, we have reports of dead people all over the country"
		elseif r == 1 then
			gevhdl001.title = "Tuberculosis spreads"
			gevhdl001.text = "We have started to receive numerous reports about a new disease infection point in our country; said disease is tuberculosis and it's mostly lethal"
		elseif r == 2 then
			gevhdl001.title = "Disentry surges"
			gevhdl001.text = "A new infection of disentry has started to take a toll on the population"
		elseif r == 3 then
			gevhdl001.title = "Cholera surges"
			gevhdl001.text = "We're starting to see an uprise in cholera cases, an abnormal number of people report having seen or contacted with an infected person"
		else
			gevhdl001.title = "Flu"
			gevhdl001.text = "A new variant of the flu has appeared in one of our provinces - it is not very lethal, however we can't ignore the potential of it becoming more severe as time goes on"
		end
		gevhdl001:update(gevhdl002)
		decision = Decision:new{
			ref_name = "gevhdl001_decision_0",
			name = "We will do what we can to save our people!",
			decision_fn = function(ref_name)
				local prov_list = Nation:get(ref_name):get_owned_provinces()
				for k, province in pairs(prov_list) do
					local pops = province:get_pops()
					for k, pop in pairs(pops) do
						pop.militancy = pop.militancy + 0.1
						province:update_pop(pop)
					end
					province:update_pops()
				end
			end,
			effects = "+0.5 militancy"
		}
		gevhdl001:add_decision(decision)
		decision = Decision:new{
			ref_name = "gevhdl001_decision_1",
			name = "We can't do anything about it",
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
			effects = "+0.5 militancy"
		}
		gevhdl001:add_decision(decision)
		return EVENT_DO_MANY_TIMES
	end,
    title = "",
	text = ""
}
gevhdl001:register()
gevhdl001:add_receivers(table.unpack(Nation:get_all()))
