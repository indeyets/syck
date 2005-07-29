/*
 * lsyck.c
 *
 * $Author$
 * $Date$
 *
 * Copyright (C) 2005 Zachary P. Landau <kapheine@divineinvasion.net>
 */

#include <syck.h>
#include <string.h>
#include <stdlib.h>

#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

struct emitter_xtra {
	lua_State *L;
	luaL_Buffer output;
	int id;
};

struct parser_xtra {
	lua_State *L;
};

SYMID
lua_syck_parser_handler(SyckParser *p, SyckNode *n)
{
	struct parser_xtra *bonus = (struct parser_xtra *)p->bonus;
	int o, o2, o3 = -1;
	SYMID oid;
	int i;

	switch (n->kind) {
		case syck_str_kind:
			if (n->type_id == NULL || strcmp(n->type_id, "str") == 0) {
				lua_pushlstring(bonus->L, n->data.str->ptr, n->data.str->len);
				o = lua_gettop(bonus->L);
			}
			else if (strcmp(n->type_id, "null") == 0)
			{
				lua_pushnil(bonus->L);
				o = lua_gettop(bonus->L);
			}
			else if (strcmp(n->type_id, "bool#yes") == 0)
			{
				lua_pushboolean(bonus->L, 1);
				o = lua_gettop(bonus->L);
			}
			else if (strcmp(n->type_id, "bool#no") == 0)
			{
				lua_pushboolean(bonus->L, 0);
				o = lua_gettop(bonus->L);
			}
			else if (strcmp(n->type_id, "int#hex") == 0)
			{
				long intVal = strtol(n->data.str->ptr, NULL, 16);
				lua_pushnumber(bonus->L, intVal);
				o = lua_gettop(bonus->L);
			}
			else if (strcmp(n->type_id, "int") == 0)
			{
				long intVal = strtol(n->data.str->ptr, NULL, 10);
				lua_pushnumber(bonus->L, intVal);
				o = lua_gettop(bonus->L);
			}
			else
			{
				lua_pushlstring(bonus->L, n->data.str->ptr, n->data.str->len);
				o = lua_gettop(bonus->L);
			}
			break;

		case syck_seq_kind:
			lua_newtable(bonus->L);
			o = lua_gettop(bonus->L);
			for ( i=0; i < n->data.list->idx; i++ )
			{
				oid = syck_seq_read(n, i);
				syck_lookup_sym(p, oid, (char **)&o2);
				lua_pushvalue(bonus->L, o2);
				lua_rawseti(bonus->L, o, i+1);
			}
			break;

		case syck_map_kind:
			lua_newtable(bonus->L);
			o = lua_gettop(bonus->L);
			for ( i=0; i < n->data.pairs->idx; i++ )
			{
				oid = syck_map_read(n, map_key, i);
				syck_lookup_sym(p, oid, (char **)&o2);
				oid = syck_map_read(n, map_value, i);
				syck_lookup_sym(p, oid, (char **)&o3);

				lua_pushvalue(bonus->L, o2);
				lua_pushvalue(bonus->L, o3);
				lua_settable(bonus->L, o);
			}
			break;
	}
	oid = syck_add_sym(p, (char *)o);
	return oid;
}

void lua_syck_emitter_handler(SyckEmitter *e, st_data_t data)
{
	struct emitter_xtra *bonus = (struct emitter_xtra *)e->bonus;
	int type = lua_type(bonus->L, data);
	char buf[30];		/* find a better way, if possible */
	
	switch (type)
	{
		case LUA_TBOOLEAN:
			if (lua_toboolean(bonus->L, data))
				strcpy(buf, "true");
			else
				strcpy(buf, "false");
			syck_emit_scalar(e, "boolean", scalar_none, 0, 0, 0, (char *)buf, strlen(buf));
			break;
		case LUA_TSTRING:
			syck_emit_scalar(e, "string", scalar_none, 0, 0, 0, (char *)lua_tostring(bonus->L, data), lua_strlen(bonus->L, data));
			break;
		case LUA_TNUMBER:
			/* should handle floats as well */
			snprintf(buf, sizeof(buf), "%i", (int)lua_tonumber(bonus->L, data));
			syck_emit_scalar(e, "number", scalar_none, 0, 0, 0, buf, strlen(buf));
			break;
		case LUA_TTABLE:
			syck_emit_seq(e, "table", seq_none);
			lua_pushnil(bonus->L);  /* first key */
			while (lua_next(bonus->L, -2) != 0) {
				/* `key' is at index -2 and `value' at index -1 */
				syck_emit_item(e, -1);
				lua_pop(bonus->L, 1);  /* removes `value'; keeps `key' for next iteration */

			}
			syck_emit_end(e);
			break;
	}
}

static void lua_syck_mark_emitter(SyckEmitter *e, int idx)
{
	struct emitter_xtra *bonus = (struct emitter_xtra *)e->bonus;
	int type = lua_type(bonus->L, idx);

	switch (type) {
		case LUA_TTABLE:
			lua_pushnil(bonus->L);  /* first key */
			while (lua_next(bonus->L, -2) != 0) {
				/* `key' is at index -2 and `value' at index -1 */
				//syck_emitter_mark_node(e, bonus->id++);
				syck_emitter_mark_node(e, bonus->id++);
				lua_syck_mark_emitter(e, -1);
				lua_pop(bonus->L, 1);
			}
			break;
		default:
			syck_emitter_mark_node(e, bonus->id++);
			break;
	}
}


void lua_syck_output_handler(SyckEmitter *e, char *str, long len)
{
	struct emitter_xtra *bonus = (struct emitter_xtra *)e->bonus;
	luaL_addlstring(&bonus->output, str, len);
}

static int syck_load(lua_State *L)
{
	struct parser_xtra *bonus;
	SyckParser *parser;
	SYMID v;
	int obj;

	if (!luaL_checkstring(L, 1))
		luaL_typerror(L, 1, "string");

	parser = syck_new_parser();
	parser->bonus = S_ALLOC_N(struct emitter_xtra, 1);

	bonus = (struct parser_xtra *)parser->bonus;
	bonus->L = lua_newthread(L);

	syck_parser_str(parser, (char *)lua_tostring(L, 1), lua_strlen(L, 1), NULL);
	syck_parser_handler(parser, lua_syck_parser_handler);
	v = syck_parse(parser);
	syck_lookup_sym(parser, v, (char **)&obj);

	syck_free_parser(parser);

	lua_xmove(bonus->L, L, 1);

	return 1;
}

static int syck_dump(lua_State *L)
{
	SyckEmitter *emitter;
	struct emitter_xtra *bonus;

	emitter = syck_new_emitter();
	emitter->bonus = S_ALLOC_N(struct emitter_xtra, 1);

	bonus = (struct emitter_xtra *)emitter->bonus;
	bonus->L = lua_newthread(L);
	bonus->id = 1;
	luaL_buffinit(L, &bonus->output);

	syck_emitter_handler(emitter, lua_syck_emitter_handler);
	syck_output_handler(emitter, lua_syck_output_handler);

	lua_pushvalue(L, -2);
	lua_xmove(L, bonus->L, 1);

	lua_syck_mark_emitter(emitter, -1);

	syck_emit(emitter, -1);
	syck_emitter_flush(emitter, 0);

	luaL_pushresult(&bonus->output);
	syck_free_emitter(emitter);

	return 1;
}

static const luaL_reg sycklib[] = {
	{"load",	syck_load },
	{"dump",	syck_dump },
	{NULL, NULL}
};

LUALIB_API int luaopen_syck(lua_State *L)
{
	luaL_openlib(L, "syck", sycklib, 0);
	return 1;
}
