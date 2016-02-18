dnl -*- shell-script -*-
dnl
dnl Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
dnl                         University Research and Technology
dnl                         Corporation.  All rights reserved.
dnl Copyright (c) 2004-2005 The University of Tennessee and The University
dnl                         of Tennessee Research Foundation.  All rights
dnl                         reserved.
dnl Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
dnl                         University of Stuttgart.  All rights reserved.
dnl Copyright (c) 2004-2005 The Regents of the University of California.
dnl                         All rights reserved.
dnl Copyright (c) 2009      Cisco Systems, Inc.  All rights reserved.
dnl $COPYRIGHT$
dnl
dnl Additional copyrights may follow
dnl
dnl $HEADER$
dnl

dnl we only want one :)
m4_define(MCA_opal_memory_CONFIGURE_MODE, STOP_AT_FIRST)

AC_DEFUN([MCA_opal_memory_CONFIG],[
        AC_ARG_WITH([memory-manager],
            [AC_HELP_STRING([--with-memory-manager=TYPE],
                           [Use TYPE for intercepting memory management
                            calls to control memory pinning.])])

        memory_base_found=0
        memory_base_want=1
        AS_IF([test "$with_memory_manager" = "no"], [memory_base_want=0])
        MCA_CONFIGURE_FRAMEWORK($1, $2, $memory_base_want)

        AC_DEFINE_UNQUOTED([OPAL_MEMORY_HAVE_COMPONENT], [$memory_base_found],
            [Whether any opal memory mca components were found])

        # See if someone set to use their header file
        if test "$memory_base_include" = "" ; then
            memory_base_include="base/empty.h"
        fi

        AC_DEFINE_UNQUOTED([MCA_memory_IMPLEMENTATION_HEADER],
                           ["opal/mca/memory/$memory_base_include"],
                           [Header to include for parts of the memory implementation])

	    ######################################################################
	    # if memory hook available
	    ######################################################################
	    memory_hook_found=1
	    AS_IF([test "$memory_hook_found" -eq 1],
	        [memory_hook_found=0 AC_CHECK_HEADER([malloc.h],
	             [AC_CHECK_FUNC([__malloc_initialize_hook],
	                 [AC_CHECK_FUNC([__malloc_hook],
	                     [AC_CHECK_FUNC([__realloc_hook],
	                         [AC_CHECK_FUNC([__free_hook],
	                            [memory_hook_found=1])])])])])])
	    AC_MSG_CHECKING([whether the system can use malloc hooks])
	    AS_IF([test "$memory_hook_found" = "0"],
	          [AC_MSG_RESULT([no])],
	          [AC_MSG_RESULT([yes])])
	    AC_DEFINE_UNQUOTED([MEMORY_HAVE_MALLOC_HOOK_SUPPORT], [$memory_hook_found],
	                   	   [Whether the system has Memory Allocation Hooks])
	
	    AC_ARG_ENABLE(memory-malloc-alignment,
	        AC_HELP_STRING([--enable-memory-malloc-alignment], [Enable support for allocated memory alignment. Default: enabled if supported, disabled otherwise.]))
	
	    malloc_align_enabled=0
	    AS_IF([test "$enable_memory_malloc_alignment" != "no"],
	        [malloc_align_enabled=$memory_hook_found])
	
	    AS_IF([test "$enable_memory_malloc_alignment" = "yes" && test "$malloc_align_enabled" = "0"],
	          [AC_MSG_ERROR([memory malloc alignment is requested but __malloc_hook is not available])])
	    AC_MSG_CHECKING([whether the memory will use malloc alignment])
	    AS_IF([test "$malloc_align_enabled" = "0"],
	          [AC_MSG_RESULT([no])],
	          [AC_MSG_RESULT([yes])])
	
	    AC_DEFINE_UNQUOTED(MEMORY_MALLOC_ALIGN_ENABLED, [$malloc_align_enabled],
	                       [Whether the memory malloc alignment is enabled])
])
