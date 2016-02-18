/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2009      Cisco Systems, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */


#include "opal_config.h"

#include "opal/constants.h"
#include "opal/mca/mca.h"
#include "opal/mca/base/base.h"
#include "opal/mca/memory/memory.h"
#include "opal/mca/memory/base/base.h"
#include "opal/mca/memory/base/empty.h"
#include "opal/memoryhooks/memory.h"
#include "opal/util/show_help.h"


/*
 * The following file was created by configure.  It contains extern
 * statements and the definition of an array of pointers to each
 * component's public mca_base_component_t struct.
 */
#include "opal/mca/memory/base/static-components.h"


#if MEMORY_MALLOC_ALIGN_ENABLED

#if HAVE_MALLOC_H
#include <malloc.h>
#endif

static int use_memalign;
static size_t memalign_threshold;

static void *(*prev_malloc_hook)(size_t, const void *);

/* This is a memory allocator hook. The purpose of this is to make
 * every malloc aligned.
 * There two basic cases here:
 *
 * 1. Memory manager for Open MPI is enabled. Then memalign below will
 * be overridden by __memalign_hook which is set to
 * opal_memory_linux_memalign_hook.  Thus, _malloc_hook is going to
 * use opal_memory_linux_memalign_hook.
 *
 * 2. No memory manager support. The memalign below is just regular glibc
 * memalign which will be called through __malloc_hook instead of malloc.
 */
static void *_opal_memory_malloc_align_hook(size_t sz, const void* caller);
#endif /* MEMORY_MALLOC_ALIGN_ENABLED */

static int empty_process(void)
{
    return OPAL_SUCCESS;
}


/*
 * Local variables
 */
static opal_memory_base_component_2_0_0_t empty_component = {
    /* Don't care about the version info */
    { 0, },
    /* Don't care about the data */
    { 0, },
    /* Empty / safe functions to call if no memory component is selected */
    empty_process,
    opal_memory_base_component_register_empty,
    opal_memory_base_component_deregister_empty,
    opal_memory_base_component_malloc_set_alignment,
};


/*
 * Globals
 */
opal_memory_base_component_2_0_0_t *opal_memory = &empty_component;


static int opal_memory_base_register(mca_base_register_flag_t flags)
{
    int ret;

#if MEMORY_MALLOC_ALIGN_ENABLED
    use_memalign = -1;
    ret = mca_base_framework_var_register(&opal_memory_base_framework,
                                 "memalign",
                                 "[64 | 32 | 0] - Enable memory alignment for all malloc calls (default: disabled).",
                                 MCA_BASE_VAR_TYPE_INT,
                                 NULL,
                                 0,
                                 0,
                                 OPAL_INFO_LVL_5,
                                 MCA_BASE_VAR_SCOPE_READONLY,
                                 &use_memalign);
    if (0 > ret) {
        return ret;
    }

    memalign_threshold = 12288;
    ret = mca_base_framework_var_register(&opal_memory_base_framework,
                                 "memalign_threshold",
                                 "Allocating memory more than memory_linux_memalign_threshold"
                                 "bytes will automatically be aligned to the value of memory_linux_memalign bytes."
                                 "(default: 12288)",
                                 MCA_BASE_VAR_TYPE_SIZE_T,
                                 NULL,
                                 0,
                                 0,
                                 OPAL_INFO_LVL_5,
                                 MCA_BASE_VAR_SCOPE_READONLY,
                                 &memalign_threshold);
    if (0 > ret) {
        return ret;
    }

    if (use_memalign != -1
        && use_memalign != 32
        && use_memalign != 64
        && use_memalign != 0){
        opal_show_help("help-opal-memory-linux.txt", "invalid mca param value",
                       true, "Wrong memalign parameter value. Allowed values: 64, 32, 0.",
                       "memory_linux_memalign is reset to 32");
        use_memalign = 32;
    }
#endif /* MEMORY_MALLOC_ALIGN_ENABLED */

    return (0 > ret) ? ret : OPAL_SUCCESS;
}


/*
 * Function for finding and opening either all MCA components, or the one
 * that was specifically requested via a MCA parameter.
 */
static int opal_memory_base_open(mca_base_open_flag_t flags)
{
    int ret;

    /* Open up all available components */
    ret = mca_base_framework_components_open (&opal_memory_base_framework, flags);
    if (ret != OPAL_SUCCESS) {
        return ret;
    }

    /* can only be zero or one */
    if (opal_list_get_size(&opal_memory_base_framework.framework_components) == 1) {
        mca_base_component_list_item_t *item;
        item = (mca_base_component_list_item_t*)
            opal_list_get_first(&opal_memory_base_framework.framework_components);
        opal_memory = (opal_memory_base_component_2_0_0_t*)
            item->cli_component;
    }

#if MEMORY_MALLOC_ALIGN_ENABLED
    /* save original call */
    prev_malloc_hook = NULL;

    if (use_memalign > 0 &&
        (opal_mem_hooks_support_level() &
            (OPAL_MEMORY_FREE_SUPPORT | OPAL_MEMORY_CHUNK_SUPPORT)) != 0) {
        prev_malloc_hook = __malloc_hook;
        __malloc_hook = _opal_memory_malloc_align_hook;
    }
#endif /* MEMORY_MALLOC_ALIGN_ENABLED */

    /* All done */
    return OPAL_SUCCESS;
}

static int opal_memory_base_close(void)
{

#if MEMORY_MALLOC_ALIGN_ENABLED
    /* restore original call */
    if (prev_malloc_hook) {
        __malloc_hook = prev_malloc_hook;
        prev_malloc_hook = NULL;
    }
#endif /* MEMORY_MALLOC_ALIGN_ENABLED */

    /* Close all available modules that are open */
    return mca_base_framework_components_close (&opal_memory_base_framework, NULL);
}

#if MEMORY_HAVE_MALLOC_HOOK_SUPPORT
void opal_memory_base_malloc_init_hook(void)
{
#if MEMORY_LINUX_PTMALLOC2
    /* Of course it is abstraction violation but unfortunately there is no way
     * to do this thing using correct approach. It is an attempt to keep
     * all clumsy code related memory framework locally.
     */
    extern void opal_memory_linux_malloc_init_hook(void);
    opal_memory_linux_malloc_init_hook();
#endif /* MEMORY_LINUX_PTMALLOC2 */
}
#endif /* MEMORY_HAVE_MALLOC_HOOK_SUPPORT */


void opal_memory_base_component_malloc_set_alignment(int memalign, size_t threshold)
{
#if MEMORY_MALLOC_ALIGN_ENABLED
    /* ignore cases when this capability is enabled explicitly using
     * mca variables
     */
    if ((NULL == prev_malloc_hook) && (-1 == use_memalign)) {
        if (memalign == 0 || memalign == 32 || memalign == 64) {
            use_memalign = memalign;
            memalign_threshold = threshold;
            if ((opal_mem_hooks_support_level() &
                    (OPAL_MEMORY_FREE_SUPPORT | OPAL_MEMORY_CHUNK_SUPPORT)) != 0) {
                prev_malloc_hook = __malloc_hook;
                __malloc_hook = _opal_memory_malloc_align_hook;
            }
        }
    }
#endif /* MEMORY_MALLOC_ALIGN_ENABLED */
}

#if MEMORY_MALLOC_ALIGN_ENABLED
static void *_opal_memory_malloc_align_hook(size_t sz, const void* caller)
{
    if (sz < memalign_threshold) {
        return prev_malloc_hook(sz, caller);
    } else {
        return memalign(use_memalign, sz);
    }
}
#endif /* MEMORY_MALLOC_ALIGN_ENABLED */

/* Use default register/close functions */
MCA_BASE_FRAMEWORK_DECLARE(opal, memory, "memory hooks", opal_memory_base_register,
                           opal_memory_base_open, opal_memory_base_close,
                           mca_memory_base_static_components, 0);
