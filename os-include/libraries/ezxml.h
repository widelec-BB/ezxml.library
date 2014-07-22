/* ezxml.h
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

#ifndef LIBRARIES_EZXML_H
#define LIBRARIES_EZXML_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ezxml *ezxml_t;

struct ezxml {
    STRPTR name;      /* tag name 															  */
    STRPTR *attr;     /* tag attributes { name, value, name, value, ... NULL }  */
    STRPTR txt;       /* tag character content, empty string if none 			  */
    ULONG  off;       /* tag offset from start of parent tag character content  */
    ezxml_t next;     /* next tag with same name in this section at this depth  */
    ezxml_t sibling;  /* next tag with different name in same section and depth */
    ezxml_t ordered;  /* next tag, same section and depth, in original order	  */
    ezxml_t child;    /* head of sub tag list, NULL if none							  */
    ezxml_t parent;   /* parent tag, NULL if current tag is root tag				  */
    SHORT flags;      /* additional information											  */
};

#ifdef __cplusplus
}
#endif

#endif /* LIBRARIES_EZXML_H */
