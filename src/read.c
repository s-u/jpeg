#include "rjcommon.h"
/* compatibility implementation of jpeg_mem_src() if not provided by jpeg */
#include "jcompat.h"

/* create an R object containing the initialized decompression
   structure. The object will ensure proper release of the jpeg struct. */
static SEXP Rjpeg_decompress(struct jpeg_decompress_struct **cinfo_ptr) {
    SEXP dco;
    struct jpeg_decompress_struct *cinfo = (struct jpeg_decompress_struct*) malloc(sizeof(struct jpeg_decompress_struct));

    if (!cinfo)
	Rf_error("Unable to allocate jpeg decompression structure");
    
    cinfo->err = Rjpeg_new_err();
    
    jpeg_create_decompress(cinfo);

    *cinfo_ptr = cinfo;

    dco = PROTECT(R_MakeExternalPtr(cinfo, R_NilValue, R_NilValue));
    R_RegisterCFinalizerEx(dco, Rjpeg_fin, TRUE);
    UNPROTECT(1);
    return dco;
}


#define RX_swap32(X) (X) = (((unsigned int)X) >> 24) | ((((unsigned int)X) >> 8) & 0xff00) | (((unsigned int)X) << 24) | ((((unsigned int)X) & 0xff00) << 8)

SEXP read_jpeg(SEXP sFn, SEXP sNative) {
    const char *fn;
    SEXP res = R_NilValue, dim, dco;
    int native = asInteger(sNative);
    FILE *f = 0;
    J_COLOR_SPACE color_space;

    struct jpeg_decompress_struct *cinfo;

    dco = PROTECT(Rjpeg_decompress(&cinfo));
    
    if (TYPEOF(sFn) == RAWSXP)
	jpeg_mem_src(cinfo, (unsigned char*) RAW(sFn), (unsigned long) LENGTH(sFn));
    else {
	if (TYPEOF(sFn) != STRSXP || LENGTH(sFn) < 1) Rf_error("invalid filename");
	fn = CHAR(STRING_ELT(sFn, 0));
	f = fopen(fn, "rb");
	if (!f) Rf_error("unable to open %s", fn);
	jpeg_stdio_src(cinfo, f);
    }

    jpeg_read_header(cinfo, TRUE);

    color_space = cinfo->out_color_space;

    jpeg_start_decompress(cinfo);
	
    {
	int need_swap = 0;
	int width = cinfo->output_width, height = cinfo->output_height, pln = cinfo->output_components;
	int rowbytes = width * pln;
	unsigned char *image;
	JSAMPROW line;

#if VERBOSE_INFO
	Rprintf("jpeg: %d x %d [%d], %d bytes (color space: %d -> %d)\n", width, height, pln, rowbytes,
		cinfo->jpeg_color_space, color_space);
#endif

	/* on little-endian machines it's all well, but on big-endian ones we'll have to swap */
#if ! defined (__BIG_ENDIAN__) && ! defined (__LITTLE_ENDIAN__)   /* old compiler so have to use run-time check */
	{
	    char bo[4] = { 1, 0, 0, 0 };
	    int bi;
	    memcpy(&bi, bo, 4);
	    if (bi != 1)
		need_swap = 1;
	}
#endif
#ifdef __BIG_ENDIAN__
	need_swap = 1;
#endif

	/* allocate data for row pointers and the image using R's allocation */
	image = (unsigned char *) R_alloc(rowbytes, height);
	
	while (cinfo->output_scanline < cinfo->output_height) {
	    line = image + rowbytes * cinfo->output_scanline;
	    jpeg_read_scanlines(cinfo, &line, 1);
	}
	
	/* native output - vector of integers */
	if (native) {
	    if (pln < 1 || pln > 4 || pln == 2) {
		Rf_error("native output for %d planes is not possible.", pln);
	    }
	    
	    res = PROTECT(allocVector(INTSXP, width * height));
	    if (pln == 4) { /* 4 planes - efficient - just copy it all */
		int *idata = INTEGER(res);
		memcpy(idata, image, rowbytes * height);

		if (need_swap) {
		    int *ide = idata;
		    idata = INTEGER(res);
		    for (; idata < ide; idata++)
			RX_swap32(*idata);
		}
	    } else if (pln == 3) { /* RGB */
		int i, n = width * height, *idata = INTEGER(res);
		unsigned char *c = image;
		for (i = 0; i < n; i++) {
		    *(idata++) = R_RGB((unsigned int) c[0],
				       (unsigned int) c[1],
				       (unsigned int) c[2]);
		    c += 3;
		}
	    } else { /* gray */
		int i, n = width * height, *idata = INTEGER(res);
		unsigned char *c = image;
		for (i = 0; i < n; i++) {
		    *(idata++) = R_RGB((unsigned int) *c,
				       (unsigned int) *c,
				       (unsigned int) *c);
		    c++;
		}
	    }
	    dim = allocVector(INTSXP, 2);
	    INTEGER(dim)[0] = height;
	    INTEGER(dim)[1] = width;
	    setAttrib(res, R_DimSymbol, dim);
	    setAttrib(res, R_ClassSymbol, mkString("nativeRaster"));
	    setAttrib(res, install("channels"), ScalarInteger(pln));
	    UNPROTECT(1);
	} else {
	    int x, y, p, pls = width * height;
	    double *data;

	    res = PROTECT(allocVector(REALSXP, height * rowbytes));
	    data = REAL(res);

	    for(y = 0; y < height; y++)
		for (x = 0; x < width; x++)
		    for (p = 0; p < pln; p++)
			data[y + x * height + p * pls] = ((double)image[y * rowbytes + x * pln + p]) / 255.0;

	    dim = allocVector(INTSXP, (pln > 1) ? 3 : 2);
	    INTEGER(dim)[0] = height;
	    INTEGER(dim)[1] = width;
	    if (pln > 1)
		INTEGER(dim)[2] = pln;
	    setAttrib(res, R_DimSymbol, dim);
	    UNPROTECT(1);
	}
    }

    if (f) fclose(f);

    /* call the finalizer directly so we don't need to wait
       for the garbage collection */
    Rjpeg_fin(dco);
    UNPROTECT(1);

    if (color_space != JCS_GRAYSCALE && color_space != JCS_RGB) {
	SEXP cs0, cs1;
	const char *csn = "unknown";
	PROTECT(res);
	cs0 = Rf_install("color.space");
	if (color_space == JCS_YCbCr) csn = "YCbCr";
	if (color_space == JCS_CMYK) csn = "CMYK";
	if (color_space == JCS_YCCK) csn = "YCbCrK";
	cs1 = PROTECT(mkString(csn));
	setAttrib(res, cs0, cs1);
	UNPROTECT(2);
    }

    return res;
}
