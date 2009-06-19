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
  };
}

/*
 * TODO: Handle the case when not all methods are overwritten in
 * director classes.
 */
static int upcall;

#endif /* __cplusplus */

#endif
