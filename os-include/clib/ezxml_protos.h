/* ezxml_protos.h
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

#ifndef _EZXML_H
#define _EZXML_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif /* EXEC_TYPES_H */

#ifndef PROTO_DOS_H
#include <proto/dos.h>
#endif /* PROTO_DOS_H */

#ifndef LIBRARIES_EZXML_H
#include <libraries/ezxml.h>
#endif /* LIBRARIES_EZXML_H */

ezxml_t ezxml_parse_str(STRPTR, ULONG len);

ezxml_t ezxml_parse_fd(BPTR fd);

ezxml_t ezxml_parse_file(CONST_STRPTR file);

ezxml_t ezxml_child(ezxml_t xml, CONST_STRPTR name);

ezxml_t ezxml_next(ezxml_t xml);

ezxml_t ezxml_idx(ezxml_t xml, ULONG idx);

STRPTR ezxml_name(ezxml_t xml);

STRPTR ezxml_txt(ezxml_t xml);

CONST_STRPTR ezxml_attr(ezxml_t xml, CONST_STRPTR attr);

ezxml_t ezxml_get(ezxml_t xml, ...);

STRPTR ezxml_toxml(ezxml_t xml);

CONST_STRPTR *ezxml_pi(ezxml_t xml, CONST_STRPTR target);

VOID ezxml_free(ezxml_t xml);
    
CONST_STRPTR ezxml_error(ezxml_t xml);

ezxml_t ezxml_new(CONST_STRPTR name);

ezxml_t ezxml_new_d(CONST_STRPTR name);

ezxml_t ezxml_add_child(ezxml_t xml, CONST_STRPTR name, ULONG off);

ezxml_t ezxml_add_child_d(ezxml_t xml, CONST_STRPTR name, ULONG off);

ezxml_t ezxml_set_txt(ezxml_t xml, CONST_STRPTR txt);

ezxml_t ezxml_set_txt_d(ezxml_t xml, CONST_STRPTR txt);

ezxml_t ezxml_set_attr(ezxml_t xml, CONST_STRPTR name, CONST_STRPTR value);

ezxml_t ezxml_set_attr_d(ezxml_t xml, CONST_STRPTR name, CONST_STRPTR value);

ezxml_t ezxml_set_flag(ezxml_t xml, SHORT flag);

ezxml_t ezxml_cut(ezxml_t xml);

ezxml_t ezxml_insert(ezxml_t xml, ezxml_t dest, ULONG off);

ezxml_t ezxml_move(ezxml_t xml, ezxml_t dest, ULONG off);

VOID ezxml_remove(ezxml_t xml);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _EZXML_H */
