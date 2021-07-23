#include <stdlib.h>
#include <R_ext/Rdynload.h>
#include <Rinternals.h>

/* read.c */
extern SEXP read_jpeg(SEXP sFn, SEXP sNative);
/* write.c */
extern SEXP write_jpeg(SEXP image, SEXP sFn, SEXP sQuality, SEXP sBg, SEXP sColorsp);

static const R_CallMethodDef CAPI[] = {
    {"read_jpeg",  (DL_FUNC) &read_jpeg , 2},
    {"write_jpeg", (DL_FUNC) &write_jpeg, 5},
    {NULL, NULL, 0}
};

void R_init_jpeg(DllInfo *dll)
{
    R_registerRoutines(dll, NULL, CAPI, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
}
