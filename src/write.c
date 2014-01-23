#include "rjcommon.h"

/* alpha - blending:  X * A + (1 - X) * BG   */
#define ABLEND(X, A, BG)  (JSAMPLE) ((((unsigned int)X) * ((unsigned int)A) + ((unsigned int)BG) * (255 - ((unsigned int)A))) / 255)

/* create an R object containing the initialized compression
   structure. The object will ensure proper release of the jpeg struct. */
static SEXP Rjpeg_compress(struct jpeg_compress_struct **cinfo_ptr) {
    SEXP dco;
    struct jpeg_compress_struct *cinfo = (struct jpeg_compress_struct*) malloc(sizeof(struct jpeg_compress_struct));

    if (!cinfo)
	Rf_error("Unable to allocate jpeg decompression structure");
    
    cinfo->err = Rjpeg_new_err();
    
    jpeg_create_compress(cinfo);

    *cinfo_ptr = cinfo;

    dco = PROTECT(R_MakeExternalPtr(cinfo, R_NilValue, R_NilValue));
    R_RegisterCFinalizerEx(dco, Rjpeg_fin, TRUE);
    UNPROTECT(1);
    return dco;
}

METHODDEF(void) dst_noop_fn (struct jpeg_compress_struct *cinfo) { }

METHODDEF(boolean)
empty_output_buffer (struct jpeg_compress_struct *cinfo)
{
    JSAMPLE *buf = (JSAMPLE*) Rjpeg_mem_ptr(cinfo);
    unsigned long size = Rjpeg_mem_size(cinfo);
    size *= 2;
    buf = realloc(buf, size);
    if (!buf)
	Rf_error("Unable to enlarge output buffer to %lu bytes.", size);
    cinfo->dest->next_output_byte = buf + size / 2;
    cinfo->dest->free_in_buffer = size / 2;
    Rjpeg_mem_ptr(cinfo) = buf;
    Rjpeg_mem_size(cinfo) = size;

    return TRUE;
}

/* size of the initial buffer; it is doubled when exceeded */
#define INIT_SIZE 65536

#include <Rinternals.h>
/* for R_RED, ..., R_ALPHA */
#include <R_ext/GraphicsEngine.h>

#define RX_swap32(X) (X) = (((unsigned int)X) >> 24) | ((((unsigned int)X) >> 8) & 0xff00) | (((unsigned int)X) << 24) | ((((unsigned int)X) & 0xff00) << 8)

static unsigned int clip_alpha(double v) {
    if (v < 0.0) v = 0.0;
    if (v > 1.0) v = 1.0;
    return (unsigned int)(v * 255.0);
}

SEXP write_jpeg(SEXP image, SEXP sFn, SEXP sQuality, SEXP sBg, SEXP sColorsp) {
    SEXP res = R_NilValue, dims, dco;
    const char *fn;
    double quality = Rf_asReal(sQuality);
    int planes = 1, width, height, native = 0, raw_array = 0, outpl, bg, cmyk = 0;
    FILE *f = 0;
    struct jpeg_compress_struct *cinfo;
    
    if (Rf_length(sBg) < 1)
	Rf_error("invalid background color specification");
    bg = RGBpar(sBg, 0);

    if (Rf_inherits(image, "nativeRaster") && TYPEOF(image) == INTSXP)
	native = 1;
    
    if (TYPEOF(image) == RAWSXP)
	raw_array = 1;

    if (!native && !raw_array && TYPEOF(image) != REALSXP)
	Rf_error("image must be a matrix or array of raw or real numbers");
    
    dims = Rf_getAttrib(image, R_DimSymbol);
    if (dims == R_NilValue || TYPEOF(dims) != INTSXP || LENGTH(dims) < 2 || LENGTH(dims) > 3)
	Rf_error("image must be a matrix or an array of two or three dimensions");

    if (TYPEOF(sColorsp) == STRSXP && LENGTH(sColorsp) == 1 && !strcmp(CHAR(STRING_ELT(sColorsp, 0)), "CMYK"))
	cmyk = 1;

    if (raw_array && LENGTH(dims) == 3) { /* raw arrays have either bpp, width, height or width, height dimensions */
	planes = INTEGER(dims)[0];
	width = INTEGER(dims)[1];
	height = INTEGER(dims)[2];
    } else { /* others have width, height[, bpp] */
	width = INTEGER(dims)[1];
	height = INTEGER(dims)[0];
	if (LENGTH(dims) == 3)
	    planes = INTEGER(dims)[2];
    }

    if (cmyk && planes != 4)
	Rf_error("CMYK image must have exactly 4 planes");

    if (planes < 1 || planes > 4)
	Rf_error("image must have either 1 (grayscale), 2 (GA), 3 (RGB) or 4 (RGBA) planes");

    if (native && planes > 1)
	Rf_error("native raster must be a matrix");

    if (native) { /* nativeRaster should have a "channels" attribute if it has anything else than 4 channels */
	SEXP cha = Rf_getAttrib(image, Rf_install("channels"));
	if (cmyk)
	    Rf_error("CMYK cannot be represented by nativeRaster");
	if (cha != R_NilValue) {
	    planes = Rf_asInteger(cha);
	    if (planes < 1 || planes > 4)
		planes = 4;
	} else
	    planes = 4;
    }

    /* FIXME: for JPEG 3-channel raw array may also make sense ...*/
    if (raw_array) {
	if (planes != 4)
	    Rf_error("Only RGBA format is supported as raw data");
	native = 1; /* from now on we treat raw arrays like native */
    }

    dco = PROTECT(Rjpeg_compress(&cinfo));

    if (TYPEOF(sFn) == RAWSXP) {
	JSAMPLE *buf = (JSAMPLE*) malloc(INIT_SIZE);
	if (!buf)
	    Rf_error("Unable to allocate output buffer");

	if (!cinfo->dest)
	    cinfo->dest = (struct jpeg_destination_mgr *)
		(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
					    sizeof(struct jpeg_destination_mgr));
	cinfo->dest->init_destination = dst_noop_fn;
	cinfo->dest->empty_output_buffer = empty_output_buffer;
	/* unfortunately the design of dest is flawed (to say it mildly)
	   since it doesn't call term on error/abort so it's useless */
	cinfo->dest->term_destination = dst_noop_fn;
	cinfo->dest->next_output_byte = buf;
	cinfo->dest->free_in_buffer = INIT_SIZE;
	Rjpeg_mem_ptr(cinfo) = buf;
	Rjpeg_mem_size(cinfo) = INIT_SIZE;
    } else {
	if (TYPEOF(sFn) != STRSXP || LENGTH(sFn) < 1) Rf_error("invalid filename");
	fn = CHAR(STRING_ELT(sFn, 0));
	f = fopen(fn, "wb");
	if (!f) Rf_error("unable to create %s", fn);
	jpeg_stdio_dest(cinfo, f);
    }

    /* JPEG only supports RGB or G (apart from CMYK) */
    outpl = cmyk ? 4 : ((planes > 2) ? 3 : 1);

    cinfo->image_width = width;
    cinfo->image_height = height;
    cinfo->input_components = outpl;
    cinfo->in_color_space = cmyk ? JCS_CMYK : ((outpl == 3) ? JCS_RGB : JCS_GRAYSCALE);

    jpeg_set_defaults(cinfo);

    if (quality < 0.0) quality = 0.0;
    if (quality > 1.0) quality = 1.0;
    if (isnan(quality)) quality = 0.7;
    jpeg_set_quality(cinfo, (int) (quality * 100.0 + 0.49), FALSE);
    /* jpeg_simple_progression(cinfo); optional */
    jpeg_start_compress(cinfo, TRUE);

    {
	int rowbytes = width * outpl;
	JSAMPROW   row_pointer;
	JSAMPLE *  flat_rows;
	
	flat_rows = (JSAMPLE*) R_alloc(height, width * outpl);
	
	if (!native) {
	    int x, y, p, pls = width * height;
	    double *data = REAL(image);
	    for(y = 0; y < height; y++)
		for (x = 0; x < width; x++)
		    for (p = 0; p < outpl; p++) {
			double v = data[y + x * height + p * pls];
			if (v < 0) v = 0;
			if (v > 255.0) v = 1.0;
			flat_rows[y * rowbytes +  x * outpl + p] = (unsigned char)(v * 255.0 + 0.5);
		    }
	    /* if there is alpha, we need to blend the background */
	    if (planes == 2) {
		for(y = 0; y < height; y++)
		    for (x = 0; x < width; x++) {
			unsigned int a = clip_alpha(data[y + x * height + pls]);
			if (a != 255) flat_rows[y * rowbytes + x] = ABLEND(flat_rows[y * rowbytes + x], a, R_RED(bg));
		    }
	    } else if (planes == 4 && !cmyk) {
		for(y = 0; y < height; y++)
		    for (x = 0; x < width; x++) {
			unsigned int a = clip_alpha(data[y + x * height + 3 * pls]);
			if (a != 255) {
			    flat_rows[y * rowbytes + x * 3]     = ABLEND(flat_rows[y * rowbytes + x * 3]    , a, R_RED(bg));
			    flat_rows[y * rowbytes + x * 3 + 1] = ABLEND(flat_rows[y * rowbytes + x * 3 + 1], a, R_GREEN(bg));
			    flat_rows[y * rowbytes + x * 3 + 2] = ABLEND(flat_rows[y * rowbytes + x * 3 + 2], a, R_BLUE(bg));
			}
		    }
	    }
	} else {
	    if (planes == 4 && cmyk) { /* CMYK - from raw input, not really native */
		memcpy(flat_rows, (char*) INTEGER(image), rowbytes * height);
	    } else if (planes == 4) { /* RGBA */
		int x, y, *idata = INTEGER(image);
		for (y = 0; y < height; y++)
		    for (x = 0; x < rowbytes; idata++) {
			flat_rows[y * rowbytes + x++] = ABLEND(R_RED(*idata),   R_ALPHA(*idata), R_RED(bg));
			flat_rows[y * rowbytes + x++] = ABLEND(R_GREEN(*idata), R_ALPHA(*idata), R_GREEN(bg));
			flat_rows[y * rowbytes + x++] = ABLEND(R_BLUE(*idata),  R_ALPHA(*idata), R_BLUE(bg));
		    }
	    } else if (planes == 3) { /* RGB */
		int x, y, *idata = INTEGER(image);
		for (y = 0; y < height; y++)
		    for (x = 0; x < rowbytes; idata++) {
			flat_rows[y * rowbytes + x++] = R_RED(*idata);
			flat_rows[y * rowbytes + x++] = R_GREEN(*idata);
			flat_rows[y * rowbytes + x++] = R_BLUE(*idata);
		    }
	    } else if (planes == 2) { /* GA */
		int x, y, *idata = INTEGER(image);
		for (y = 0; y < height; y++)
		    for (x = 0; x < rowbytes; idata++)
			flat_rows[y * rowbytes + x++] = ABLEND(R_RED(*idata), R_ALPHA(*idata), R_RED(bg));
	    } else { /* gray */
		int x, y, *idata = INTEGER(image);
		for (y = 0; y < height; y++)
		    for (x = 0; x < rowbytes; idata++)
			flat_rows[y * rowbytes + x++] = R_RED(*idata);
	    }
	}
	
        while (cinfo->next_scanline < cinfo->image_height) {
            row_pointer = flat_rows + cinfo->next_scanline * rowbytes;
            jpeg_write_scanlines(cinfo, &row_pointer, 1);
	}
    }

    jpeg_finish_compress(cinfo);

    if (f) { /* if it is a file, just return */
	fclose(f);
	Rjpeg_fin(dco);
	UNPROTECT(1);
	return R_NilValue;
    }
    
    {
	unsigned long len = (char*)cinfo->dest->next_output_byte - (char*)Rjpeg_mem_ptr(cinfo);
	res = Rf_allocVector(RAWSXP, len);
	memcpy(RAW(res), Rjpeg_mem_ptr(cinfo), len);
    }
    
    UNPROTECT(1);
    return res;
}
