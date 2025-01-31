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
--  	lua/nations/america.lua
--
-- 	Abstract:
--      Does important stuff
-- ----------------------------------------------------------------------------

--------------------------------------------------------------
-- America
-- Initial nations
argentina = Nation:new{ ref_name = "argentina" }
argentina.name = _("Argentina")
argentina.adjective = "Argentine"
argentina:register()
argentina:set_ideology(republic)
argentina:add_accepted_language(spanish)
argentina:add_client_hint(monarchy, _("Argentina"), 0xace3e1)
argentina:add_client_hint(republic, _("Argentina"), 0xace3e1)

brazil = Nation:new{ ref_name = "brazil" }
brazil.name = _("Brazil")
brazil.adjective = "Brazilian"
brazil:register()
brazil:set_ideology(monarchy)
brazil:add_accepted_language(portuguese)
brazil:add_client_hint(monarchy, _("Brazil"), 0x60c483)

gran_colombia = Nation:new{ ref_name = "gran_colombia" }
gran_colombia.name = _("Gran colombia")
gran_colombia.adjective = "Colombian"
gran_colombia:register()
gran_colombia:set_ideology(republic)
gran_colombia:add_accepted_language(spanish)
gran_colombia:add_client_hint(monarchy, _("Gran colombia"), 0xe6e6b5)
gran_colombia:add_client_hint(republic, _("Gran colombia"), 0xe6e6b5)

peru = Nation:new{ ref_name = "peru" }
peru.name = _("Peru")
peru.adjective = "Peruvian"
peru:register()
peru:set_ideology(republic)
peru:add_accepted_language(spanish)
peru:add_client_hint(monarchy, _("Peru"), 0xe63d2e)
peru:add_client_hint(republic, _("Peru"), 0xe63d2e)

bolivia = Nation:new{ ref_name = "bolivia" }
bolivia.name = _("Bolivia")
bolivia.adjective = _("Bolivian")
bolivia:register()
bolivia:set_ideology(republic)
bolivia:add_accepted_language(spanish)
bolivia:add_client_hint(republic, _("Bolivia"), 0xded6ba)
bolivia:add_client_hint(monarchy, _("Bolivia"), 0xded6ba)

paraguay = Nation:new{ ref_name = "paraguay" }
paraguay.name = _("Paraguay")
paraguay.adjective = _("Paraguayan")
paraguay:register()
paraguay:set_ideology(republic)
paraguay:add_accepted_language(spanish)
paraguay:add_client_hint(monarchy, _("Paraguay"), 0x99c2bf)
paraguay:add_client_hint(republic, _("Paraguay"), 0x99c2bf)

chile = Nation:new{ ref_name = "chile" }
chile.name = _("Chile")
chile.adjective = _("Chilean")
chile:register()
chile:set_ideology(republic)
chile:add_accepted_language(spanish)
chile:add_client_hint(monarchy, _("Chile"), 0xad6dcf)
chile:add_client_hint(republic, _("Chile"), 0xad6dcf)

upca = Nation:new{ ref_name = "upca" }
upca.name = _("United Provinces of Central America")
upca:register()
upca:set_ideology(republic)
upca:add_accepted_language(spanish)
upca:add_client_hint(republic, _("United Provinces of Central America"), 0x5bb2cf)
upca:add_client_hint(communism, _("United Provinces of Central America"), 0x5bb2cf)
upca:add_client_hint(socialism, _("United Provinces of Central America"), 0x5bb2cf)

miskito_coast = Nation:new{ ref_name = "miskito_coast" }
miskito_coast.name = _("Miskito Coast")
miskito_coast:register()
miskito_coast:set_ideology(monarchy)
miskito_coast:add_accepted_language(spanish)
miskito_coast:add_client_hint(monarchy, _("Miskito Coast"), 0xd49285)

mexico = Nation:new{ ref_name = "mexico" }
mexico.name = _("Mexico")
mexico.adjective = _("Mexican")
mexico:register()
mexico:set_ideology(republic)
mexico:add_accepted_language(spanish)
mexico:add_client_hint(republic, _("Mexico"), 0x5ac780)
mexico:add_client_hint(monarchy, _("Mexican Empire"), 0x5ac780)

usa = Nation:new{ ref_name = "usa" }
usa.name = _("United states")
usa.adjective = _("American")
usa.noun = _("American")
usa:register()
usa:set_ideology(democracy)
usa:add_accepted_language(spanish)
usa:add_accepted_language(english)
usa:add_client_hint(democracy, _("United States of America"), 0x6ea5cc)
usa:add_client_hint(fascism, _("United States of America"), 0x6ea5cc)
usa:add_client_hint(monarchy, _("United States of America"), 0x6ea5cc)
usa:add_client_hint(socialism, _("United States of America"), 0x6ea5cc)
usa:add_client_hint(communism, _("United States of America"), 0x6ea5cc)

haiti = Nation:new{ ref_name = "haiti" }
haiti.name = _("Haiti")
haiti:register()
haiti:set_ideology(republic)
haiti:add_accepted_language(french)
haiti:add_client_hint(republic, _("Haiti"), 0xb147cc)
haiti:add_client_hint(monarchy, _("Hayti"), 0xb147cc)

mapuche = Nation:new{ ref_name = "mapuche" }
mapuche.name = _("Mapuche")
mapuche:register()
mapuche:set_ideology(tribal)
mapuche:add_accepted_language(mapuche)
mapuche:add_client_hint(tribal, _("Mapuche"), 0x8c9ebd)

canada = Nation:new{ ref_name = "canada" }
canada.name = _("Canada")
canada:register()
canada:set_ideology(monarchy)
canada:add_accepted_language(english)
canada:add_client_hint(monarchy, _("Canada"), 0x5f9376)

upper_canada = Nation:new{ ref_name = "upper_canada" }
upper_canada.name = _("Upper Canada")
upper_canada:register()
upper_canada:set_ideology(monarchy)
upper_canada:add_accepted_language(english)
upper_canada:add_client_hint(monarchy, _("Upper Canada"), 0xaa2913)

lower_canada = Nation:new{ ref_name = "lower_canada" }
lower_canada.name = _("Lower Canada")
lower_canada:register()
lower_canada:set_ideology(monarchy)
lower_canada:add_accepted_language(english)
lower_canada:add_client_hint(monarchy, _("Lower Canada"), 0xdf1f1f)

ruperts_land = Nation:new{ ref_name = "ruperts_land" }
ruperts_land.name = _("Rupert's Land")
ruperts_land:register()
ruperts_land:set_ideology(monarchy)
ruperts_land:add_accepted_language(english)
ruperts_land:add_client_hint(monarchy, _("Rupert's Land"), 0xeb8f8f)

-- Releasable nations
texas = Nation:new{ ref_name = "texas" }
texas.name = _("Texas")
texas.adjective = _("Texan")
texas:register()
texas:add_accepted_language(english)
texas:add_accepted_language(spanish)
texas:add_client_hint(republic, _("Texas"), 0x8be2f7)
texas:add_client_hint(fascism, _("Texas"), 0x8be2f7)
texas:add_client_hint(socialism, _("Texas"), 0x8be2f7)
texas:add_client_hint(communism, _("Texas"), 0x8be2f7)
texas:add_client_hint(monarchy, _("Texas"), 0x8be2f7)

csa = Nation:new{ ref_name = "csa" }
csa.name = _("Confederate States of America")
csa.adjective = _("Confederate")
csa:register()
csa:add_accepted_language(english)
csa:add_client_hint(republic, _("Confederate States of America"), 0x8be2f7)
csa:add_client_hint(monarchy, _("Confederate States of America"), 0x8be2f7)

uruguay = Nation:new{ ref_name = "uruguay" }
uruguay.name = _("Uruguay")
uruguay.adjective = _("Uruguayan")
uruguay:register()
uruguay:add_accepted_language(spanish)
uruguay:add_client_hint(monarchy, _("Uruguay"), 0xdce6e8)
uruguay:add_client_hint(republic, _("Uruguay"), 0xdce6e8)

panama = Nation:new{ ref_name = "panama" }
panama.name = _("Panama")
panama.adjective = _("Panaman")
panama:register()
panama:add_accepted_language(spanish)
panama:add_client_hint(monarchy, _("Panama"), 0xa8a8e3)
panama:add_client_hint(republic, _("Panama"), 0xa8a8e3)
--------------------------------------------------------------