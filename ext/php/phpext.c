/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2007 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Why the lucky stiff                                         |
  |          Alexey Zakhlestin <indeyets@gmail.com>                      |
  +----------------------------------------------------------------------+

  $Id$ 
*/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <syck.h>

#include "php.h"
#include "zend_exceptions.h"
#include "zend_interfaces.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_syck.h"

#ifndef true
# define true 1
# define false 0
#endif

#define PHP_SYCK_VERSION "0.9.2-dev"

/**
 * SyckException class
**/
#define PHP_SYCK_EXCEPTION_PARENT "UnexpectedValueException"
#define PHP_SYCK_EXCEPTION_PARENT_LC "unexpectedvalueexception"
#define PHP_SYCK_EXCEPTION_NAME "SyckException"

static zend_class_entry *spl_ce_RuntimeException;
zend_class_entry *syck_exception_entry;

PHP_SYCK_API zend_class_entry *php_syck_get_exception_base()
{
	TSRMLS_FETCH();

	if (!spl_ce_RuntimeException) {
		zend_class_entry **pce;

		if (zend_hash_find(CG(class_table), PHP_SYCK_EXCEPTION_PARENT_LC, sizeof(PHP_SYCK_EXCEPTION_PARENT_LC), (void **) &pce) == SUCCESS) {
			spl_ce_RuntimeException = *pce;
			return *pce;
		}
	} else {
		return spl_ce_RuntimeException;
	}

	return zend_exception_get_default(TSRMLS_C);
}


static double inline zero()		{ return 0.0; }
static double inline one()		{ return 1.0; }
static double inline inf()		{ return one() / zero(); }
static double inline nanphp()	{ return zero() / zero(); }

typedef struct {
	char *output;
	size_t output_size;
	size_t output_alloc;
	unsigned char level;
	zval **stack;
} php_syck_emitter_xtra;

void psex_init(php_syck_emitter_xtra *ptr)
{
	ptr->output = NULL;
	ptr->output_size = 0;
	ptr->output_alloc = 0;
	ptr->stack = emalloc(sizeof(zval *) * 255);
	ptr->level = 0;
}

void psex_free(php_syck_emitter_xtra *ptr)
{
	if (ptr->output) {
		efree(ptr->output);
		ptr->output = NULL;
	}

	if (ptr->stack) {
		efree(ptr->stack);
		ptr->stack = NULL;
	}

	ptr->output_size = 0;
	ptr->output_alloc = 0;
	ptr->level = 0;
}

void psex_add_output(php_syck_emitter_xtra *ptr, const char *data, size_t len)
{
	while (ptr->output_size + len > ptr->output_alloc) {
		if (ptr->output_alloc == 0) {
			ptr->output_alloc = 8192;
			ptr->output = emalloc(ptr->output_alloc);
		} else {
			ptr->output_alloc += 8192;
			ptr->output = erealloc(ptr->output, ptr->output_alloc);
		}
	}

	strncpy(ptr->output + ptr->output_size, data, len);
	ptr->output_size += len;
}

int psex_push_obj(php_syck_emitter_xtra *ptr, zval *obj)
{
	if (ptr->level == 255)
		return 0;

	ptr->stack[++(ptr->level)] = obj;
	return 1;
}

zval * psex_pop_obj(php_syck_emitter_xtra *ptr)
{
	if (ptr->level == 0)
		return NULL;

	return ptr->stack[(ptr->level)--];
}




/**
 * returns
 *   0 for sequential numeric arrays
 *   1 for insequential or associative arrays
**/
static int psex_determine_array_type(HashTable *myht TSRMLS_DC) /* {{{ */
{
	int i = myht ? zend_hash_num_elements(myht) : 0;

	if (i > 0) {
		char *key = NULL;
		uint key_len;
		HashPosition pos;
		ulong index, idx = 0;

		zend_hash_internal_pointer_reset_ex(myht, &pos);

		for (;; zend_hash_move_forward_ex(myht, &pos)) {
			i = zend_hash_get_current_key_ex(myht, &key, &key_len, &index, 0, &pos);

			if (i == HASH_KEY_NON_EXISTANT)
				break;

			if (i == HASH_KEY_IS_STRING) {
				return 1;
			} else {
				if (index != idx) {
					return 1;
				}
			}

			idx++;
		}
	}

	return 0;
}




function_entry syck_functions[] = {
	PHP_FE(syck_load, NULL)
	PHP_FE(syck_dump, NULL)
	{NULL, NULL, NULL}	/* Must be the last line in syck_functions[] */
};

static zend_module_dep syck_deps[] = {
	ZEND_MOD_REQUIRED("spl")
	ZEND_MOD_REQUIRED("hash")
	{NULL, NULL, NULL}
};

zend_module_entry syck_module_entry = {
	STANDARD_MODULE_HEADER_EX, NULL,
	syck_deps,
	"syck",
	syck_functions,
	PHP_MINIT(syck),	/* module init function */
	NULL,				/* module shutdown function */
	NULL,				/* request init function */
	NULL,				/* request shutdown function */
	PHP_MINFO(syck),	/* module info function */
	PHP_SYCK_VERSION,	/* Replace with version number for your extension */
	STANDARD_MODULE_PROPERTIES
};


#ifdef COMPILE_DL_SYCK
ZEND_GET_MODULE(syck)
#endif

PHP_MINIT_FUNCTION(syck)
{
	zend_class_entry ce;

	INIT_CLASS_ENTRY(ce, PHP_SYCK_EXCEPTION_NAME, NULL);
	syck_exception_entry = zend_register_internal_class_ex(&ce, php_syck_get_exception_base(), PHP_SYCK_EXCEPTION_PARENT TSRMLS_CC);
	return SUCCESS;
}

PHP_MINFO_FUNCTION(syck)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "Extension version", PHP_SYCK_VERSION);
	php_info_print_table_header(2, "Syck library version", SYCK_VERSION);
	php_info_print_table_end();
}


SYMID php_syck_handler(SyckParser *p, SyckNode *n)
{
	zval *o = NULL;

	MAKE_STD_ZVAL(o);

	switch (n->kind) {
		case syck_str_kind:
			if (n->type_id == NULL || strcmp(n->type_id, "str") == 0) {
				ZVAL_STRINGL(o, n->data.str->ptr, n->data.str->len, 1);
			} else if (strcmp(n->type_id, "null") == 0) {
				ZVAL_NULL(o);
			} else if (strcmp(n->type_id, "bool#yes") == 0) {
				ZVAL_BOOL(o, 1);
			} else if (strcmp(n->type_id, "bool#no") == 0) {
				ZVAL_BOOL(o, 0);
			} else if (strcmp(n->type_id, "bool") == 0) {
				/* implicit boolean */
				char *ptr = n->data.str->ptr;
				size_t len = n->data.str->len;
				TSRMLS_FETCH();

				if (len == 1) {
					if (ptr[0] == 'y' || ptr[0] == 'Y') {
						ZVAL_BOOL(o, 1);
					} else if (ptr[0] == 'n' || ptr[0] == 'N') {
						ZVAL_BOOL(o, 0);
					}
				} else if (len == 2) {
					if (strncmp(ptr, "on", len) == 0 || strncmp(ptr, "On", len) == 0 || strncmp(ptr, "ON", len) == 0) {
						ZVAL_BOOL(o, 1);
					} else if (strncmp(ptr, "no", len) == 0 || strncmp(ptr, "No", len) == 0 || strncmp(ptr, "NO", len) == 0) {
						ZVAL_BOOL(o, 0);
					}
				} else if (len == 3) {
					if (strncmp(ptr, "yes", len) == 0 || strncmp(ptr, "Yes", len) == 0 || strncmp(ptr, "YES", len) == 0) {
						ZVAL_BOOL(o, 1);
					} else if (strncmp(ptr, "off", len) == 0 || strncmp(ptr, "Off", len) == 0 || strncmp(ptr, "OFF", len) == 0) {
						ZVAL_BOOL(o, 0);
					}
				} else if (len == 4) {
					if (strncmp(ptr, "true", len) == 0 || strncmp(ptr, "True", len) == 0 || strncmp(ptr, "TRUE", len) == 0) {
						ZVAL_BOOL(o, 1);
					}
				} else if (len == 3) {
					if (strncmp(ptr, "false", len) == 0 || strncmp(ptr, "False", len) == 0 || strncmp(ptr, "FALSE", len) == 0) {
						ZVAL_BOOL(o, 1);
					}
				}

				if (Z_TYPE_P(o) != IS_BOOL) {
					zend_throw_exception_ex(syck_exception_entry, 0 TSRMLS_CC, "!bool specified, but value \"%s\" (len=%d) is incorrect on line %d, col %d: '%s'", ptr, len, p->linect, p->cursor - p->lineptr, p->lineptr);
				}
			} else if (strcmp(n->type_id, "int#hex") == 0) {
				long intVal;
				syck_str_blow_away_commas(n);
				intVal = strtol(n->data.str->ptr, NULL, 16);
				ZVAL_LONG(o, intVal);
			} else if (strcmp(n->type_id, "int#base60") == 0) {
				char *ptr = NULL, *end = NULL;
				long sixty = 1;
				long total = 0;

				syck_str_blow_away_commas(n);
				ptr = n->data.str->ptr;
				end = n->data.str->ptr + n->data.str->len;

				while (end > ptr) {
					long bnum = 0;
					char *colon = end - 1;
					while (colon >= ptr && *colon != ':') {
						colon--;
					}

					if (colon >= ptr && *colon == ':')
						*colon = '\0';

					bnum = strtol(colon + 1, NULL, 10);
					total += bnum * sixty;
					sixty *= 60;
					end = colon;
				}

				ZVAL_LONG(o, total);
			} else if (strcmp(n->type_id, "int#oct") == 0) {
				long intVal;
				syck_str_blow_away_commas(n);
				intVal = strtol(n->data.str->ptr, NULL, 8);
				ZVAL_LONG(o, intVal);
			} else if (strcmp(n->type_id, "int") == 0) {
				long intVal;
				syck_str_blow_away_commas(n);
				intVal = strtol(n->data.str->ptr, NULL, 10);
				ZVAL_LONG(o, intVal);
			} else if (strcmp(n->type_id, "float") == 0 || strcmp(n->type_id, "float#fix") == 0 || strcmp(n->type_id, "float#exp") == 0) {
				double f;
				syck_str_blow_away_commas(n);
				f = strtod(n->data.str->ptr, NULL);

				ZVAL_DOUBLE(o, f);
			} else if (strcmp(n->type_id, "float#base60") == 0) {
				char *ptr = NULL, *end = NULL;
				long multiplier = 1;
				double total = 0;

				syck_str_blow_away_commas(n);
				ptr = n->data.str->ptr;
				end = n->data.str->ptr + n->data.str->len;

				while (end > ptr) {
					double bnum = 0;
					char *colon = end - 1;
					while (colon >= ptr && *colon != ':') {
						colon--;
					}

					if (colon >= ptr && *colon == ':')
						*colon = '\0';

					bnum = strtod(colon + 1, NULL);
					total += bnum * multiplier;
					multiplier *= 60;
					end = colon;
				}

				ZVAL_DOUBLE(o, total);
			} else if (strcmp(n->type_id, "float#nan") == 0) {
				ZVAL_DOUBLE(o, nanphp());
			} else if (strcmp(n->type_id, "float#inf") == 0) {
				ZVAL_DOUBLE(o, inf());
			} else if (strcmp(n->type_id, "float#neginf") == 0) {
				ZVAL_DOUBLE(o, -inf());
			} else if (strncmp(n->type_id, "timestamp", 9) == 0 || strcmp(n->type_id, "php/object::Datetime") == 0) {
				zval fname, param, *params[1];
				TSRMLS_FETCH();

				ZVAL_STRING(&fname, "date_create", 1);
				INIT_ZVAL(param);
				params[0] = &param;
				ZVAL_STRINGL(params[0], n->data.str->ptr, n->data.str->len, 1);

				call_user_function(EG(function_table), NULL, &fname, o, 1, params TSRMLS_CC);

				zval_dtor(&fname);
				zval_dtor(params[0]);
			} else if (strncmp(n->type_id, "php/object::", 12) == 0) {
				/* Some custom php-class packed in a string. Only ones implementing Serializable ar supported */
				size_t classname_len = strlen(n->type_id) - 12;
				char *classname = emalloc(classname_len + 1);
				zend_class_entry **ce;
				zval *param = NULL;
				TSRMLS_FETCH();

				strncpy(classname, n->type_id + 12, classname_len + 1);

				if (FAILURE == zend_lookup_class_ex(classname, classname_len, 1, &ce TSRMLS_CC)) {
					zend_throw_exception_ex(syck_exception_entry, 0 TSRMLS_CC, "Couldn't find %s class on line %d, col %d: '%s'", classname, p->linect, p->cursor - p->lineptr, p->lineptr);
					efree(classname);
					break;
				}

				if (0 == instanceof_function_ex(*ce, zend_ce_serializable, 1 TSRMLS_CC)) {
					zend_throw_exception_ex(syck_exception_entry, 0 TSRMLS_CC, "Class %s doesn't implement Serializable on line %d, col %d: '%s'", classname, p->linect, p->cursor - p->lineptr, p->lineptr);
					efree(classname);
					break;
				}
				efree(classname);

				object_init_ex(o, *ce);
				MAKE_STD_ZVAL(param);
				ZVAL_STRINGL(param, n->data.str->ptr, n->data.str->len, 1);
				zend_call_method_with_1_params(&o, *ce, NULL, "unserialize", NULL, param);
				zval_ptr_dtor(&param);
			} else {
				php_error(E_NOTICE, "syck extension didn't handle %s type => treating as a string", n->type_id);
				ZVAL_STRINGL(o, n->data.str->ptr, n->data.str->len, 1);
			}
		break;

		case syck_seq_kind:
		{
			if (NULL == n->type_id || strcmp(n->type_id, "php/array") == 0) {
				size_t i;

				/* Just an array */
				array_init(o);

				for (i = 0; i < n->data.list->idx; i++) {
					SYMID oid = syck_seq_read(n, i);
					zval *o2 = NULL;

					syck_lookup_sym(p, oid, (char **) &o2); /* retrieving child-node */

					add_index_zval(o, i, o2);
				}
			} else if (strncmp(n->type_id, "php/array::", 11) == 0) {
				/* some class which implements ArrayAccess */
				size_t i;
				size_t classname_len = strlen(n->type_id) - 11;
				char *classname = emalloc(classname_len + 1);
				zend_class_entry **ce;
				TSRMLS_FETCH();

				strncpy(classname, n->type_id + 11, classname_len + 1);

				if (FAILURE == zend_lookup_class_ex(classname, classname_len, 1, &ce TSRMLS_CC)) {
					zend_throw_exception_ex(syck_exception_entry, 0 TSRMLS_CC, "Couldn't find %s class on line %d, col %d: '%s'", classname, p->linect, p->cursor - p->lineptr, p->lineptr);
					efree(classname);
					break;
				}

				if (0 != instanceof_function_ex(*ce, zend_ce_arrayaccess, 1 TSRMLS_CC)) {
					object_init_ex(o, *ce);

					for (i = 0; i < n->data.list->idx; i++) {
						SYMID oid = syck_seq_read(n, i);
						zval *o2 = NULL;
						zval *key = NULL;

						syck_lookup_sym(p, oid, (char **) &o2); /* retrieving child-node */

						MAKE_STD_ZVAL(key);
						ZVAL_LONG(key, i);

						zend_call_method_with_2_params(&o, *ce, NULL, "offsetset", NULL, key, o2);
						zval_ptr_dtor(&o2);
						efree(key);
					}

					if (zend_hash_exists(&(*ce)->function_table, "__wakeup", 9)) {
						zend_call_method_with_0_params(&o, *ce, NULL, "__wakeup", NULL);
					}
				} else {
					zend_throw_exception_ex(syck_exception_entry, 0 TSRMLS_CC, "Class %s doesn't implement ArrayAccess on line %d, col %d: '%s'", classname, p->linect, p->cursor - p->lineptr, p->lineptr);
					efree(classname);
					break;
				}



				efree(classname);
			} else {
				/* something else */
				php_error(E_NOTICE, "syck extension didn't handle %s type", n->type_id);
			}
		}
		break;

		case syck_map_kind:
		{
			if (NULL == n->type_id || strcmp(n->type_id, "php/hash") == 0) {
				SYMID oid;
				size_t i;
				zval *o2 = NULL, *o3 = NULL;
				zval *res = NULL;

				array_init(o);

				for (i = 0; i < n->data.pairs->idx; i++) {
					oid = syck_map_read(n, map_key, i);
					syck_lookup_sym(p, oid, (char **) &o2); /* retrieving key-node */

					if (o2->type == IS_STRING || o2->type == IS_LONG) {
						oid = syck_map_read(n, map_value, i);
						syck_lookup_sym(p, oid, (char **) &o3); /* retrieving value-node */

						if (o2->type == IS_LONG) {
							add_index_zval(o, Z_LVAL_P(o2), o3);
						} else {
							add_assoc_zval(o, Z_STRVAL_P(o2), o3);
						}
					}

					zval_ptr_dtor(&o2);
				}
			} else if (strncmp(n->type_id, "php/hash::", 10) == 0) {
				/* some classm which implements ArrayAccess */
				SYMID oid;
				size_t i;
				zval *o2 = NULL, *o3 = NULL;
				size_t classname_len = strlen(n->type_id) - 10;
				char *classname = emalloc(classname_len + 1);
				zend_class_entry **ce;
				TSRMLS_FETCH();

				strncpy(classname, n->type_id + 10, classname_len + 1);

				if (FAILURE == zend_lookup_class_ex(classname, classname_len, 1, &ce TSRMLS_CC)) {
					zend_throw_exception_ex(syck_exception_entry, 0 TSRMLS_CC, "Couldn't find %s class on line %d, col %d: '%s'", classname, p->linect, p->cursor - p->lineptr, p->lineptr);
					efree(classname);
					break;
				}

				if (0 == instanceof_function_ex(*ce, zend_ce_arrayaccess, 1 TSRMLS_CC)) {
					zend_throw_exception_ex(syck_exception_entry, 0 TSRMLS_CC, "Class %s doesn't implement ArrayAccess on line %d, col %d: '%s'", classname, p->linect, p->cursor - p->lineptr, p->lineptr);
					efree(classname);
					break;
				}

				object_init_ex(o, *ce);

				for (i = 0; i < n->data.pairs->idx; i++) {
					oid = syck_map_read(n, map_key, i);
					syck_lookup_sym(p, oid, (char **) &o2); /* retrieving key-node */

					if (o2->type == IS_STRING || o2->type == IS_LONG) {
						oid = syck_map_read(n, map_value, i);
						syck_lookup_sym(p, oid, (char **) &o3); /* retrieving value-node */

						zend_call_method_with_2_params(&o, *ce, NULL, "offsetset", NULL, o2, o3);
						zval_ptr_dtor(&o3);
					}

					zval_ptr_dtor(&o2);
				}

				if (zend_hash_exists(&(*ce)->function_table, "__wakeup", 9)) {
					zend_call_method_with_0_params(&o, *ce, NULL, "__wakeup", NULL);
				}

				efree(classname);
			} else {
				/* something else */
				php_error(E_NOTICE, "syck extension didn't handle %s type", n->type_id);
			}
		}
		break;
	}

	return syck_add_sym(p, (char *)o); /* storing node */
}

SyckNode * php_syck_badanchor_handler(SyckParser *p, const char *str)
{
	SyckNode *res;
	char *endl = p->cursor;
	TSRMLS_FETCH();

	while (*endl != '\0' && *endl != '\n')
		endl++;

	endl[0] = '\0';

	res = syck_alloc_str();

	zend_throw_exception_ex(syck_exception_entry, 0 TSRMLS_CC, "bad anchor \"%s\" on line %d, col %d", str, p->linect, p->cursor - p->lineptr - strlen(str));

	return res;
}

enum st_retval my_cleaner(char *key, char *value, char *smth)
{
	zval *ptr = (zval *)value;

	zval_ptr_dtor(&ptr);
	return ST_DELETE;
}

void php_syck_ehandler(SyckParser *p, const char *str)
{
	char *endl = p->cursor;
	TSRMLS_FETCH();

	while (*endl != '\0' && *endl != '\n')
		endl++;

	endl[0] = '\0';

	zend_throw_exception_ex(syck_exception_entry, 0 TSRMLS_CC, "%s on line %d, col %d: '%s'", str, p->linect, p->cursor - p->lineptr, p->lineptr);

	st_foreach(p->syms, my_cleaner, NULL);
}

void php_syck_emitter_handler(SyckEmitter *e, st_data_t id)
{
	php_syck_emitter_xtra *bonus = (php_syck_emitter_xtra *) e->bonus;
	zval *data = bonus->stack[id];
	TSRMLS_FETCH();

	switch (Z_TYPE_P(data)) {
		case IS_NULL:
			syck_emit_scalar(e, "null", scalar_none, 0, 0, 0, "", 0);
		break;

		case IS_BOOL:
		{
			char *bool_s = Z_BVAL_P(data) ? "true" : "false";
			syck_emit_scalar(e, "boolean", scalar_none, 0, 0, 0, bool_s, strlen(bool_s));
		}
		break;

		case IS_LONG:
		{
			size_t res_size;
			char *res = NULL;

			res_size = snprintf(res, 0, "%ld", Z_LVAL_P(data)); /* getting size ("0" doesn't let output) */
			res = emalloc(res_size + 1);
			snprintf(res, res_size + 1, "%ld", Z_LVAL_P(data));

			syck_emit_scalar(e, "number", scalar_none, 0, 0, 0, res, res_size);
			efree(res);
		}
		break;

		case IS_DOUBLE:
		{
			size_t res_size;
			char *res = NULL;

			res_size = snprintf(res, 0, "%f", Z_DVAL_P(data)); /* getting size ("0" doesn't let output) */
			res = emalloc(res_size + 1);
			snprintf(res, res_size + 1, "%f", Z_DVAL_P(data));

			syck_emit_scalar(e, "number", scalar_none, 0, 0, 0, res, res_size);
			efree(res);
		}
		break;

		case IS_STRING:
			syck_emit_scalar(e, "str", scalar_2quote, 0, 0, 0, Z_STRVAL_P(data), Z_STRLEN_P(data));
		break;

		case IS_ARRAY:
		{
			HashTable *tbl = Z_ARRVAL_P(data);
			int flat_and_short = false;

			/* we try to detect if array is short and flat */
			if (tbl->nNumOfElements <= 6) {
				flat_and_short = true;

				for (zend_hash_internal_pointer_reset(tbl); zend_hash_has_more_elements(tbl) == SUCCESS; zend_hash_move_forward(tbl)) {
					zval **ppzval = NULL;

					zend_hash_get_current_data(tbl, (void **)&ppzval);

					if (Z_TYPE_P(*ppzval) == IS_ARRAY || Z_TYPE_P(*ppzval) == IS_OBJECT) {
						flat_and_short = false;
					}
				}
			}

			if (0 == psex_determine_array_type(tbl TSRMLS_CC)) {
				/* indexed array */
				if (flat_and_short)
					syck_emit_seq(e, "table", seq_inline);
				else
					syck_emit_seq(e, "table", seq_none);

				for (zend_hash_internal_pointer_reset(tbl); zend_hash_has_more_elements(tbl) == SUCCESS; zend_hash_move_forward(tbl)) {
					zval **ppzval = NULL;

					zend_hash_get_current_data(tbl, (void **)&ppzval);
					if (psex_push_obj(bonus, *ppzval)) {
						syck_emit_item(e, bonus->level);
						psex_pop_obj(bonus);
					}
				}

				syck_emit_end(e);
			} else {
				/* associative array */
				if (flat_and_short)
					syck_emit_map(e, "table", map_inline);
				else
					syck_emit_map(e, "table", map_none);

				for (zend_hash_internal_pointer_reset(tbl); zend_hash_has_more_elements(tbl) == SUCCESS; zend_hash_move_forward(tbl)) {
					zval **ppzval = NULL, kzval;
					char *key = NULL;
					uint key_len;
					ulong idx;
					size_t key_type;

					zend_hash_get_current_key_ex(tbl, (char **)&key, &key_len, &idx, 0, NULL);
					zend_hash_get_current_data(tbl, (void **)&ppzval);
					key_type = zend_hash_get_current_key_type_ex(tbl, NULL);

					if (key_type == HASH_KEY_IS_LONG) {
						key_len = 1 + snprintf(key, 0, "%ld", idx); /* getting size ("0" doesn't let output) */
						key = emalloc(key_len);
						snprintf(key, key_len, "%ld", idx);
					}

					ZVAL_STRINGL(&kzval, key, key_len - 1, 1);

					if (psex_push_obj(bonus, &kzval)) {
						syck_emit_item(e, bonus->level);
						psex_pop_obj(bonus);

						if (psex_push_obj(bonus, *ppzval)) {
							syck_emit_item(e, bonus->level);
							psex_pop_obj(bonus);
						}
					}

					zval_dtor(&kzval);

					if (key_type == HASH_KEY_IS_LONG) {
						efree(key);
					}
				}

				syck_emit_end(e);
			}
		}
		break;

		case IS_OBJECT:
		{
			char *name = NULL;
			zend_uint name_len;
			zend_class_entry *ce;
			TSRMLS_FETCH();

			ce = Z_OBJCE_P(data);
			zend_get_object_classname(data, &name, &name_len TSRMLS_CC);

			/* DateTime is encoded as timestamp */
			if (strncmp(name, "DateTime", name_len) == 0) {
				zval *retval = NULL;
				zval constant;

				zend_get_constant("DateTime::ISO8601", 17, &constant TSRMLS_CC);
				zend_call_method_with_1_params(&data, ce, NULL, "format", &retval, &constant);

				zval_dtor(&constant);

				syck_emit_scalar(e, "tag:yaml.org,2002:timestamp#iso8601", scalar_none, 0, 0, 0, Z_STRVAL_P(retval), Z_STRLEN_P(retval));
				zval_dtor(retval);
				efree(retval);
			} else {
				if (0 != instanceof_function_ex(ce, zend_ce_serializable, 1 TSRMLS_CC)) {
					char *prefix = "tag:php:object::";
					size_t prefix_len = strlen(prefix) + 1;
					char *tagname = emalloc(name_len + prefix_len);
					zval *serialized = NULL;

					snprintf(tagname, name_len + prefix_len, "%s%s", prefix, name);

					zend_call_method_with_0_params(&data, ce, NULL, "serialize", &serialized);
					syck_emit_scalar(e, tagname, scalar_2quote, 0, 0, 0, Z_STRVAL_P(serialized), Z_STRLEN_P(serialized));

					efree(tagname);
				}
			}

			efree(name);
		}
		break;

		case IS_RESOURCE:
		default:
			/* something unknown */
		break;
	}
}

void php_syck_output_handler(SyckEmitter *e, const char *str, long len)
{
	php_syck_emitter_xtra *bonus = (php_syck_emitter_xtra *) e->bonus;
	psex_add_output(bonus, str, (size_t)len);
}

/* {{{ proto object syck_load(string arg)
   Return PHP object from a YAML string */
PHP_FUNCTION(syck_load)
{
	char *arg = NULL;
	int arg_len;
	SYMID v;
	zval *obj = NULL;
	SyckParser *parser;

	if (ZEND_NUM_ARGS() != 1) {
		WRONG_PARAM_COUNT;
	}

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	parser = syck_new_parser();

	syck_parser_handler(parser, php_syck_handler);
	syck_parser_bad_anchor_handler(parser, php_syck_badanchor_handler);
	syck_parser_error_handler(parser, php_syck_ehandler);

	syck_parser_implicit_typing(parser, 1);
	syck_parser_taguri_expansion(parser, 0);

	syck_parser_str(parser, arg, arg_len, NULL);

	v = syck_parse(parser);

	if (1 == syck_lookup_sym(parser, v, (char **) &obj)) {
		*return_value = *obj;
		zval_copy_ctor(return_value);
		zval_ptr_dtor(&obj);
	}


	syck_free_parser(parser);
}
/* }}} */


/* {{{ proto object syck_load(string arg)
   Convert PHP object into YAML string */
PHP_FUNCTION(syck_dump)
{
	SyckEmitter *emitter;
	php_syck_emitter_xtra *extra;
	zval *ptr;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &ptr) == FAILURE) {
		return;
	}

	extra = emalloc(sizeof(php_syck_emitter_xtra));
	psex_init(extra);
	psex_push_obj(extra, ptr);

	emitter = syck_new_emitter();
	emitter->bonus = extra;

	syck_emitter_handler(emitter, php_syck_emitter_handler);
	syck_output_handler(emitter, php_syck_output_handler);

	syck_emit(emitter, extra->level);
	syck_emitter_flush(emitter, 0);

	ZVAL_STRINGL(return_value, extra->output, extra->output_size, 1);

	psex_free(extra);
	efree(extra);
	syck_free_emitter(emitter);
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
