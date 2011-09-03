/* R-related tools (mapping of jpeg error handling to R)
   common to all tasks */

#ifndef R_J_COMMON_H
#define R_J_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jpeglib.h>

#include "jcompat.h"

#if (BITS_IN_JSAMPLE != 8)
#error "Sorry, only 8-bit libjpeg is supported"
#endif

#define USE_RINTERNALS 1
#include <Rinternals.h>
/* for R_RGB / R_RGBA */
#include <R_ext/GraphicsEngine.h>

METHODDEF(void)
Rjpeg_error_exit(j_common_ptr cinfo)
{
    char buffer[JMSG_LENGTH_MAX];
    
    (*cinfo->err->format_message) (cinfo, buffer);
    Rf_error("JPEG decompression error: %s", buffer);
}

METHODDEF(void)
Rjpeg_output_message (j_common_ptr cinfo)
{
    char buffer[JMSG_LENGTH_MAX];
    
    (*cinfo->err->format_message) (cinfo, buffer);
    REprintf("JPEG decompression: %s", buffer);
}

static void Rjpeg_fin(SEXP dco) {
    struct jpeg_common_struct *cinfo = (struct jpeg_common_struct*) R_ExternalPtrAddr(dco);
    if (cinfo) {
        jpeg_destroy(cinfo);
        free(cinfo->err);
        free(cinfo);
    }
    /* make it a NULL ptr in case this was not a finalizer call */
    CAR(dco) = 0;
}

#endif
