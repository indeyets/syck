//
// pyext.c
//
// $Author$
// $Date$
//
// Copyright (C) 2003 why the lucky stiff
//
#include <Python.h>
#include <syck.h>

static double zero()    { return 0.0; }
static double one() { return 1.0; }
static double inf() { return one() / zero(); }
static double i_nan() { return zero() / zero(); }

PyObject *
syck_PyIntMaker( long num )
{
    if ( num > PyInt_GetMax() )
    {
        return PyLong_FromLong( num );
    }
    else
    {
        return PyInt_FromLong( num );
    }
}

SYMID
python_syck_handler(p, n)
    SyckParser *p;
    SyckNode *n;
{
    SYMID oid;
    PyObject *o, *o2, *o3;
    long i;

    switch (n->kind)
    {
        case syck_str_kind:
            if ( n->type_id == NULL || strcmp( n->type_id, "str" ) == 0 )
            {
                o = PyString_FromStringAndSize( n->data.str->ptr, n->data.str->len );
            }
            else if ( strcmp( n->type_id, "null" ) == 0 )
            {
                o = Py_None;
            }
            else if ( strcmp( n->type_id, "int#hex" ) == 0 )
            {
                long i2 = strtol( n->data.str->ptr, NULL, 16 );
                o = syck_PyIntMaker( i2 );
            }
            else if ( strcmp( n->type_id, "int#oct" ) == 0 )
            {
                long i2 = strtol( n->data.str->ptr, NULL, 8 );
                o = syck_PyIntMaker( i2 );
            }
            else if ( strcmp( n->type_id, "int" ) == 0 )
            {
                long i2;
                syck_str_blow_away_commas( n );
                i2 = strtol( n->data.str->ptr, NULL, 10 );
                o = syck_PyIntMaker( i2 );
            }
            else if ( strcmp( n->type_id, "float" ) == 0 || strcmp( n->type_id, "float#exp" ) == 0 || strcmp( n->type_id, "float#fix" ) == 0 )
            {
                double f;
                syck_str_blow_away_commas( n );
                f = strtod( n->data.str->ptr, NULL );
                o = PyFloat_FromDouble( f );
            }
            else if ( strcmp( n->type_id, "float#nan" ) == 0 )
            {
                o = PyFloat_FromDouble( i_nan() );
            }
            else if ( strcmp( n->type_id, "float#inf" ) == 0 )
            {
                o = PyFloat_FromDouble( inf() );
            }
            else if ( strcmp( n->type_id, "float#neginf" ) == 0 )
            {
                o = PyFloat_FromDouble( -inf() );
            }
            else
                o = PyString_FromStringAndSize( n->data.str->ptr, n->data.str->len );
        break;

        case syck_seq_kind:
            o = PyList_New( n->data.list->idx );
            for ( i = 0; i < n->data.list->idx; i++ )
            {
                oid = syck_seq_read( n, i );
                syck_lookup_sym( p, oid, &o2 );
                PyList_SetItem( o, i, o2 );
            }
        break;

        case syck_map_kind:
            o = PyDict_New();
            for ( i = 0; i < n->data.pairs->idx; i++ )
            {
                oid = syck_map_read( n, map_key, i );
                syck_lookup_sym( p, oid, &o2 );
                oid = syck_map_read( n, map_value, i );
                syck_lookup_sym( p, oid, &o3 );
                PyDict_SetItem( o, o2, o3 );
            }
        break;
    }
    oid = syck_add_sym( p, o );
    return oid;
}

static PyObject *
syck_load( self, args )
    PyObject *self;
    PyObject *args;
{
    PyObject *obj;
    SYMID v;
    char *yamlstr;
    SyckParser *parser = syck_new_parser();

    if (!PyArg_ParseTuple(args, "s", &yamlstr))
        return NULL;
    syck_parser_str_auto( parser, yamlstr, NULL );
    syck_parser_handler( parser, python_syck_handler );
    syck_parser_error_handler( parser, NULL );
    syck_parser_implicit_typing( parser, 1 );
    syck_parser_taguri_expansion( parser, 0 );
    v = syck_parse( parser );
    syck_lookup_sym( parser, v, &obj );
    syck_free_parser( parser );

    return obj;
}

static PyMethodDef SyckMethods[] = {
    {"load",  syck_load, METH_VARARGS,
     "Load from a YAML string."},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

void
initsyck(void)
{
        (void) Py_InitModule("syck", SyckMethods);
}
