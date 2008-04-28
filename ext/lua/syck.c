/*
 * syck.c
 *
 * $Author$
 * $Date$
 *
 * Copyright (C) 2005 Zachary P. Landau <kapheine@divineinvasion.net>
 * slact touched this in 2008. blame him.
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
	lua_State *L; //syck parser thread state
	lua_State *orig; //original API stack state
};

/*
static void stackDump (lua_State *L) {
      int i;
      int top = lua_gettop(L);
      for (i = 1; i <= top; i++) {  // repeat for each level 
        int t = lua_type(L, i);
		printf("%i:	", (top-i+1));
		switch (t) {
    
          case LUA_TSTRING:  // strings 
            printf("[string]");
            break;
    
          case LUA_TBOOLEAN:  // booleans 
            printf(lua_toboolean(L, i) ? "true" : "false");
            break;
    
          case LUA_TNUMBER:  // numbers 
            printf("%g", lua_tonumber(L, i));
            break;
    
          default:  // other values 
            printf("%s", lua_typename(L, t));
            break;
    
        }
        printf("\n");  // put a separator 
      }
      printf("\n----------\n");  // end the listing 
	  }
*/
SYMID
lua_syck_parser_handler(SyckParser *p, SyckNode *n)
{
	struct parser_xtra *bonus = (struct parser_xtra *)p->bonus;
	int o, o2, o3 = -1;
	SYMID oid;
	int i;

	if(!lua_checkstack(bonus->L, 1))
		luaL_error(bonus->orig,"syck parser wanted too much stack space.");
	
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
			else if (strcmp(n->type_id, "float") == 0 || strcmp(n->type_id, "float#fix") == 0 || strcmp(n->type_id, "float#exp") == 0) 
			{
				double f;
				syck_str_blow_away_commas(n);
				f = strtod(n->data.str->ptr, NULL);
				lua_pushnumber(bonus->L, f);
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
	int type = lua_type(bonus->L, -1);
	char buf[32];		/* find a better way, if possible */
	
	switch (type)
	{
		case LUA_TBOOLEAN:
			if (lua_toboolean(bonus->L, -1))
				strcpy(buf, "true");
			else
				strcpy(buf, "false");
			syck_emit_scalar(e, "boolean", scalar_none, 0, 0, 0, (char *)buf, strlen(buf));
			break;
		case LUA_TSTRING:
			syck_emit_scalar(e, "string", scalar_none, 0, 0, 0, (char *)lua_tostring(bonus->L, -1), lua_strlen(bonus->L, -1));
			break;
		case LUA_TNUMBER:
			; lua_Number lnum; //is it an int or a float?
			lnum = lua_tonumber(bonus->L, -1);
			int asInt = lnum;
			if(asInt == lnum)
				snprintf(buf, sizeof(buf), "%i", asInt);
			else
			{
				snprintf(buf, sizeof(buf), "%f", lnum);
				/* Remove trailing zeroes after the decimal point */
				int k;
				for (k = strlen(buf) - 1; buf[k] == '0' && buf[k - 1] != '.'; k--)
					buf[k] = '\0';
			}
			syck_emit_scalar(e, "number", scalar_none, 0, 0, 0, buf, strlen(buf));
			break;
		case LUA_TTABLE:
			if (luaL_getn(bonus->L, -1) > 0) {			/* treat it as an array */
				syck_emit_seq(e, "table", seq_none);
				lua_pushnil(bonus->L);  /* first key */
				while (lua_next(bonus->L, -2) != 0) {
					/* `key' is at index -2 and `value' at index -1 */
					syck_emit_item(e, bonus->id++);
					lua_pop(bonus->L, 1);  /* removes `value'; keeps `key' for next iteration */

				}
				syck_emit_end(e);
			} else {									/* treat it as a map */
				syck_emit_map(e, "table", map_none);
				lua_pushnil(bonus->L);
				while (lua_next(bonus->L, -2) != 0) {
					lua_pushvalue(bonus->L, -2);
					syck_emit_item(e, bonus->id++);
					lua_pop(bonus->L, 1);
					syck_emit_item(e, bonus->id++);
					lua_pop(bonus->L, 1);
				}
				syck_emit_end(e);
			}
			break;
	}

	bonus->id++;
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

void lua_syck_error_handler(SyckParser *p, char *msg)
{
	//struct parser_xtra *bonus = (struct parser_xtra *)p->bonus;
	luaL_error(((struct parser_xtra *)p->bonus)->orig, "Error at [Line %d, Col %d]: %s\n", p->linect, p->cursor - p->lineptr, msg );
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
	parser->bonus = S_ALLOC_N(struct parser_xtra, 1);

	bonus = (struct parser_xtra *)parser->bonus;
	bonus->orig = L;
	bonus->L = lua_newthread(L);

	syck_parser_str(parser, (char *)lua_tostring(L, 1), lua_strlen(L, 1), NULL);
	syck_parser_handler(parser, lua_syck_parser_handler);
	syck_parser_error_handler(parser, lua_syck_error_handler);
	v = syck_parse(parser);
	syck_lookup_sym(parser, v, (char **)&obj);

	syck_free_parser(parser);

	lua_pop(L,1); //pop the thread, we don't need it anymore.
	lua_xmove(bonus->L, L, 1);

	if ( parser->bonus != NULL )
		S_FREE( parser->bonus );
	
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
	luaL_buffinit(L, &bonus->output);

	syck_emitter_handler(emitter, lua_syck_emitter_handler);
	syck_output_handler(emitter, lua_syck_output_handler);

	lua_pushvalue(L, -2);
	lua_xmove(L, bonus->L, 1);

	bonus->id = 1;
	lua_syck_mark_emitter(emitter, bonus->id);

	bonus->id = 1;
	syck_emit(emitter, bonus->id);
	syck_emitter_flush(emitter, 0);

	luaL_pushresult(&bonus->output);
	
	syck_free_emitter(emitter);
	
	if (bonus != NULL )
		S_FREE( bonus );
	

	return 1;
}

static const luaL_reg sycklib[] = {
	{"load",	syck_load },
	{"dump",	syck_dump },
	{NULL, NULL}
};


static void set_info (lua_State *L)
{
// Assumes the table is on top of the stack.
	lua_pushliteral (L, "_COPYRIGHT");
	lua_pushliteral (L, "Copyright (C) why the lucky stiff");
	lua_settable (L, -3);
	lua_pushliteral (L, "_DESCRIPTION");
	lua_pushliteral (L, "YAML handling through the syck library");
	lua_settable (L, -3);
	lua_pushliteral (L, "_VERSION");
	lua_pushliteral (L, "0.56");
	lua_settable (L, -3);
}

LUALIB_API int luaopen_syck(lua_State *L)
{
	luaL_openlib(L, "syck", sycklib, 0);
	set_info(L);
	return 1;
}