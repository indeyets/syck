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

SYMID
syck_parse_handler(n)
    SyckNode *n;
{
    VALUE obj;
    long i;

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
                rb_hash_aset( obj, syck_map_read( n, map_key, i ), syck_map_read( n, map_value, i ) );
            }
        break;
    }
    return obj;
}

VALUE
rb_syck_load(argc, argv)
    int argc;
    VALUE *argv;
{
    VALUE port, proc;
    VALUE v;
    //OpenFile *fptr;
    SyckParser *parser = syck_new_parser();

    rb_scan_args(argc, argv, "11", &port, &proc);
    if (rb_respond_to(port, rb_intern("to_str"))) {
	    //arg.taint = OBJ_TAINTED(port); /* original taintedness */
	    //StringValue(port);	       /* possible conversion */
	    syck_parser_str( parser, RSTRING(port)->ptr, RSTRING(port)->len, NULL );
    }

    syck_parser_handler( parser, syck_parse_handler );
    return syck_parse( parser );
}

void
Init_syck()
{
    VALUE rb_syck = rb_define_module( "Syck" );

    rb_define_module_function(rb_syck, "load", rb_syck_load, -1);
}

