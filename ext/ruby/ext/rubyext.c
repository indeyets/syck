//
// rubyext.c
//
// $Author$
// $Date$
//
// Copyright (C) 2003 why the lucky stiff
//

#include "ruby.h"
#include "syck.h"
#include <sys/types.h>
#include <time.h>

static ID time_s_mkutc, s_read, s_binmode;
static VALUE sym_model, sym_generic;
VALUE cNode;

static double zero()    { return 0.0; }
static double one() { return 1.0; }
static double inf() { return one() / zero(); }
static double nan() { return zero() / zero(); }

//
// Read from IO classes
// 
long
rb_syck_io_str_read( char *buf, SyckIoStr *str, long max_size, long skip )
{
    long len = 0;

    ASSERT( str != NULL );
    max_size -= skip;
    if ( max_size < 0 ) max_size = 0;

    if ( max_size > 0 )
    {
        //
        // IO read
        //
        VALUE src = (VALUE)str->ptr;
        VALUE n = LONG2NUM(max_size);
        VALUE str = rb_funcall2(src, s_read, 1, &n);
        if (!NIL_P(str))
        {
            len = RSTRING(str)->len;
            memcpy( buf + skip, RSTRING(str)->ptr, len );
        }
    }
    len += skip;
    buf[len] = '\0';
    return len;
}

//
// Determine if IO is String class or IO subclass
//
void
syck_parser_assign_io(parser, port)
	SyckParser *parser;
	VALUE port;
{
    if (rb_respond_to(port, rb_intern("to_str"))) {
	    //arg.taint = OBJ_TAINTED(port); /* original taintedness */
	    //StringValue(port);	       /* possible conversion */
	    syck_parser_str( parser, RSTRING(port)->ptr, RSTRING(port)->len, NULL );
    }
    else if (rb_respond_to(port, s_read)) {
        if (rb_respond_to(port, s_binmode)) {
            rb_funcall2(port, s_binmode, 0, 0);
        }
        //arg.taint = Qfalse;
	    syck_parser_str( parser, (char *)port, 0, rb_syck_io_str_read );
    }
    else {
        rb_raise(rb_eTypeError, "instance of IO needed");
    }
}

//
// Function for creating timestamps
//
SYMID
rb_syck_mktime(str)
    char *str;
{
    VALUE time;
    char *ptr = str;
    VALUE year, mon, day, hour, min, sec;

    // Year
    ptr[4] = '\0';
    year = INT2FIX(strtol(ptr, NULL, 10));

    // Month
    ptr += 4;
    while ( !isdigit( *ptr ) ) ptr++;
    mon = INT2FIX(strtol(ptr, NULL, 10));

    // Day
    ptr += 2;
    while ( !isdigit( *ptr ) ) ptr++;
    day = INT2FIX(strtol(ptr, NULL, 10));

    // Hour
    ptr += 2;
    while ( !isdigit( *ptr ) ) ptr++;
    hour = INT2FIX(strtol(ptr, NULL, 10));

    // Minute 
    ptr += 2;
    while ( !isdigit( *ptr ) ) ptr++;
    min = INT2FIX(strtol(ptr, NULL, 10));

    // Second 
    ptr += 2;
    while ( !isdigit( *ptr ) ) ptr++;
    sec = INT2FIX(strtol(ptr, NULL, 10));

    time = rb_funcall(rb_cTime, time_s_mkutc, 6, year, mon, day, hour, min, sec );
    return time;
}

//
// Node handler
// - Loads data into Node classes
//
SYMID
rb_syck_parse_handler(p, n)
    SyckParser *p;
    SyckNode *n;
{
    VALUE t, v, obj;
    int i;

    obj = rb_obj_alloc(cNode);
    if ( n->type_id != NULL )
    {
        t = rb_str_new2(n->type_id);
        rb_iv_set(obj, "@type_id", t);
    }

    switch (n->kind)
    {
        case syck_str_kind:
            rb_iv_set(obj, "@kind", ID2SYM(rb_intern("str")));
            v = rb_str_new( n->data.str->ptr, n->data.str->len );
        break;

        case syck_seq_kind:
            rb_iv_set(obj, "@kind", ID2SYM(rb_intern("seq")));
            v = rb_ary_new2( n->data.list->idx );
            for ( i = 0; i < n->data.list->idx; i++ )
            {
                rb_ary_store( v, i, syck_seq_read( n, i ) );
            }
        break;

        case syck_map_kind:
            rb_iv_set(obj, "@kind", ID2SYM(rb_intern("map")));
            v = rb_hash_new();
            for ( i = 0; i < n->data.pairs->idx; i++ )
            {
                rb_hash_aset( v, syck_map_read( n, map_key, i ), syck_map_read( n, map_value, i ) );
            }
        break;
    }

	if ( p->bonus != 0 )
	{
		VALUE proc = (VALUE)p->bonus;
		rb_funcall(proc, rb_intern("call"), 1, v);
	}
    rb_iv_set(obj, "@value", v);
    return obj;
}

//
// Node handler
// - Converts data into native Ruby types
//
SYMID
rb_syck_load_handler(p, n)
    SyckParser *p;
    SyckNode *n;
{
    VALUE obj;
    long i;
    int str = 0;

    switch (n->kind)
    {
        case syck_str_kind:
            if ( n->type_id == NULL || strcmp( n->type_id, "str" ) == 0 )
            {
                obj = rb_str_new( n->data.str->ptr, n->data.str->len );
            }
            else if ( strcmp( n->type_id, "null" ) == 0 )
            {
                obj = Qnil;
            }
            else if ( strcmp( n->type_id, "bool#yes" ) == 0 )
            {
                obj = Qtrue;
            }
            else if ( strcmp( n->type_id, "bool#no" ) == 0 )
            {
                obj = Qfalse;
            }
            else if ( strcmp( n->type_id, "int#hex" ) == 0 )
            {
                obj = rb_cstr2inum( n->data.str->ptr, 16 );
            }
            else if ( strcmp( n->type_id, "int#oct" ) == 0 )
            {
                obj = rb_cstr2inum( n->data.str->ptr, 8 );
            }
            else if ( strcmp( n->type_id, "int" ) == 0 )
            {
                syck_str_blow_away_commas( n );
                obj = rb_cstr2inum( n->data.str->ptr, 10 );
            }
            else if ( strcmp( n->type_id, "float#exp" ) == 0 || strcmp( n->type_id, "float#fix" ) == 0 )
            {
                double f;
                syck_str_blow_away_commas( n );
                f = strtod( n->data.str->ptr, NULL );
                obj = rb_float_new( f );
            }
            else if ( strcmp( n->type_id, "float#nan" ) == 0 )
            {
                obj = rb_float_new( nan() );
            }
            else if ( strcmp( n->type_id, "float#inf" ) == 0 )
            {
                obj = rb_float_new( inf() );
            }
            else if ( strcmp( n->type_id, "float#neginf" ) == 0 )
            {
                obj = rb_float_new( -inf() );
            }
            else if ( strcmp( n->type_id, "timestamp" ) == 0 )
            {
                obj = rb_syck_mktime( n->data.str->ptr );
            }
            else if ( strcmp( n->type_id, "timestamp#iso8601" ) == 0 )
            {
                obj = rb_syck_mktime( n->data.str->ptr );
            }
            else if ( strcmp( n->type_id, "timestamp#spaced" ) == 0 )
            {
                obj = rb_syck_mktime( n->data.str->ptr );
            }
            else if ( strcmp( n->type_id, "timestamp#ymd" ) == 0 )
            {
                S_REALLOC_N( n->data.str->ptr, char, 22 );
                strcat( n->data.str->ptr, "t12:00:00Z" );
                obj = rb_syck_mktime( n->data.str->ptr );
            }
            else
            {
                obj = rb_str_new( n->data.str->ptr, n->data.str->len );
            }
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
                rb_hash_aset( obj, syck_map_read( n, map_key, i ), syck_map_read( n, map_value, i ) );
            }
        break;
    }
	if ( p->bonus != 0 )
	{
		VALUE proc = (VALUE)p->bonus;
		rb_funcall(proc, rb_intern("call"), 1, obj);
	}
    return obj;
}

//
// Display friendly errors
//
void
rb_syck_err_handler(p, msg)
    SyckParser *p;
    char *msg;
{
    p->cursor[1] = '\0';
    rb_raise(rb_eLoadError, "%s on line %d, col %d: `%s'",
           msg,
           p->linect,
           p->cursor - p->lineptr, 
           p->lineptr); 
}

//
// Load data based on the model requested
//
void
syck_set_model( parser, model )
	SyckParser *parser;
	VALUE model;
{
	if ( model == sym_generic )
	{
		syck_parser_handler( parser, rb_syck_parse_handler );
		syck_parser_error_handler( parser, rb_syck_err_handler );
		syck_parser_implicit_typing( parser, 1 );
		syck_parser_taguri_expansion( parser, 1 );
	}
	else
	{
		syck_parser_handler( parser, rb_syck_load_handler );
		syck_parser_error_handler( parser, rb_syck_err_handler );
		syck_parser_implicit_typing( parser, 1 );
		syck_parser_taguri_expansion( parser, 0 );
	}
}

static VALUE
rb_run_syck_parse(parser)
    SyckParser *parser;
{
    return syck_parse(parser);
}

static VALUE
rb_syck_ensure(parser)
    SyckParser *parser;
{
    syck_free_parser( parser );
    return 0;
}

//
// YAML::Syck::Parser.new
//
VALUE 
syck_parser_new( class, options )
	VALUE class, options;
{
	VALUE argv[1];
    SyckParser *parser = syck_new_parser();
	VALUE pobj = Data_Wrap_Struct( class, 0, syck_free_parser, parser );

	argv[0] = options;
	rb_obj_call_init(pobj, 1, argv);
	return pobj;
}

//
// YAML::Syck::Parser.initialize( options )
//
static VALUE
syck_parser_initialize( self, options )
    VALUE self, options;
{
	rb_iv_set(self, "@options", options);
	return self;
}

//
// YAML::Syck::Parser.load( IO or String )
//
VALUE
syck_parser_load(argc, argv, self)
    int argc;
    VALUE *argv;
	VALUE self;
{
    VALUE port, proc, v, model;
	SyckParser *parser;

    rb_scan_args(argc, argv, "11", &port, &proc);
	Data_Get_Struct(self, SyckParser, parser);
	syck_parser_assign_io(parser, port);

	model = rb_hash_aref( rb_iv_get( self, "@options" ), sym_model );
	syck_set_model( parser, model );

	parser->bonus = 0;
	if ( !NIL_P( proc ) ) parser->bonus = (void *)proc;

    v = syck_parse( parser );
    if ( v == NULL )
        return Qnil;

    //v = rb_ensure(rb_run_syck_parse, (VALUE)&parser, rb_syck_ensure, (VALUE)&parser);

    return v;
}

//
// YAML::Syck::Parser.load_documents( IO or String ) { |doc| }
//
VALUE
syck_parser_load_documents(argc, argv, self)
    int argc;
    VALUE *argv;
	VALUE self;
{
    VALUE port, proc, v, model;
	SyckParser *parser;

    rb_scan_args(argc, argv, "1&", &port, &proc);
	Data_Get_Struct(self, SyckParser, parser);
	syck_parser_assign_io(parser, port);

	model = rb_hash_aref( rb_iv_get( self, "@options" ), sym_model );
	syck_set_model( parser, model );

    while ( v != NULL )
	{
    	v = syck_parse( parser );
		rb_funcall( proc, rb_intern("call"), 1, v );
	}

    //v = rb_ensure(rb_run_syck_parse, (VALUE)&parser, rb_syck_ensure, (VALUE)&parser);

    return v;
}

//
// YAML::Syck::Node.initialize
//
static VALUE
syck_node_initialize( self, type_id, val )
    VALUE self, type_id, val;
{
    rb_iv_set( self, "@type_id", type_id );
    rb_iv_set( self, "@value", val );
}

//
// Initialize Syck extension
//
void
Init_syck()
{
    VALUE rb_yaml = rb_define_module( "YAML" );
    VALUE rb_syck = rb_define_module_under( rb_yaml, "Syck" );
	VALUE cParser = rb_define_class_under( rb_syck, "Parser", rb_cObject );

	//
	// Global symbols
	//
    time_s_mkutc = rb_intern("utc");
    s_read = rb_intern("read");
    s_binmode = rb_intern("binmode");
	sym_model = ID2SYM(rb_intern("Model"));
	sym_generic = ID2SYM(rb_intern("Generic"));

	//
	// Define YAML::Syck::Parser class
	//
    rb_define_attr( cParser, "options", 1, 1 );
	rb_define_singleton_method(cParser, "new", syck_parser_new, 1);
    rb_define_method(cParser, "initialize", syck_parser_initialize, 1);
    rb_define_method(cParser, "load", syck_parser_load, -1);
    rb_define_method(cParser, "load_documents", syck_parser_load_documents, -1);

    //
    // Define YAML::Syck::Node class
    //
    cNode = rb_define_class_under( rb_syck, "Node", rb_cObject );
    rb_define_attr( cNode, "kind", 1, 1 );
    rb_define_attr( cNode, "type_id", 1, 1 );
    rb_define_attr( cNode, "value", 1, 1 );
    rb_define_attr( cNode, "anchor", 1, 1 );
    rb_define_method( cNode, "initialize", syck_node_initialize, 2);
}

