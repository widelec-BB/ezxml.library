/* ezxml.library.h
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

#ifndef __EZXML_LIBRARY_H__
#define __EZXML_LIBRARY_H__

#define TO_STRING(x) #x
#define MACRO_TO_STRING(x) TO_STRING(x)

#define	VERSION  8
#define	REVISION 6
#define	DATE     __AMIGADATE__
#define	VERS     "ezxml.library " MACRO_TO_STRING(VERSION)"."MACRO_TO_STRING(REVISION)
#define	VSTRING  VERS " " __AMIGADATE__"© 2011-2012 by Filip \"widelec\" Maryjañski, written by Aaron Voisine\r\n"
#define	VERSTAG  "\0$VER: " VSTRING " " __AMIGADATE__"© 2011-2012 by Filip \"widelec\" Maryjañski, written by Aaron Voisine"

#endif /* __EZXML_LIBRARY_H__ */
