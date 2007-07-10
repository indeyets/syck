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

#ifndef PHP_SYCK_H
# define PHP_SYCK_H

extern zend_module_entry syck_module_entry;
# define phpext_syck_ptr &syck_module_entry

# ifdef PHP_WIN32
#  define PHP_SYCK_API __declspec(dllexport)
# else
#  define PHP_SYCK_API
# endif

# ifdef ZTS
#  include "TSRM.h"
# endif

PHP_MINIT_FUNCTION(syck);
PHP_MINFO_FUNCTION(syck);

PHP_FUNCTION(syck_load);
PHP_FUNCTION(syck_dump);

/* 
	Declare any global variables you may need between the BEGIN
	and END macros here:     

ZEND_BEGIN_MODULE_GLOBALS(syck)
	int   global_value;
	char *global_string;
ZEND_END_MODULE_GLOBALS(syck)

#ifdef ZTS
# define SYCK_G(v) TSRMG(syck_globals_id, zend_syck_globals *, v)
#else
# define SYCK_G(v) (syck_globals.v)
#endif
*/

#endif	/* PHP_SYCK_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
