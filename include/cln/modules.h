// Macros for correct module ordering.

#ifndef _CL_MODULES_H
#define _CL_MODULES_H

// The order of initialization of different compilation units is not
// specified in C++. AIX 4 has a linker which apparently does order
// the modules according to dependencies, so that low-level modules
// will be initialized earlier than the high-level modules which depend
// on them. I have a patch for GNU ld that does the same thing.
//
// But for now, I take a half-automatic approach to the correct module
// ordering problem: PROVIDE/REQUIRE, as in Common Lisp.
//
// CL_PROVIDE(module) must be the first code-generating entity in a module.
// Inline function definitions can precede it, but global variable/function/
// class definitions may not precede it.
// Afterwards, any number of CL_REQUIRE(othermodule) is allowed.
// At the end of the module, there must be a corresponding
// CL_PROVIDE_END(module). (Sorry for this, it's really needed.)
//
// These macros work only with g++, and only in optimizing mode. But who
// wants to use CLN with other C++ compilers anyway...

// How to apply these macros:
// 1. Find out about variables which need to be initialized.
//    On Linux/ELF, you can use a command like
//    $ nm -o libcln.a | grep -v ' [UTtRrW] ' | sort +1
//    A symbol of type "D" or "d" lies in the preinitialized DATA section,
//    a symbol of type "B" or "b" lies in the uninitialized BSS section.
//    All of them have to be checked.
//  - Those which contain POD (= plain old data, i.e. scalar values or
//    class instances without nontrivial constructors) are already fully
//    initialized by the linker and can be discarded from these considerations.
//  - Those which are static variables inside a function (you recognize
//    them: g++ appends a dot and a number to their name) are initialized
//    the first time the function is entered. They can be discarded from
//    our considerations as well.
// 2. Find out which of these variables are publically exposed (to the user of
//    the library) through the library's include files, either directly or
//    through inline functions, or indirectly through normal function calls.
//    These variables can be referenced from any user module U, hence any
//    such module must CL_REQUIRE(M) the variable's definition module M.
//    Since there is no CL_REQUIRE_IF_NEEDED(M) macro (which is equivalent
//    to CL_REQUIRE(M) if the required module will be part of the executable
//    but does nothing if M is not used), we must preventively put the
//    CL_REQUIRE(M) into the header file. Hopefully M is either used anyway
//    or does not bring in too much code into the executable.
// 3. Variables which are not publicly exposed but used internally by the
//    library can be handled by adding a CL_REQUIRE in all the library's
//    modules which directly or indirectly use the variable.
// 4. Variables and functions which can be reasonably assumed to not be
//    accessed or executed during initialization need not be treated.
//    For example, I/O to external streams, exception handling facilities,
//    number theory stuff, etc.

// OK, stop reading here, because it's getting obscene.

#if defined(__GNUC__) && defined(__OPTIMIZE__) && !(defined(__hppa__) && (__GNUC__ == 2) && (__GNUC_MINOR__ < 8)) && !defined(NO_PROVIDE_REQUIRE)
  #ifdef ASM_UNDERSCORE
    #define ASM_UNDERSCORE_PREFIX "_"
  #else
    #define ASM_UNDERSCORE_PREFIX ""
  #endif
  // Globalize a label defined in the same translation unit.
  // See macro ASM_GLOBALIZE_LABEL in the egcs sources.
  #if defined(__i386__) || defined(__m68k__) || defined(__mips__) || defined(__mips64__) || defined(__alpha__) || defined(__rs6000__)
    // Some m68k systems use "xdef" or "global" or ".global"...
    #define CL_GLOBALIZE_LABEL(label)  __asm__("\t.globl " label);
  #endif
  #if defined(__sparc__) || defined(__sparc64__) || defined(__arm__)
    // Some arm systems use "EXPORT" or ".globl"...
    #define CL_GLOBALIZE_LABEL(label)  __asm__("\t.global " label);
  #endif
  #if defined(__hppa__)
    #define CL_GLOBALIZE_LABEL(label)  __asm__("\t.EXPORT " label ",ENTRY,PRIV_LEV=3");
  #endif
  #if defined(__m88k__)
    #define CL_GLOBALIZE_LABEL(label)  __asm__("\tglobal " label);
  #endif
  #if defined(__convex__)
    #define CL_GLOBALIZE_LABEL(label)  __asm__(".globl " label);
  #endif
  #if defined(__ia64__)
    #define CL_GLOBALIZE_LABEL(label)  __asm__("\t.global " label);
  #endif
  #ifndef CL_GLOBALIZE_LABEL
    #define CL_GLOBALIZE_LABEL(label)
  #endif
  #if defined(__rs6000__) || defined(_WIN32)
    #define CL_GLOBALIZE_JUMP_LABEL(label)  CL_GLOBALIZE_LABEL(#label)
  #else
    #define CL_GLOBALIZE_JUMP_LABEL(label)
  #endif
  #ifdef CL_NEED_GLOBALIZE_CTORDTOR
    #define CL_GLOBALIZE_CTORDTOR_LABEL(label)  CL_GLOBALIZE_LABEL(label)
  #else
    #define CL_GLOBALIZE_CTORDTOR_LABEL(label)
  #endif
  // Output a label inside a function.
  // See macro ASM_OUTPUT_LABEL in the egcs sources.
  #if defined(__hppa__)
    #define CL_OUTPUT_LABEL(label)  ASM_VOLATILE ("\n" label)
  #else
    #define CL_OUTPUT_LABEL(label)  ASM_VOLATILE ("\n" label ":")
  #endif
  // ASM_VOLATILE(string) is for asms without arguments only!!
  #if ((__GNUC__ == 2) && (__GNUC_MINOR__ >= 91)) || (__GNUC__ >= 3)
    // avoid warning caused by the volatile keyword
    #define ASM_VOLATILE  __asm__
  #else
    // need volatile to avoid reordering
    #define ASM_VOLATILE  __asm__ __volatile__
  #endif
  // CL_JUMP_TO(addr) jumps to an address, like  goto *(void*)(addr),
  // except that the latter inhibits inlining of the function containing it
  // in gcc-2.95. For new CPUs, look for "jump" and "indirect_jump" in gcc's
  // machine description.
  #if defined(__i386__)
    #define CL_JUMP_TO(addr)  ASM_VOLATILE("jmp %*%0" : : "rm" ((void*)(addr)))
  #endif
  #if defined(__m68k__)
    #define CL_JUMP_TO(addr)  ASM_VOLATILE("jmp %0@" : : "a" ((void*)(addr)))
  #endif
  #if defined(__mips__)
    #define CL_JUMP_TO(addr)  ASM_VOLATILE("%*j %0" : : "d" ((void*)(addr)))
  #endif
  #if defined(__sparc__) || defined(__sparc64__)
    #define CL_JUMP_TO(addr)  ASM_VOLATILE("jmp %0\n\tnop" : : "r" ((void*)(addr)))
  #endif
  #if defined(__alpha__)
    #define CL_JUMP_TO(addr)  ASM_VOLATILE("jmp $31,(%0),0" : : "r" ((void*)(addr)))
  #endif
  #if defined(__hppa__)
    //#define CL_JUMP_TO(addr)  ASM_VOLATILE("bv,n 0(%0)" : : "r" ((void*)(addr)))
    #define CL_JUMP_TO(addr)  ASM_VOLATILE("b " #addr "\n\tnop")
  #endif
  #if defined(__arm__)
    #define CL_JUMP_TO(addr)  ASM_VOLATILE("mov pc,%0" : : "r" ((void*)(addr)))
  #endif
  #if defined(__rs6000__) || defined(__powerpc__) || defined(__ppc__)
    //#define CL_JUMP_TO(addr)  ASM_VOLATILE("mtctr %0\n\tbctr" : : "r" ((void*)(addr)))
    #define CL_JUMP_TO(addr)  ASM_VOLATILE("b " #addr)
  #endif
  #if defined(__m88k__)
    #define CL_JUMP_TO(addr)  ASM_VOLATILE("jmp %0" : : "r" ((void*)(addr)))
  #endif
  #if defined(__convex__)
    #define CL_JUMP_TO(addr)  ASM_VOLATILE("jmp (%0)" : : "r" ((void*)(addr)))
  #endif
  #if defined(__ia64__)
    #define CL_JUMP_TO(addr)  ASM_VOLATILE("br %0" : : "b" ((void*)(addr)))
  #endif
  #define CL_PROVIDE(module)  \
    extern "C" void cl_module__##module##__firstglobalfun () {}		\
    extern "C" void cl_module__##module##__ctorend (void);		\
    extern "C" void cl_module__##module##__dtorend (void);		\
    CL_GLOBALIZE_JUMP_LABEL(cl_module__##module##__ctorend)		\
    CL_GLOBALIZE_JUMP_LABEL(cl_module__##module##__dtorend)		\
    CL_GLOBALIZE_CTORDTOR_LABEL(					\
               ASM_UNDERSCORE_PREFIX CL_GLOBAL_CONSTRUCTOR_PREFIX	\
               "cl_module__" #module "__firstglobalfun")		\
    CL_GLOBALIZE_CTORDTOR_LABEL(					\
               ASM_UNDERSCORE_PREFIX CL_GLOBAL_DESTRUCTOR_PREFIX	\
               "cl_module__" #module "__firstglobalfun")		\
    static int cl_module__##module##__counter;				\
    struct cl_module__##module##__controller {				\
      inline cl_module__##module##__controller ()			\
        { if (cl_module__##module##__counter++)				\
            { CL_JUMP_TO(cl_module__##module##__ctorend); }		\
        }								\
      inline ~cl_module__##module##__controller ()			\
        { CL_OUTPUT_LABEL (ASM_UNDERSCORE_PREFIX "cl_module__" #module "__dtorend"); } \
    };									\
    static cl_module__##module##__controller cl_module__##module##__ctordummy;
  #define CL_PROVIDE_END(module)  \
    struct cl_module__##module##__destroyer {				\
      inline cl_module__##module##__destroyer ()			\
        { CL_OUTPUT_LABEL (ASM_UNDERSCORE_PREFIX "cl_module__" #module "__ctorend"); } \
      inline ~cl_module__##module##__destroyer ()			\
        { if (--cl_module__##module##__counter)				\
            { CL_JUMP_TO(cl_module__##module##__dtorend); }		\
        }								\
    };									\
    static cl_module__##module##__destroyer cl_module__##module##__dtordummy;
  #define CL_REQUIRE(module)  \
    extern "C" void cl_module__##module##__ctor (void)			\
      __asm__ (ASM_UNDERSCORE_PREFIX CL_GLOBAL_CONSTRUCTOR_PREFIX	\
               "cl_module__" #module "__firstglobalfun");		\
    extern "C" void cl_module__##module##__dtor (void)			\
      __asm__ (ASM_UNDERSCORE_PREFIX CL_GLOBAL_DESTRUCTOR_PREFIX	\
               "cl_module__" #module "__firstglobalfun");		\
    struct _CL_REQUIRE_CLASSNAME(module,__LINE__) {			\
      inline _CL_REQUIRE_CLASSNAME(module,__LINE__) ()			\
        { cl_module__##module##__ctor (); }				\
      inline ~_CL_REQUIRE_CLASSNAME(module,__LINE__) ()			\
        { cl_module__##module##__dtor (); }				\
    };									\
    static _CL_REQUIRE_CLASSNAME(module,__LINE__)			\
      _CL_REQUIRE_CLASSNAME(module##_requirer,__LINE__);
  #define _CL_REQUIRE_CLASSNAME(module,line) __CL_REQUIRE_CLASSNAME(module,line)
  #define __CL_REQUIRE_CLASSNAME(module,line) cl_module__##module##__##line
#else
  #define CL_PROVIDE(module)
  #define CL_PROVIDE_END(module)
  #define CL_REQUIRE(module)
#endif

#endif /* _CL_MODULES_H */