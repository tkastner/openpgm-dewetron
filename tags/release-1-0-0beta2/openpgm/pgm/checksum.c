/* vim:ts=8:sts=8:sw=4:noai:noexpandtab
 *
 * PGM checksum routines
 *
 * Copyright (c) 2006-2008 Miru Limited.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>

#include <glib.h>

#include "pgm/checksum.h"

#ifdef CONFIG_CKSUM_COPY
#	include "pgm/csum-copy.h"
#endif


/* globals */


/* Calculate an IP header style checksum
 */

guint16
pgm_inet_checksum (
	const void*	addr,
	int		len,
	int		csum
	)
{
	int nleft = len;
	const guint16 *w = (guint16*)addr;
	guint answer;
	int sum = csum;

	while (nleft > 1) {
		sum += *w++;
		nleft -= 2;
	}
	if (nleft == 1)
		sum += g_htons(*(guchar *)w<<8);

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	answer = ~sum;
	return (answer);
}

/* Calculate a partial (unfolded) PGM style checksum
 */

guint32
pgm_csum_partial_ (
	const void*	src,
	int		len,
	guint32		csum
	)
{
	guint32 sum = csum;
	guint16 odd_byte;

	while (len > 1) {
		sum += *(guint16*)src;
		src += 2;
		if (sum & 0x80000000)
			sum = (sum & 0xFFFF) + (sum >> 16);
		len -= 2;
	}

	if (len) {
		odd_byte = 0;
		*(guchar*)&odd_byte = *(guchar*)src;
		sum += odd_byte;
	}

	return sum;
}

/* Calculate & copy a partial PGM checksum
 */

guint32
pgm_csum_partial_copy_ (
	const void*	src,
	void*		dst,
	int		len,
	guint32		csum
	)
{
	memcpy (dst, src, len);
	return pgm_csum_partial (dst, len, csum);
}

/* Fold 32 bit checksum accumulator into 16 bit final value.
 */

guint16
pgm_csum_fold (
	guint32		csum
	)
{
	while (csum >> 16)
		csum = (csum & 0xffff) + (csum >> 16);

	return csum == 0xffff ? csum : ~csum;
}

/* Add together two unfolded checksum accumulators
 */

guint32
pgm_csum_block_add (
	guint32		csum,
	guint32		csum2,
	int		offset
	)
{
	if (offset & 1)			/* byte magic on odd offset */
		csum2 = ((csum2 & 0xff00ff) << 8) +
			((csum2 >> 8) & 0xff00ff);

	csum += csum2;
	return csum + (csum < csum2);
}

/* eof */