//
// phpext.c
//
// $Author$
// $Date$
//
// Copyright (C) 2003 why the lucky stiff
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <syck.h>

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_syck.h"

/* {{{ syck_functions[]
 *
 * Every user visible function must have an entry in syck_functions[].
 */
function_entry syck_functions[] = {
	PHP_FE(syck_load,				NULL)
	{NULL, NULL, NULL}	/* Must be the last line in syck_functions[] */
};
/* }}} */

/* {{{ syck_module_entry
 */
zend_module_entry syck_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"syck",
	syck_functions,
	NULL,            /* module init function */
	NULL,            /* module shutdown function */
	NULL,            /* request init function */
	NULL,            /* request shutdown function */
	PHP_MINFO(syck), /* module info function */
#if ZEND_MODULE_API_NO >= 20010901
	"0.1", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_SYCK
ZEND_GET_MODULE(syck)
#endif

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("syck.global_value",      "42", PHP_INI_ALL, OnUpdateInt, global_value, zend_syck_globals, syck_globals)
    STD_PHP_INI_ENTRY("syck.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_syck_globals, syck_globals)
PHP_INI_END()
*/
/* }}} */

/* {{{ php_syck_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_syck_init_globals(zend_syck_globals *syck_globals)
{
	syck_globals->global_value = 0;
	syck_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(syck)
{
	/* If you have INI entries, uncomment these lines 
	ZEND_INIT_MODULE_GLOBALS(syck, php_syck_init_globals, NULL);
	REGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(syck)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(syck)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(syck)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(syck)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "syck support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */


SYMID
php_syck_handler(p, n)
    SyckParser *p;
    SyckNode *n;
{
    SYMID oid;
    zval *o, *o2, *o3;
    unsigned int i;

	MAKE_STD_ZVAL(o);
    switch (n->kind)
    {
        case syck_str_kind:
            if ( n->type_id == NULL || strcmp( n->type_id, "str" ) == 0 )
            {
				ZVAL_STRINGL( o, n->data.str->ptr, n->data.str->len, 1);
            }
            else if ( strcmp( n->type_id, "null" ) == 0 )
            {
                ZVAL_NULL( o );
            }
            else if ( strcmp( n->type_id, "bool#yes" ) == 0 )
            {
				ZVAL_BOOL( o, 1 );
            }
            else if ( strcmp( n->type_id, "bool#no" ) == 0 )
            {
				ZVAL_BOOL( o, 0 );
            }
            else if ( strcmp( n->type_id, "int#hex" ) == 0 )
            {
				long intVal = strtol( n->data.str->ptr, NULL, 16 );
				ZVAL_LONG( o, intVal );
            }
            else if ( strcmp( n->type_id, "int#oct" ) == 0 )
            {
				long intVal = strtol( n->data.str->ptr, NULL, 8 );
				ZVAL_LONG( o, intVal );
            }
            else if ( strcmp( n->type_id, "int" ) == 0 )
            {
				long intVal = strtol( n->data.str->ptr, NULL, 10 );
				ZVAL_LONG( o, intVal );
            }
            else if ( strcmp( n->type_id, "float" ) == 0 )
            {
				//double floatVal = strtol( n->data.str->ptr );
				ZVAL_DOUBLE( o, 3.45 );
            }
            else
            {
				ZVAL_STRINGL(o, n->data.str->ptr, n->data.str->len, 1);
            }
        break;

        case syck_seq_kind:
			array_init(o);
            for ( i = 0; i < n->data.list->idx; i++ )
            {
                oid = syck_seq_read( n, i );
                syck_lookup_sym( p, oid, &o2 );
				add_index_zval( o, i, o2 );
            }
        break;

        case syck_map_kind:
			array_init(o);
            for ( i = 0; i < n->data.pairs->idx; i++ )
            {
                oid = syck_map_read( n, map_key, i );
                syck_lookup_sym( p, oid, &o2 );
                oid = syck_map_read( n, map_value, i );
                syck_lookup_sym( p, oid, &o3 );
				if ( o2->type == IS_STRING )
				{
					add_assoc_zval( o, o2->value.str.val, o3 );
				}
            }
        break;
    }
    oid = syck_add_sym( p, o );
    return oid;
}

/* {{{ proto object syck_load(string arg)
   Return PHP object from a YAML string */
PHP_FUNCTION(syck_load)
{
	char *arg = NULL;
	int arg_len;
	SYMID v;
	zval *obj;
	SyckParser *parser;

	if (ZEND_NUM_ARGS() != 1) WRONG_PARAM_COUNT;
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	parser = syck_new_parser();
    syck_parser_str( parser, arg, arg_len, NULL );
    syck_parser_handler( parser, php_syck_handler );
    syck_parser_implicit_typing( parser, 1 );
    syck_parser_taguri_expansion( parser, 0 );
    v = syck_parse( parser );
    syck_lookup_sym( parser, v, &obj );
    syck_free_parser( parser );

	*return_value = *obj;
	zval_copy_ctor(return_value);
}
/* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
