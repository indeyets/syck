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
