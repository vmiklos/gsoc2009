/* -----------------------------------------------------------------------------
 * See the LICENSE file for information on copyright, usage and redistribution
 * of SWIG, and the README file for authors - http://www.swig.org/release.html.
 *
 * director.swg
 *
 * This file contains support for director classes that proxy
 * method calls from C++ to PHP extensions.
 * ----------------------------------------------------------------------------- */

#ifndef SWIG_DIRECTOR_PHP_HEADER_
#define SWIG_DIRECTOR_PHP_HEADER_

#ifdef __cplusplus

/*
  Use -DSWIG_DIRECTOR_STATIC if you prefer to avoid the use of the
  'Swig' namespace. This could be useful for multi-modules projects.
*/
#ifdef SWIG_DIRECTOR_STATIC
/* Force anonymous (static) namespace */
#define Swig
#endif

namespace Swig {
  class Director {
    protected:
      zval *swig_self;
    public:
      Director(zval* self) : swig_self(self) {
      }
      bool is_overriden_method(char *cname, char *lc_fname) {
        zval classname;
        zend_class_entry **ce;
        zend_function *mptr;
        int name_len = strlen(lc_fname);
        
        ZVAL_STRING(&classname, cname, 0);
        if (zend_lookup_class(Z_STRVAL_P(&classname), Z_STRLEN_P(&classname), &ce TSRMLS_CC) != SUCCESS) {
          return false;
        }
        if (zend_hash_find(&(*ce)->function_table, lc_fname, name_len + 1, (void**) &mptr) != SUCCESS) {
          return false;
        }
        // common.scope points to the declaring class
        return strcmp(mptr->common.scope->name, cname);
      }
  };
}

#endif /* __cplusplus */

#endif
