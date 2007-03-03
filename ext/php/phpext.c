//
// phpext.c
//
// $Author$
// $Date$
//
// Copyright (C) 2003 why the lucky stiff
// Copyright © 2007 Alexey Zakhlestin <indeyets@gmail.com>
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <syck.h>

#include "php.h"
#include "zend_exceptions.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_syck.h"

#define PHP_SYCK_EXCEPTION_PARENT "UnexpectedValueException"
#define PHP_SYCK_EXCEPTION_PARENT_LC "unexpectedvalueexception"
#define PHP_SYCK_EXCEPTION_NAME "SyckException"

static zend_class_entry *spl_ce_RuntimeException;

PHP_SYCK_API zend_class_entry *php_syck_get_exception_base(TSRMLS_DC)
{
#if defined(HAVE_SPL) && ((PHP_MAJOR_VERSION > 5) || (PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION >= 1))
	if (!spl_ce_RuntimeException) {
		zend_class_entry **pce;

		if (zend_hash_find(CG(class_table), PHP_SYCK_EXCEPTION_PARENT_LC, sizeof(PHP_SYCK_EXCEPTION_PARENT_LC), (void **) &pce) == SUCCESS) {
			spl_ce_RuntimeException = *pce;
			return *pce;
		}
	} else {
		return spl_ce_RuntimeException;
	}
#endif
#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 2)
	return zend_exception_get_default();
#else
	return zend_exception_get_default(TSRMLS_C);
#endif
}


static double zero()	{ return 0.0; }
static double one() { return 1.0; }
static double inf() { return one() / zero(); }
static double nanphp() { return zero() / zero(); }

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
	PHP_MINIT(syck),	/* module init function */
	NULL,			/* module shutdown function */
	NULL,			/* request init function */
	NULL,			/* request shutdown function */
	PHP_MINFO(syck),	/* module info function */
#if ZEND_MODULE_API_NO >= 20010901
	"0.2",			/* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_SYCK
ZEND_GET_MODULE(syck)
#endif

/**
 * "Merge" class
**/
static int le_mergekeyp;
zend_class_entry *merge_key_entry;

static zend_function_entry mergekey_functions[] = {
	PHP_FALIAS(mergekey, mergekey_init, NULL)
	{ NULL, NULL, NULL }
};

PHP_FUNCTION(mergekey_init)
{
	object_init_ex(getThis(), merge_key_entry);
}

static void destroy_MergeKey_resource(zend_rsrc_list_entry *resource TSRMLS_DC)
{
}


/**
 * SyckException class
**/

zend_class_entry *syck_exception_entry;
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(syck)
{
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, PHP_SYCK_EXCEPTION_NAME, NULL);
	syck_exception_entry = zend_register_internal_class_ex(&ce, php_syck_get_exception_base(TSRMLS_CC), PHP_SYCK_EXCEPTION_PARENT TSRMLS_CC);

	le_mergekeyp = zend_register_list_destructors_ex(destroy_MergeKey_resource, NULL, "MergeKey", module_number);
	INIT_CLASS_ENTRY(ce, "mergekey", mergekey_functions);
	merge_key_entry = zend_register_internal_class(&ce TSRMLS_CC);

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



SYMID php_syck_handler(SyckParser *p, SyckNode *n)
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
				double f;
				syck_str_blow_away_commas( n );
				f = strtod( n->data.str->ptr, NULL );
				ZVAL_DOUBLE( o, f );
			}
			else if ( strcmp( n->type_id, "float#nan" ) == 0 )
			{
				ZVAL_DOUBLE( o, nanphp() );
			}
			else if ( strcmp( n->type_id, "float#inf" ) == 0 )
			{
				ZVAL_DOUBLE( o, inf() );
			}
			else if ( strcmp( n->type_id, "float#neginf" ) == 0 )
			{
				ZVAL_DOUBLE( o, -inf() );
			}
			else if ( strcmp( n->type_id, "merge" ) == 0 )
			{
				TSRMLS_FETCH();
				MAKE_STD_ZVAL( o );
				object_init_ex( o, merge_key_entry );
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

void php_syck_ehandler(SyckParser *p, char *str)
{
	TSRMLS_FETCH();
	zend_throw_exception(syck_exception_entry, str, 0 TSRMLS_CC);
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

	syck_parser_handler( parser, php_syck_handler );
	syck_parser_error_handler( parser, php_syck_ehandler );

	syck_parser_implicit_typing( parser, 1 );
	syck_parser_taguri_expansion( parser, 0 );

	syck_parser_str( parser, arg, arg_len, NULL );

	v = syck_parse( parser );

	if (1 == syck_lookup_sym( parser, v, &obj )) {
		*return_value = *obj;
		zval_copy_ctor(return_value);
	}

	syck_free_parser( parser );
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
