/*
 * rubyext.c
 *
 * $Author$
 * $Date$
 *
 * Copyright (C) 2003-2005 why the lucky stiff
 */

#include "ruby.h"
#include "syck.h"
#include <sys/types.h>
#include <time.h>

typedef struct RVALUE {
    union {
#if 0
	struct {
	    unsigned long flags;	/* always 0 for freed obj */
	    struct RVALUE *next;
	} free;
#endif
	struct RBasic  basic;
	struct RObject object;
	struct RClass  klass;
	/*struct RFloat  flonum;*/
	/*struct RString string;*/
	struct RArray  array;
	/*struct RRegexp regexp;*/
	struct RHash   hash;
	/*struct RData   data;*/
	struct RStruct rstruct;
	/*struct RBignum bignum;*/
	/*struct RFile   file;*/
    } as;
} RVALUE;

typedef struct {
   long hash;
   char *buffer;
   long length;
   long remaining;
   int  printed;
} bytestring_t;

#define RUBY_DOMAIN   "ruby.yaml.org,2002"

#ifndef StringValue
#define StringValue(v)
#endif
#ifndef rb_attr_get
#define rb_attr_get(o, i)  rb_ivar_get(o, i)
#endif

/*
 * symbols and constants
 */
static ID s_new, s_utc, s_at, s_to_f, s_to_i, s_read, s_binmode, s_call, s_cmp, s_transfer, s_update, s_dup, s_haskey, s_match, s_keys, s_to_str, s_unpack, s_tr_bang, s_default_set, s_tag_subclasses, s_resolver, s_push, s_block, s_emitter, s_level, s_detect_implicit, s_node_import, s_out, s_input, s_intern, s_yaml_new, s_yaml_initialize, s_to_yaml, s_write, s_set_resolver;
static ID s_tags, s_domain, s_kind, s_name, s_options, s_type_id, s_value;
static VALUE sym_model, sym_generic, sym_input, sym_bytecode;
static VALUE sym_scalar, sym_seq, sym_map;
static VALUE cDate, cParser, cResolver, cNode, cCNode, cPrivateType, cDomainType, cBadAlias, cDefaultKey, cMergeKey, cEmitter;
static VALUE cOut, cOutSeq, cOutMap, cOutSeq, cOutScalar;
static VALUE oDefaultResolver, oGenericResolver;

/*
 * my private collection of numerical oddities.
 */
static double S_zero()    { return 0.0; }
static double S_one() { return 1.0; }
static double S_inf() { return S_one() / S_zero(); }
static double S_nan() { return S_zero() / S_zero(); }

static VALUE syck_node_transform( VALUE );

/*
 * handler prototypes
 */
SYMID rb_syck_parse_handler _((SyckParser *, SyckNode *));
SYMID rb_syck_load_handler _((SyckParser *, SyckNode *));
void rb_syck_err_handler _((SyckParser *, char *));
SyckNode * rb_syck_bad_anchor_handler _((SyckParser *, char *));
void rb_syck_output_handler _((SyckEmitter *, char *, long));
void rb_syck_emitter_handler _((SyckEmitter *, st_data_t));
int syck_parser_assign_io _((SyckParser *, VALUE));

struct parser_xtra {
    VALUE data;  /* Borrowed this idea from marshal.c to fix [ruby-core:8067] problem */
    VALUE proc;
    VALUE resolver;
    int taint;
};

struct emitter_xtra {
    VALUE oid;
    VALUE data;
    VALUE port;
};

/*
 * Convert YAML to bytecode
 */
VALUE
rb_syck_compile(self, port)
	VALUE self, port;
{
    SYMID oid;
    int taint;
    char *ret;
    VALUE bc;
    bytestring_t *sav; 

    SyckParser *parser = syck_new_parser();
	taint = syck_parser_assign_io(parser, port);
    syck_parser_handler( parser, syck_yaml2byte_handler );
    syck_parser_error_handler( parser, NULL );
    syck_parser_implicit_typing( parser, 0 );
    syck_parser_taguri_expansion( parser, 0 );
    oid = syck_parse( parser );
    syck_lookup_sym( parser, oid, (char **)&sav );

    ret = S_ALLOC_N( char, strlen( sav->buffer ) + 3 );
    ret[0] = '\0';
    strcat( ret, "D\n" );
    strcat( ret, sav->buffer );

    syck_free_parser( parser );

    bc = rb_str_new2( ret );
    if ( taint )      OBJ_TAINT( bc );
    return bc;
}

/*
 * read from io.
 */
long
rb_syck_io_str_read( char *buf, SyckIoStr *str, long max_size, long skip )
{
    long len = 0;

    ASSERT( str != NULL );
    max_size -= skip;

    if ( max_size <= 0 ) max_size = 0;
    else
    {
        /*
         * call io#read.
         */
        VALUE src = (VALUE)str->ptr;
        VALUE n = LONG2NUM(max_size);
        VALUE str2 = rb_funcall2(src, s_read, 1, &n);
        if (!NIL_P(str2))
        {
            len = RSTRING(str2)->len;
            memcpy( buf + skip, RSTRING(str2)->ptr, len );
        }
    }
    len += skip;
    buf[len] = '\0';
    return len;
}

/*
 * determine: are we reading from a string or io?
 * (returns tainted? boolean)
 */
int
syck_parser_assign_io(parser, port)
	SyckParser *parser;
	VALUE port;
{
    int taint = Qtrue;
    if (rb_respond_to(port, s_to_str)) {
	    taint = OBJ_TAINTED(port); /* original taintedness */
	    StringValue(port);	       /* possible conversion */
	    syck_parser_str( parser, RSTRING(port)->ptr, RSTRING(port)->len, NULL );
    }
    else if (rb_respond_to(port, s_read)) {
        if (rb_respond_to(port, s_binmode)) {
            rb_funcall2(port, s_binmode, 0, 0);
        }
        syck_parser_str( parser, (char *)port, 0, rb_syck_io_str_read );
    }
    else {
        rb_raise(rb_eTypeError, "instance of IO needed");
    }
    return taint;
}

/*
 * Get value in hash by key, forcing an empty hash if nil.
 */
VALUE
syck_get_hash_aref(hsh, key)
    VALUE hsh, key;
{
   VALUE val = rb_hash_aref( hsh, key );
   if ( NIL_P( val ) ) 
   {
       val = rb_hash_new();
       rb_hash_aset(hsh, key, val);
   }
   return val;
}

/*
 * creating timestamps
 */
SYMID
rb_syck_mktime(str, len)
    char *str;
    long len;
{
    VALUE time;
    char *ptr = str;
    VALUE year = INT2FIX(0);
    VALUE mon = INT2FIX(0);
    VALUE day = INT2FIX(0);
    VALUE hour = INT2FIX(0);
    VALUE min = INT2FIX(0);
    VALUE sec = INT2FIX(0);
    long usec;

    /* Year*/
    if ( ptr[0] != '\0' && len > 0 ) {
        year = INT2FIX(strtol(ptr, NULL, 10));
    }

    /* Month*/
    ptr += 4;
    if ( ptr[0] != '\0' && len > ptr - str ) {
        while ( !ISDIGIT( *ptr ) ) ptr++;
        mon = INT2FIX(strtol(ptr, NULL, 10));
    }

    /* Day*/
    ptr += 2;
    if ( ptr[0] != '\0' && len > ptr - str ) {
        while ( !ISDIGIT( *ptr ) ) ptr++;
        day = INT2FIX(strtol(ptr, NULL, 10));
    }

    /* Hour*/
    ptr += 2;
    if ( ptr[0] != '\0' && len > ptr - str ) {
        while ( !ISDIGIT( *ptr ) ) ptr++;
        hour = INT2FIX(strtol(ptr, NULL, 10));
    }

    /* Minute */
    ptr += 2;
    if ( ptr[0] != '\0' && len > ptr - str ) {
        while ( !ISDIGIT( *ptr ) ) ptr++;
        min = INT2FIX(strtol(ptr, NULL, 10));
    }

    /* Second */
    ptr += 2;
    if ( ptr[0] != '\0' && len > ptr - str ) {
        while ( !ISDIGIT( *ptr ) ) ptr++;
        sec = INT2FIX(strtol(ptr, NULL, 10));
    }

    /* Millisecond */
    ptr += 2;
    if ( len > ptr - str && *ptr == '.' )
    {
        char *padded = syck_strndup( "000000", 6 );
        char *end = ptr + 1;
        while ( isdigit( *end ) ) end++;
        MEMCPY(padded, ptr + 1, char, end - (ptr + 1));
        usec = strtol(padded, NULL, 10);
    }
    else
    {
        usec = 0;
    }

    /* Time Zone*/
    while ( len > ptr - str && *ptr != 'Z' && *ptr != '+' && *ptr != '-' && *ptr != '\0' ) ptr++;
    if ( len > ptr - str && ( *ptr == '-' || *ptr == '+' ) )
    {
        time_t tz_offset = strtol(ptr, NULL, 10) * 3600;
        time_t tmp;

        while ( *ptr != ':' && *ptr != '\0' ) ptr++;
        if ( *ptr == ':' )
        {
            ptr += 1;
            if ( tz_offset < 0 )
            {
                tz_offset -= strtol(ptr, NULL, 10) * 60;
            }
            else
            {
                tz_offset += strtol(ptr, NULL, 10) * 60;
            }
        }

        /* Make TZ time*/
        time = rb_funcall(rb_cTime, s_utc, 6, year, mon, day, hour, min, sec);
        tmp = NUM2LONG(rb_funcall(time, s_to_i, 0)) - tz_offset;
        return rb_funcall(rb_cTime, s_at, 2, LONG2NUM(tmp), LONG2NUM(usec));
    }
    else
    {
        /* Make UTC time*/
        return rb_funcall(rb_cTime, s_utc, 7, year, mon, day, hour, min, sec, LONG2NUM(usec));
    }
}

/*
 * handles merging of an array of hashes
 * (see http://www.yaml.org/type/merge/)
 */
VALUE
syck_merge_i( entry, hsh )
    VALUE entry, hsh;
{
	if ( rb_obj_is_kind_of( entry, rb_cHash ) )
	{
		rb_funcall( hsh, s_update, 1, entry );
	}
    return Qnil;
}

/*
 * build a syck node from a Ruby VALUE
 */
SyckNode *
rb_new_syck_node( obj, type_id )
    VALUE obj, type_id;
{
    long i = 0;
    SyckNode *n = NULL;

    if (rb_respond_to(obj, s_to_str)) 
    {
	    StringValue(obj);	       /* possible conversion */
        n = syck_alloc_str();
        n->data.str->ptr = RSTRING(obj)->ptr;
        n->data.str->len = RSTRING(obj)->len;
    }
	else if ( rb_obj_is_kind_of( obj, rb_cArray ) )
    {
        n = syck_alloc_seq();
        for ( i = 0; i < RARRAY(obj)->len; i++ )
        {
            syck_seq_add(n, rb_ary_entry(obj, i));
        }
    }
    else if ( rb_obj_is_kind_of( obj, rb_cHash ) )
    {
        VALUE keys;
        n = syck_alloc_map();
        keys = rb_funcall( obj, s_keys, 0 );
        for ( i = 0; i < RARRAY(keys)->len; i++ )
        {
            VALUE key = rb_ary_entry(keys, i);
            syck_map_add(n, key, rb_hash_aref(obj, key));
        }
    }

    if ( n!= NULL && rb_respond_to( type_id, s_to_str ) ) 
    {
        StringValue(type_id);
        n->type_id = syck_strndup( RSTRING(type_id)->ptr, RSTRING(type_id)->len );
    }

    return n;
}

/*
 * default handler for ruby.yaml.org types
 */
int
yaml_org_handler( n, ref )
    SyckNode *n;
    VALUE *ref;
{
    char *type_id = n->type_id;
    int transferred = 0;
    long i = 0;
    VALUE obj = Qnil;

    if ( type_id != NULL && strncmp( type_id, "tag:yaml.org,2002:", 18 ) == 0 )
    {
        type_id += 18;
    }

    switch (n->kind)
    {
        case syck_str_kind:
            transferred = 1;
            if ( type_id == NULL )
            {
                obj = rb_str_new( n->data.str->ptr, n->data.str->len );
            }
            else if ( strcmp( type_id, "null" ) == 0 )
            {
                obj = Qnil;
            }
            else if ( strcmp( type_id, "binary" ) == 0 )
            {
                VALUE arr;
                obj = rb_str_new( n->data.str->ptr, n->data.str->len );
                rb_funcall( obj, s_tr_bang, 2, rb_str_new2( "\n\t " ), rb_str_new2( "" ) );
                arr = rb_funcall( obj, s_unpack, 1, rb_str_new2( "m" ) );
                obj = rb_ary_shift( arr );
            }
            else if ( strcmp( type_id, "bool#yes" ) == 0 )
            {
                obj = Qtrue;
            }
            else if ( strcmp( type_id, "bool#no" ) == 0 )
            {
                obj = Qfalse;
            }
            else if ( strcmp( type_id, "int#hex" ) == 0 )
            {
                syck_str_blow_away_commas( n );
                obj = rb_cstr2inum( n->data.str->ptr, 16 );
            }
            else if ( strcmp( type_id, "int#oct" ) == 0 )
            {
                syck_str_blow_away_commas( n );
                obj = rb_cstr2inum( n->data.str->ptr, 8 );
            }
            else if ( strcmp( type_id, "int#base60" ) == 0 )
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
                    if ( *colon == ':' ) *colon = '\0';

                    bnum = strtol( colon + 1, NULL, 10 );
                    total += bnum * sixty;
                    sixty *= 60;
                    end = colon;
                }
                obj = INT2FIX(total);
            }
            else if ( strncmp( type_id, "int", 3 ) == 0 )
            {
                syck_str_blow_away_commas( n );
                obj = rb_cstr2inum( n->data.str->ptr, 10 );
            }
            else if ( strcmp( type_id, "float#base60" ) == 0 )
            {
                char *ptr, *end;
                long sixty = 1;
                double total = 0.0;
                syck_str_blow_away_commas( n );
                ptr = n->data.str->ptr;
                end = n->data.str->ptr + n->data.str->len;
                while ( end > ptr )
                {
                    double bnum = 0;
                    char *colon = end - 1;
                    while ( colon >= ptr && *colon != ':' )
                    {
                        colon--;
                    }
                    if ( *colon == ':' ) *colon = '\0';

                    bnum = strtod( colon + 1, NULL );
                    total += bnum * sixty;
                    sixty *= 60;
                    end = colon;
                }
                obj = rb_float_new( total );
            }
            else if ( strcmp( type_id, "float#nan" ) == 0 )
            {
                obj = rb_float_new( S_nan() );
            }
            else if ( strcmp( type_id, "float#inf" ) == 0 )
            {
                obj = rb_float_new( S_inf() );
            }
            else if ( strcmp( type_id, "float#neginf" ) == 0 )
            {
                obj = rb_float_new( -S_inf() );
            }
            else if ( strncmp( type_id, "float", 5 ) == 0 )
            {
                double f;
                syck_str_blow_away_commas( n );
                f = strtod( n->data.str->ptr, NULL );
                obj = rb_float_new( f );
            }
            else if ( strcmp( type_id, "timestamp#iso8601" ) == 0 )
            {
                obj = rb_syck_mktime( n->data.str->ptr, n->data.str->len );
            }
            else if ( strcmp( type_id, "timestamp#spaced" ) == 0 )
            {
                obj = rb_syck_mktime( n->data.str->ptr, n->data.str->len );
            }
            else if ( strcmp( type_id, "timestamp#ymd" ) == 0 )
            {
                char *ptr = n->data.str->ptr;
                VALUE year, mon, day;

                /* Year*/
                ptr[4] = '\0';
                year = INT2FIX(strtol(ptr, NULL, 10));

                /* Month*/
                ptr += 4;
                while ( !ISDIGIT( *ptr ) ) ptr++;
                mon = INT2FIX(strtol(ptr, NULL, 10));

                /* Day*/
                ptr += 2;
                while ( !ISDIGIT( *ptr ) ) ptr++;
                day = INT2FIX(strtol(ptr, NULL, 10));

                if ( !cDate ) {
                    /*
                     * Load Date module
                     */
                    rb_require( "date" );
                    cDate = rb_const_get( rb_cObject, rb_intern("Date") );
                }

                obj = rb_funcall( cDate, s_new, 3, year, mon, day );
            }
            else if ( strncmp( type_id, "timestamp", 9 ) == 0 )
            {
                obj = rb_syck_mktime( n->data.str->ptr, n->data.str->len );
            }
			else if ( strncmp( type_id, "merge", 5 ) == 0 )
			{
				obj = rb_funcall( cMergeKey, s_new, 0 );
			}
			else if ( strncmp( type_id, "default", 7 ) == 0 )
			{
				obj = rb_funcall( cDefaultKey, s_new, 0 );
			}
            else if ( n->data.str->style == scalar_plain &&
                      n->data.str->len > 1 && 
                      strncmp( n->data.str->ptr, ":", 1 ) == 0 )
            {
                obj = rb_funcall( oDefaultResolver, s_transfer, 2, 
                                  rb_str_new2( "ruby/sym" ), 
                                  rb_str_new( n->data.str->ptr + 1, n->data.str->len - 1 ) );
            }
            else if ( strcmp( type_id, "str" ) == 0 )
            {
                obj = rb_str_new( n->data.str->ptr, n->data.str->len );
            }
            else
            {
                transferred = 0;
                obj = rb_str_new( n->data.str->ptr, n->data.str->len );
            }
        break;

        case syck_seq_kind:
            if ( type_id == NULL || strcmp( type_id, "seq" ) == 0 )
            {
                transferred = 1;
            }
            obj = rb_ary_new2( n->data.list->idx );
            for ( i = 0; i < n->data.list->idx; i++ )
            {
                rb_ary_store( obj, i, syck_seq_read( n, i ) );
            }
        break;

        case syck_map_kind:
            if ( type_id == NULL || strcmp( type_id, "map" ) == 0 )
            {
                transferred = 1;
            }
            obj = rb_hash_new();
            for ( i = 0; i < n->data.pairs->idx; i++ )
            {
				VALUE k = syck_map_read( n, map_key, i );
				VALUE v = syck_map_read( n, map_value, i );
				int skip_aset = 0;

				/*
				 * Handle merge keys
				 */
				if ( rb_obj_is_kind_of( k, cMergeKey ) )
				{
					if ( rb_obj_is_kind_of( v, rb_cHash ) )
					{
						VALUE dup = rb_funcall( v, s_dup, 0 );
						rb_funcall( dup, s_update, 1, obj );
						obj = dup;
						skip_aset = 1;
					}
					else if ( rb_obj_is_kind_of( v, rb_cArray ) )
					{
						VALUE end = rb_ary_pop( v );
						if ( rb_obj_is_kind_of( end, rb_cHash ) )
						{
							VALUE dup = rb_funcall( end, s_dup, 0 );
							v = rb_ary_reverse( v );
							rb_ary_push( v, obj );
							rb_iterate( rb_each, v, syck_merge_i, dup );
							obj = dup;
							skip_aset = 1;
						}
					}
				}
                else if ( rb_obj_is_kind_of( k, cDefaultKey ) )
                {
                    rb_funcall( obj, s_default_set, 1, v );
                    skip_aset = 1;
                }

				if ( ! skip_aset )
				{
					rb_hash_aset( obj, k, v );
				}
            }
        break;
    }

    *ref = obj;
    return transferred;
}

/*
 * {native mode} node handler
 * - Converts data into native Ruby types
 */
SYMID
rb_syck_load_handler(p, n)
    SyckParser *p;
    SyckNode *n;
{
    VALUE obj = Qnil;
    struct parser_xtra *bonus = (struct parser_xtra *)p->bonus;
    VALUE resolver = bonus->resolver;
    if ( NIL_P( resolver ) )
    {
        resolver = oDefaultResolver;
    }

    /*
     * Check for resolver
     */
    obj = rb_funcall( resolver, s_node_import, 1, Data_Wrap_Struct( cCNode, NULL, NULL, n ) );

    /*
     * ID already set, let's alter the symbol table to accept the new object
     */
    if (n->id > 0 && !NIL_P(obj))
    {
        MEMCPY((void *)n->id, (void *)obj, RVALUE, 1);
        MEMZERO((void *)obj, RVALUE, 1);
        obj = n->id;
    }

    if ( bonus->taint)      OBJ_TAINT( obj );
	if ( bonus->proc != 0 ) rb_funcall(bonus->proc, s_call, 1, obj);

    rb_hash_aset(bonus->data, INT2FIX(RHASH(bonus->data)->tbl->num_entries), obj);
    return obj;
}

/*
 * friendly errors.
 */
void
rb_syck_err_handler(p, msg)
    SyckParser *p;
    char *msg;
{
    char *endl = p->cursor;

    while ( *endl != '\0' && *endl != '\n' )
        endl++;

    endl[0] = '\0';
    rb_raise(rb_eArgError, "%s on line %d, col %d: `%s'",
           msg,
           p->linect,
           p->cursor - p->lineptr, 
           p->lineptr); 
}

/*
 * provide bad anchor object to the parser.
 */
SyckNode *
rb_syck_bad_anchor_handler(p, a)
    SyckParser *p;
    char *a;
{
    VALUE anchor_name = rb_str_new2( a );
    SyckNode *badanc = syck_new_map( rb_str_new2( "name" ), anchor_name );
    badanc->type_id = syck_strndup( "tag:ruby.yaml.org,2002:object:YAML::Syck::BadAlias", 53 );
    return badanc;
}

/*
 * data loaded based on the model requested.
 */
void
syck_set_model( p, input, model )
	VALUE p, input, model;
{
    SyckParser *parser;
	Data_Get_Struct(p, SyckParser, parser);
    syck_parser_handler( parser, rb_syck_load_handler );
    /* WARN: gonna be obsoleted soon!! */
	if ( model == sym_generic )
	{
        rb_funcall( p, s_set_resolver, 1, oGenericResolver );
	}
    syck_parser_implicit_typing( parser, 1 );
    syck_parser_taguri_expansion( parser, 1 );

    if ( NIL_P( input ) )
    {
        input = rb_ivar_get( p, s_input ); 
    }
    if ( input == sym_bytecode )
    {
        syck_parser_set_input_type( parser, syck_bytecode_utf8 );
    }
    else
    {
        syck_parser_set_input_type( parser, syck_yaml_utf8 );
    }
    syck_parser_error_handler( parser, rb_syck_err_handler );
    syck_parser_bad_anchor_handler( parser, rb_syck_bad_anchor_handler );
}

/*
 * mark parser nodes
 */
static void
syck_mark_parser(parser)
    SyckParser *parser;
{
    struct parser_xtra *bonus;
    rb_gc_mark(parser->root);
    rb_gc_mark(parser->root_on_error);
    if ( parser->bonus != NULL )
    {
        bonus = (struct parser_xtra *)parser->bonus;
        rb_gc_mark( bonus->data );
        rb_gc_mark( bonus->proc );
    }
}

/*
 * Free the parser and any bonus attachment.
 */
void
rb_syck_free_parser(p)
    SyckParser *p;
{
    struct parser_xtra *bonus = (struct parser_xtra *)p->bonus;
    if ( bonus != NULL ) S_FREE( bonus );
    syck_free_parser(p);
}

/*
 * YAML::Syck::Parser.new
 */
VALUE 
syck_parser_new(argc, argv, class)
    int argc;
    VALUE *argv;
	VALUE class;
{
	VALUE pobj, resolver, options, init_argv[2];
    SyckParser *parser = syck_new_parser();

    rb_scan_args(argc, argv, "01*", &resolver, &options);
	pobj = Data_Wrap_Struct( class, syck_mark_parser, rb_syck_free_parser, parser );

    syck_parser_set_root_on_error( parser, Qnil );

    if ( ! rb_obj_is_instance_of( options, rb_cHash ) )
    {
        options = rb_hash_new();
    }
	init_argv[0] = resolver;
	init_argv[1] = options;
	rb_obj_call_init(pobj, 2, init_argv);
	return pobj;
}

/*
 * YAML::Syck::Parser.initialize( resolver, options )
 */
static VALUE
syck_parser_initialize( self, resolver, options )
    VALUE self, resolver, options;
{
    rb_ivar_set(self, s_resolver, resolver);
    rb_ivar_set(self, s_options, options);
	return self;
}

/*
 * YAML::Syck::Parser.bufsize = Integer
 */
static VALUE
syck_parser_bufsize_set( self, size )
    VALUE self, size;
{
	SyckParser *parser;

	Data_Get_Struct(self, SyckParser, parser);
    if ( rb_respond_to( size, s_to_i ) ) {
        parser->bufsize = NUM2INT(rb_funcall(size, s_to_i, 0));
    }
	return self;
}

/*
 * YAML::Syck::Parser.bufsize => Integer
 */
static VALUE
syck_parser_bufsize_get( self )
    VALUE self;
{
	SyckParser *parser;

	Data_Get_Struct(self, SyckParser, parser);
	return INT2FIX( parser->bufsize );
}

/*
 * YAML::Syck::Parser.load( IO or String )
 */
VALUE
syck_parser_load(argc, argv, self)
    int argc;
    VALUE *argv;
	VALUE self;
{
    VALUE port, proc, model, input;
	SyckParser *parser;
    struct parser_xtra *bonus = S_ALLOC_N( struct parser_xtra, 1 );
    volatile VALUE hash;	/* protect from GC */

    rb_scan_args(argc, argv, "11", &port, &proc);
	Data_Get_Struct(self, SyckParser, parser);

    input = rb_hash_aref( rb_attr_get( self, s_options ), sym_input );
    model = rb_hash_aref( rb_attr_get( self, s_options ), sym_model );
	syck_set_model( self, input, model );

	bonus->taint = syck_parser_assign_io(parser, port);
    bonus->data = hash = rb_hash_new();
    bonus->resolver = rb_attr_get( self, s_resolver );
	if ( NIL_P( proc ) ) bonus->proc = 0;
    else                 bonus->proc = proc;
    
	parser->bonus = (void *)bonus;

    return syck_parse( parser );
}

/*
 * YAML::Syck::Parser.load_documents( IO or String ) { |doc| }
 */
VALUE
syck_parser_load_documents(argc, argv, self)
    int argc;
    VALUE *argv;
	VALUE self;
{
    VALUE port, proc, v, input, model;
	SyckParser *parser;
    struct parser_xtra *bonus = S_ALLOC_N( struct parser_xtra, 1 );
    volatile VALUE hash;

    rb_scan_args(argc, argv, "1&", &port, &proc);
	Data_Get_Struct(self, SyckParser, parser);

    input = rb_hash_aref( rb_attr_get( self, s_options ), sym_input );
    model = rb_hash_aref( rb_attr_get( self, s_options ), sym_model );
	syck_set_model( self, input, model );
    
	bonus->taint = syck_parser_assign_io(parser, port);
    bonus->resolver = rb_attr_get( self, s_resolver );
    bonus->proc = 0;
    parser->bonus = (void *)bonus;

    while ( 1 )
	{
        /* Reset hash for tracking nodes */
        bonus->data = hash = rb_hash_new();

        /* Parse a document */
    	v = syck_parse( parser );
        if ( parser->eof == 1 )
        {
            break;
        }

        /* Pass document to block */
		rb_funcall( proc, s_call, 1, v );
	}

    return Qnil;
}

/*
 * YAML::Syck::Parser#set_resolver
 */
VALUE
syck_parser_set_resolver( self, resolver )
    VALUE self, resolver;
{
    rb_ivar_set( self, s_resolver, resolver );
    return self;
}

/*
 * YAML::Syck::Resolver.initialize
 */
static VALUE
syck_resolver_initialize( self )
    VALUE self;
{
    VALUE tags = rb_hash_new();
    rb_ivar_set(self, s_tags, rb_hash_new());
    return self;
}

/*
 * YAML::Syck::Resolver#add_type
 */
VALUE
syck_resolver_add_type( self, taguri, cls )
    VALUE self, taguri, cls;
{
    VALUE tags = rb_attr_get(self, s_tags);
    rb_hash_aset( tags, taguri, cls );
    return Qnil;
}

/*
 * YAML::Syck::Resolver#use_types_at
 */
VALUE
syck_resolver_use_types_at( self, hsh )
    VALUE self, hsh;
{
    rb_ivar_set( self, s_tags, hsh );
    return Qnil;
}

/*
 * YAML::Syck::Resolver#detect_implicit 
 */
VALUE
syck_resolver_detect_implicit( self, val )
    VALUE self, val;
{
    char *type_id;
    return rb_str_new2( "" );
}

/*
 * YAML::Syck::Resolver#node_import
 */
VALUE
syck_resolver_node_import( self, node )
    VALUE self, node;
{
    SyckNode *n;
    VALUE obj;
    int i = 0;
	Data_Get_Struct(node, SyckNode, n);

    switch (n->kind)
    {
        case syck_str_kind:
            obj = rb_str_new( n->data.str->ptr, n->data.str->len );
        break;

        case syck_seq_kind:
            obj = rb_ary_new2( n->data.list->idx );
            for ( i = 0; i < n->data.list->idx; i++ )
            {
                rb_ary_store( obj, i, syck_seq_read( n, i ) );
            }
        break;

        case syck_map_kind:
            obj = rb_hash_new();
            for ( i = 0; i < n->data.pairs->idx; i++ )
            {
				VALUE k = syck_map_read( n, map_key, i );
				VALUE v = syck_map_read( n, map_value, i );
				int skip_aset = 0;

				/*
				 * Handle merge keys
				 */
				if ( rb_obj_is_kind_of( k, cMergeKey ) )
				{
					if ( rb_obj_is_kind_of( v, rb_cHash ) )
					{
						VALUE dup = rb_funcall( v, s_dup, 0 );
						rb_funcall( dup, s_update, 1, obj );
						obj = dup;
						skip_aset = 1;
					}
					else if ( rb_obj_is_kind_of( v, rb_cArray ) )
					{
						VALUE end = rb_ary_pop( v );
						if ( rb_obj_is_kind_of( end, rb_cHash ) )
						{
							VALUE dup = rb_funcall( end, s_dup, 0 );
							v = rb_ary_reverse( v );
							rb_ary_push( v, obj );
							rb_iterate( rb_each, v, syck_merge_i, dup );
							obj = dup;
							skip_aset = 1;
						}
					}
				}
                else if ( rb_obj_is_kind_of( k, cDefaultKey ) )
                {
                    rb_funcall( obj, s_default_set, 1, v );
                    skip_aset = 1;
                }

				if ( ! skip_aset )
				{
					rb_hash_aset( obj, k, v );
				}
            }
        break;
    }

    if ( n->type_id != NULL )
    {
        obj = rb_funcall( self, s_transfer, 2, rb_str_new2( n->type_id ), obj );
    }
    return obj;
}

/*
 * YAML::Syck::Resolver#transfer
 */
VALUE
syck_resolver_transfer( self, type, val )
    VALUE self, type, val;
{
    if (NIL_P(type) || !RSTRING(type)->ptr || RSTRING(type)->len == 0) 
    {
        type = rb_funcall( self, s_detect_implicit, 1, val );
    }

    if ( ! (NIL_P(type) || !RSTRING(type)->ptr || RSTRING(type)->len == 0) )
    {
        VALUE tags = rb_attr_get(self, s_tags);
        VALUE target_class = rb_hash_aref( tags, type );
        VALUE subclass = Qnil;
        VALUE obj = Qnil;

        /*
         * Should no tag match exactly, check for subclass format
         */
        if ( NIL_P( target_class ) )
        {
            VALUE parts = rb_str_split( type, ":" );
            VALUE subclass_parts = rb_ary_new();
            VALUE colon = rb_str_new2( ":" );
            while ( RARRAY(parts)->len > 1 )
            {
                VALUE partial;
                rb_ary_unshift( subclass_parts, rb_ary_pop( parts ) );
                partial = rb_ary_join( parts, colon );
                target_class = rb_hash_aref( tags, partial );
                if ( NIL_P( target_class ) )
                {
                    rb_str_append( partial, colon );
                    target_class = rb_hash_aref( tags, partial );
                }

                /*
                 * Possible subclass found, see if it supports subclassing
                 */
                if ( ! NIL_P( target_class ) )
                {
                    if ( rb_respond_to( target_class, s_call ) )
                    {
                        break;
                    }
                    else if ( rb_respond_to( target_class, s_tag_subclasses ) &&
                         RTEST( rb_funcall( target_class, s_tag_subclasses, 0 ) ) )
                    {
                        subclass = rb_ary_join( subclass_parts, colon );
                        break;
                    }
                }

                target_class = Qnil;
            }
        }

        /* rb_raise(rb_eTypeError, "invalid typing scheme: %s given",
         *         scheme);
         */

        if ( TYPE(subclass) == T_STRING && rb_const_defined( rb_cObject, rb_funcall( subclass, s_intern, 0 ) ) )
        {
            subclass = rb_const_get( rb_cObject, rb_funcall( subclass, s_intern, 0 ) );
        }
        else
        {
            subclass = target_class;
        }

        if ( rb_respond_to( target_class, s_call ) )
        {
            obj = rb_funcall( target_class, s_call, 2, type, val );
        }
        else
        {
            if ( rb_respond_to( target_class, s_yaml_new ) )
            {
                obj = rb_funcall( subclass, s_yaml_new, 2, type, val );
            }
            else if ( !NIL_P( subclass ) )
            {
                obj = rb_obj_alloc( subclass );
            }

            if ( rb_respond_to( obj, s_yaml_initialize ) )
            {
                rb_funcall( obj, s_yaml_initialize, 2, type, val );
            }
        }
        val = obj;
    }

    return val;
}

/*
 * YAML::Syck::DefaultResolver#detect_implicit 
 */
VALUE
syck_defaultresolver_detect_implicit( self, val )
    VALUE self, val;
{
    char *type_id;

    if ( TYPE(val) == T_STRING )
    {
        StringValue(val);
        type_id = syck_match_implicit( RSTRING(val)->ptr, RSTRING(val)->len );
        return rb_str_new2( type_id );
    }

    return rb_str_new2( "" );
}

/*
 * YAML::Syck::DefaultResolver#node_import
 */
VALUE
syck_defaultresolver_node_import( self, node )
    VALUE self, node;
{
    SyckNode *n;
    VALUE obj;
	Data_Get_Struct( node, SyckNode, n );
    if ( !yaml_org_handler( n, &obj ) )
    {
        obj = rb_funcall( self, s_transfer, 2, rb_str_new2( n->type_id ), obj );
    }
    return obj;
}

/*
 * YAML::Syck::GenericResolver#node_import
 */
VALUE
syck_genericresolver_node_import( self, node )
    VALUE self, node;
{
    SyckNode *n;
    int i = 0;
    VALUE t, obj, v = Qnil;
	Data_Get_Struct(node, SyckNode, n);

    obj = rb_obj_alloc(cNode);
    if ( n->type_id != NULL )
    {
        t = rb_str_new2(n->type_id);
        rb_ivar_set(obj, s_type_id, t);
    }

    switch (n->kind)
    {
        case syck_str_kind:
            rb_ivar_set(obj, s_kind, sym_scalar);
            v = rb_str_new( n->data.str->ptr, n->data.str->len );
        break;

        case syck_seq_kind:
            rb_ivar_set(obj, s_kind, sym_seq);
            v = rb_ary_new2( n->data.list->idx );
            for ( i = 0; i < n->data.list->idx; i++ )
            {
                rb_ary_store( v, i, syck_seq_read( n, i ) );
            }
        break;

        case syck_map_kind:
            rb_ivar_set(obj, s_kind, sym_map);
            v = rb_hash_new();
            for ( i = 0; i < n->data.pairs->idx; i++ )
            {
                VALUE key = syck_node_transform( syck_map_read( n, map_key, i ) );
                VALUE val = rb_ary_new();
                rb_ary_push(val, syck_map_read( n, map_key, i ));
                rb_ary_push(val, syck_map_read( n, map_value, i ));

                rb_hash_aset( v, key, val );
            }
        break;
    }

    rb_ivar_set(obj, s_value, v);
    return obj;
}

/*
 * YAML::Syck::BadAlias.initialize
 */
VALUE
syck_badalias_initialize( self, val )
    VALUE self, val;
{
    rb_ivar_set( self, s_name, val );
    return self;
}

/*
 * YAML::Syck::BadAlias.<=>
 */
VALUE
syck_badalias_cmp( alias1, alias2 )
    VALUE alias1, alias2;
{
    VALUE str1 = rb_ivar_get( alias1, s_name ); 
    VALUE str2 = rb_ivar_get( alias2, s_name ); 
    VALUE val = rb_funcall( str1, s_cmp, 1, str2 );
    return val;
}

/*
 * YAML::Syck::DomainType.initialize
 */
VALUE
syck_domaintype_initialize( self, domain, type_id, val )
    VALUE self, type_id, val;
{
    rb_ivar_set( self, s_domain, domain );
    rb_ivar_set( self, s_type_id, type_id );
    rb_ivar_set( self, s_value, val );
    return self;
}

/*
 * YAML::Syck::PrivateType.initialize
 */
VALUE
syck_privatetype_initialize( self, type_id, val )
    VALUE self, type_id, val;
{
    rb_ivar_set( self, s_type_id, type_id );
    rb_ivar_set( self, s_value, val );
    return self;
}

/*
 * YAML::Syck::Node.initialize
 */
VALUE
syck_node_initialize( self, type_id, val )
    VALUE self, type_id, val;
{
    rb_ivar_set( self, s_type_id, type_id );
    rb_ivar_set( self, s_value, val );
    return self;
}

VALUE
syck_node_thash( entry, t )
    VALUE entry, t;
{
    VALUE key, val;
    key = rb_ary_entry( entry, 0 );
    val = syck_node_transform( rb_ary_entry( rb_ary_entry( entry, 1 ), 1 ) );
    rb_hash_aset( t, key, val );
    return Qnil;
}

VALUE
syck_node_ahash( entry, t )
    VALUE entry, t;
{
    VALUE val = syck_node_transform( entry );
    rb_ary_push( t, val );
    return Qnil;
}

/*
 * YAML::Syck::Node.transform
 */
VALUE
syck_node_transform( self )
    VALUE self;
{
    VALUE t = Qnil;
    VALUE type_id = rb_attr_get( self, s_type_id );
    VALUE val = rb_attr_get( self, s_value );
    if ( rb_obj_is_instance_of( val, rb_cHash ) )
    {
        t = rb_hash_new();
        rb_iterate( rb_each, val, syck_node_thash, t );
    }
    else if ( rb_obj_is_instance_of( val, rb_cArray ) )
    {
        t = rb_ary_new();
        rb_iterate( rb_each, val, syck_node_ahash, t );
    }
    else
    {
        t = val;
    }
    return rb_funcall( oDefaultResolver, s_transfer, 2, type_id, t );
}

/*
 * Emitter callback: assembles YAML document events from
 * Ruby symbols.  This is a brilliant way to do it.
 * No one could possibly object.
 */
void
rb_syck_emitter_handler(e, data)
    SyckEmitter *e;
    st_data_t data;
{
    VALUE n = (VALUE)data;
    VALUE tag = rb_ivar_get( n, s_type_id ); 
    char *type_id = NULL;
    if ( !NIL_P( tag ) ) {
        StringValue(tag);
        type_id = RSTRING(tag)->ptr;
    }

	if ( rb_obj_is_kind_of( n, cOutMap ) )
    {
        int i;
        syck_emit_map( e, type_id );
        for ( i = 0; i < RARRAY(n)->len; i++ )
        {
            syck_emit_item( e, (st_data_t)rb_ary_entry(n, i) );
        }
        syck_emit_end( e );
    }
    else if ( rb_obj_is_kind_of( n, cOutSeq ) )
    {
        int i;
        syck_emit_seq( e, type_id );
        for ( i = 0; i < RARRAY(n)->len; i++ )
        {
            syck_emit_item( e, (st_data_t)rb_ary_entry(n, i) );
        }
        syck_emit_end( e );
    }
    else if ( rb_obj_is_kind_of( n, cOutScalar ) )
    {
        syck_emit_scalar( e, type_id, block_arbitrary, 0, 0, 0, RSTRING(n)->ptr, RSTRING(n)->len );
    }
    else
    {
        // TODO: Handle strange nodes
        syck_emit_scalar( e, NULL, block_arbitrary, 0, 0, 0, "<Unknown>", 9 );
    }
}

/*
 * Handle output from the emitter
 */
void 
rb_syck_output_handler( emitter, str, len )
    SyckEmitter *emitter;
    char *str;
    long len;
{
    struct emitter_xtra *bonus = (struct emitter_xtra *)emitter->bonus;
    VALUE dest = bonus->port;
    if ( rb_respond_to( dest, s_to_str ) ) {
        rb_str_cat( dest, str, len );
    } else {
        rb_io_write( dest, rb_str_new( str, len ) );
    }
}

/*
 * Helper function for marking nodes in the anchor
 * symbol table.
 */
void
syck_out_mark( emitter, node )
    VALUE emitter, node;
{
    SyckEmitter *emitterPtr;
    struct emitter_xtra *bonus;
    Data_Get_Struct(emitter, SyckEmitter, emitterPtr);
    bonus = (struct emitter_xtra *)emitterPtr->bonus;
    /* syck_emitter_mark_node( emitterPtr, (st_data_t)node ); */
    if ( !NIL_P( bonus->oid ) ) {
        rb_hash_aset( bonus->data, bonus->oid, node );
    }
}

/*
 * Mark emitter values.
 */
static void
syck_mark_emitter(emitter)
    SyckEmitter *emitter;
{
    struct emitter_xtra *bonus;
    if ( emitter->bonus != NULL )
    {
        bonus = (struct emitter_xtra *)emitter->bonus;
        rb_gc_mark( bonus->data );
        rb_gc_mark( bonus->port );
    }
}

/*
 * Free the emitter and any bonus attachment.
 */
void
rb_syck_free_emitter(e)
    SyckEmitter *e;
{
    struct emitter_xtra *bonus = (struct emitter_xtra *)e->bonus;
    if ( bonus != NULL ) S_FREE( bonus );
    syck_free_emitter(e);
}

/*
 * YAML::Syck::Emitter.new
 */
VALUE 
syck_emitter_new(argc, argv, class)
    int argc;
    VALUE *argv;
	VALUE class;
{
	VALUE pobj, options, init_argv[1];
    SyckEmitter *emitter = syck_new_emitter();

    rb_scan_args(argc, argv, "01", &options);

	pobj = Data_Wrap_Struct( class, syck_mark_emitter, rb_syck_free_emitter, emitter );
    syck_emitter_handler( emitter, rb_syck_emitter_handler );
    syck_output_handler( emitter, rb_syck_output_handler );

	init_argv[0] = options;
	rb_obj_call_init(pobj, 1, init_argv);
    rb_ivar_set( pobj, s_out, rb_funcall( cOut, s_new, 1, pobj ) );
	return pobj;
}

/*
 * YAML::Syck::Emitter.reset( options )
 */
VALUE
syck_emitter_reset( self, options )
    VALUE self, options;
{
    SyckEmitter *emitter;
    struct emitter_xtra *bonus;
    volatile VALUE hash;	/* protect from GC */

	Data_Get_Struct(self, SyckEmitter, emitter);
    bonus = (struct emitter_xtra *)emitter->bonus;
    if ( bonus != NULL ) S_FREE( bonus );

    bonus = S_ALLOC_N( struct emitter_xtra, 1 );
	bonus->port = rb_str_new2( "" );
    bonus->data = hash = rb_hash_new();

    if ( ! rb_obj_is_instance_of( options, rb_cHash ) )
    {
        if ( rb_respond_to( options, s_to_str ) || rb_respond_to( options, s_write ) )
        {
            bonus->port = options;
        }
        options = rb_hash_new();
    }
    else
    {
        rb_ivar_set(self, s_options, options);
    }
    
    emitter->bonus = (void *)bonus;
    rb_ivar_set(self, s_level, INT2FIX(0));
    rb_ivar_set(self, s_resolver, Qnil);
	return self;
}

/*
 * YAML::Syck::Emitter.emit( object_id ) { |out| ... }
 */
VALUE
syck_emitter_emit( argc, argv, self )
    int argc;
    VALUE *argv;
    VALUE self;
{
    VALUE oid, proc;
    char *anchor_name;
    SyckEmitter *emitter;
    struct emitter_xtra *bonus;
    SYMID symple;
    int level = FIX2INT(rb_ivar_get(self, s_level)) + 1;
    rb_ivar_set(self, s_level, INT2FIX(level));

    rb_scan_args(argc, argv, "1&", &oid, &proc);
	Data_Get_Struct(self, SyckEmitter, emitter);
    bonus = (struct emitter_xtra *)emitter->bonus;

    /* Calculate anchors, normalize nodes, build a simpler symbol table */
    bonus->oid = oid;
    if ( !NIL_P( oid ) && RTEST( rb_funcall( bonus->data, s_haskey, 1, oid ) ) ) {
        symple = rb_hash_aref( bonus->data, oid );
    } else {
        symple = rb_funcall( proc, s_call, 1, rb_ivar_get( self, s_out ) );
    }
    syck_emitter_mark_node( emitter, (st_data_t)symple );

    /* Second pass, build emitted string */
    if ( level == 1 ) 
    {
        syck_emit(emitter, (st_data_t)symple);
        syck_emitter_flush(emitter, 0);

        return bonus->port;
    }
    
    level -= 1;
    rb_ivar_set(self, s_level, INT2FIX(level));
    return symple;
}

/*
 * YAML::Syck::Emitter#set_resolver
 */
VALUE
syck_emitter_set_resolver( self, resolver )
    VALUE self, resolver;
{
    rb_ivar_set( self, s_resolver, resolver );
    return self;
}

/*
 * YAML::Syck::Out::initialize
 */
VALUE
syck_out_initialize( self, emitter )
    VALUE self, emitter;
{
    rb_ivar_set( self, s_emitter, emitter );
    return self;
}

/*
 * YAML::Syck::Out::map
 */
VALUE
syck_out_map( self, type_id )
    VALUE self, type_id;
{
    VALUE map = rb_funcall( cOutMap, s_new, 2, rb_ivar_get( self, s_emitter ), type_id );
    syck_out_mark( rb_ivar_get( self, s_emitter ), map );
    rb_yield( map );
    return map;
}

/*
 * YAML::Syck::Out::seq
 */
VALUE
syck_out_seq( self, type_id )
    VALUE self, type_id;
{
    VALUE seq = rb_funcall( cOutSeq, s_new, 2, rb_ivar_get( self, s_emitter ), type_id );
    syck_out_mark( rb_ivar_get( self, s_emitter ), seq );
    rb_yield( seq );
    return seq;
}

/*
 * YAML::Syck::Out::scalar
 */
VALUE
syck_out_scalar( self, type_id, str, block )
    VALUE self, type_id, str, block;
{
    VALUE scalar = rb_funcall( cOutScalar, s_new, 3, type_id, str, block );
    syck_out_mark( rb_ivar_get( self, s_emitter ), scalar );
    return scalar;
}

/*
 * YAML::Syck::OutMap#initialize
 */
VALUE
syck_out_map_initialize( self, emitter, type_id )
    VALUE self, emitter, type_id;
{
    VALUE super_argv[1];
    rb_call_super( 0, super_argv );
    rb_ivar_set( self, s_type_id, type_id );
    rb_ivar_set( self, s_emitter, emitter );
    return self;
}

/*
 * YAML::Syck::OutMap#add
 */
VALUE
syck_out_map_add( self, k, v )
    VALUE self, k, v;
{
    VALUE emitter = rb_ivar_get( self, s_emitter );
    rb_funcall( self, s_push, 1, rb_funcall( k, s_to_yaml, 1, emitter ) );
    rb_funcall( self, s_push, 1, rb_funcall( v, s_to_yaml, 1, emitter ) );
    return self;
}

/*
 * YAML::Syck::OutSeq#initialize
 */
VALUE
syck_out_seq_initialize( self, emitter, type_id )
    VALUE self, emitter, type_id;
{
    VALUE super_argv[1];
    rb_call_super( 0, super_argv );
    rb_ivar_set( self, s_type_id, type_id );
    rb_ivar_set( self, s_emitter, emitter );
    return self;
}

/*
 * YAML::Syck::OutSeq#add
 */
VALUE
syck_out_seq_add( self, v )
    VALUE self, v;
{
    VALUE emitter = rb_ivar_get( self, s_emitter );
    v = rb_funcall( v, s_to_yaml, 1, emitter );
    rb_funcall( self, s_push, 1, v );
    return self;
}

/*
 * YAML::Syck::OutScalar#initialize
 */
VALUE
syck_out_scalar_initialize( self, type_id, str, block )
    VALUE self, type_id, str, block;
{
    VALUE super_argv[1];
    super_argv[0] = str;
    rb_call_super( 1, super_argv );
    rb_ivar_set( self, s_type_id, type_id );
    rb_ivar_set( self, s_block, block );
    return self;
}

/*
 * Initialize Syck extension
 */
void
Init_syck()
{
    VALUE rb_yaml = rb_define_module( "YAML" );
    VALUE rb_syck = rb_define_module_under( rb_yaml, "Syck" );
    rb_define_const( rb_syck, "VERSION", rb_str_new2( SYCK_VERSION ) );
    rb_define_module_function( rb_syck, "compile", rb_syck_compile, 1 );

	/*
	 * Global symbols
	 */
    s_new = rb_intern("new");
    s_utc = rb_intern("utc");
    s_at = rb_intern("at");
    s_to_f = rb_intern("to_f");
    s_to_i = rb_intern("to_i");
    s_read = rb_intern("read");
    s_binmode = rb_intern("binmode");
    s_transfer = rb_intern("transfer");
    s_call = rb_intern("call");
    s_cmp = rb_intern("<=>");
    s_intern = rb_intern("intern");
	s_update = rb_intern("update");
    s_detect_implicit = rb_intern("detect_implicit");
	s_dup = rb_intern("dup");
    s_default_set = rb_intern("default=");
	s_match = rb_intern("match");
	s_push = rb_intern("push");
    s_haskey = rb_intern("has_key?");
	s_keys = rb_intern("keys");
    s_node_import = rb_intern("node_import");
	s_to_str = rb_intern("to_str");
	s_tr_bang = rb_intern("tr!");
    s_unpack = rb_intern("unpack");
	s_write = rb_intern("write");
    s_tag_subclasses = rb_intern( "tag_subclasses?" );
    s_emitter = rb_intern( "emitter" );
    s_level = rb_intern( "level" );
    s_set_resolver = rb_intern( "set_resolver" );
    s_to_yaml = rb_intern( "to_yaml" );
    s_yaml_new = rb_intern("yaml_new");
    s_yaml_initialize = rb_intern("yaml_initialize");

    s_block = rb_intern("@block");
    s_tags = rb_intern("@tags");
    s_kind = rb_intern("@kind");
    s_name = rb_intern("@name");
    s_options = rb_intern("@options");
    s_type_id = rb_intern("@type_id");
    s_resolver = rb_intern("@resolver");
    s_value = rb_intern("@value");
    s_out = rb_intern("@out");
	s_input = rb_intern("@input");

	sym_model = ID2SYM(rb_intern("Model"));
	sym_generic = ID2SYM(rb_intern("Generic"));
	sym_bytecode = ID2SYM(rb_intern("bytecode"));
    sym_map = ID2SYM(rb_intern("map"));
    sym_scalar = ID2SYM(rb_intern("scalar"));
    sym_seq = ID2SYM(rb_intern("seq"));

    /*
     * Define YAML::Syck::Resolver class
     */
    cResolver = rb_define_class_under( rb_syck, "Resolver", rb_cObject );
    rb_define_attr( cResolver, "tags", 1, 1 );
    rb_define_method( cResolver, "initialize", syck_resolver_initialize, 0 );
    rb_define_method( cResolver, "add_type", syck_resolver_add_type, 2 );
    rb_define_method( cResolver, "use_types_at", syck_resolver_use_types_at, 1 );
    rb_define_method( cResolver, "detect_implicit", syck_resolver_detect_implicit, 1 );
    rb_define_method( cResolver, "transfer", syck_resolver_transfer, 2 );
    rb_define_method( cResolver, "node_import", syck_resolver_node_import, 1 );

    oDefaultResolver = rb_funcall( cResolver, rb_intern( "new" ), 0 );
    rb_define_singleton_method( oDefaultResolver, "node_import", syck_defaultresolver_node_import, 1 );
    rb_define_singleton_method( oDefaultResolver, "detect_implicit", syck_defaultresolver_detect_implicit, 1 );
    rb_define_const( rb_syck, "DefaultResolver", oDefaultResolver );
    oGenericResolver = rb_funcall( cResolver, rb_intern( "new" ), 0 );
    rb_define_singleton_method( oGenericResolver, "node_import", syck_genericresolver_node_import, 1 );
    rb_define_const( rb_syck, "GenericResolver", oGenericResolver );

    /*
     * Define YAML::Syck::Parser class
     */
    cParser = rb_define_class_under( rb_syck, "Parser", rb_cObject );
    rb_define_attr( cParser, "options", 1, 1 );
    rb_define_attr( cParser, "resolver", 1, 1 );
    rb_define_attr( cParser, "input", 1, 1 );
	rb_define_singleton_method( cParser, "new", syck_parser_new, -1 );
    rb_define_method(cParser, "initialize", syck_parser_initialize, 2);
    rb_define_method(cParser, "bufsize=", syck_parser_bufsize_set, 1 );
    rb_define_method(cParser, "bufsize", syck_parser_bufsize_get, 0 );
    rb_define_method(cParser, "load", syck_parser_load, -1);
    rb_define_method(cParser, "load_documents", syck_parser_load_documents, -1);
    rb_define_method(cParser, "set_resolver", syck_parser_set_resolver, 1);

    /*
     * Define YAML::Syck::CNode class
     */
    cCNode = rb_define_class_under( rb_syck, "CNode", rb_cObject );

    /*
     * Define YAML::Syck::Node class
     */
    cNode = rb_define_class_under( rb_syck, "Node", rb_cObject );
    rb_define_attr( cNode, "kind", 1, 1 );
    rb_define_attr( cNode, "type_id", 1, 1 );
    rb_define_attr( cNode, "value", 1, 1 );
    rb_define_attr( cNode, "anchor", 1, 1 );
    rb_define_attr( cNode, "resolver", 1, 1 );
    rb_define_method( cNode, "initialize", syck_node_initialize, 2 );
    rb_define_method( cNode, "transform", syck_node_transform, 0);

    /*
     * Define YAML::Syck::PrivateType class
     */
    cPrivateType = rb_define_class_under( rb_syck, "PrivateType", rb_cObject );
    rb_define_attr( cPrivateType, "type_id", 1, 1 );
    rb_define_attr( cPrivateType, "value", 1, 1 );
    rb_define_method( cPrivateType, "initialize", syck_privatetype_initialize, 2);

    /*
     * Define YAML::Syck::DomainType class
     */
    cDomainType = rb_define_class_under( rb_syck, "DomainType", rb_cObject );
    rb_define_attr( cDomainType, "domain", 1, 1 );
    rb_define_attr( cDomainType, "type_id", 1, 1 );
    rb_define_attr( cDomainType, "value", 1, 1 );
    rb_define_method( cDomainType, "initialize", syck_domaintype_initialize, 3);

    /*
     * Define YAML::Syck::BadAlias class
     */
    cBadAlias = rb_define_class_under( rb_syck, "BadAlias", rb_cObject );
    rb_define_attr( cBadAlias, "name", 1, 1 );
    rb_define_method( cBadAlias, "initialize", syck_badalias_initialize, 1);
    rb_define_method( cBadAlias, "<=>", syck_badalias_cmp, 1);
    rb_include_module( cBadAlias, rb_const_get( rb_cObject, rb_intern("Comparable") ) );

	/*
	 * Define YAML::Syck::MergeKey class
	 */
	cMergeKey = rb_define_class_under( rb_syck, "MergeKey", rb_cObject );

	/*
	 * Define YAML::Syck::DefaultKey class
	 */
	cDefaultKey = rb_define_class_under( rb_syck, "DefaultKey", rb_cObject );

    /*
     * Define YAML::Syck::Out classes
     */
    cOut = rb_define_class_under( rb_syck, "Out", rb_cObject );
    rb_define_attr( cOut, "emitter", 1, 1 );
    rb_define_method( cOut, "initialize", syck_out_initialize, 1 );
    rb_define_method( cOut, "map", syck_out_map, 1 );
    rb_define_method( cOut, "seq", syck_out_seq, 1 );
    rb_define_method( cOut, "scalar", syck_out_scalar, 3 );
    cOutMap = rb_define_class_under( rb_syck, "OutMap", rb_cArray );
    rb_define_attr( cOutMap, "emitter", 1, 1 );
    rb_define_method( cOutMap, "initialize", syck_out_map_initialize, 2 );
    rb_define_method( cOutMap, "add", syck_out_map_add, 2 );
    cOutSeq = rb_define_class_under( rb_syck, "OutSeq", rb_cArray );
    rb_define_attr( cOutSeq, "emitter", 1, 1 );
    rb_define_method( cOutSeq, "initialize", syck_out_seq_initialize, 2 );
    rb_define_method( cOutSeq, "add", syck_out_seq_add, 1 );
    cOutScalar = rb_define_class_under( rb_syck, "OutScalar", rb_cString );
    rb_define_method( cOutScalar, "initialize", syck_out_scalar_initialize, 3 );
    rb_define_attr( cOutScalar, "type_id", 1, 1 );
    rb_define_attr( cOutScalar, "block", 1, 1 );

    /*
     * Define YAML::Syck::Emitter class
     */
    cEmitter = rb_define_class_under( rb_syck, "Emitter", rb_cObject );
    rb_define_attr( cEmitter, "level", 1, 1 );
	rb_define_singleton_method( cEmitter, "new", syck_emitter_new, -1 );
    rb_define_method( cEmitter, "initialize", syck_emitter_reset, 1 );
    rb_define_method( cEmitter, "reset", syck_emitter_reset, 1 );
    rb_define_method( cEmitter, "emit", syck_emitter_emit, -1 );
    rb_define_method( cEmitter, "set_resolver", syck_emitter_set_resolver, 1);
}

