#ifndef LUA_H
#define LUA_H

#include <lua5.4/lua.hpp>

int World_LuaAddGood(lua_State * L);
int World_LuaAddIndustryType(lua_State * L);
int World_LuaAddInputToIndustryType(lua_State * L);
int World_LuaAddOutputToIndustryType(lua_State * L);
int World_LuaAddNation(lua_State * L);
int World_LuaAddProvince(lua_State * L);
int World_LuaGettext(lua_State * L);

#endif
