/* libfuntable.c
 *
 * Original sources copyright 2004-2006 Aaron Voisine <aaron@voisine.org>
 * ezxml.library copyright 2011-2012 Filip "widelec" Maryjanski
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "libdata.h"

void LIB_Open(void);
void LIB_Close(void);
void LIB_Expunge(void);
void LIB_Reserved(void);
void ezxml_parse_str(void);
void ezxml_parse_fd(void);
void ezxml_parse_file(void);
void ezxml_parse_fp(void);
void ezxml_child(void);
void ezxml_idx(void);
void ezxml_attr(void);
void ezxml_get(void);
void ezxml_toxml(void);
void ezxml_pi(void);
void ezxml_free(void);
void ezxml_error(void);
void ezxml_new(void);
void ezxml_add_child(void);
void ezxml_set_txt(void);
void ezxml_set_attr(void);
void ezxml_set_flag(void);
void ezxml_cut(void);
void ezxml_insert(void);
void ezxml_next(void);
void ezxml_name(void);
void ezxml_txt(void);
void ezxml_new_d(void);
void ezxml_add_child_d(void);
void ezxml_set_txt_d(void);
void ezxml_set_attr_d(void);
void ezxml_move(void);
void ezxml_remove(void);

ULONG LibFuncTable[] =
{
	FUNCARRAY_BEGIN,
	FUNCARRAY_32BIT_NATIVE,
	(ULONG) &LIB_Open,
	(ULONG) &LIB_Close,
	(ULONG) &LIB_Expunge,
	(ULONG) &LIB_Reserved,
	0xffffffff,
	FUNCARRAY_32BIT_SYSTEMV,
	(ULONG) &ezxml_parse_str,
	(ULONG) &ezxml_parse_fd,
	(ULONG) &ezxml_parse_file,
	(ULONG) &ezxml_parse_fp,
	(ULONG) &ezxml_child,
	(ULONG) &ezxml_idx,
	(ULONG) &ezxml_attr,
	(ULONG) &ezxml_get,
	(ULONG) &ezxml_toxml,
	(ULONG) &ezxml_pi,
	(ULONG) &ezxml_free,
	(ULONG) &ezxml_error,
	(ULONG) &ezxml_new,
	(ULONG) &ezxml_add_child,
	(ULONG) &ezxml_set_txt,
	(ULONG) &ezxml_set_attr,
	(ULONG) &ezxml_set_flag,
	(ULONG) &ezxml_cut,
	(ULONG) &ezxml_insert,
	(ULONG) &ezxml_next,
	(ULONG) &ezxml_name,
	(ULONG) &ezxml_txt,
	(ULONG) &ezxml_new_d,
	(ULONG) &ezxml_add_child_d,
	(ULONG) &ezxml_set_txt_d,
	(ULONG) &ezxml_set_attr_d,
	(ULONG) &ezxml_move,
	(ULONG) &ezxml_remove,
	0xffffffff,
	FUNCARRAY_END
};

