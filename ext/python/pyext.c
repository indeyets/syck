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

staticforward PyTypeObject SyckNodeType;

static double zero() { return 0.0; }
static double one() { return 1.0; }
static double inf() { return one() / zero(); }
static double i_nan() { return zero() / zero(); }

SYMID py_syck_load_handler( SyckParser *p, SyckNode *n );
static PyObject *py_syck_load( PyObject *self, PyObject *args );
SYMID py_syck_parse_handler( SyckParser *p, SyckNode *n );
static PyObject *py_syck_parse( PyObject *self, PyObject *args );

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

/*
 * node object
 */
static struct PyMethodDef SyckNodeMethods[] = {
    { "transform",  py_syck_node_transform, METH_VARARGS,
      "Transform a node into native Python objects." },
    { NULL, NULL, 0, NULL }        /* Sentinel */
};

static SyckNode *
py_syck_node_alloc( char *kind, char *type_id, char *value )
{
    SyckNode *self;
    self = PyObject_NEW( SyckNode, &SyckNodeType );
    if ( self == NULL ) return NULL;
    self->kind = kind;
    self->type_id = type_id;
    self->value = value;
    return self;
}

static PyObject *
py_syck_node_new( PyObject *self, PyObject *args )
{
    char *kind, *type_id, *value;
    if ( ! PyArg_ParseTuple( args, "sss", &kind, &type_id, &value ) )
    {
        return NULL;
    }
    return (PyObject *) py_syck_node_alloc( kind, type_id, value );
}

static void
py_syck_node_free( SyckNode *self )
{
    if ( self->type_id != NULL )
        S_FREE( self->type_id );
    if ( self->anchor != NULL )
        S_FREE( self->anchor );
    S_FREE( self );
    PyMem_DEL( self );
}

static PyTypeObject SyckNodeType = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "Node",
    sizeof(SyckNode),
    0,

    (destructor)  py_syck_node_free,
    (printfunc)   0,
    (getattrfunc) py_syck_node_getattr,
    (setattrfunc) 0,
    (cmpfunc)     0,
    (reprfunc)    py_syck_node_repr,

    0,
    0,   /* &py_syck_node_as_sequence, */
    0,
    (hashfunc)    0,
    (ternaryfunc) 0,
    (reprfunc)    0,
};

/*
 * default handler for ruby.yaml.org types
 */
int
yaml_org_handler( p, n, ref )
    SyckParser *p;
    SyckNode *n;
    PyObject **ref;
{
    SYMID oid;
    PyObject *o, *o2, *o3;
    char *type_id = n->type_id;
    int transferred = 0;
    long i = 0;

    o = Py_None;
    switch (n->kind)
    {
        case syck_str_kind:
            transferred = 1;
            if ( type_id == NULL || strcmp( type_id, "str" ) == 0 )
            {
                o = PyString_FromStringAndSize( n->data.str->ptr, n->data.str->len );
            }
            else if ( strcmp( type_id, "null" ) == 0 )
            {
                o = Py_None;
            }
            else if ( strcmp( type_id, "binary" ) == 0 )
            {
                PyObject *str64 = PyString_FromStringAndSize( n->data.str->ptr, n->data.str->len );
                PyObject *base64mod = PyImport_ImportModule( "base64" );
                PyObject *decodestring = PyObject_GetAttr( base64mod, PyString_FromString( "decodestring" ) );
                o = PyObject_CallFunction( decodestring, "S", str64 );
            }
            else if ( strcmp( type_id, "bool#yes" ) == 0 )
            {
                o = syck_PyIntMaker( 1 );
            }
            else if ( strcmp( type_id, "bool#no" ) == 0 )
            {
                o = syck_PyIntMaker( 0 );
            }
            else if ( strcmp( type_id, "int#hex" ) == 0 )
            {
                long i2 = strtol( n->data.str->ptr, NULL, 16 );
                o = syck_PyIntMaker( i2 );
            }
            else if ( strcmp( type_id, "int#oct" ) == 0 )
            {
                long i2 = strtol( n->data.str->ptr, NULL, 8 );
                o = syck_PyIntMaker( i2 );
            }
            else if ( strncmp( type_id, "int", 3 ) == 0 )
            {
                long i2;
                syck_str_blow_away_commas( n );
                i2 = strtol( n->data.str->ptr, NULL, 10 );
                o = syck_PyIntMaker( i2 );
            }
            else if ( strcmp( type_id, "float#nan" ) == 0 )
            {
                o = PyFloat_FromDouble( i_nan() );
            }
            else if ( strcmp( type_id, "float#inf" ) == 0 )
            {
                o = PyFloat_FromDouble( inf() );
            }
            else if ( strcmp( type_id, "float#neginf" ) == 0 )
            {
                o = PyFloat_FromDouble( -inf() );
            }
            else if ( strncmp( type_id, "float", 5 ) == 0 )
            {
                double f;
                syck_str_blow_away_commas( n );
                f = strtod( n->data.str->ptr, NULL );
                o = PyFloat_FromDouble( f );
            }
            else if ( strcmp( type_id, "timestamp#iso8601" ) == 0 )
            {
                o = PyString_FromStringAndSize( n->data.str->ptr, n->data.str->len );
            }
            else if ( strcmp( type_id, "timestamp#spaced" ) == 0 )
            {
                o = PyString_FromStringAndSize( n->data.str->ptr, n->data.str->len );
            }
            else if ( strcmp( type_id, "timestamp#ymd" ) == 0 )
            {
                o = PyString_FromStringAndSize( n->data.str->ptr, n->data.str->len );
            }
            else if ( strncmp( type_id, "timestamp", 9 ) == 0 )
            {
                o = PyString_FromStringAndSize( n->data.str->ptr, n->data.str->len );
            }
			else if ( strncmp( type_id, "merge", 5 ) == 0 )
			{
                o = PyString_FromStringAndSize( n->data.str->ptr, n->data.str->len );
			}
            else
            {
                transferred = 0;
                o = PyString_FromStringAndSize( n->data.str->ptr, n->data.str->len );
            }
        break;

        case syck_seq_kind:
            o = PyList_New( n->data.list->idx );
            for ( i = 0; i < n->data.list->idx; i++ )
            {
                oid = syck_seq_read( n, i );
                syck_lookup_sym( p, oid, (char **)&o2 );
                PyList_SetItem( o, i, o2 );
            }
            if ( type_id == NULL || strcmp( type_id, "seq" ) == 0 )
            {
                transferred = 1;
            }
        break;

        case syck_map_kind:
            o = PyDict_New();
            for ( i = 0; i < n->data.pairs->idx; i++ )
            {
                oid = syck_map_read( n, map_key, i );
                syck_lookup_sym( p, oid, (char **)&o2 );
                oid = syck_map_read( n, map_value, i );
                syck_lookup_sym( p, oid, (char **)&o3 );
                PyDict_SetItem( o, o2, o3 );
            }
            if ( type_id == NULL || strcmp( type_id, "map" ) == 0 )
            {
                transferred = 1;
            }
        break;
    }

    *ref = o;
    return transferred;
}

SYMID
py_syck_load_handler(p, n)
    SyckParser *p;
    SyckNode *n;
{
    SYMID oid;
    PyObject *o;

    /*
     * Attempt common transfers
     */
    int transferred = yaml_org_handler( p, n, &o );
    oid = syck_add_sym( p, (char *)o );
    return oid;
}

static PyObject *
py_syck_load( self, args )
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
    syck_parser_handler( parser, py_syck_load_handler );
    syck_parser_error_handler( parser, NULL );
    syck_parser_implicit_typing( parser, 1 );
    syck_parser_taguri_expansion( parser, 0 );

    v = syck_parse( parser );
    syck_lookup_sym( parser, v, (char **)&obj );

    syck_free_parser( parser );

    return obj;
}

SYMID
py_syck_parse_handler(p, n)
    SyckParser *p;
    SyckNode *n;
{
    SYMID oid;
    PyObject *o;

    /*
     * Attempt common transfers
     */
    int transferred = yaml_org_handler( p, n, &o );
    oid = syck_add_sym( p, (char *)o );
    return oid;
}

static PyObject *
py_syck_parse( self, args )
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
    syck_parser_handler( parser, py_syck_parse_handler );
    syck_parser_error_handler( parser, NULL );
    syck_parser_implicit_typing( parser, 1 );
    syck_parser_taguri_expansion( parser, 1 );

    v = syck_parse( parser );
    syck_lookup_sym( parser, v, (char **)&obj );

    syck_free_parser( parser );

    return obj;
}

static PyMethodDef SyckMethods[] = {

    { "load",  py_syck_load, METH_VARARGS,
      "Load from a YAML string." },
    { "parse", py_syck_parse, METH_VARARGS,
      "Parse a YAML string into objects representing nodes." },

    { "Node", py_syck_node_new, METH_VARARGS,
      "Create a syck.Node object." },

    { NULL, NULL, 0, NULL }        /* Sentinel */

};

void
initsyck(void)
{
    PyObject *syck, *syck_node;
    syck = Py_InitModule( "syck", SyckMethods );
}

