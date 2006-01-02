/* ----------------------------------------------------------------------------- 
 * naming.c
 *
 *     Functions for generating various kinds of names during code generation
 * 
 * Author(s) : David Beazley (beazley@cs.uchicago.edu)
 *
 * Copyright (C) 1999-2000.  The University of Chicago
 * See the file LICENSE for information on usage and redistribution.	
 * ----------------------------------------------------------------------------- */

char cvsroot_naming_c[] = "$Header$";

#include "swig.h"
#include "cparse.h"
#include "swigkeys.h"
#include <ctype.h>

/* Hash table containing naming data */

static Hash *naming_hash = 0;

/* #define SWIG_DEBUG  */

/* -----------------------------------------------------------------------------
 * Swig_name_register()
 *
 * Register a new naming format.
 * ----------------------------------------------------------------------------- */

void
Swig_name_register(const String_or_char *method, const String_or_char *format) {
  if (!naming_hash) naming_hash = NewHash();
  Setattr(naming_hash,method,format);
}

void
Swig_name_unregister(const String_or_char *method) {
  if (naming_hash) {
    Delattr(naming_hash,method);
  }
}

static int name_mangle(String *r) {
  char  *c;
  int    special;
  special = 0;
  Replaceall(r,"::","_");
  c = Char(r);
  while (*c) {
    if (!isalnum((int) *c) && (*c != '_')) {
      special = 1;
      switch(*c) {
      case '+':
	*c = 'a';
	break;
      case '-':
	*c = 's';
	break;
      case '*':
	*c = 'm';
	break;
      case '/':
	*c = 'd';
	break;
      case '<':
	*c = 'l';
	break;
      case '>':
	*c = 'g';
	break;
      case '=':
	*c = 'e';
	break;
      case ',':
	*c = 'c';
	break;
      case '(':
	*c = 'p';
	break;
      case ')':
	*c = 'P';
	break;
      case '[':
	*c = 'b';
	break;
      case ']':
	*c = 'B';
	break;
      case '^':
	*c = 'x';
	break;
      case '&':
	*c = 'A';
	break;
      case '|':
	*c = 'o';
	break;
      case '~':
	*c = 'n';
	break;
      case '!':
	*c = 'N';
	break;
      case '%':
	*c = 'M';
	break;
      case '.':
	*c = 'f';
	break;
      case '?':
	*c = 'q';
	break;
      default:
	*c = '_';
	break;
      }
    }
    c++;
  }
  if (special) Append(r,"___");
  return special;
}

/* -----------------------------------------------------------------------------
 * Swig_name_mangle()
 *
 * Converts all of the non-identifier characters of a string to underscores.
 * ----------------------------------------------------------------------------- */

String *
Swig_name_mangle(const String_or_char *s) {
#if 0
  String *r = NewString(s);
  name_mangle(r);
  return r;
#else
  return Swig_string_mangle(s);
#endif
}

/* -----------------------------------------------------------------------------
 * Swig_name_wrapper()
 *
 * Returns the name of a wrapper function.
 * ----------------------------------------------------------------------------- */

String *
Swig_name_wrapper(const String_or_char *fname) {
  String *r;
  String *f;

  r = NewStringEmpty();
  if (!naming_hash) naming_hash = NewHash();
  f = HashGetAttr(naming_hash,k_wrapper);
  if (!f) {
    Append(r,"_wrap_%f");
  } else {
    Append(r,f);
  }
  Replace(r,"%f",fname, DOH_REPLACE_ANY);
  name_mangle(r);
  return r;
}


/* -----------------------------------------------------------------------------
 * Swig_name_member()
 *
 * Returns the name of a class method.
 * ----------------------------------------------------------------------------- */

String *
Swig_name_member(const String_or_char *classname, const String_or_char *mname) {
  String *r;
  String *f;
  String *rclassname;
  char   *cname;

  rclassname = SwigType_namestr(classname);
  r = NewStringEmpty();
  if (!naming_hash) naming_hash = NewHash();
  f = HashGetAttr(naming_hash,k_member);
  if (!f) {
    Append(r,"%c_%m");
  } else {
    Append(r,f);
  }
  cname = Char(rclassname);
  if ((strncmp(cname,"struct ", 7) == 0) ||
      ((strncmp(cname,"class ", 6) == 0)) ||
      ((strncmp(cname,"union ", 6) == 0))) {
    cname = strchr(cname, ' ')+1;
  }
  Replace(r,"%c",cname, DOH_REPLACE_ANY);
  Replace(r,"%m",mname, DOH_REPLACE_ANY);
  /*  name_mangle(r);*/
  Delete(rclassname);
  return r;
}

/* -----------------------------------------------------------------------------
 * Swig_name_get()
 *
 * Returns the name of the accessor function used to get a variable.
 * ----------------------------------------------------------------------------- */

String *
Swig_name_get(const String_or_char *vname) {
  String *r;
  String *f;

#ifdef SWIG_DEBUG
  Printf(stdout,"Swig_name_get:  '%s'\n", vname); 
#endif

  r = NewStringEmpty();
  if (!naming_hash) naming_hash = NewHash();
  f = HashGetAttr(naming_hash,k_get);
  if (!f) {
    Append(r,"%v_get");
  } else {
    Append(r,f);
  }
  Replace(r,"%v",vname, DOH_REPLACE_ANY);
  /* name_mangle(r); */
  return r;
}

/* ----------------------------------------------------------------------------- 
 * Swig_name_set()
 *
 * Returns the name of the accessor function used to set a variable.
 * ----------------------------------------------------------------------------- */

String *
Swig_name_set(const String_or_char *vname) {
  String *r;
  String *f;

  r = NewStringEmpty();
  if (!naming_hash) naming_hash = NewHash();
  f = HashGetAttr(naming_hash,k_set);
  if (!f) {
    Append(r,"%v_set");
  } else {
    Append(r,f);
  }
  Replace(r,"%v",vname, DOH_REPLACE_ANY);
  /* name_mangle(r); */
  return r;
}

/* -----------------------------------------------------------------------------
 * Swig_name_construct()
 *
 * Returns the name of the accessor function used to create an object.
 * ----------------------------------------------------------------------------- */

String *
Swig_name_construct(const String_or_char *classname) {
  String *r;
  String *f;
  String *rclassname;
  char *cname;

  rclassname = SwigType_namestr(classname);
  r = NewStringEmpty();
  if (!naming_hash) naming_hash = NewHash();
  f = HashGetAttr(naming_hash,k_construct);
  if (!f) {
    Append(r,"new_%c");
  } else {
    Append(r,f);
  }

  cname = Char(rclassname);
  if ((strncmp(cname,"struct ", 7) == 0) ||
      ((strncmp(cname,"class ", 6) == 0)) ||
      ((strncmp(cname,"union ", 6) == 0))) {
    cname = strchr(cname, ' ')+1;
  }
  Replace(r,"%c",cname, DOH_REPLACE_ANY);
  Delete(rclassname);
  return r;
}


/* -----------------------------------------------------------------------------
 * Swig_name_copyconstructor()
 *
 * Returns the name of the accessor function used to copy an object.
 * ----------------------------------------------------------------------------- */

String *
Swig_name_copyconstructor(const String_or_char *classname) {
  String *r;
  String *f;
  String *rclassname;
  char *cname;

  rclassname = SwigType_namestr(classname);
  r = NewStringEmpty();
  if (!naming_hash) naming_hash = NewHash();
  f = HashGetAttr(naming_hash,k_construct);
  if (!f) {
    Append(r,"copy_%c");
  } else {
    Append(r,f);
  }

  cname = Char(rclassname);
  if ((strncmp(cname,"struct ", 7) == 0) ||
      ((strncmp(cname,"class ", 6) == 0)) ||
      ((strncmp(cname,"union ", 6) == 0))) {
    cname = strchr(cname, ' ')+1;
  }

  Replace(r,"%c",cname, DOH_REPLACE_ANY);
  Delete(rclassname);
  return r;
}

/* -----------------------------------------------------------------------------
 * Swig_name_destroy()
 *
 * Returns the name of the accessor function used to destroy an object.
 * ----------------------------------------------------------------------------- */

String *Swig_name_destroy(const String_or_char *classname) {
  String *r;
  String *f;
  String *rclassname;
  char *cname;
  rclassname = SwigType_namestr(classname);
  r = NewStringEmpty();
  if (!naming_hash) naming_hash = NewHash();
  f = HashGetAttr(naming_hash,k_destroy);
  if (!f) {
    Append(r,"delete_%c");
  } else {
    Append(r,f);
  }

  cname = Char(rclassname);
  if ((strncmp(cname,"struct ", 7) == 0) ||
      ((strncmp(cname,"class ", 6) == 0)) ||
      ((strncmp(cname,"union ", 6) == 0))) {
    cname = strchr(cname, ' ')+1;
  }
  Replace(r,"%c",cname, DOH_REPLACE_ANY);
  Delete(rclassname);
  return r;
}


/* -----------------------------------------------------------------------------
 * Swig_name_disown()
 *
 * Returns the name of the accessor function used to disown an object.
 * ----------------------------------------------------------------------------- */

String *Swig_name_disown(const String_or_char *classname) {
  String *r;
  String *f;
  String *rclassname;
  char *cname;
  rclassname = SwigType_namestr(classname);
  r = NewStringEmpty();
  if (!naming_hash) naming_hash = NewHash();
  f = HashGetAttr(naming_hash,k_disown);
  if (!f) {
    Append(r,"disown_%c");
  } else {
    Append(r,f);
  }

  cname = Char(rclassname);
  if ((strncmp(cname,"struct ", 7) == 0) ||
      ((strncmp(cname,"class ", 6) == 0)) ||
      ((strncmp(cname,"union ", 6) == 0))) {
    cname = strchr(cname, ' ')+1;
  }
  Replace(r,"%c",cname, DOH_REPLACE_ANY);
  Delete(rclassname);
  return r;
}


/* -----------------------------------------------------------------------------
 * Swig_name_object_set()
 *
 * Sets an object associated with a name and optional declarators. 
 * ----------------------------------------------------------------------------- */

void 
Swig_name_object_set(Hash *namehash, String *name, SwigType *decl, DOH *object) {
  DOH *n;

#ifdef SWIG_DEBUG
  Printf(stdout,"Swig_name_object_set:  '%s', '%s'\n", name, decl); 
#endif
  n = HashGetAttr(namehash,name);
  if (!n) {
    n = NewHash();
    Setattr(namehash,name,n);
    Delete(n);
  }
  /* Add an object based on the declarator value */
  if (!decl) {
    Setattr(n,k_start,object);
  } else {
    SwigType *cd = Copy(decl);
    Setattr(n,cd,object);
    Delete(cd);
  }
}

/* -----------------------------------------------------------------------------
 * Swig_name_object_get()
 *
 * Return an object associated with an optional class prefix, name, and 
 * declarator.   This function operates according to name matching rules
 * described for the %rename directive in the SWIG manual.
 * ----------------------------------------------------------------------------- */

static DOH *get_object(Hash *n, String *decl) {
  DOH *rn = 0;
  if (!n) return 0;
  if (decl) {
    rn = Getattr(n,decl);
  } else {
    rn = Getattr(n,k_start);
  }
  return rn;
}

static
DOH *
name_object_get(Hash *namehash, String *tname, SwigType *decl, SwigType *ncdecl) {
  DOH* rn = 0;
  Hash   *n = HashGetAttr(namehash,tname);
  if (n) {
    rn = get_object(n,decl);
    if ((!rn) && ncdecl) rn = get_object(n,ncdecl);
    if (!rn) rn = get_object(n,0);
  }
  return rn;
}

DOH *
Swig_name_object_get(Hash *namehash, String *prefix, String *name, SwigType *decl) {
  String *tname = NewStringEmpty();
  DOH    *rn = 0;
  char   *ncdecl = 0;

  if (!namehash) return 0;

  /* DB: This removed to more tightly control feature/name matching */
  /*  if ((decl) && (SwigType_isqualifier(decl))) {
    ncdecl = strchr(Char(decl),'.');
    ncdecl++;
  }
  */

  /* Perform a class-based lookup (if class prefix supplied) */
  if (prefix) {
    if (Len(prefix)) {
      Printf(tname,"%s::%s", prefix, name);
      rn = name_object_get(namehash, tname, decl, ncdecl);
      if (!rn) {
	String *cls = Swig_scopename_last(prefix);
	if (!Equal(cls,prefix)) {
	  Clear(tname);
	  Printf(tname,"*::%s::%s",cls,name);
	  rn = name_object_get(namehash, tname, decl, ncdecl);
	}
	Delete(cls);
      }
      /* A template-based class lookup, check name first */
      if (!rn && SwigType_istemplate(name)) {
	String *t_name = SwigType_templateprefix(name);
	if (!Equal(t_name,name)) {
	  rn = Swig_name_object_get(namehash, prefix, t_name, decl);
	}
	Delete(t_name);
      }
      /* A template-based class lookup */
      if (!rn && SwigType_istemplate(prefix)) {
	String *t_prefix = SwigType_templateprefix(prefix);
	if (Strcmp(t_prefix,prefix) != 0) {
	  String *t_name = SwigType_templateprefix(name);
	  rn = Swig_name_object_get(namehash, t_prefix, t_name, decl);
	  Delete(t_name);
	}
	Delete(t_prefix);
      }
      /* A wildcard-based class lookup */
      if (!rn) {
	if (!Equal(name,k_start)) {
	  rn = Swig_name_object_get(namehash, prefix, k_start, decl);
	}
      }
    }
    if (!rn) {
      Clear(tname);
      Printf(tname,"*::%s",name);
      rn = name_object_get(namehash, tname, decl, ncdecl);
    }
  } else {
    /* Lookup in the global namespace only */
    Clear(tname);
    Printf(tname,"::%s",name);
    rn = name_object_get(namehash, tname, decl, ncdecl);
  }
  /* Catch-all */
  if (!rn) {
    rn = name_object_get(namehash, name, decl, ncdecl);
  }
  if (!rn) {
    rn = name_object_get(namehash, k_start, decl, ncdecl);
  }
  Delete(tname);
  return rn;
}

/* -----------------------------------------------------------------------------
 * Swig_name_object_inherit()
 *
 * Implements name-based inheritance scheme. 
 * ----------------------------------------------------------------------------- */

void
Swig_name_object_inherit(Hash *namehash, String *base, String *derived) {
  Iterator ki;
  String *bprefix;
  String *dprefix;
  char *cbprefix;
  int   plen;

  if (!namehash) return;
  
  bprefix = NewStringf("%s::",base);
  dprefix = NewStringf("%s::",derived);
  cbprefix = Char(bprefix);
  plen = strlen(cbprefix);
  for (ki = First(namehash); ki.key; ki = Next(ki)) {
    char *k = Char(ki.key);
    if (strncmp(k,cbprefix,plen) == 0) {
      Iterator oi;
      String *nkey = NewStringf("%s%s",dprefix,k+plen);
      Hash *n = ki.item;
      Hash *newh = HashGetAttr(namehash,nkey);
      if (!newh) {
	newh = NewHash();
	Setattr(namehash,nkey,newh);
	Delete(newh);
      }
      for (oi = First(n); oi.key; oi = Next(oi)) {
	if (!Getattr(newh,oi.key)) {
	  String *ci = Copy(oi.item);
	  Setattr(newh,oi.key,ci);
	  Delete(ci);
	}
      }
      Delete(nkey);
    }
  }
  Delete(bprefix);
  Delete(dprefix);
}

/* -----------------------------------------------------------------------------
 * merge_features()
 *
 * Given a hash, this function merges the features in the hash into the node.
 * ----------------------------------------------------------------------------- */

static void merge_features(Hash *features, Node *n) {
  Iterator ki;

  if (!features) return;
  for (ki = First(features); ki.key; ki = Next(ki)) {
    String *ci = Copy(ki.item);    
    Setattr(n,ki.key,ci);
    Delete(ci);
  }
}

/* -----------------------------------------------------------------------------
 * Swig_features_get()
 *
 * Attaches any features in the features hash to the node that matches
 * the declaration, decl.
 * ----------------------------------------------------------------------------- */

static 
void features_get(Hash *features, String *tname, SwigType *decl, SwigType *ncdecl, Node *node)
{
  Node *n = Getattr(features,tname);
#ifdef SWIG_DEBUG
  Printf(stdout,"  features_get: %s\n", tname);
#endif
  if (n) {
    merge_features(get_object(n,0),node);
    if (ncdecl) merge_features(get_object(n,ncdecl),node);
    merge_features(get_object(n,decl),node);
  }
}

void
Swig_features_get(Hash *features, String *prefix, String *name, SwigType *decl, Node *node) {
  char   *ncdecl = 0;
  String *rdecl = 0;
  SwigType *rname = 0;
  if (!features) return;

  /* MM: This removed to more tightly control feature/name matching */
  /*
  if ((decl) && (SwigType_isqualifier(decl))) {
    ncdecl = strchr(Char(decl),'.');
    ncdecl++;
  }
  */

  /* very specific hack for template constructors/destructors */
  if (name && SwigType_istemplate(name) &&
      (HashCheckAttr(node,k_nodetype,k_constructor)
       || HashCheckAttr(node, k_nodetype,k_destructor))) {
    String *nprefix = NewStringEmpty();
    String *nlast = NewStringEmpty();
    String *tprefix;
    Swig_scopename_split(name, &nprefix, &nlast);    
    tprefix = SwigType_templateprefix(nlast);
    Delete(nlast);
    if (Len(nprefix)) {
      Append(nprefix,"::");
      Append(nprefix,tprefix);
      Delete(tprefix);
      rname = nprefix;      
    } else {
      rname = tprefix;
      Delete(nprefix);
    }
    rdecl = Copy(decl);
    Replaceall(rdecl,name,rname);
    decl = rdecl;
    name = rname;
  }
  
#ifdef SWIG_DEBUG
  Printf(stdout,"Swig_features_get: %s %s %s\n", prefix, name, decl);
#endif

  /* Global features */
  features_get(features, "", 0, 0, node);
  if (name) {
    String *tname = NewStringEmpty();
    /* Catch-all */
    features_get(features, name, decl, ncdecl, node);
    /* Perform a class-based lookup (if class prefix supplied) */
    if (prefix) {
      /* A class-generic feature */
      if (Len(prefix)) {
	Printf(tname,"%s::",prefix);
	features_get(features, tname, decl, ncdecl, node);
      }
      /* A wildcard-based class lookup */
      Clear(tname);
      Printf(tname,"*::%s",name);
      features_get(features, tname, decl, ncdecl, node);
      /* A specific class lookup */
      if (Len(prefix)) {
	/* A template-based class lookup */
	if (SwigType_istemplate(prefix)) {
	  String *tprefix = SwigType_templateprefix(prefix);
	  Clear(tname);
	  Printf(tname,"%s::%s",tprefix,name);
	  features_get(features, tname, decl, ncdecl, node);
	  Delete(tprefix);
	}
	Clear(tname);
	Printf(tname,"%s::%s",prefix,name);
	features_get(features, tname, decl, ncdecl, node);
      }
    } else {
      /* Lookup in the global namespace only */
      Clear(tname);
      Printf(tname,"::%s",name);
      features_get(features, tname, decl, ncdecl, node);
    }
    Delete(tname);
  }
  if (name && SwigType_istemplate(name)) {
    String *dname = Swig_symbol_template_deftype(name,0);
    if (Strcmp(dname,name)) {    
      Swig_features_get(features, prefix, dname, decl, node);
    }
    Delete(dname);
  }

  Delete(rname);
  Delete(rdecl);
}


/* -----------------------------------------------------------------------------
 * Swig_feature_set()
 *
 * Sets a feature name and value. Also sets optional feature attributes as
 * passed in by featureattribs. Optional feature attributes are given a full name
 * concatenating the feature name plus ':' plus the attribute name.
 * ----------------------------------------------------------------------------- */

void 
Swig_feature_set(Hash *features, const String_or_char *name, SwigType *decl, const String_or_char *featurename, String *value, Hash *featureattribs) {
  Hash *n;
  Hash *fhash;

#ifdef SWIG_DEBUG
  Printf(stdout,"Swig_feature_set: %s %s %s %s\n", name, decl, featurename,value);  
#endif

  n = Getattr(features,name);
  if (!n) {
    n = NewHash();
    Setattr(features,name,n);
    Delete(n);
  }
  if (!decl) {
    fhash = Getattr(n,k_start);
    if (!fhash) {
      fhash = NewHash();
      Setattr(n,k_start,fhash);
      Delete(fhash);
    }
  } else {
    fhash = Getattr(n,decl);
    if (!fhash) {
      String *cdecl_ = Copy(decl);
      fhash = NewHash();
      Setattr(n,cdecl_,fhash);
      Delete(cdecl_);
      Delete(fhash);
    }
  }
  if (value) {
    Setattr(fhash,featurename,value);
  } else {
    Delattr(fhash,featurename);
  }

  {
    /* Add in the optional feature attributes */
    Hash *attribs = featureattribs;
    while(attribs) {
      String *attribname = Getattr(attribs,k_name);
      String *featureattribname = NewStringf("%s:%s", featurename, attribname);
      if (value) {
        String *attribvalue = Getattr(attribs,k_value);
        Setattr(fhash,featureattribname,attribvalue);
      } else {
        Delattr(fhash,featureattribname);
      }
      attribs = nextSibling(attribs);
      Delete(featureattribname);
    }
  }

  if (name && SwigType_istemplate(name)) {
    String *dname = Swig_symbol_template_deftype(name,0);
    if (Strcmp(dname,name)) {    
      Swig_feature_set(features, dname, decl, featurename, value, featureattribs);
    }
    Delete(dname);
  }
}


/*
  The rename/namewarn engine
*/

static Hash   *namewarn_hash = 0;
Hash *Swig_name_namewarn_hash() {
  if (!namewarn_hash) namewarn_hash = NewHash();
  return namewarn_hash;
}

static Hash   *rename_hash = 0;
Hash *Swig_name_rename_hash() {
  if (!rename_hash) rename_hash = NewHash();
  return rename_hash;
}

static List   *namewarn_list = 0;
List *Swig_name_namewarn_list() {
  if (!namewarn_list) namewarn_list = NewList();
  return namewarn_list;
}

static List   *rename_list = 0;
List *Swig_name_rename_list() {
  if (!rename_list) rename_list = NewList();
  return rename_list;
}


/* -----------------------------------------------------------------------------
 * int Swig_need_name_warning(Node *n)
 *
 * Detects if a node needs name warnings 
 *
 * ----------------------------------------------------------------------------- */

int Swig_need_name_warning(Node *n)
{
  int need = 1;
  /* 
     we don't use name warnings for:
     - class forwards, no symbol is generated at the target language.
     - template declarations, only for real instances using %template(name).
     - typedefs, they have no effect at the target language.
  */
  if (checkAttribute(n,k_nodetype,k_classforward)) {
    need = 0;
  } else if (checkAttribute(n,k_storage,k_typedef))  {
    need = 0;
  } else if (Getattr(n,k_hidden)) {
    need = 0;
  } else if (Getattr(n,k_ignore)) {
    need = 0;
  } else if (Getattr(n,k_templatetype)) {
    need = 0;
  }
  return need;
}

/* -----------------------------------------------------------------------------
 * Swig_nodes_are_equivalent(Node* a, Node* b, int a_inclass)
 * Restores attributes saved by a previous call to Swig_require() or Swig_save().
 * ----------------------------------------------------------------------------- */

static int nodes_are_equivalent(Node* a, Node* b, int a_inclass)
{
  /* they must have the same type */
  String *ta = Getattr(a,k_nodetype);
  String *tb = Getattr(b,k_nodetype);  
  if (Cmp(ta, tb) != 0) return 0;
  
  /* cdecl case */
  if (Cmp(ta, k_cdecl) == 0) {
    /* typedef */
    String *a_storage = Getattr(a,k_storage);
    String *b_storage = Getattr(b,k_storage);

    if ((Cmp(a_storage,k_typedef) == 0)
	|| (Cmp(b_storage,k_typedef) == 0)) {	
      if (Cmp(a_storage, b_storage) == 0) {
	String *a_type = (Getattr(a,k_type));
	String *b_type = (Getattr(b,k_type));
	if (Cmp(a_type, b_type) == 0) return 1;
      }
      return 0;
    }

    /* static functions */
    if ((Cmp(a_storage, k_static) == 0) 
	|| (Cmp(b_storage, k_static) == 0)) {
      if (Cmp(a_storage, b_storage) != 0) return 0;
    }

    /* friend methods */
    
    if (!a_inclass || (Cmp(a_storage,k_friend) == 0)) {
      /* check declaration */

      String *a_decl = (Getattr(a,k_decl));
      String *b_decl = (Getattr(b,k_decl));
      if (Cmp(a_decl, b_decl) == 0) {
	/* check return type */
	String *a_type = (Getattr(a,k_type));
	String *b_type = (Getattr(b,k_type));
	if (Cmp(a_type, b_type) == 0) {
	  /* check parameters */
	  Parm *ap = (Getattr(a,k_parms));
	  Parm *bp = (Getattr(b,k_parms));
	  while (ap && bp) {
	    SwigType *at = Getattr(ap,k_type);
	    SwigType *bt = Getattr(bp,k_type);
	    if (Cmp(at, bt) != 0) return 0;
	    ap = nextSibling(ap);
	    bp = nextSibling(bp);
	  }
	  if (ap || bp) {
	    return 0;
	  } else {
            Node *a_template = Getattr(a,k_template);
            Node *b_template = Getattr(b,k_template);
            /* Not equivalent if one is a template instantiation (via %template) and the other is a non-templated function */
            if ((a_template && !b_template) || (!a_template && b_template)) return 0;
          }
	  return 1;
	}
      }
    }
  } else {
    /* %constant case */  
    String *a_storage = Getattr(a,k_storage);
    String *b_storage = Getattr(b,k_storage);
    if ((Cmp(a_storage, "%constant") == 0) 
	|| (Cmp(b_storage, "%constant") == 0)) {
      if (Cmp(a_storage, b_storage) == 0) {
	String *a_type = (Getattr(a,k_type));
	String *b_type = (Getattr(b,k_type));
	if ((Cmp(a_type, b_type) == 0)
	    && (Cmp(Getattr(a,k_value), Getattr(b,k_value)) == 0))
	  return 1;
      }
      return 0;
    }
  }
  return 0;
}

int Swig_need_redefined_warn(Node* a, Node* b, int InClass)
{
  String *a_symname = Getattr(a,k_symname);
  String *b_symname = Getattr(b,k_symname);
  /* always send a warning if a 'rename' is involved */
  if ((a_symname && Cmp(a_symname,Getattr(a,k_name)))
      || (b_symname && Cmp(b_symname,Getattr(b,k_name))))
    return 1;

  return !nodes_are_equivalent(a, b, InClass);
}


/* -----------------------------------------------------------------------------
 * int Swig_need_protected(Node* n)
 *
 * Detects when we need to fully register the protected member.
 * 
 * ----------------------------------------------------------------------------- */

int Swig_need_protected(Node* n)
{
  /* First, 'n' looks like a function */
  /* if (!Swig_director_mode()) return 0; */
  String *nodetype = Getattr(n,k_nodetype);
  if ((Equal(nodetype,k_cdecl)) && SwigType_isfunction(Getattr(n,k_decl))) {
    String *storage = Getattr(n,k_storage);
    /* and the function is declared like virtual, or it has no
       storage. This eliminates typedef, static and so on. */ 
    return !storage || Equal(storage,k_virtual);
  } else if (Equal(nodetype,k_constructor) || Equal(nodetype,k_destructor)) {
    return 1;
  }

  return 0;
}


static void Swig_name_object_attach_keys(const char *keys[], Hash *nameobj) 
{
  Node *kw = nextSibling(nameobj);
  List *matchlist = 0;
  while (kw) {
    Node *next = nextSibling(kw);
    String *kname = Getattr(kw,k_name);
    char *ckey = kname ? Char(kname) :  0;
    if (ckey) {
      const char **rkey;
      if (strncmp(ckey,"match",5) == 0) {
	if (!matchlist) matchlist = NewList();
	Append(matchlist,kw);
	deleteNode(kw);
      } else {
	for (rkey = keys; *rkey != 0; ++rkey) {
	  if (strcmp(ckey,*rkey) == 0) {
	    Setattr(nameobj, *rkey, Getattr(kw,k_value));
	    deleteNode(kw);
	  }
	}
      }
    }
    kw = next;
  }
  if (matchlist) {
    Setattr(nameobj,k_matchlist,matchlist);
    Delete(matchlist);
  }
  
}



void 
Swig_name_nameobj_add(Hash *name_hash, List *name_list,
		      String *prefix, const char *name, SwigType *decl, Hash *nameobj) {
  String *nname = 0;

  if (name && Len(name)) {
    String *target_fmt = Getattr(nameobj,k_targetfmt);
    nname = prefix ? NewStringf("%s::%s",prefix, name) : NewString(name);
    if (target_fmt) {
      String *tmp = NewStringf(Getattr(nameobj,k_targetfmt), nname);
      Delete(nname);
      nname = tmp;
    }
  }
  
  if (!nname || !Len(nname) || Getattr(nameobj,k_sourcefmt) || Getattr(nameobj,k_matchlist)) {
    if (decl) Setattr(nameobj,k_decl, decl);
    if (nname && Len(nname)) Setattr(nameobj,k_targetname, nname);
    Append(name_list, nameobj);
  } else {  
    Swig_name_object_set(name_hash,nname,decl,nameobj);
  }
  Delete(nname);
}


DOH *Swig_node_getattr(Node *n, const char *cattr)
{
  const char *rattr = strstr(cattr,"$");
  if (rattr) {
    String *nattr = NewStringWithSize(cattr, rattr-cattr);
    Node *rn = Getattr(n,nattr);
    if (rn) {
      DOH *res = Swig_node_getattr(rn, rattr + 1);
      Delete(nattr);
      return res;
    } else {
      return 0;
    }  
  } else {
    DOH *res = Getattr(n,cattr);
    return res;
  }
}

int Swig_name_match_nameobj(Hash *rn, Node *n) {
  int match = 1;
  List *matchlist = HashGetAttr(rn,k_matchlist);
  if (matchlist) {
    int i;
    int ilen = Len(matchlist);
    for (i = 0; match && (i < ilen); ++i) {
      Node *kw = Getitem(matchlist,i);
      String *key = Getattr(kw,k_name);
      char *ckey = key ? Char(key) : 0;
      if (ckey && (strncmp(ckey,"match",5) == 0)) {
	match = 0;
	if (n) {
	  const char *csep = strstr(ckey,"$");
	  String *nval = csep ? Swig_node_getattr(n,csep + 1) : Getattr(n,k_nodetype);
	  String *kwval = Getattr(kw,k_value);
	  if (nval && Equal(nval, kwval)) {
	    match = 1;
	  }
	}
      }
    }
  }
  return match;
}

Hash *Swig_name_nameobj_lget(List *namelist, Node *n, String *prefix, String *name, String *decl) {
  Hash *res = 0;
  if (namelist) {
    int len = Len(namelist);
    int i;
    int match = 0;
    for (i = 0; !match && (i < len); i++) {
      Hash *rn = Getitem(namelist,i);
      String *rdecl = HashGetAttr(rn,k_decl);
      if (rdecl && (!decl || !Equal(rdecl,decl))) {
	continue;
      } else if (Swig_name_match_nameobj(rn, n)) {
	String *tname = HashGetAttr(rn,k_targetname);
	String *sfmt = HashGetAttr(rn,k_sourcefmt);
	String *sname = 0;
	int fullname = GetFlag(namelist,k_fullname);
	if (sfmt) {
	  if (fullname && prefix) {
	    sname = NewStringf(sfmt, prefix, name);
	  } else {
	    sname = NewStringf(sfmt, name);
	  }
	} else {
	  if (fullname && prefix) {
	    sname = NewStringf("%s::%s", prefix, name);
	  } else {
	    sname = NewString(name);
	  }
	}
	match = !tname || Equal(sname,tname);
	Delete(sname);
      }
      if (match) {
	res = rn;
	break;
      }
    }
  }
  return res;
}


void Swig_name_namewarn_add(String *prefix, String *name, SwigType *decl, Hash *namewrn) {
  const char *namewrn_keys[] = {"rename", "error", "fullname", "sourcefmt", "targetfmt", 0};
  Swig_name_object_attach_keys(namewrn_keys, namewrn);
  Swig_name_nameobj_add(Swig_name_namewarn_hash(), Swig_name_namewarn_list(),
			prefix, name, decl, namewrn);
}

/* Return the node name when it requires to emit a name warning */
Hash *Swig_name_namewarn_get(Node *n, String *prefix, String *name,SwigType *decl) {
  if (!namewarn_hash && !namewarn_list) return 0;
  if (n) {
    /* Return in the obvious cases */
    if (!name || !Swig_need_name_warning(n)) {
      return 0;
    } else {
      String *access = Getattr(n,k_access);	
      int is_public = !access || Equal(access,k_public);
      if (!is_public && !Swig_need_protected(n)) {
	return 0;
      }
    }
  }
  if (name) {
    /* Check to see if the name is in the hash */
    Hash *wrn = Swig_name_object_get(Swig_name_namewarn_hash(),prefix,name,decl);
    if (wrn && !Swig_name_match_nameobj(wrn, n)) wrn = 0;
    if (!wrn) {
      wrn = Swig_name_nameobj_lget(Swig_name_namewarn_list(), n, prefix, name, decl);
    }
    if (wrn && HashGetAttr(wrn,k_error)) {
      if (n) {
	Swig_error(Getfile(n), Getline(n), "%s\n", HashGetAttr(wrn,k_name));
      } else {
	Swig_error(cparse_file, cparse_line,"%s\n", HashGetAttr(wrn,k_name));
      }
    } 
    return wrn;
  } else {
    return 0;
  }
}

String *Swig_name_warning(Node *n, String *prefix, String *name,SwigType *decl) {
  Hash *wrn = Swig_name_namewarn_get(n, prefix, name,decl);
  return (name && wrn) ? HashGetAttr(wrn,k_name) : 0;
}



static void single_rename_add(String *prefix, String *name, SwigType *decl, Hash *newname) {
  Swig_name_nameobj_add(Swig_name_rename_hash(), Swig_name_rename_list(),
			prefix, name, decl, newname);
}

/* Add a new rename. Works much like new_feature including default argument handling. */
void Swig_name_rename_add(String *prefix, String *name, SwigType *decl, Hash *newname, 
			  ParmList *declaratorparms) {
  
  ParmList *declparms = declaratorparms;
  
  const char *rename_keys[] = {"fullname", "sourcefmt", "targetfmt", 0};
  Swig_name_object_attach_keys(rename_keys, newname);
  
  /* Add the name */
  single_rename_add(prefix, name, decl, newname);
  
  /* Add extra names if there are default parameters in the parameter list */
  if (decl) {
    int constqualifier = SwigType_isconst(decl);
    while (declparms) {
      if (ParmList_has_defaultargs(declparms)) {
	
        /* Create a parameter list for the new rename by copying all
           but the last (defaulted) parameter */
        ParmList* newparms = ParmList_copy_all_except_last_parm(declparms);
	
        /* Create new declaration - with the last parameter removed */
        SwigType *newdecl = Copy(decl);
        Delete(SwigType_pop_function(newdecl)); /* remove the old parameter list from newdecl */
        SwigType_add_function(newdecl,newparms);
        if (constqualifier)
          SwigType_add_qualifier(newdecl,"const");
	
        single_rename_add(prefix, name, newdecl, newname);
        declparms = newparms;
        Delete(newdecl);
      } else {
        declparms = 0;
      }
    }
  }
}


/* Create a name applying rename/namewarn if needed */
static String *apply_rename(String *newname, int fullname, String *prefix, String *name)
{
  String *result = 0;
  if (newname && Len(newname)) {
    if (Strcmp(newname,"$ignore") == 0) {
      result = Copy(newname);
    } else {
      char *cnewname = Char(newname);
      if (cnewname) {
	int destructor = name && (*(Char(name)) == '~');
	String *fmt = newname;
	String *tmp = 0;
	if (destructor && (*cnewname != '~')) {
	  fmt = tmp = NewStringf("~%s", newname);
	}
	/* use name as a fmt, but avoid C++ "%" and "%=" operators */
	if (Len(newname) > 1 && strstr(cnewname,"%") && !(strcmp(cnewname,"%=") == 0)) { 
	  if (fullname && prefix) {
	    result = NewStringf(fmt,prefix,name);
	  } else {
	    result = NewStringf(fmt,name);
	  }
	} else {
	  result = Copy(newname);
	}
	if (tmp) Delete(tmp);
      }
    }
  }
  return result;
}

String *
Swig_name_make(Node *n, String *prefix, String *name, SwigType *decl, String *oldname) {
  String *nname = 0;
  String *result = 0;
  Hash *wrn = 0;
  if (namewarn_hash || namewarn_list) {
    Hash *rn =  Swig_name_object_get(Swig_name_rename_hash(), prefix, name, decl);
    if (!rn || !Swig_name_match_nameobj(rn,n)) {
      rn = Swig_name_nameobj_lget(Swig_name_rename_list(), n, prefix, name, decl);
    }
    if (rn) {
      String *newname = HashGetAttr(rn,k_name);
      int fullname = GetFlag(rn,k_fullname);
      result = apply_rename(newname, fullname, prefix, name);
    }
    
    nname = result ? result : name;
    wrn = Swig_name_namewarn_get(n, prefix, nname, decl);
    if (wrn) {
      String *rename = HashGetAttr(wrn,k_rename);
      if (rename) {
	String *msg = HashGetAttr(wrn,k_name);
	int fullname = GetFlag(wrn,k_fullname);
	if (result) Delete(result);
	result = apply_rename(rename, fullname, prefix, name);
	if ((msg) && (Len(msg))) {
	  if (!Getmeta(nname,"already_warned")) {
	    if (n)  {
	      SWIG_WARN_NODE_BEGIN(n);
	      Swig_warning(0,Getfile(n), Getline(n), "%s\n", msg);
	      SWIG_WARN_NODE_END(n);
	    } else {
	      Swig_warning(0,Getfile(name),Getline(name), "%s\n", msg);
	    }
	    Setmeta(nname,"already_warned","1");
	  }
	}
      }
    }
  }
  if (!result || !Len(result)) {
    if (result) Delete(result);
    result = oldname ? Copy(oldname): Copy(name);
  }
  return result;
}





void Swig_name_inherit(String *base, String *derived) {
  /*  Printf(stdout,"base = '%s', derived = '%s'\n", base, derived); */
  Swig_name_object_inherit(Swig_name_rename_hash(),base,derived);
  Swig_name_object_inherit(Swig_name_namewarn_hash(),base,derived);
  Swig_name_object_inherit(Swig_cparse_features(),base,derived);
}
