/* ----------------------------------------------------------------------------
 * perl5.cxx
 *
 *     Generate Perl5 wrappers
 *
 * Author(s) : David Beazley (beazley@cs.uchicago.edu)
 *             Loic Dachary (loic@ceic.com)
 *             David Fletcher
 *             Gary Holt
 *             Jason Stewart (jason@openinformatics.com)
 *
 * Copyright (C) 1999-2000.  The University of Chicago
 * See the file LICENSE for information on usage and redistribution.
 * ------------------------------------------------------------------------- */

char cvsroot_perl5_cxx[] = "$Header$";

#include "swigmod.h"

#ifndef MACSWIG
#include "swigconfig.h"
#endif

static const char *usage = (char*)"\
Perl5 Options (available with -perl5)\n\
     -ldflags        - Print runtime libraries to link with\n\
     -static         - Omit code related to dynamic loading.\n\
     -nopm           - Do not generate the .pm file.\n\
     -proxy          - Create proxy classes.\n\
     -const          - Wrap constants as constants and not variables (implies -shadow).\n\
     -compat         - Compatibility mode.\n\n";

static int     compat = 0;

static int     no_pmfile = 0;

static int     export_all = 0;

/*
 * pmfile
 *   set by the -pm flag, overrides the name of the .pm file
 */
static String *pmfile = 0;

/*
 * module
 *   set by the %module directive, e.g. "Xerces". It will determine
 *   the name of the .pm file, and the dynamic library.
 */
static String       *module = 0;

/*
 * fullmodule
 *   the fully namespace qualified name of the module, e.g. "XML::Xerces"
 *   it will be used to set the package namespace in the .pm file, as
 *   well as the name of the initialization methods in the glue library
 */
static String       *fullmodule = 0;
/*
 * cmodule
 *   the namespace of the internal glue code, set to the value of
 *   module with a 'c' appended
 */
static String       *cmodule = 0; 

static String       *command_tab = 0;
static String       *constant_tab = 0;
static String       *variable_tab = 0;

static File        *f_runtime = 0;
static File        *f_header = 0;
static File        *f_wrappers = 0;
static File        *f_init = 0;
static File        *f_pm = 0;
static String       *pm;                   /* Package initialization code */
static String       *magic;                /* Magic variable wrappers     */

static int          is_static = 0;

/* The following variables are used to manage Perl5 classes */

static  int        blessed = 0;                /* Enable object oriented features */
static  int        do_constants = 0;           /* Constant wrapping */
static  List       *classlist = 0;             /* List of classes */
static  int        have_constructor = 0;
static  int        have_destructor= 0;
static  int        have_data_members = 0;
static  String    *class_name = 0;            /* Name of the class (what Perl thinks it is) */
static  String    *real_classname = 0;        /* Real name of C/C++ class */
static  String   *fullclassname = 0;

static  String   *pcode = 0;                 /* Perl code associated with each class */
static  String   *blessedmembers = 0;        /* Member data associated with each class */
static  int      member_func = 0;            /* Set to 1 when wrapping a member function */
static  String   *func_stubs = 0;            /* Function stubs */
static  String   *const_stubs = 0;           /* Constant stubs */
static  int       num_consts = 0;            /* Number of constants */
static  String   *var_stubs = 0;             /* Variable stubs */
static  String   *exported = 0;              /* Exported symbols */
static  String   *pragma_include = 0;
static  Hash     *operators = 0;
static  int       have_operators = 0;

class PERL5 : public Language {
public:

  /* Test to see if a type corresponds to something wrapped with a shadow class */
  Node *is_shadow(SwigType *t) {
    Node *n;
    n = classLookup(t);
    /*  Printf(stdout,"'%s' --> '%x'\n", t, n); */
    if (n) {
      if (!Getattr(n,"perl5:proxy")) {
	setclassname(n);
      }
      return Getattr(n,"perl5:proxy");
    }
    return 0;
  }

  /* ------------------------------------------------------------
   * main()
   * ------------------------------------------------------------ */

  virtual void main(int argc, char *argv[]) {
    int i = 1;

    SWIG_library_directory("perl5");
    for (i = 1; i < argc; i++) {
      if (argv[i]) {
	if(strcmp(argv[i],"-package") == 0) {
	  Printf(stderr,"*** -package is no longer supported\n*** use the directive '%module A::B::C' in your interface file instead\n*** see the Perl section in the manual for details.\n");
	  SWIG_exit(EXIT_FAILURE);
	} else if(strcmp(argv[i],"-interface") == 0) {
	  Printf(stderr,"*** -interface is no longer supported\n*** use the directive '%module A::B::C' in your interface file instead\n*** see the Perl section in the manual for details.\n");
	  SWIG_exit(EXIT_FAILURE);
	} else if (strcmp(argv[i],"-exportall") == 0) {
	  export_all = 1;
	  Swig_mark_arg(i);
	} else if (strcmp(argv[i],"-static") == 0) {
	  is_static = 1;
	  Swig_mark_arg(i);
	} else if ((strcmp(argv[i],"-shadow") == 0) || ((strcmp(argv[i],"-proxy") == 0))) {
	  blessed = 1;
	  Swig_mark_arg(i);
	} else if (strcmp(argv[i],"-const") == 0) {
	  do_constants = 1;
	  blessed = 1;
	  Swig_mark_arg(i);
	} else if (strcmp(argv[i],"-nopm") == 0) {
	    no_pmfile = 1;
	    Swig_mark_arg(i);
	} else if (strcmp(argv[i],"-pm") == 0) {
	    Swig_mark_arg(i);
	    i++;
	    pmfile = NewString(argv[i]);
	    Swig_mark_arg(i);
	} else if (strcmp(argv[i],"-compat") == 0) {
	  compat = 1;
	  Swig_mark_arg(i);
	} else if (strcmp(argv[i],"-help") == 0) {
	  fputs(usage,stderr);
	} else if (strcmp (argv[i], "-ldflags") == 0) {
	  printf("%s\n", SWIG_PERL_RUNTIME);
	  SWIG_exit (EXIT_SUCCESS);
	}
      }
    }

    Preprocessor_define("SWIGPERL 1", 0);
    Preprocessor_define("SWIGPERL5 1", 0);
    SWIG_typemap_lang("perl5");
    SWIG_config_file("perl5.swg");
    allow_overloading();
  }

  /* ------------------------------------------------------------
   * top()
   * ------------------------------------------------------------ */

  virtual int top(Node *n) {

    /* Initialize all of the output files */
    String *outfile = Getattr(n,"outfile");

    f_runtime = NewFile(outfile,"w");
    if (!f_runtime) {
      Printf(stderr,"*** Can't open '%s'\n", outfile);
      SWIG_exit(EXIT_FAILURE);
    }
    f_init = NewString("");
    f_header = NewString("");
    f_wrappers = NewString("");

  /* Register file targets with the SWIG file handler */
    Swig_register_filebyname("header",f_header);
    Swig_register_filebyname("wrapper",f_wrappers);
    Swig_register_filebyname("runtime",f_runtime);
    Swig_register_filebyname("init",f_init);

    classlist = NewList();

    pm             = NewString("");
    func_stubs     = NewString("");
    var_stubs      = NewString("");
    const_stubs    = NewString("");
    exported       = NewString("");
    magic          = NewString("");
    pragma_include = NewString("");

    command_tab    = NewString("static swig_command_info swig_commands[] = {\n");
    constant_tab   = NewString("static swig_constant_info swig_constants[] = {\n");
    variable_tab   = NewString("static swig_variable_info swig_variables[] = {\n");

    Swig_banner(f_runtime);
  
    if (NoInclude) {
      Printf(f_runtime,"#define SWIG_NOINCLUDE\n");
    }

    module = Copy(Getattr(n,"name"));

  /* If we're in blessed mode, change the package name to "packagec" */

    if (blessed) {
      cmodule = NewStringf("%sc",module);
    } else {
      cmodule = NewString(module);
    }
    fullmodule = NewString(module);

  /* Create a .pm file
   * Need to strip off any prefixes that might be found in
   * the module name */

    if (no_pmfile) {
      f_pm = NewString(0);
    } else {
      if (pmfile == NULL) {
	char *m = Char(module) + Len(module);
	while (m != Char(module)) {
	  if (*m == ':') {
	    m++;
	    break;
	  }
	  m--;
	}
	pmfile = NewStringf("%s.pm", m);
      }
      String *filen = NewStringf("%s%s", Swig_file_dirname(outfile),pmfile);
      if ((f_pm = NewFile(filen,"w")) == 0) {
	Printf(stderr,"Unable to open %s\n", filen);
	SWIG_exit (EXIT_FAILURE);
      }
      Swig_register_filebyname("pm",f_pm);
    }
    {
      String *tmp = NewString(fullmodule);
      Replaceall(tmp,":","_");
      Printf(f_header,"#define SWIG_init    boot_%s\n\n", tmp);
      Printf(f_header,"#define SWIG_name   \"%s::boot_%s\"\n", cmodule, tmp);
      Printf(f_header,"#define SWIG_prefix \"%s::\"\n", cmodule);
      Delete(tmp);
    }

    Printf(f_pm,"# This file was automatically generated by SWIG\n");
    Printf(f_pm,"package %s;\n",fullmodule);

    Printf(f_pm,"require Exporter;\n");
    if (!is_static) {
      Printf(f_pm,"require DynaLoader;\n");
      Printf(f_pm,"@ISA = qw(Exporter DynaLoader);\n");
    } else {
      Printf(f_pm,"@ISA = qw(Exporter);\n");
    }

    /* Start creating magic code */

    Printv(magic,
	   "#ifdef PERL_OBJECT\n",
	   "#define MAGIC_CLASS _wrap_", module, "_var::\n",
	   "class _wrap_", module, "_var : public CPerlObj {\n",
	   "public:\n",
	   "#else\n",
	   "#define MAGIC_CLASS\n",
	   "#endif\n",
	   "SWIGCLASS_STATIC int swig_magic_readonly(pTHX_ SV *sv, MAGIC *mg) {\n",
	   tab4, "MAGIC_PPERL\n",
	   tab4, "sv = sv; mg = mg;\n",
	   tab4, "croak(\"Value is read-only.\");\n",
	   tab4, "return 0;\n",
	   "}\n",
	   NIL);

    Printf(f_wrappers,"#ifdef __cplusplus\nextern \"C\" {\n#endif\n");

  /* emit wrappers */
    Language::top(n);

    String *base = NewString("");
  
  /* Dump out variable wrappers */

    Printv(magic,
	   "\n\n#ifdef PERL_OBJECT\n",
	   "};\n",
	   "#endif\n",
	   NIL);

    Printf(f_header,"%s\n", magic);

    String *type_table = NewString("");
    SwigType_emit_type_table(f_runtime,type_table);

  /* Patch the type table to reflect the names used by shadow classes */
    if (blessed) {
      Node     *cls;
      for (cls = Firstitem(classlist); cls; cls = Nextitem(classlist)) {
	if (Getattr(cls,"perl5:proxy")) {
	  SwigType *type = Copy(Getattr(cls,"classtype"));
	  SwigType_add_pointer(type);
	  String *mangle = NewStringf("\"%s\"", SwigType_manglestr(type));
	  String *rep = NewStringf("\"%s\"", Getattr(cls,"perl5:proxy"));
	  Replaceall(type_table,mangle,rep);
	  Delete(mangle);
	  Delete(rep);
	  Delete(type);
	}
      }
    }

    Printf(f_wrappers,"%s",type_table);
    Delete(type_table);

    Printf(constant_tab,"{0}\n};\n");
    Printv(f_wrappers,constant_tab,NIL);

    Printf(f_wrappers,"#ifdef __cplusplus\n}\n#endif\n");

    Printf(f_init,"\t ST(0) = &PL_sv_yes;\n");
    Printf(f_init,"\t XSRETURN(1);\n");
    Printf(f_init,"}\n");

    /* Finish off tables */
    Printf(variable_tab, "{0}\n};\n");
    Printv(f_wrappers,variable_tab,NIL);

    Printf(command_tab,"{0,0}\n};\n");
    Printv(f_wrappers,command_tab,NIL);


    Printf(f_pm,"package %s;\n", cmodule);

    if (!is_static) {
      Printf(f_pm,"bootstrap %s;\n", fullmodule);
    } else {
      String *tmp = NewString(fullmodule);
      Replaceall(tmp,":","_");
      Printf(f_pm,"boot_%s();\n", tmp);
      Delete(tmp);
    }
    Printf(f_pm,"%s",pragma_include);
    Printf(f_pm,"package %s;\n", fullmodule);
    Printf(f_pm,"@EXPORT = qw( %s);\n",exported);

    if (blessed) {

      Printv(base,
	     "\n# ---------- BASE METHODS -------------\n\n",
	     "package ", fullmodule, ";\n\n",
	     NIL);

      /* Write out the TIE method */

      Printv(base,
	     "sub TIEHASH {\n",
	     tab4, "my ($classname,$obj) = @_;\n",
	     tab4, "return bless $obj, $classname;\n",
	     "}\n\n",
	     NIL);

      /* Output a CLEAR method.   This is just a place-holder, but by providing it we
       * can make declarations such as
       *     %$u = ( x => 2, y=>3, z =>4 );
       *
       * Where x,y,z are the members of some C/C++ object. */

      Printf(base,"sub CLEAR { }\n\n");

      /* Output default firstkey/nextkey methods */

      Printf(base, "sub FIRSTKEY { }\n\n");
      Printf(base, "sub NEXTKEY { }\n\n");

      /* Output a 'this' method */

      Printv(base,
	     "sub this {\n",
	     tab4, "my $ptr = shift;\n",
	     tab4, "return tied(%$ptr);\n",
	     "}\n\n",
	     NIL);

      Printf(f_pm,"%s",base);

      /* Emit function stubs for stand-alone functions */

      Printf(f_pm,"\n# ------- FUNCTION WRAPPERS --------\n\n");
      Printf(f_pm,"package %s;\n\n",fullmodule);
      Printf(f_pm,"%s",func_stubs);

      /* Emit package code for different classes */
      Printf(f_pm,"%s",pm);

      if (num_consts > 0) {
	/* Emit constant stubs */
	Printf(f_pm,"\n# ------- CONSTANT STUBS -------\n\n");
	Printf(f_pm,"package %s;\n\n",fullmodule);
	Printf(f_pm,"%s",const_stubs);
      }

      /* Emit variable stubs */

      Printf(f_pm,"\n# ------- VARIABLE STUBS --------\n\n");
      Printf(f_pm,"package %s;\n\n",fullmodule);
      Printf(f_pm,"%s",var_stubs);
    }

    Printf(f_pm,"1;\n");
    Close(f_pm);
    Delete(f_pm);
    Delete(base);

    /* Close all of the files */
    Dump(f_header,f_runtime);
    Dump(f_wrappers,f_runtime);
    Wrapper_pretty_print(f_init,f_runtime);
    Delete(f_header);
    Delete(f_wrappers);
    Delete(f_init);
    Close(f_runtime);
    Delete(f_runtime);
    return SWIG_OK;
  }

  /* ------------------------------------------------------------
   * importDirective(Node *n)
   * ------------------------------------------------------------ */

  virtual int importDirective(Node *n) {
    if (blessed) {
      String *modname = Getattr(n,"module");
      if (modname) {
	Printf(f_pm,"require %s;\n", modname);
      }
    }
    return Language::importDirective(n);
  }

  /* ------------------------------------------------------------
   * functionWrapper()
   * ------------------------------------------------------------ */

  virtual int functionWrapper(Node *n)  {
    String *name = Getattr(n,"name");
    String *iname = Getattr(n,"sym:name");
    SwigType *d = Getattr(n,"type");
    ParmList *l = Getattr(n,"parms");
    String   *overname = 0;

    Parm *p;
    int   i;
    Wrapper *f;
    char  source[256],target[256],temp[256];
    String  *tm;
    String *cleanup, *outarg;
    int    num_saved = 0;
    int    num_arguments, num_required;
    int    varargs = 0;

    if (Getattr(n,"sym:overloaded")) {
      overname = Getattr(n,"sym:overname");
    } else {
      if (!addSymbol(iname,n)) return SWIG_ERROR;
    }

    f       = NewWrapper();
    cleanup = NewString("");
    outarg  = NewString("");

    String *wname = Swig_name_wrapper(iname);
    if (overname) {
      Append(wname,overname);
    }
    Setattr(n,"wrap:name",wname);
    Printv(f->def, "XS(", wname, ") {\n", NIL);
    Printv(f->def, "char _swigmsg[SWIG_MAX_ERRMSG] = \"\";\n", NIL);
    Printv(f->def, "const char *_swigerr = _swigmsg;\n","{\n",NIL);

    emit_args(d, l, f);
    emit_attach_parmmaps(l,f);
    Setattr(n,"wrap:parms",l);

    num_arguments = emit_num_arguments(l);
    num_required  = emit_num_required(l);
    varargs       = emit_isvarargs(l);

    Wrapper_add_local(f,"argvi","int argvi = 0");

    /* Check the number of arguments */
    if (!varargs) {
      Printf(f->code,"    if ((items < %d) || (items > %d)) {\n", num_required, num_arguments);
    } else {
      Printf(f->code,"    if (items < %d) {\n", num_required);
    }
    Printf(f->code,"        SWIG_croak(\"Usage: %s\");\n", usage_func(Char(iname),d,l));
    Printf(f->code,"}\n");

    /* Write code to extract parameters. */
    i = 0;
    for (i = 0, p = l; i < num_arguments; i++) {
    
      /* Skip ignored arguments */
    
      while (checkAttribute(p,"tmap:in:numinputs","0")) {
	p = Getattr(p,"tmap:in:next");
      }

      SwigType *pt = Getattr(p,"type");
      String   *ln = Getattr(p,"lname");

      /* Produce string representation of source and target arguments */
      sprintf(source,"ST(%d)",i);
      sprintf(target,"%s", Char(ln));

      if (i >= num_required) {
	Printf(f->code,"    if (items > %d) {\n", i);
      }
      if ((tm = Getattr(p,"tmap:in"))) {
	Replaceall(tm,"$target",target);
	Replaceall(tm,"$source",source);
	Replaceall(tm,"$input", source);
	Setattr(p,"emit:input",source);       /* Save input location */
	Printf(f->code,"%s\n",tm);
	p = Getattr(p,"tmap:in:next");
      } else {
	Swig_warning(WARN_TYPEMAP_IN_UNDEF, input_file, line_number,
		     "Unable to use type %s as a function argument.\n",SwigType_str(pt,0));
	p = nextSibling(p);
      }
      if (i >= num_required) {
	Printf(f->code,"    }\n");
      }
    }

    if (varargs) {
      if (p && (tm = Getattr(p,"tmap:in"))) {
	sprintf(source,"ST(%d)",i);
	Replaceall(tm,"$input", source);
	Setattr(p,"emit:input", source);
	Printf(f->code,"if (items >= %d) {\n", i);
	Printv(f->code, tm, "\n", NIL);
	Printf(f->code,"}\n");
      }
    }

    /* Insert constraint checking code */
    for (p = l; p;) {
      if ((tm = Getattr(p,"tmap:check"))) {
	Replaceall(tm,"$target",Getattr(p,"lname"));
	Printv(f->code,tm,"\n",NIL);
	p = Getattr(p,"tmap:check:next");
      } else {
	p = nextSibling(p);
      }
    }
  
    /* Insert cleanup code */
    for (i = 0, p = l; p; i++) {
      if ((tm = Getattr(p,"tmap:freearg"))) {
	Replaceall(tm,"$source",Getattr(p,"lname"));
	Replaceall(tm,"$arg",Getattr(p,"emit:input"));
	Replaceall(tm,"$input",Getattr(p,"emit:input"));
	Printv(cleanup,tm,"\n",NIL);
	p = Getattr(p,"tmap:freearg:next");
      } else {
	p = nextSibling(p);
      }
    }

    /* Insert argument output code */
    num_saved = 0;
    for (i=0,p = l; p;i++) {
      if ((tm = Getattr(p,"tmap:argout"))) {
	Replaceall(tm,"$source",Getattr(p,"lname"));
	Replaceall(tm,"$target","ST(argvi)");
	Replaceall(tm,"$result","ST(argvi)");
	String *in = Getattr(p,"emit:input");
	if (in) {
	  sprintf(temp,"_saved[%d]", num_saved);
	  Replaceall(tm,"$arg",temp);
	  Replaceall(tm,"$input",temp);
	  Printf(f->code,"_saved[%d] = %s;\n", num_saved, in);
	  num_saved++;
	}
	Printv(outarg,tm,"\n",NIL);
	p = Getattr(p,"tmap:argout:next");
      } else {
	p = nextSibling(p);
      }
    }

    /* If there were any saved arguments, emit a local variable for them */
    if (num_saved) {
      sprintf(temp,"_saved[%d]",num_saved);
      Wrapper_add_localv(f,"_saved","SV *",temp,NIL);
    }

    /* Now write code to make the function call */

    emit_action(n,f);

    if ((tm = Swig_typemap_lookup_new("out",n,"result",0))) {
      Replaceall(tm,"$source","result");
      Replaceall(tm,"$target","ST(argvi)");
      Replaceall(tm,"$result", "ST(argvi)");
      Printf(f->code, "%s\n", tm);
    } else {
      Swig_warning(WARN_TYPEMAP_OUT_UNDEF, input_file, line_number,
		   "Unable to use return type %s in function %s.\n", SwigType_str(d,0), name);
    }

    /* If there were any output args, take care of them. */

    Printv(f->code,outarg,NIL);

    /* If there was any cleanup, do that. */

    Printv(f->code,cleanup,NIL);

    if (Getattr(n,"feature:new"))  {
      if ((tm = Swig_typemap_lookup_new("newfree",n,"result",0))) {
	Replaceall(tm,"$source","result");
	Printf(f->code,"%s\n",tm);
      }
    }

    if ((tm = Swig_typemap_lookup_new("ret",n,"result", 0))) {
      Replaceall(tm,"$source","result");
      Printf(f->code,"%s\n", tm);
    }

    Printf(f->code,"    XSRETURN(argvi);\n");
    Printf(f->code,"fail:\n");
    Printv(f->code,cleanup,NIL);
    Printv(f->code,"(void) _swigerr;\n", NIL);
    Printv(f->code,"}\n",NIL);
    Printv(f->code,"croak(_swigerr);\n", NIL);
    Printv(f->code,"}\n",NIL);

  /* Add the dXSARGS last */

    Wrapper_add_local(f,"dXSARGS","dXSARGS");

    /* Substitute the cleanup code */
    Replaceall(f->code,"$cleanup",cleanup);
    Replaceall(f->code,"$symname",iname);

  /* Dump the wrapper function */

    Wrapper_print(f,f_wrappers);

    /* Now register the function */

    if (!Getattr(n,"sym:overloaded")) {
      Printf(command_tab,"{\"%s::%s\", %s},\n", cmodule, iname, wname);
    } else if (!Getattr(n,"sym:nextSibling")) {
      /* Generate overloaded dispatch function */
      int maxargs, ii;
      String *dispatch = Swig_overload_dispatch(n,"(*PL_markstack_ptr++);SWIG_CALLXS(%s); return;",&maxargs);
	
      /* Generate a dispatch wrapper for all overloaded functions */

      Wrapper *df       = NewWrapper();
      String  *dname    = Swig_name_wrapper(iname);

      Printv(df->def,	
	     "XS(", dname, ") {\n", NIL);
    
      Wrapper_add_local(df,"dXSARGS", "dXSARGS");
      Replaceid(dispatch,"argc","items");
      for (ii = 0; ii < maxargs; ii++) {
	char pat[128];
	char rep[128];
	sprintf(pat,"argv[%d]",ii);
	sprintf(rep,"ST(%d)",ii);
	Replaceall(dispatch,pat,rep);
      }
      Printv(df->code,dispatch,"\n",NIL);
      Printf(df->code,"croak(\"No matching function for overloaded '%s'\");\n", iname);
      Printf(df->code,"XSRETURN(0);\n");
      Printv(df->code,"}\n",NIL);
      Wrapper_print(df,f_wrappers);
      Printf(command_tab,"{\"%s::%s\", %s},\n", cmodule, iname, dname);
      DelWrapper(df);
      Delete(dispatch);
      Delete(dname);
    }
    if (!Getattr(n,"sym:nextSibling")) {
      if (export_all) {
	Printf(exported,"%s ", iname);
      }
      
      /* --------------------------------------------------------------------
       * Create a stub for this function, provided it's not a member function
       * -------------------------------------------------------------------- */
      
      if ((blessed) && (!member_func)) {
	int    need_stub = 0;
	String *func = NewString("");
	
	/* We'll make a stub since we may need it anyways */
	
	Printv(func, "sub ", iname, " {\n",
	       tab4, "my @args = @_;\n",
	       NIL);
	
	Printv(func, tab4, "my $result = ", cmodule, "::", iname, "(@args);\n", NIL);
	
	/* Now check to see what kind of return result was found.
	 * If this function is returning a result by 'value', SWIG did an
	 * implicit malloc/new.   We'll mark the object like it was created
	 * in Perl so we can garbage collect it. */
	
	if (is_shadow(d)) {
	  Printv(func, tab4, "return undef if (!defined($result));\n", NIL);
	  
	  /* If we're returning an object by value, put it's reference
	     into our local hash table */
	  
	  if ((!SwigType_ispointer(d) && !SwigType_isreference(d)) || Getattr(n,"feature:new")) {
	    Printv(func, tab4, "$", is_shadow(d), "::OWNER{$result} = 1;\n", NIL);
	  }
	  
	  /* We're returning a Perl "object" of some kind.  Turn it into a tied hash */
	  Printv(func,
		 tab4, "my %resulthash;\n",
		 tab4, "tie %resulthash, ref($result), $result;\n",
		 tab4, "return bless \\%resulthash, ref($result);\n",
		 "}\n",
		 NIL);
	  
	  need_stub = 1;
	} else {
	  /* Hmmm.  This doesn't appear to be anything I know about */
	  Printv(func, tab4, "return $result;\n", "}\n", NIL);
	}
	
	/* Now check if we needed the stub.  If so, emit it, otherwise
	 * Emit code to hack Perl's symbol table instead */
	
	if (need_stub) {
	  Printf(func_stubs,"%s",func);
	} else {
	  Printv(func_stubs,"*", iname, " = *", cmodule, "::", iname, ";\n", NIL);
	}
	Delete(func);
      }
    }
    Delete(cleanup);
    Delete(outarg);
    DelWrapper(f);
    return SWIG_OK;
  }

  /* ------------------------------------------------------------
   * variableWrapper()
   * ------------------------------------------------------------ */

  virtual int variableWrapper(Node *n) {
    String *name  = Getattr(n,"name");
    String *iname = Getattr(n,"sym:name");
    SwigType *t   = Getattr(n,"type");
    Wrapper  *getf, *setf;
    String  *tm;

    String *set_name = NewStringf("_wrap_set_%s", iname);
    String *val_name = NewStringf("_wrap_val_%s", iname);

    if (!addSymbol(iname,n)) return SWIG_ERROR;

    getf = NewWrapper();
    setf = NewWrapper();

    /* Create a Perl function for setting the variable value */

    if (!Getattr(n,"feature:immutable")) {
      Printf(setf->def,"SWIGCLASS_STATIC int %s(pTHX_ SV* sv, MAGIC *mg) {\n", set_name);
      Printv(setf->code,
	     tab4, "MAGIC_PPERL\n",
	     tab4, "mg = mg;\n",
	     NIL);

      /* Check for a few typemaps */
      tm = Swig_typemap_lookup_new("varin",n,name,0);
      if (tm) {
	Replaceall(tm,"$source","sv");
	Replaceall(tm,"$target",name);
	Replaceall(tm,"$input","sv");
	Printf(setf->code,"%s\n", tm);
      } else {
	Swig_warning(WARN_TYPEMAP_VARIN_UNDEF, input_file, line_number, 
		     "Unable to set variable of type %s.\n", SwigType_str(t,0));
	return SWIG_NOWRAP;
      }
      Printf(setf->code,"    return 1;\n}\n");
      Replaceall(setf->code,"$symname",iname);
      Wrapper_print(setf,magic);
    }

    /* Now write a function to evaluate the variable */

    Printf(getf->def,"SWIGCLASS_STATIC int %s(pTHX_ SV *sv, MAGIC *mg) {\n", val_name);
    Printv(getf->code,
	   tab4, "MAGIC_PPERL\n",
	   tab4, "mg = mg;\n",
	   NIL);

    if ((tm = Swig_typemap_lookup_new("varout",n,name,0))) {
      Replaceall(tm,"$target","sv");
      Replaceall(tm,"$result","sv");
      Replaceall(tm,"$source",name);
      Printf(getf->code,"%s\n", tm);
    } else {
      Swig_warning(WARN_TYPEMAP_VAROUT_UNDEF, input_file, line_number,
		   "Unable to read variable of type %s\n", SwigType_str(t,0));
      return SWIG_NOWRAP;
    }
    Printf(getf->code,"    return 1;\n}\n");

    Replaceall(getf->code,"$symname",iname);
    Wrapper_print(getf,magic);

    String *tt = Getattr(n,"tmap:varout:type");
    if (tt) {
      String *tm = NewStringf("&SWIGTYPE%s", SwigType_manglestr(t));
      if (Replaceall(tt,"$1_descriptor", tm)) {
	SwigType_remember(t);
      }
      Delete(tm);
      SwigType *st = Copy(t);
      SwigType_add_pointer(st);
      tm = NewStringf("&SWIGTYPE%s", SwigType_manglestr(st));
      if (Replaceall(tt,"$&1_descriptor", tm)) {
	SwigType_remember(st);
      }
      Delete(tm);
      Delete(st);
    } else {
      tt = (String *) "0";
    }
    /* Now add symbol to the PERL interpreter */
    if (Getattr(n,"feature:immutable")) {
      Printv(variable_tab, tab4, "{ \"", cmodule, "::", iname, "\", MAGIC_CLASS swig_magic_readonly, MAGIC_CLASS ", val_name,",", tt, " },\n",NIL);

    } else {
      Printv(variable_tab, tab4, "{ \"", cmodule, "::", iname, "\", MAGIC_CLASS ", set_name, ", MAGIC_CLASS ", val_name, ",", tt, " },\n",NIL);
    }

    /* If we're blessed, try to figure out what to do with the variable
       1.  If it's a Perl object of some sort, create a tied-hash
           around it.
	   2.  Otherwise, just hack Perl's symbol table */

    if (blessed) {
      if (is_shadow(t)) {
	Printv(var_stubs,
	       "\nmy %__", iname, "_hash;\n",
	       "tie %__", iname, "_hash,\"", is_shadow(t), "\", $",
	       cmodule, "::", iname, ";\n",
	       "$", iname, "= \\%__", iname, "_hash;\n",
	       "bless $", iname, ", ", is_shadow(t), ";\n",
	       NIL);
      } else {
	Printv(var_stubs, "*", iname, " = *", cmodule, "::", iname, ";\n", NIL);
      }
    }
    if (export_all)
      Printf(exported,"$%s ", iname);

    DelWrapper(setf);
    DelWrapper(getf);
    Delete(set_name);
    Delete(val_name);
    return SWIG_OK;
  }

  /* ------------------------------------------------------------
   * constantWrapper()
   * ------------------------------------------------------------ */

  virtual int constantWrapper(Node *n) {
    String *name      = Getattr(n,"name");
    String *iname     = Getattr(n,"sym:name");
    SwigType *type  = Getattr(n,"type");
    String   *value = Getattr(n,"value");
    String   *tm;

    if (!addSymbol(iname,n)) return SWIG_ERROR;

    /* Special hook for member pointer */
    if (SwigType_type(type) == T_MPOINTER) {
      String *wname = Swig_name_wrapper(iname);
      Printf(f_wrappers, "static %s = %s;\n", SwigType_str(type,wname), value);
      value = Char(wname);
    }

    if ((tm = Swig_typemap_lookup_new("consttab",n,name,0))) {
      Replaceall(tm,"$source",value);
      Replaceall(tm,"$target",name);
      Replaceall(tm,"$value",value);
      Printf(constant_tab,"%s,\n", tm);
    } else if ((tm = Swig_typemap_lookup_new("constcode", n, name, 0))) {
      Replaceall(tm,"$source", value);
      Replaceall(tm,"$target", name);
      Replaceall(tm,"$value",value);
      Printf(f_init, "%s\n", tm);
    } else {
      Swig_warning(WARN_TYPEMAP_CONST_UNDEF, input_file, line_number,
		   "Unsupported constant value.\n");
      return SWIG_NOWRAP;
    }

    if (blessed) {
      if (is_shadow(type)) {
	Printv(var_stubs,
	       "\nmy %__", iname, "_hash;\n",
	       "tie %__", iname, "_hash,\"", is_shadow(type), "\", $",
	       cmodule, "::", iname, ";\n",
	       "$", iname, "= \\%__", iname, "_hash;\n",
	       "bless $", iname, ", ", is_shadow(type), ";\n",
	       NIL);
      } else if (do_constants) {
	Printv(const_stubs,"sub ", name, " () { $",
	       cmodule, "::", name, " }\n", NIL);
	num_consts++;
      } else {
	Printv(var_stubs, "*",iname," = *", cmodule, "::", iname, ";\n", NIL);
      }
    }
    if (export_all) {
      if (do_constants && !is_shadow(type)) {
	Printf(exported,"%s ",name);
      } else {
	Printf(exported,"$%s ",iname);
      }
    }
    return SWIG_OK;
  }

  /* ------------------------------------------------------------
   * usage_func()
   * ------------------------------------------------------------ */
  char *usage_func(char *iname, SwigType *, ParmList *l) {
    static String *temp = 0;
    Parm  *p;
    int    i;

    if (!temp) temp = NewString("");
    Clear(temp);
    Printf(temp,"%s(",iname);

    /* Now go through and print parameters */
    p = l;
    i = 0;
    while (p != 0) {
      SwigType *pt = Getattr(p,"type");
      String   *pn = Getattr(p,"name");
      if (!Getattr(p,"ignore")) {
	/* If parameter has been named, use that.   Otherwise, just print a type  */
	if (SwigType_type(pt) != T_VOID) {
	  if (Len(pn) > 0) {
	    Printf(temp,"%s",pn);
	  } else {
	    Printf(temp,"%s",SwigType_str(pt,0));
	  }
	}
	i++;
	p = nextSibling(p);
	if (p)
	  if (!Getattr(p,"ignore"))
	    Putc(',',temp);
      } else {
	p = nextSibling(p);
	if (p)
	  if ((i>0) && (!Getattr(p,"ignore")))
	    Putc(',',temp);
      }
    }
    Printf(temp,");");
    return Char(temp);
  }

  /* ------------------------------------------------------------
   * nativeWrapper()
   * ------------------------------------------------------------ */
  
  virtual int nativeWrapper(Node *n) {
    String *name = Getattr(n,"sym:name");
    String *funcname = Getattr(n,"wrap:name");

    if (!addSymbol(funcname,n)) return SWIG_ERROR;

    Printf(command_tab,"{\"%s::%s\", %s},\n", cmodule,name,funcname);
    if (export_all)
      Printf(exported,"%s ",name);
    if (blessed) {
      Printv(func_stubs,"*", name, " = *", cmodule, "::", name, ";\n", NIL);
    }
    return SWIG_OK;
  }

  /****************************************************************************
   ***                      OBJECT-ORIENTED FEATURES
   ****************************************************************************
   *** These extensions provide a more object-oriented interface to C++
   *** classes and structures.    The code here is based on extensions
   *** provided by David Fletcher and Gary Holt.
   ***
   *** I have generalized these extensions to make them more general purpose
   *** and to resolve object-ownership problems.
   ***
   *** The approach here is very similar to the Python module :
   ***       1.   All of the original methods are placed into a single
   ***            package like before except that a 'c' is appended to the
   ***            package name.
   ***
   ***       2.   All methods and function calls are wrapped with a new
   ***            perl function.   While possibly inefficient this allows
   ***            us to catch complex function arguments (which are hard to
   ***            track otherwise).
   ***
   ***       3.   Classes are represented as tied-hashes in a manner similar
   ***            to Gary Holt's extension.   This allows us to access
   ***            member data.
   ***
   ***       4.   Stand-alone (global) C functions are modified to take
   ***            tied hashes as arguments for complex datatypes (if
   ***            appropriate).
   ***
   ***       5.   Global variables involving a class/struct is encapsulated
   ***            in a tied hash.
   ***
   ****************************************************************************/
  
  
  void setclassname(Node *n) {
    String *symname = Getattr(n,"sym:name");
    String *fullname;
    String *actualpackage;
    Node   *clsmodule = Getattr(n,"module");
    
    if (!clsmodule) {
      /* imported module does not define a module name.   Oh well */
      return;
    }

    /* Do some work on the class name */
    actualpackage = Getattr(clsmodule,"name");
    if ((!compat) && (!Strchr(symname,':'))) {
      fullname = NewStringf("%s::%s",actualpackage,symname);
    } else {
      fullname = NewString(symname);
    }
    Setattr(n,"perl5:proxy", fullname);
  }
  
  /* ------------------------------------------------------------
   * classDeclaration()
   * ------------------------------------------------------------ */
  virtual int classDeclaration(Node *n) {
    /* Do some work on the class name */
    if (blessed) {
      setclassname(n);
      Append(classlist,n);
    }
    return Language::classDeclaration(n);
  }
  
  /* ------------------------------------------------------------
   * classHandler()
   * ------------------------------------------------------------ */

  virtual int classHandler(Node *n) {

    if (blessed) {
      have_constructor = 0;
      have_operators = 0;
      have_destructor = 0;
      have_data_members = 0;
      operators = NewHash();

      class_name = Getattr(n,"sym:name");

      if (!addSymbol(class_name,n)) return SWIG_ERROR;

      /* Use the fully qualified name of the Perl class */
      if (!compat) {
	fullclassname = NewStringf("%s::%s",fullmodule,class_name);
      } else {
	fullclassname = NewString(class_name);
      }
      real_classname = Getattr(n,"name");
      pcode = NewString("");
      blessedmembers = NewString("");
    }

    /* Emit all of the members */
    Language::classHandler(n);


    /* Finish the rest of the class */
    if (blessed) {
      /* Generate a client-data entry */
      SwigType *ct = NewStringf("p.%s", real_classname);
      Printv(f_init,"SWIG_TypeClientData(SWIGTYPE", SwigType_manglestr(ct),", (void*) \"",
	     fullclassname, "\");\n", NIL);
      SwigType_remember(ct);
      Delete(ct);

      Printv(pm,
	     "\n############# Class : ", fullclassname, " ##############\n",
	     "\npackage ", fullclassname, ";\n",
	     NIL);

      if (have_operators) {
	Printf(pm, "use overload\n");
	DOH *key;
	for (key = Firstkey(operators); key; key = Nextkey(operators)) {
	  char *name = Char(key);
	  //	    fprintf(stderr,"found name: <%s>\n", name);
	  if (strstr(name, "operator_equal_to")) {
	    Printv(pm, tab4, "\"==\" => sub { $_[0]->operator_equal_to($_[1])},\n",NIL);
	  } else if (strstr(name, "operator_not_equal_to")) {
	    Printv(pm, tab4, "\"!=\" => sub { $_[0]->operator_not_equal_to($_[1])},\n",NIL);
	  } else if (strstr(name, "operator_assignment")) {
	    Printv(pm, tab4, "\"=\" => sub { $_[0]->operator_assignment($_[1])},\n",NIL);
	  } else {
	    fprintf(stderr,"Unknown operator: %s\n", name);
	  }
	}
	Printv(pm, tab4, "\"fallback\" => 1;\n",NIL);	    
      }
      /* If we are inheriting from a base class, set that up */

      Printv(pm, "@ISA = qw( ",fullmodule, NIL);

      /* Handle inheritance */
      List *baselist = Getattr(n,"bases");
      if (baselist && Len(baselist)) {
	Node *base = Firstitem(baselist);
	while (base) {
	  String *bname = Getattr(base, "perl5:proxy");
	  if (!bname) {
	    base = Nextitem(baselist);
	    continue;
	  }
	  Printv(pm," ", bname, NIL);
	  base = Nextitem(baselist);
	}
      }
      Printf(pm, " );\n");

      /* Dump out a hash table containing the pointers that we own */
      Printf(pm, "%%OWNER = ();\n");
      if (have_data_members) {
	Printv(pm,
	       "%BLESSEDMEMBERS = (\n", blessedmembers, ");\n\n",
	       NIL);
      }
      if (have_data_members || have_destructor)
	Printf(pm, "%%ITERATORS = ();\n");

      /* Dump out the package methods */

      Printv(pm,pcode,NIL);
      Delete(pcode);

      /* Output methods for managing ownership */

      Printv(pm,
	     "sub DISOWN {\n",
	     tab4, "my $self = shift;\n",
	     tab4, "my $ptr = tied(%$self);\n",
	     tab4, "delete $OWNER{$ptr};\n",
	     tab4, "};\n\n",
	     "sub ACQUIRE {\n",
	     tab4, "my $self = shift;\n",
	     tab4, "my $ptr = tied(%$self);\n",
	     tab4, "$OWNER{$ptr} = 1;\n",
	     tab4, "};\n\n",
	     NIL);

      /* Only output the following methods if a class has member data */

      if (have_data_members) {

	/* Output a FETCH method.  This is actually common to all classes */
	Printv(pm,
	       "sub FETCH {\n",
	       tab4, "my ($self,$field) = @_;\n",
	       tab4, "my $member_func = \"swig_${field}_get\";\n",
	       tab4, "my $val = $self->$member_func();\n",
	       tab4, "if (exists $BLESSEDMEMBERS{$field}) {\n",
	       tab8, "return undef if (!defined($val));\n",
	       tab8, "my %retval;\n",
	       tab8, "tie %retval,$BLESSEDMEMBERS{$field},$val;\n",
	       tab8, "return bless \\%retval, $BLESSEDMEMBERS{$field};\n",
	       tab4, "}\n",
	       tab4, "return $val;\n",
	       "}\n\n",
	       NIL);

	/* Output a STORE method.   This is also common to all classes (might move to base class) */

	Printv(pm,
	       "sub STORE {\n",
	       tab4, "my ($self,$field,$newval) = @_;\n",
	       tab4, "my $member_func = \"swig_${field}_set\";\n",
	       tab4, "if (exists $BLESSEDMEMBERS{$field}) {\n",
	       tab8, "$self->$member_func(tied(%{$newval}));\n",
	       tab4, "} else {\n",
	       tab8, "$self->$member_func($newval);\n",
	       tab4, "}\n",
	       "}\n\n",
	       NIL);
      }
      Delete(operators);     operators = 0;
    }
    return SWIG_OK;
  }

  /* ------------------------------------------------------------
   * memberfunctionHandler()
   * ------------------------------------------------------------ */

  virtual int memberfunctionHandler(Node *n) {
    String   *symname = Getattr(n,"sym:name");
    SwigType *t   = Getattr(n,"type");

    String  *func;
    int      need_wrapper = 0;

    member_func = 1;
    Language::memberfunctionHandler(n);
    member_func = 0;

    if ((blessed) && (!Getattr(n,"sym:nextSibling"))) {
      func = NewString("");
  
      /* Now emit a Perl wrapper function around our member function, we might need
	 to patch up some arguments along the way */
  
      if (Strstr(symname, "operator") == symname) {
	if (Strstr(symname, "operator_equal_to")) {
	  DohSetInt(operators,"operator_equal_to",1);
	  have_operators = 1;
	} else if (Strstr(symname, "operator_not_equal_to")) {
	  DohSetInt(operators,"operator_not_equal_to",1);
	  have_operators = 1;
	} else if (Strstr(symname, "operator_assignment")) {
	  DohSetInt(operators,"operator_assignment",1);
	  have_operators = 1;
	} else {
	  Printf(stderr,"Unknown operator: %s\n", symname);
	}
	//      fprintf(stderr,"Found member_func operator: %s\n", symname);
      }

      Printv(func,
	     "sub ", symname, " {\n",
	     tab4, "my @args = @_;\n",
	     NIL);
  
      /* Okay.  We've made argument adjustments, now call into the package */
  
      Printv(func,
	     tab4, "my $result = ", cmodule, "::", Swig_name_member(class_name,symname),
	     "(@args);\n",
	     NIL);
  
      /* Now check to see what kind of return result was found.
       * If this function is returning a result by 'value', SWIG did an
       * implicit malloc/new.   We'll mark the object like it was created
       * in Perl so we can garbage collect it. */

      if (is_shadow(t)) {
	Printv(func,tab4, "return undef if (!defined($result));\n", NIL);
  
	/* If we're returning an object by value, put it's reference
	   into our local hash table */
  
	if ((!SwigType_ispointer(t) && !SwigType_isreference(t)) || Getattr(n,"feature:new")) {
	  Printv(func, tab4, "$", is_shadow(t), "::OWNER{$result} = 1; \n", NIL);
	}
  
	/* We're returning a Perl "object" of some kind.  Turn it into
	   a tied hash */
  
	Printv(func,
	       tab4, "my %resulthash;\n",
	       tab4, "tie %resulthash, ref($result), $result;\n",
	       tab4, "return bless \\%resulthash, ref($result);\n",
	       "}\n",
	       NIL);
  
	need_wrapper = 1;
      } else {
  
	/* Hmmm.  This doesn't appear to be anything I know about so just
	   return it unmodified */
  
	Printv(func, tab4,"return $result;\n", "}\n", NIL);
      }
  
      if (need_wrapper) {
	Printv(pcode,func,NIL);
      } else {
	Printv(pcode,"*",symname," = *", cmodule, "::", Swig_name_member(class_name,symname), ";\n", NIL);
      }
      Delete(func);
    }
    return SWIG_OK;
  }

  /* ------------------------------------------------------------
   * membervariableHandler()
   *
   * Adds an instance member.   This is a little hairy because data members are
   * really added with a tied-hash table that is attached to the object.
   *
   * On the low level, we will emit a pair of get/set functions to retrieve
   * values just like before.    These will then be encapsulated in a FETCH/STORE
   * method associated with the tied-hash.
   *
   * In the event that a member is an object that we have already wrapped, then
   * we need to retrieve the data a tied-hash as opposed to what SWIG normally
   * returns.   To determine this, we build an internal hash called 'BLESSEDMEMBERS'
   * that contains the names and types of tied data members.  If a member name
   * is in the list, we tie it, otherwise, we just return the normal SWIG value.
   * ----------------------------------------------------------------------------- */

  virtual int membervariableHandler(Node *n) {

    String   *symname = Getattr(n,"sym:name");
    SwigType *t  = Getattr(n,"type");

    /* Emit a pair of get/set functions for the variable */

    member_func = 1;
    Language::membervariableHandler(n);
    member_func = 0;

    if (blessed) {

      Printv(pcode,"*swig_", symname, "_get = *", cmodule, "::", Swig_name_get(Swig_name_member(class_name,symname)), ";\n", NIL);
      Printv(pcode,"*swig_", symname, "_set = *", cmodule, "::", Swig_name_set(Swig_name_member(class_name,symname)), ";\n", NIL);

      /* Now we need to generate a little Perl code for this */

      if (is_shadow(t)) {

	/* This is a Perl object that we have already seen.  Add an
	   entry to the members list*/
	Printv(blessedmembers,
	       tab4, symname, " => '", is_shadow(t), "',\n",
	       NIL);

      }
    }
    have_data_members++;
    return SWIG_OK;
  }

  /* ------------------------------------------------------------
   * constructorDeclaration()
   *
   * Emits a blessed constructor for our class.    In addition to our construct
   * we manage a Perl hash table containing all of the pointers created by
   * the constructor.   This prevents us from accidentally trying to free
   * something that wasn't necessarily allocated by malloc or new
   * ------------------------------------------------------------ */

  virtual int constructorHandler(Node *n) {

    String *symname = Getattr(n,"sym:name");

    member_func = 1;
    Language::constructorHandler(n);

    if ((blessed) && (!Getattr(n,"sym:nextSibling"))) {
      if ((Cmp(symname,class_name) == 0)) {
	/* Emit a blessed constructor  */
	Printf(pcode, "sub new {\n");
      } else {
	/* Constructor doesn't match classname so we'll just use the normal name  */
	Printv(pcode, "sub ", Swig_name_construct(symname), " () {\n", NIL);
      }

      Printv(pcode, tab4, "my $pkg = shift;\n",
	     tab4, "my @args = @_;\n", NIL);

      Printv(pcode,
	     tab4, "my $self = ", cmodule, "::", Swig_name_construct(symname), "(@args);\n",
	     tab4, "return undef if (!defined($self));\n",
	     /*	     tab4, "bless $self, \"", fullclassname, "\";\n", */
	     tab4, "$OWNER{$self} = 1;\n",
	     tab4, "my %retval;\n",
	     tab4, "tie %retval, \"", fullclassname, "\", $self;\n",
	     tab4, "return bless \\%retval, $pkg;\n",
	     "}\n\n",
	     NIL);

      have_constructor = 1;
    }
    member_func = 0;
    return SWIG_OK;
  }

  /* ------------------------------------------------------------ 
   * destructorHandler()
   * ------------------------------------------------------------ */
  
  virtual int destructorHandler(Node *n) {
    String *symname = Getattr(n,"sym:name");
    member_func = 1;
    Language::destructorHandler(n);
    if (blessed) {
      Printv(pcode,
	     "sub DESTROY {\n",
	     tab4, "return unless $_[0]->isa('HASH');\n",
	     tab4, "my $self = tied(%{$_[0]});\n",
	     tab4, "return unless defined $self;\n",
	     tab4, "delete $ITERATORS{$self};\n",
	     tab4, "if (exists $OWNER{$self}) {\n",
	     tab8,  cmodule, "::", Swig_name_destroy(symname), "($self);\n",
	     tab8, "delete $OWNER{$self};\n",
	     tab4, "}\n}\n\n",
	     NIL);
      have_destructor = 1;
    }
    member_func = 0;
    return SWIG_OK;
  }

  /* ------------------------------------------------------------
   * staticmemberfunctionHandler()
   * ------------------------------------------------------------ */

  virtual int staticmemberfunctionHandler(Node *n) {
    member_func = 1;
    Language::staticmemberfunctionHandler(n);
    member_func = 0;
    if ((blessed) && (!Getattr(n,"sym:nextSibling"))) {
      String *symname = Getattr(n,"sym:name");
      SwigType *t = Getattr(n,"type");
      if (is_shadow(t)) {
	Printv(pcode,
	       "sub ", symname, " {\n",
	       tab4, "my @args = @_;\n",
	       NIL);
	
	/* Okay.  We've made argument adjustments, now call into the package */
	
	Printv(pcode,
	       tab4, "my $result = ", cmodule, "::", Swig_name_member(class_name,symname),
	       "(@args);\n",
	       NIL);
	
	/* Now check to see what kind of return result was found.
	 * If this function is returning a result by 'value', SWIG did an
	 * implicit malloc/new.   We'll mark the object like it was created
	 * in Perl so we can garbage collect it. */
	
	Printv(pcode,tab4, "return undef if (!defined($result));\n", NIL);
	
	/* If we're returning an object by value, put it's reference
	   into our local hash table */
	
	if ((!SwigType_ispointer(t) && !SwigType_isreference(t)) || Getattr(n,"feature:new")) {
	  Printv(pcode, tab4, "$", is_shadow(t), "::OWNER{$result} = 1; \n", NIL);
	}
	
	/* We're returning a Perl "object" of some kind.  Turn it into
	   a tied hash */
	
	Printv(pcode,
	       tab4, "my %resulthash;\n",
	       tab4, "tie %resulthash, ref($result), $result;\n",
	       tab4, "return bless \\%resulthash, ref($result);\n",
	       "}\n",
	       NIL);
      } else {
	Printv(pcode,"*",symname," = *", cmodule, "::", Swig_name_member(class_name,symname), ";\n", NIL);
      }
    }
    return SWIG_OK;
  }

  /* ------------------------------------------------------------
   * staticmembervariableHandler()
   * ------------------------------------------------------------ */

  virtual int staticmembervariableHandler(Node *n) {
    Language::staticmembervariableHandler(n);
    if (blessed) {
      String *symname = Getattr(n,"sym:name");
      Printv(pcode,"*",symname," = *", cmodule, "::", Swig_name_member(class_name,symname), ";\n", NIL);
    }
    return SWIG_OK;
  }
  
  /* ------------------------------------------------------------
   * memberconstantHandler()
   * ------------------------------------------------------------ */

  virtual int memberconstantHandler(Node *n) {
    String *symname = Getattr(n,"sym:name");
    int   oldblessed = blessed;
    
    /* Create a normal constant */
    blessed = 0;
    Language::memberconstantHandler(n);
    blessed = oldblessed;
    
    if (blessed) {
      Printv(pcode, "*", symname, " = *", cmodule, "::", Swig_name_member(class_name,symname), ";\n", NIL);
    }
    return SWIG_OK;
  }

  /* ------------------------------------------------------------
   * pragma()
   *
   * Pragma directive.
   *
   * %pragma(perl5) code="String"              # Includes a string in the .pm file
   * %pragma(perl5) include="file.pl"          # Includes a file in the .pm file
   * ------------------------------------------------------------ */

  virtual int pragmaDirective(Node *n) {
    String *lang;
    String *code;
    String *value;
    if (!ImportMode) {
      lang = Getattr(n,"lang");
      code = Getattr(n,"name");
      value = Getattr(n,"value");
      if (Strcmp(lang,"perl5") == 0) {
	if (Strcmp(code,"code") == 0) {
	  /* Dump the value string into the .pm file */
	  if (value) {
	    Printf(pragma_include, "%s\n", value);
	  }
	} else if (Strcmp(code,"include") == 0) {
	  /* Include a file into the .pm file */
	  if (value) {
	    FILE *f = Swig_open(value);
	    if (!f) {
	      Printf(stderr,"%s : Line %d. Unable to locate file %s\n", input_file, line_number,value);
	    } else {
	      char buffer[4096];
	      while (fgets(buffer,4095,f)) {
		Printf(pragma_include,"%s",buffer);
	      }
	    }
	  }
	} else {
	  Printf(stderr,"%s : Line %d. Unrecognized pragma.\n", input_file,line_number);
	}
      }
    }
    return Language::pragmaDirective(n);
  }
};
  
/* -----------------------------------------------------------------------------
 * swig_perl5()    - Instantiate module
 * ----------------------------------------------------------------------------- */

extern "C" Language *
swig_perl5(void) {
  return new PERL5();
}
