/*#io
YAML ioDoc(
		    docCopyright("why the lucky stiff", 2006)
		    docLicense("BSD revised")
		    docObject("YAML")
		    docDescription("""YAML is a simple text language for sending lists, maps and objects containing primitive types.""")
		    docCategory("Databases")
		    */

#include "IoYAML.h"
#include "IoState.h"
#include "IoNumber.h"
#include "IoList.h"
#include "IoMap.h"
#include "IoSeq.h"
#include "UArray.h"

#define DATA(self) ((IoYAMLData *)IoObject_dataPointer(self))

IoTag *IoYAML_newTag(void *state)
{
	IoTag *tag = IoTag_newWithName_("YAML");
	IoTag_state_(tag, state);
	IoTag_cloneFunc_(tag, (IoTagCloneFunc *)IoYAML_rawClone);
	IoTag_freeFunc_(tag, (IoTagFreeFunc *)IoYAML_free);
	IoTag_markFunc_(tag, (IoTagMarkFunc *)IoYAML_mark);
	return tag;
}

IoYAML *IoYAML_proto(void *state) 
{
	IoObject *self = IoObject_new(state);
	//self->tag = IoYAML_tag(state);
	IoObject_tag_(self, IoYAML_newTag(state));
	
	IoObject_setDataPointer_(self, calloc(1, sizeof(IoYAMLData)));
	DATA(self)->path = IOSYMBOL(".");
	
	IoState_registerProtoWithFunc_(state, self, IoYAML_proto);
	
	{
		IoMethodTable methodTable[] = {
		{"load", IoYAML_load},
		{"dump", IoYAML_dump},
		{NULL, NULL},
		};
		IoObject_addMethodTable_(self, methodTable);
	}
	return self;
}

IoYAML *IoYAML_rawClone(IoYAML *proto) 
{ 
	IoObject *self = IoObject_rawClonePrimitive(proto);
	IoObject_setDataPointer_(self, cpalloc(IoObject_dataPointer(proto), sizeof(IoYAMLData)));
	return self;
}

/* ----------------------------------------------------------- */

IoYAML *IoYAML_new(void *state)
{
	IoObject *proto = IoState_protoWithInitFunction_(state, IoYAML_proto);
	return IOCLONE(proto);
}

IoYAML *IoYAML_newWithPath_(void *state, IoSymbol *path)
{
	IoYAML *self = IoYAML_new(state);
	DATA(self)->path = IOREF(path);
	return self;
}

void IoYAML_free(IoYAML *self) 
{ 
	free(IoObject_dataPointer(self)); 
}

void IoYAML_mark(IoYAML *self) 
{ 
	IoObject_shouldMark((IoObject *)DATA(self)->path); 
}

SYMID IoYAML_parseHandler(SyckParser *p, SyckNode *n)
{
  IoYAML *self = (IoYAML *)p->bonus;
	IoSymbol *o, *o2, *o3;
	SYMID oid;
	int i;

	switch (n->kind) {
		case syck_str_kind:
			if (n->type_id == NULL || strcmp(n->type_id, "str") == 0) {
        o = IOSEQ(n->data.str->ptr, n->data.str->len);
			}
			else if (strcmp(n->type_id, "null") == 0)
			{
        o = IONIL(self);
			}
			else if (strcmp(n->type_id, "bool#yes") == 0)
			{
        o = IOTRUE(self);
			}
			else if (strcmp(n->type_id, "bool#no") == 0)
			{
        o = IOFALSE(self);
			}
			else if (strcmp(n->type_id, "int#hex") == 0)
			{
				long intVal = strtol(n->data.str->ptr, NULL, 16);
        o = IONUMBER(intVal);
			}
      else if (strcmp(n->type_id, "int#base60") == 0)
      {
          char *ptr, *end;
          long sixty = 1;
          long total = 0;
          syck_str_blow_away_commas( n );
          ptr = n->data.str->ptr;
          end = n->data.str->ptr + n->data.str->len;
          while ( end > ptr )
          {
              long bnum = 0;
              char *colon = end - 1;
              while ( colon >= ptr && *colon != ':' )
              {
                  colon--;
              }
              if ( colon >= ptr && *colon == ':' ) *colon = '\0';

              bnum = strtol( colon + 1, NULL, 10 );
              total += bnum * sixty;
              sixty *= 60;
              end = colon;
          }
          o = IONUMBER(total);
      }
			else if (strcmp(n->type_id, "int") == 0)
			{
				long intVal = strtol(n->data.str->ptr, NULL, 10);
        o = IONUMBER(intVal);
			}
			else
			{
        o = IOSEQ(n->data.str->ptr, n->data.str->len);
			}
			break;

		case syck_seq_kind:
      o = IoList_new(IOSTATE);
			for ( i=0; i < n->data.list->idx; i++ )
			{
				oid = syck_seq_read(n, i);
				syck_lookup_sym(p, oid, (char **)&o2);
        IoList_rawAppend_(o, o2);
			}
			break;

		case syck_map_kind:
      o = IoMap_new(IOSTATE);
			for ( i=0; i < n->data.pairs->idx; i++ )
			{
				oid = syck_map_read(n, map_key, i);
				syck_lookup_sym(p, oid, (char **)&o2);
				oid = syck_map_read(n, map_value, i);
				syck_lookup_sym(p, oid, (char **)&o3);

        o2 = IoSeq_rawAsSymbol(o2);
        IoMap_rawAtPut(o, (IoSymbol *)o2, o3);
			}
			break;
	}
	oid = syck_add_sym(p, (char *)o);
	return oid;
}

IoObject *IoYAML_load(IoYAML *self, IoObject *locals, IoMessage *m)
{
	/*#io
	docSlot("load(aString)", 
		   "Loads YAML data from a string.")
	*/
	
	// struct parser_xtra *bonus;
	SyckParser *parser;
	SYMID v;
	IoObject *obj;
	IoSymbol *s = IoMessage_locals_seqArgAt_(m, locals, 0);
  char *str = CSTRING(s);

	parser = syck_new_parser();
	// parser->bonus = S_ALLOC_N(struct emitter_xtra, 1);

	// bonus = (struct parser_xtra *)parser->bonus;
	// bonus->L = lua_newthread(L);

  parser->bonus = (void *)self;
	syck_parser_str(parser, str, strlen(str), NULL);
	syck_parser_handler(parser, IoYAML_parseHandler);
	v = syck_parse(parser);
	syck_lookup_sym(parser, v, (char **)&obj);

	syck_free_parser(parser);

	return obj;
}

typedef struct {
  IoYAML *self;
  UArray *buffer;
} IoYAMLout;

static void IoYAML_emitHandler(SyckEmitter *e, st_data_t data)
{
	IoYAMLout *out = (IoYAMLout *)e->bonus;
  IoYAML *self = out->self;
  IoObject *obj = (IoObject *)data;
	
  if (ISNIL(obj))
  {
    syck_emit_scalar(e, "null", scalar_none, 0, 0, 0, "", 0);
  }
  else if (ISBOOL(obj))
  {
    char *bool_s = ISTRUE(obj) ? "true" : "false";
    syck_emit_scalar(e, "boolean", scalar_none, 0, 0, 0, (char *)bool_s, strlen(bool_s));
  }
  else if (ISSEQ(obj))
  {
     if(!obj) {} else syck_emit_scalar(e, "string", scalar_none, 0, 0, 0, (char *)IOSEQ_BYTES(obj), IOSEQ_LENGTH(obj));
  }
  else if (ISNUMBER(obj))
  {
    /* should handle floats as well */
    char buf[30];		/* find a better way, if possible */
    snprintf(buf, sizeof(buf), "%i", (int)CNUMBER(obj));
    syck_emit_scalar(e, "number", scalar_none, 0, 0, 0, buf, strlen(buf));
  }
  else if (ISLIST(obj))
  {
    syck_emit_seq(e, "seq", seq_none);
    size_t len = IoList_rawSize(obj);
    size_t i = 0;
    for (i = 0; i < len; i++)
    {
      syck_emit_item(e, (st_data_t)IoList_rawAt_(obj, i));
    }
    syck_emit_end(e);
  }
  else if (ISMAP(obj))
  {
    syck_emit_map(e, "map", map_none);
    IoList *keys = IoMap_rawKeys(obj);
    size_t len = IoList_rawSize(keys);
    size_t i = 0;
    for (i = 0; i < len; i++)
    {
      IoSymbol *k = IoList_rawAt_(keys, i);
      syck_emit_item(e, (st_data_t)k);
      syck_emit_item(e, (st_data_t)IoMap_rawAt(obj, k));
    }
    syck_emit_end(e);
  }
}

void IoYAML_outputHandler(SyckEmitter *e, char *str, long len)
{
	IoYAMLout *out = (IoYAMLout *)e->bonus;
  UArray_appendBytes_size_(out->buffer, str, len);
}

IoObject *IoYAML_dump(IoYAML *self, IoObject *locals, IoMessage *m)
{
  UArray *buffer = UArray_new();
	IoObject *obj = IoMessage_locals_valueArgAt_(m, locals, 0);
	SyckEmitter *emitter = syck_new_emitter();
  IoYAMLout *out = S_ALLOC(IoYAMLout);
  out->self = self;
  out->buffer = buffer;
  emitter->bonus = (void *)out;

	syck_emitter_handler(emitter, IoYAML_emitHandler);
	syck_output_handler(emitter, IoYAML_outputHandler);
	syck_emit(emitter, (st_data_t)obj);
	syck_emitter_flush(emitter, 0);
	syck_free_emitter(emitter);
  S_FREE(out);

  return IoSeq_newWithUArray_copy_(IOSTATE, buffer, 0);
}
