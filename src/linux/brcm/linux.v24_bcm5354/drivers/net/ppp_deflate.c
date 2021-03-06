/*
 *  ==FILEVERSION 980319==
 *
 * ppp_deflate.c - interface the zlib procedures for Deflate compression
 * and decompression (as used by gzip) to the PPP code.
 * This version is for use with Linux kernel 1.3.X.
 *
 * Copyright (c) 1994 The Australian National University.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, provided that the above copyright
 * notice appears in all copies.  This software is provided without any
 * warranty, express or implied. The Australian National University
 * makes no representations about the suitability of this software for
 * any purpose.
 *
 * IN NO EVENT SHALL THE AUSTRALIAN NATIONAL UNIVERSITY BE LIABLE TO ANY
 * PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF
 * THE AUSTRALIAN NATIONAL UNIVERSITY HAS BEEN ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * THE AUSTRALIAN NATIONAL UNIVERSITY SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE AUSTRALIAN NATIONAL UNIVERSITY HAS NO
 * OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS,
 * OR MODIFICATIONS.
 *
 * From: deflate.c,v 1.1 1996/01/18 03:17:48 paulus Exp
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/init.h>

#include <linux/ppp_defs.h>
#include <linux/ppp-comp.h>

#include <linux/zlib.h>

/*
 * State for a Deflate (de)compressor.
 */
struct ppp_deflate_state {
    int		seqno;
    int		w_size;
    int		unit;
    int		mru;
    z_stream	strm;
    struct compstat stats;
};

#define DEFLATE_OVHD	2		/* Deflate overhead/packet */

static void	*z_comp_alloc __P((unsigned char *options, int opt_len));
static void	*z_decomp_alloc __P((unsigned char *options, int opt_len));
static void	z_comp_free __P((void *state));
static void	z_decomp_free __P((void *state));
static int	z_comp_init __P((void *state, unsigned char *options,
				 int opt_len,
				 int unit, int hdrlen, int debug));
static int	z_decomp_init __P((void *state, unsigned char *options,
				   int opt_len,
				   int unit, int hdrlen, int mru, int debug));
static int	z_compress __P((void *state, unsigned char *rptr,
				unsigned char *obuf,
				int isize, int osize));
static void	z_incomp __P((void *state, unsigned char *ibuf, int icnt));
static int	z_decompress __P((void *state, unsigned char *ibuf,
				int isize, unsigned char *obuf, int osize));
static void	z_comp_reset __P((void *state));
static void	z_decomp_reset __P((void *state));
static void	z_comp_stats __P((void *state, struct compstat *stats));

static void
z_comp_free(arg)
    void *arg;
{
	struct ppp_deflate_state *state = (struct ppp_deflate_state *) arg;

	if (state) {
		zlib_deflateEnd(&state->strm);
		if (state->strm.workspace)
			vfree(state->strm.workspace);
		kfree(state);
		MOD_DEC_USE_COUNT;
	}
}

/*
 * Allocate space for a compressor.
 */
static void *
z_comp_alloc(options, opt_len)
    unsigned char *options;
    int opt_len;
{
	struct ppp_deflate_state *state;
	int w_size;

	if (opt_len != CILEN_DEFLATE
	    || (options[0] != CI_DEFLATE && options[0] != CI_DEFLATE_DRAFT)
	    || options[1] != CILEN_DEFLATE
	    || DEFLATE_METHOD(options[2]) != DEFLATE_METHOD_VAL
	    || options[3] != DEFLATE_CHK_SEQUENCE)
		return NULL;
	w_size = DEFLATE_SIZE(options[2]);
	if (w_size < DEFLATE_MIN_SIZE || w_size > DEFLATE_MAX_SIZE)
		return NULL;

	state = (struct ppp_deflate_state *) kmalloc(sizeof(*state),
						     GFP_KERNEL);
	if (state == NULL)
		return NULL;

	MOD_INC_USE_COUNT;
	memset (state, 0, sizeof (struct ppp_deflate_state));
	state->strm.next_in   = NULL;
	state->w_size         = w_size;
	state->strm.workspace = vmalloc(zlib_deflate_workspacesize());
	if (state->strm.workspace == NULL)
		goto out_free;

	if (zlib_deflateInit2(&state->strm, Z_DEFAULT_COMPRESSION,
			 DEFLATE_METHOD_VAL, -w_size, 8, Z_DEFAULT_STRATEGY)
	    != Z_OK)
		goto out_free;
	return (void *) state;

out_free:
	z_comp_free(state);
	return NULL;
}

static int
z_comp_init(arg, options, opt_len, unit, hdrlen, debug)
    void *arg;
    unsigned char *options;
    int opt_len, unit, hdrlen, debug;
{
	struct ppp_deflate_state *state = (struct ppp_deflate_state *) arg;

	if (opt_len < CILEN_DEFLATE
	    || (options[0] != CI_DEFLATE && options[0] != CI_DEFLATE_DRAFT)
	    || options[1] != CILEN_DEFLATE
	    || DEFLATE_METHOD(options[2]) != DEFLATE_METHOD_VAL
	    || DEFLATE_SIZE(options[2]) != state->w_size
	    || options[3] != DEFLATE_CHK_SEQUENCE)
		return 0;

	state->seqno = 0;
	state->unit  = unit;

	zlib_deflateReset(&state->strm);

	return 1;
}

static void
z_comp_reset(arg)
    void *arg;
{
	struct ppp_deflate_state *state = (struct ppp_deflate_state *) arg;

	state->seqno = 0;
	zlib_deflateReset(&state->strm);
}

int
z_compress(arg, rptr, obuf, isize, osize)
    void *arg;
    unsigned char *rptr;	/* uncompressed packet (in) */
    unsigned char *obuf;	/* compressed packet (out) */
    int isize, osize;
{
	struct ppp_deflate_state *state = (struct ppp_deflate_state *) arg;
	int r, proto, off, olen, oavail;
	unsigned char *wptr;

	/*
	 * Check that the protocol is in the range we handle.
	 */
	proto = PPP_PROTOCOL(rptr);
	if (proto > 0x3fff || proto == 0xfd || proto == 0xfb)
		return 0;

	/* Don't generate compressed packets which are larger than
	   the uncompressed packet. */
	if (osize > isize)
		osize = isize;

	wptr = obuf;

	/*
	 * Copy over the PPP header and store the 2-byte sequence number.
	 */
	wptr[0] = PPP_ADDRESS(rptr);
	wptr[1] = PPP_CONTROL(rptr);
	wptr[2] = PPP_COMP >> 8;
	wptr[3] = PPP_COMP;
	wptr += PPP_HDRLEN;
	wptr[0] = state->seqno >> 8;
	wptr[1] = state->seqno;
	wptr += DEFLATE_OVHD;
	olen = PPP_HDRLEN + DEFLATE_OVHD;
	state->strm.next_out = wptr;
	state->strm.avail_out = oavail = osize - olen;
	++state->seqno;

	off = (proto > 0xff) ? 2 : 3;	/* skip 1st proto byte if 0 */
	rptr += off;
	state->strm.next_in = rptr;
	state->strm.avail_in = (isize - off);

	for (;;) {
		r = zlib_deflate(&state->strm, Z_PACKET_FLUSH);
		if (r != Z_OK) {
			break;
		}
		if (state->strm.avail_out == 0) {
			olen += oavail;
			state->strm.next_out = NULL;
			state->strm.avail_out = oavail = 1000000;
		} else {
			break;		/* all done */
		}
	}
	olen += oavail - state->strm.avail_out;

	/*
	 * See if we managed to reduce the size of the packet.
	 */
	if (olen < isize) {
		state->stats.comp_bytes += olen;
		state->stats.comp_packets++;
	} else {
		state->stats.inc_bytes += isize;
		state->stats.inc_packets++;
		olen = 0;
	}
	state->stats.unc_bytes += isize;
	state->stats.unc_packets++;

	return olen;
}

static void
z_comp_stats(arg, stats)
    void *arg;
    struct compstat *stats;
{
	struct ppp_deflate_state *state = (struct ppp_deflate_state *) arg;

	*stats = state->stats;
}

static void
z_decomp_free(arg)
    void *arg;
{
	struct ppp_deflate_state *state = (struct ppp_deflate_state *) arg;

	if (state) {
		zlib_inflateEnd(&state->strm);
		if (state->strm.workspace)
			kfree(state->strm.workspace);
		kfree(state);
		MOD_DEC_USE_COUNT;
	}
}

/*
 * Allocate space for a decompressor.
 */
static void *
z_decomp_alloc(options, opt_len)
    unsigned char *options;
    int opt_len;
{
	struct ppp_deflate_state *state;
	int w_size;

	if (opt_len != CILEN_DEFLATE
	    || (options[0] != CI_DEFLATE && options[0] != CI_DEFLATE_DRAFT)
	    || options[1] != CILEN_DEFLATE
	    || DEFLATE_METHOD(options[2]) != DEFLATE_METHOD_VAL
	    || options[3] != DEFLATE_CHK_SEQUENCE)
		return NULL;
	w_size = DEFLATE_SIZE(options[2]);
	if (w_size < DEFLATE_MIN_SIZE || w_size > DEFLATE_MAX_SIZE)
		return NULL;

	state = (struct ppp_deflate_state *) kmalloc(sizeof(*state), GFP_KERNEL);
	if (state == NULL)
		return NULL;

	MOD_INC_USE_COUNT;
	memset (state, 0, sizeof (struct ppp_deflate_state));
	state->w_size         = w_size;
	state->strm.next_out  = NULL;
	state->strm.workspace = kmalloc(zlib_inflate_workspacesize(),
					GFP_KERNEL);
	if (state->strm.workspace == NULL)
		goto out_free;

	if (zlib_inflateInit2(&state->strm, -w_size) != Z_OK)
		goto out_free;
	return (void *) state;

out_free:
	z_decomp_free(state);
	return NULL;
}

static int
z_decomp_init(arg, options, opt_len, unit, hdrlen, mru, debug)
    void *arg;
    unsigned char *options;
    int opt_len, unit, hdrlen, mru, debug;
{
	struct ppp_deflate_state *state = (struct ppp_deflate_state *) arg;

	if (opt_len < CILEN_DEFLATE
	    || (options[0] != CI_DEFLATE && options[0] != CI_DEFLATE_DRAFT)
	    || options[1] != CILEN_DEFLATE
	    || DEFLATE_METHOD(options[2]) != DEFLATE_METHOD_VAL
	    || DEFLATE_SIZE(options[2]) != state->w_size
	    || options[3] != DEFLATE_CHK_SEQUENCE)
		return 0;

	state->seqno = 0;
	state->unit  = unit;
	state->mru   = mru;

	zlib_inflateReset(&state->strm);

	return 1;
}

static void
z_decomp_reset(arg)
    void *arg;
{
	struct ppp_deflate_state *state = (struct ppp_deflate_state *) arg;

	state->seqno = 0;
	zlib_inflateReset(&state->strm);
}

/*
 * Decompress a Deflate-compressed packet.
 *
 * Because of patent problems, we return DECOMP_ERROR for errors
 * found by inspecting the input data and for system problems, but
 * DECOMP_FATALERROR for any errors which could possibly be said to
 * be being detected "after" decompression.  For DECOMP_ERROR,
 * we can issue a CCP reset-request; for DECOMP_FATALERROR, we may be
 * infringing a patent of Motorola's if we do, so we take CCP down
 * instead.
 *
 * Given that the frame has the correct sequence number and a good FCS,
 * errors such as invalid codes in the input most likely indicate a
 * bug, so we return DECOMP_FATALERROR for them in order to turn off
 * compression, even though they are detected by inspecting the input.
 */
int
z_decompress(arg, ibuf, isize, obuf, osize)
    void *arg;
    unsigned char *ibuf;
    int isize;
    unsigned char *obuf;
    int osize;
{
	struct ppp_deflate_state *state = (struct ppp_deflate_state *) arg;
	int olen, seq, r;
	int decode_proto, overflow;
	unsigned char overflow_buf[1];

	if (isize <= PPP_HDRLEN + DEFLATE_OVHD) {
		return DECOMP_ERROR;
	}

	/* Check the sequence number. */
	seq = (ibuf[PPP_HDRLEN] << 8) + ibuf[PPP_HDRLEN+1];
	if (seq != (state->seqno & 0xffff)) {
		return DECOMP_ERROR;
	}
	++state->seqno;

	/*
	 * Fill in the first part of the PPP header.  The protocol field
	 * comes from the decompressed data.
	 */
	obuf[0] = PPP_ADDRESS(ibuf);
	obuf[1] = PPP_CONTROL(ibuf);
	obuf[2] = 0;

	/*
	 * Set up to call inflate.  We set avail_out to 1 initially so we can
	 * look at the first byte of the output and decide whether we have
	 * a 1-byte or 2-byte protocol field.
	 */
	state->strm.next_in = ibuf + PPP_HDRLEN + DEFLATE_OVHD;
	state->strm.avail_in = isize - (PPP_HDRLEN + DEFLATE_OVHD);
	state->strm.next_out = obuf + 3;
	state->strm.avail_out = 1;
	decode_proto = 1;
	overflow = 0;

	/*
	 * Call inflate, supplying more input or output as needed.
	 */
	for (;;) {
		r = zlib_inflate(&state->strm, Z_PACKET_FLUSH);
		if (r != Z_OK) {
			return DECOMP_FATALERROR;
		}
		if (state->strm.avail_out != 0)
			break;		/* all done */
		if (decode_proto) {
			state->strm.avail_out = osize - PPP_HDRLEN;
			if ((obuf[3] & 1) == 0) {
				/* 2-byte protocol field */
				obuf[2] = obuf[3];
				--state->strm.next_out;
				++state->strm.avail_out;
			}
			decode_proto = 0;
		} else if (!overflow) {
			/*
			 * We've filled up the output buffer; the only way to
			 * find out whether inflate has any more characters
			 * left is to give it another byte of output space.
			 */
			state->strm.next_out = overflow_buf;
			state->strm.avail_out = 1;
			overflow = 1;
		} else {
			return DECOMP_FATALERROR;
		}
	}

	if (decode_proto) {
		return DECOMP_ERROR;
	}

	olen = osize + overflow - state->strm.avail_out;
	state->stats.unc_bytes += olen;
	state->stats.unc_packets++;
	state->stats.comp_bytes += isize;
	state->stats.comp_packets++;

	return olen;
}

/*
 * Incompressible data has arrived - add it to the history.
 */
static void
z_incomp(arg, ibuf, icnt)
    void *arg;
    unsigned char *ibuf;
    int icnt;
{
	struct ppp_deflate_state *state = (struct ppp_deflate_state *) arg;
	int proto, r;

	/*
	 * Check that the protocol is one we handle.
	 */
	proto = PPP_PROTOCOL(ibuf);
	if (proto > 0x3fff || proto == 0xfd || proto == 0xfb)
		return;

	++state->seqno;

	/*
	 * We start at the either the 1st or 2nd byte of the protocol field,
	 * depending on whether the protocol value is compressible.
	 */
	state->strm.next_in = ibuf + 3;
	state->strm.avail_in = icnt - 3;
	if (proto > 0xff) {
		--state->strm.next_in;
		++state->strm.avail_in;
	}

	r = zlib_inflateIncomp(&state->strm);
	if (r != Z_OK) {
		/* gak! */
		return;
	}

	/*
	 * Update stats.
	 */
	state->stats.inc_bytes += icnt;
	state->stats.inc_packets++;
	state->stats.unc_bytes += icnt;
	state->stats.unc_packets++;
}

/*************************************************************
 * Module interface table
 *************************************************************/

/* These are in ppp_generic.c */
extern int  ppp_register_compressor   (struct compressor *cp);
extern void ppp_unregister_compressor (struct compressor *cp);

/*
 * Procedures exported to if_ppp.c.
 */
struct compressor ppp_deflate = {
	CI_DEFLATE,		/* compress_proto */
	z_comp_alloc,		/* comp_alloc */
	z_comp_free,		/* comp_free */
	z_comp_init,		/* comp_init */
	z_comp_reset,		/* comp_reset */
	z_compress,		/* compress */
	z_comp_stats,		/* comp_stat */
	z_decomp_alloc,		/* decomp_alloc */
	z_decomp_free,		/* decomp_free */
	z_decomp_init,		/* decomp_init */
	z_decomp_reset,		/* decomp_reset */
	z_decompress,		/* decompress */
	z_incomp,		/* incomp */
	z_comp_stats,		/* decomp_stat */
};

struct compressor ppp_deflate_draft = {
	CI_DEFLATE_DRAFT,	/* compress_proto */
	z_comp_alloc,		/* comp_alloc */
	z_comp_free,		/* comp_free */
	z_comp_init,		/* comp_init */
	z_comp_reset,		/* comp_reset */
	z_compress,		/* compress */
	z_comp_stats,		/* comp_stat */
	z_decomp_alloc,		/* decomp_alloc */
	z_decomp_free,		/* decomp_free */
	z_decomp_init,		/* decomp_init */
	z_decomp_reset,		/* decomp_reset */
	z_decompress,		/* decompress */
	z_incomp,		/* incomp */
	z_comp_stats,		/* decomp_stat */
};

int __init deflate_init(void)
{  
        int answer = ppp_register_compressor(&ppp_deflate);
	ppp_register_compressor(&ppp_deflate_draft);
        return answer;
}
     
void __exit deflate_cleanup(void)
{
	ppp_unregister_compressor(&ppp_deflate);
	ppp_unregister_compressor(&ppp_deflate_draft);
}

module_init(deflate_init);
module_exit(deflate_cleanup);
MODULE_LICENSE("Dual BSD/GPL");
