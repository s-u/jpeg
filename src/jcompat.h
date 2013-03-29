/* compatibility functions for older libjpeg versions */

#ifndef J_COMPAT_H
#define J_COMPAT_H

#include <jpeglib.h>
#include <jerror.h>

/* memory-based source is new in v8 so we need to provide it for
   older jpeglib versions since they are still quite common */
#if (JPEG_LIB_VERSION < 80)

METHODDEF(void) noop_fn (struct jpeg_decompress_struct *cinfo) { }

static JOCTET eoi_buf[2] = { 255, JPEG_EOI };

METHODDEF(boolean) /* attempt to read beyond EOF - respond with EOI */
fill_input_buffer (struct jpeg_decompress_struct *cinfo)
{
    WARNMS(cinfo, JWRN_JPEG_EOF);
    cinfo->src->next_input_byte = eoi_buf;
    cinfo->src->bytes_in_buffer = sizeof(eoi_buf);
    return TRUE;
}

METHODDEF(void)
skip_input_data (struct jpeg_decompress_struct *cinfo, long num_bytes)
{
    struct jpeg_source_mgr * src = cinfo->src;

    if (num_bytes > 0) {
	/* is the skip beyond the buffer ? */
	if (num_bytes > (long) src->bytes_in_buffer) {
	    fill_input_buffer(cinfo);
	    /* it's an error anyway so bail out */
	    return;
	}
	src->next_input_byte += (size_t) num_bytes;
	src->bytes_in_buffer -= (size_t) num_bytes;
    }
}

/* libjpeg-turbo 1.2.90 reportedly breaks as it is doing something nasty
   with the JPEG_LIB_VERSION and it defines jpeg_mem_src even though
   it masquarades as jpeg < 8 ... strange, but to work around it we make sure
   that our compatibility layer uses a different symbol name */
#ifdef jpeg_mem_src
#undef jpeg_mem_src
#endif
#define jpeg_mem_src jcompat_jpeg_mem_src

static void jpeg_mem_src (struct jpeg_decompress_struct *cinfo,
			  unsigned char *inbuffer, unsigned long insize) {
    struct jpeg_source_mgr *src;
    if (!insize)
	ERREXIT(cinfo, JERR_INPUT_EMPTY);
    
    if (!cinfo->src)
	src = cinfo->src = (struct jpeg_source_mgr *)
	    (*cinfo->mem->alloc_small) ((struct jpeg_common_struct*) cinfo, JPOOL_PERMANENT,
					sizeof(struct jpeg_source_mgr));
    else
	src = cinfo->src;

    src->init_source = noop_fn;
    src->fill_input_buffer = fill_input_buffer;
    src->skip_input_data = skip_input_data;
    src->resync_to_restart = jpeg_resync_to_restart;
    src->term_source = noop_fn;
    src->bytes_in_buffer = (size_t) insize;
    src->next_input_byte = (JOCTET *) inbuffer;
}

#endif

#endif
