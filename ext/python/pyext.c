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

static PyObject *
py_syck_node_transform( SyckNode *self, PyObject *args )
{
    return Py_None;
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
py_syck_node_alloc( enum syck_kind_tag kind, char *type_id, PyObject *value )
{
    SyckNode *self;
    self = PyObject_NEW( SyckNode, &SyckNodeType );
    if ( self == NULL ) return NULL;
    self->kind = kind;
    self->type_id = syck_strndup( type_id, strlen( type_id ) );
    self->id = 0;
    Py_XINCREF( value );
    self->shortcut = (void *)value;
    return self;
}

static PyObject *
py_syck_node_new( enum syck_kind_tag type, char *type_id, PyObject *value )
{
    return (PyObject *) py_syck_node_alloc( type, type_id, value );
}

static PyObject *
py_syck_node_new2( PyObject *self, PyObject *args )
{
    enum syck_kind_tag type;
    char *kind, *type_id;
    PyObject *obj, *value;
    if ( ! PyArg_ParseTuple( args, "ssO", &kind, &type_id, &value ) )
    {
        return NULL;
    }
    obj = py_syck_node_new( syck_str_kind, type_id, value );
    //PyObject_SetAttrString( obj, "kind", PyString_FromString( kind ) );
    return obj;
}

static void
py_syck_node_free( SyckNode *self )
{
    PyObject *value = PyObject_GetAttrString( (PyObject *)self, "value" );
    Py_XDECREF( value );
    if ( self->type_id != NULL )
        S_FREE( self->type_id );
    if ( self->anchor != NULL )
        S_FREE( self->anchor );
    S_FREE( self );
    PyMem_DEL( self );
}

static PyObject *
py_syck_node_getattr( SyckNode *self, char *name )
{
    if ( strcmp( name, "kind" ) == 0 )
    {
        char *kind_str;
        if ( self->kind == syck_map_kind )
        {
            kind_str = "map";
        }
        else if ( self->kind == syck_seq_kind )
        {
            kind_str = "seq";
        }
        else
        {
            kind_str = "str";
        }
        return PyString_FromString( kind_str ); 
    }
    else if ( strcmp( name, "type_id" ) == 0 )
    {
        return PyString_FromString( self->type_id );
    }
    else if ( strcmp( name, "value" ) == 0 )
    {
        return (PyObject *)self->shortcut;
    }
    return Py_FindMethod( SyckNodeMethods, (PyObject *)self, name );
}

static int
py_syck_node_setattr( SyckNode *self, char *name, PyObject *value )
{
    if ( strcmp( name, "kind" ) == 0 )
    {
        enum syck_kind_tag type;
        char *kind = PyString_AsString( value );
        if ( strcmp( kind, "map" ) == 0 )
        {
            type = syck_map_kind;
        }
        else if ( strcmp( kind, "seq" ) == 0 )
        {
            type = syck_seq_kind;
        }
        else
        {
            type = syck_str_kind;
        }
        self->kind = type; 
        return 1;
    }
    else if ( strcmp( name, "type_id" ) == 0 )
    {
        self->type_id = PyString_AsString( value );
        return 1;
    }
    else if ( strcmp( name, "value" ) == 0 )
    {
        self->shortcut = (void *)value;
        return 1;
    }
    return 0;
}

static PyObject *
py_syck_node_repr( SyckNode *self )
{
    /*
    PyObject *value = PyObject_GetAttrString( (PyObject *)self, "value" );
    char *value_repr = PyString_AsString( PyObject_Repr( value ) );
    char *kind_str = PyString_AsString( py_syck_node_getattr( self, "kind" ) );
    char *type_id_str = PyString_AsString( py_syck_node_getattr( self, "type_id" ) );
    char *repr = S_ALLOC_N( char, strlen( value_repr ) + strlen( kind_str ) +
            strlen( type_id_str ) + 65 );
    sprintf( repr, "<syck.Node, kind = %s, type_id = %s, value = %s at %x>",
            kind_str, type_id_str, value_repr, self );
    return PyString_FromString( repr );
    */
    return PyString_FromString( "<syck.Node>" );
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
    (setattrfunc) py_syck_node_setattr,
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
                o = PyString_FromStringAndSize( n->data.str->ptr,
                                                n->data.str->len );
            }
            else if ( strcmp( type_id, "null" ) == 0 )
            {
                o = Py_None;
            }
            else if ( strcmp( type_id, "binary" ) == 0 )
            {
                PyObject *str64 = PyString_FromStringAndSize( 
                                     n->data.str->ptr, n->data.str->len );
                PyObject *base64mod = PyImport_ImportModule( "base64" );
                PyObject *decodestring = PyObject_GetAttr( base64mod, 
                                      PyString_FromString( "decodestring" ) );
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
                o = PyString_FromStringAndSize( n->data.str->ptr, 
                                                n->data.str->len );
            }
            else if ( strcmp( type_id, "timestamp#spaced" ) == 0 )
            {
                o = PyString_FromStringAndSize( n->data.str->ptr, 
                                                n->data.str->len );
            }
            else if ( strcmp( type_id, "timestamp#ymd" ) == 0 )
            {
                o = PyString_FromStringAndSize( n->data.str->ptr,
                                                n->data.str->len );
            }
            else if ( strncmp( type_id, "timestamp", 9 ) == 0 )
            {
                o = PyString_FromStringAndSize( n->data.str->ptr, 
                                                n->data.str->len );
            }
            else if ( strncmp( type_id, "merge", 5 ) == 0 )
            {
                o = PyString_FromStringAndSize( n->data.str->ptr, 
                                                n->data.str->len );
            }
            else
            {
                transferred = 0;
                o = PyString_FromStringAndSize( n->data.str->ptr, 
                                                n->data.str->len );
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

void
py_syck_error_handler( SyckParser *p, char *msg )
{
   PyErr_Format(PyExc_TypeError, "Error at [Line %d, Col %d]: %s\n",
        p->linect,
        p->cursor - p->lineptr,
        msg );
}

static PyObject *
py_syck_load( self, args )
    PyObject *self;
    PyObject *args;
{
    PyObject *obj = NULL;
    SYMID v;
    char *yamlstr;
    SyckParser *parser = syck_new_parser();

    if (!PyArg_ParseTuple(args, "s", &yamlstr))
        return NULL;

    syck_parser_str_auto( parser, yamlstr, NULL );
    syck_parser_handler( parser, py_syck_load_handler );
    syck_parser_error_handler( parser, py_syck_error_handler);
    syck_parser_implicit_typing( parser, 1 );
    syck_parser_taguri_expansion( parser, 0 );

    v = syck_parse( parser );
    if(v)
         syck_lookup_sym( parser, v, (char **)&obj );
    else
        if(!PyErr_Occurred()) {
            Py_INCREF(Py_None);
            obj = Py_None;
        }
    syck_free_parser( parser );
    return obj;
}

SYMID
py_syck_parse_handler(p, n)
    SyckParser *p;
    SyckNode *n;
{
    SYMID oid;
    PyObject *o, *o2, *o3;
    PyObject *v = Py_None;
    long i = 0;
    
    switch ( n->kind )
    {
        case syck_str_kind:
            v = PyString_FromStringAndSize( n->data.str->ptr, 
                                            n->data.str->len );
        break;

        case syck_seq_kind:
            v = PyList_New( n->data.list->idx );
            for ( i = 0; i < n->data.list->idx; i++ )
            {
                oid = syck_seq_read( n, i );
                syck_lookup_sym( p, oid, (char **)&o2 );
                PyList_SetItem( v, i, o2 );
            }
        break;

        case syck_map_kind:
            v = PyDict_New();
            for ( i = 0; i < n->data.pairs->idx; i++ )
            {
                oid = syck_map_read( n, map_key, i );
                syck_lookup_sym( p, oid, (char **)&o2 );
                oid = syck_map_read( n, map_value, i );
                syck_lookup_sym( p, oid, (char **)&o3 );
                PyDict_SetItem( v, o2, o3 );
            }
        break;
    }

    o = py_syck_node_new( n->kind, n->type_id, v );
    oid = syck_add_sym( p, (char *)o );
    return oid;
}

static PyObject *
py_syck_parse( self, args )
    PyObject *self;
    PyObject *args;
{
    PyObject *obj = NULL;
    SYMID v;
    char *yamlstr;
    SyckParser *parser = syck_new_parser();

    if (!PyArg_ParseTuple(args, "s", &yamlstr))
        return NULL;

    syck_parser_str_auto( parser, yamlstr, NULL );
    syck_parser_handler( parser, py_syck_parse_handler );
    syck_parser_error_handler( parser, py_syck_error_handler);
    syck_parser_implicit_typing( parser, 1 );
    syck_parser_taguri_expansion( parser, 1 );

    v = syck_parse( parser );
    if(v)
         syck_lookup_sym( parser, v, (char **)&obj );
    else
        if(!PyErr_Occurred()) {
            Py_INCREF(Py_None);
            obj = Py_None;
        }
    syck_free_parser( parser );
    return obj;
}

static PyMethodDef SyckMethods[] = {

    { "load",  py_syck_load, METH_VARARGS,
      "Load from a YAML string." },
    { "parse", py_syck_parse, METH_VARARGS,
      "Parse a YAML string into objects representing nodes." },
    { "Node", py_syck_node_new2, METH_VARARGS,
      "Create a syck.Node object." },
    { NULL, NULL, 0, NULL }        /* Sentinel */
};

void
initsyck(void)
{
    PyObject *syck;
    syck = Py_InitModule( "syck", SyckMethods );
}

