/* libfunctions.c
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

//+ AUTODOC/ezxml.library
/****** AUTODOC/ezxml.library ***********************************
* DESCRIPTION
*  ezXML - XML Parsing C Library
*  version 0.8.5
*
*  ezXML is a C library for parsing XML documents inspired by simpleXML for PHP.
*  As the name implies, it's easy to use. It's ideal for parsing XML configuration
*  files or REST web service responses. It's also fast and lightweight
*
* WARNINGS
*  Known Limitations:
*
*   - ezXML is not a validating parser
*
*   - Loads the entire XML document into memory at once and does not support
*     incremental parsing. Large XML files can still be handled though through
*     ezxml_parse_file() and ezxml_parse_fd().
*
*   - Does not currently recognize all possible well-formedness errors. It should
*     correctly handle all well-formed XML documents and will either ignore or halt
*     XML processing on well-formedness errors. More well-formedness checking will
*     be added in subsiquent releases.
*
*   - In making the character content of tags easy to access, there is no way
*     provided to keep track of the location of sub tags relative to the character
*     data. Example:
*
*         <doc>line one<br/>
*         line two</doc>
*
*     The character content of the doc tag is reported as "line one\nline two", and
*     <br/> is reported as a sub tag, but the location of <br/> within the
*     character data is not. The function ezxml_toxml() will convert an ezXML
*     structure back to XML with sub tag locations intact.
*
* SEE ALSO
*  License:
*   See included "COPYING" file.
*
* AUTHOR
*  Original code was written by Aaron Voisine.
*  The ezxml.library was created by Filip "widelec" Maryjañski.
*
* NOTES
*  Please notice that "ezxml_t structure" is only a pointer to ezxml structure. This
*  is defined in include/libraries/ezxml.h.
*
* EXAMPLE
*  Given the following example XML document:
*
*   <?xml version="1.0"?>
*    <formula1>
*      <team name="McLaren">
*        <driver>
*         <name>Kimi Raikkonen</name>
*         <points>112</points>
*        </driver>
*        <driver>
*         <name>Juan Pablo Montoya</name>
*         <points>60</points>
*        </driver>
*       </team>
*    </formula1>
*
*  This code snippet prints out a list of drivers, which team they drive for,
*  and how many championship points they have:
*
*   ezxml_t f1 = ezxml_parse_file("formula1.xml"), team, driver;
*   const char *teamname;
*
*   for(team = ezxml_child(f1, "team"); team; team = team->next)
*   {
*      teamname = ezxml_attr(team, "name");
*      for(driver = ezxml_child(team, "driver"); driver; driver = driver->next)
*      {
*        printf("%s, %s: %s\n", ezxml_child(driver, "name")->txt, teamname,
*         ezxml_child(driver, "points")->txt);
*      }
*   }
*   ezxml_free(f1);
*
*  Alternately, the following would print out the name of the second driver on the
*  first team:
*
*  ezxml_t f1 = ezxml_parse_file("formula1.xml");
*
*  printf("%s\n", ezxml_get(f1, "team", 0, "driver", 1, "name", -1)->txt);
*  ezxml_free(f1);
*
*  The -1 indicates the end of the argument list. That's pretty much all
*  there is to it.
********************************************************************************
*
*/
//-
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <proto/dos.h>
#include <libraries/dos.h>
#include "os-include/libraries/ezxml.h"
#include "libdata.h"
#include "debug.h"

#define EZXML_BUFSIZE 1024       // size of internal memory buffers
#define EZXML_NAMEM   0x80       // name is malloced
#define EZXML_TXTM    0x40       // txt is malloced
#define EZXML_DUP     0x20       // attribute name and value are strduped
#define EZXML_WS      "\t\r\n "  // whitespace
#define EZXML_ERRL    128        // maximum error string length
#define EZXML_NOMMAP

#define malloc(x) AllocVec(x, MEMF_PUBLIC | MEMF_CLEAR)
#define free(x) FreeVec(x)
#define memcpy(x, y, z) MemCopy(x, y, z, MyLibBase)
#define realloc(x, y) ReAlloc(x, y, MyLibBase)
#define strdup(x) StrNew(x, MyLibBase)

typedef struct ezxml_root *ezxml_root_t;
struct ezxml_root         // additional data for the root tag
{
	struct ezxml xml;      // is a super-struct built on top of ezxml struct
	ezxml_t cur;           // current xml tree insertion point
	STRPTR m;              // original xml string
	ULONG len;             // length of allocated memory for mmap, -1 for malloc
	STRPTR u;              // UTF-8 conversion of string if original was UTF-16
	STRPTR s;              // start of work area
	STRPTR e;              // end of work area
	STRPTR *ent;           // general entities (ampersand sequences)
	STRPTR **attr;         // default attributes
	STRPTR **pi;           // processing instructions
	SHORT standalone;      // non-zero if <?xml standalone="yes"?>
	BYTE err[EZXML_ERRL];  // error string
};

char *EZXML_NIL[] = {NULL}; // empty, null terminated array of strings

ezxml_t ezxml_child(ezxml_t xml, CONST_STRPTR name);
ezxml_t ezxml_idx(ezxml_t xml, ULONG idx);
CONST_STRPTR ezxml_attr(ezxml_t xml, CONST_STRPTR attr);
ezxml_t ezxml_vget(ezxml_t xml, va_list ap);
ezxml_t ezxml_get(ezxml_t xml, ...);
CONST_STRPTR *ezxml_pi(ezxml_t xml, CONST_STRPTR target);
ezxml_t ezxml_err(ezxml_root_t root, STRPTR s, CONST_STRPTR err, ...);
STRPTR ezxml_decode(STRPTR s, STRPTR *ent, BYTE t, struct LibBase *MyLibBase);
VOID ezxml_open_tag(ezxml_root_t root, STRPTR name, STRPTR *attr, struct LibBase *MyLibBase);
VOID ezxml_char_content(ezxml_root_t root, STRPTR s, ULONG len, BYTE t, struct LibBase *MyLibBase);
ezxml_t ezxml_close_tag(ezxml_root_t root, STRPTR name, STRPTR s);
ULONG ezxml_ent_ok(STRPTR name, STRPTR s, STRPTR *ent);
VOID ezxml_proc_inst(ezxml_root_t root, STRPTR s, ULONG len, struct LibBase *MyLibBase);
SHORT ezxml_internal_dtd(ezxml_root_t root, STRPTR s, ULONG len, struct LibBase *MyLibBase);
STRPTR ezxml_str2utf8(STRPTR *s, ULONG *len, struct LibBase *MyLibBase);
VOID ezxml_free_attr(STRPTR *attr, struct LibBase *MyLibBase);
ezxml_t ezxml_parse_str(STRPTR s, ULONG len, struct LibBase *MyLibBase);
ezxml_t ezxml_parse_fp(BPTR fp, struct LibBase *MyLibBase);
ezxml_t ezxml_parse_fd(BPTR fd, struct LibBase *MyLibBase);
ezxml_t ezxml_parse_file(CONST_STRPTR file, struct LibBase *MyLibBase);
STRPTR ezxml_ampencode(CONST_STRPTR s, ULONG len, STRPTR *dst, ULONG *dlen, ULONG *max, SHORT a, struct LibBase *MyLibBase);
STRPTR ezxml_toxml_r(ezxml_t xml, STRPTR *s, ULONG *len, ULONG *max, ULONG start, STRPTR **attr, struct LibBase *MyLibBase);
STRPTR ezxml_toxml(ezxml_t xml, struct LibBase *MyLibBase);
VOID ezxml_free(ezxml_t xml, struct LibBase *MyLibBase);
CONST_STRPTR ezxml_error(ezxml_t xml);
ezxml_t ezxml_new(CONST_STRPTR name, struct LibBase *MyLibBase);
ezxml_t ezxml_insert(ezxml_t xml, ezxml_t dest, ULONG off);
ezxml_t ezxml_add_child(ezxml_t xml, CONST_STRPTR name, ULONG off, struct LibBase *MyLibBase);
ezxml_t ezxml_set_txt(ezxml_t xml, CONST_STRPTR txt, struct LibBase *MyLibBase);
ezxml_t ezxml_set_flag(ezxml_t xml, SHORT flag);
ezxml_t ezxml_set_attr(ezxml_t xml, CONST_STRPTR name, CONST_STRPTR value, struct LibBase *MyLibBase);
ezxml_t ezxml_cut(ezxml_t xml);
ezxml_t ezxml_next(ezxml_t xml);
STRPTR ezxml_name(ezxml_t xml);
STRPTR ezxml_txt(ezxml_t xml);
ezxml_t ezxml_new_d(CONST_STRPTR name, struct LibBase *MyLibBase);
ezxml_t ezxml_add_child_d(ezxml_t xml, CONST_STRPTR name, ULONG off, struct LibBase *MyLibBase);
ezxml_t ezxml_set_txt_d(ezxml_t xml, CONST_STRPTR txt, struct LibBase *MyLibBase);
ezxml_t ezxml_set_attr_d(ezxml_t xml, CONST_STRPTR name, CONST_STRPTR value, struct LibBase *MyLibBase);
ezxml_t ezxml_move(ezxml_t xml, ezxml_t dest, ULONG off);
VOID ezxml_remove(ezxml_t xml, struct LibBase *MyLibBase);


static inline APTR MemCopy(APTR dest, CONST_APTR src, ULONG size, struct LibBase *MyLibBase)
{
	CopyMem(src, dest, size);
	return dest;
}


static APTR ReAlloc(APTR ptr, ULONG size, struct LibBase *MyLibBase)
{
	APTR result = NULL;
	ULONG* temp = (ULONG*) ptr;
	ULONG copy_size;

	if(ptr == NULL || size == 0)return NULL;

	if((result = AllocVec(size, MEMF_PUBLIC | MEMF_CLEAR)) != NULL)
	{
		if(size < ((ULONG)temp[-1] - sizeof(ULONG))) copy_size = size;
		else copy_size = ((ULONG)temp[-1] - sizeof(ULONG));

		CopyMem(ptr, result, copy_size);
		FreeVec(ptr);
	}
	else
		tprintf("out of memory - ReAlloc()!");

	return result;
}

STRPTR StrCopy(STRPTR s, STRPTR d)
{
	while(*d++ = *s++);

	return (--d);
}

ULONG StrLen(STRPTR s)
{
	STRPTR v = s;

	while(*v) v++;

	return (ULONG)(v - s);  // will fail for strings longer than 4 GB ;-)
}

STRPTR StrNew(STRPTR str, struct LibBase* MyLibBase)
{
	STRPTR n = NULL;
	ULONG len = StrLen(str);

	if(len == 0) return NULL;

	if((n = AllocVec(len + sizeof(char), MEMF_ANY | MEMF_CLEAR)))
		StrCopy(str, n);

	return n;
}

//+ ezxml.library/ezxml_child
/****** ezxml.library/ezxml_child *********************************************
* NAME
*  ezxml_child() - searches for next tag (V8)
*
* SYNOPSIS
*  ezxml_child(xml, name);
*  ezxml_t ezxml_child(ezxml_t, CONST_STRPTR);
*
* FUNCTION
*  This function search for next child tag (one level deeper)
*  with given name.
*
* INPUTS
*  xml - ezxml_t structure
*  name - name of tag
*
* RESULT
*	Return a ezxml_t structure of found tag or NULL on failure.
*
* SEE ALSO
*  ezxml_next() ezxml_idx()
********************************************************************************
*
*/
//-
ezxml_t ezxml_child(ezxml_t xml, CONST_STRPTR name)
{
	xml = (xml) ? xml->child : NULL;
	while(xml && strcmp(name, xml->name)) xml = xml->sibling;
	return xml;
}

//+ ezxml.library/ezxml_idx
/****** ezxml.library/ezxml_idx ************************************************
* NAME
*  ezxml_idx() - searches for next tag with this same name (V8)
*
* SYNOPSIS
*  ezxml_idx(xml, index)
*  ezxml_t ezxml_idx(ezxml_t, ULONG)
*
* FUNCTION
*  Searches for the index-th tag of the same name in the same section and depth.
*
* INPUTS
*  xml - ezxml_t structure
*  index - sequence number of tag
*
* RESULT
*	Return a ezxml_t structure of found tag or NULL on failure.
*
* NOTES
*  An index of 0 returns the tag given.
*
* SEE ALSO
*  ezxml_child() ezxml_next()
********************************************************************************
*
*/
//-
ezxml_t ezxml_idx(ezxml_t xml, ULONG idx)
{
	for(; xml && idx; idx--) xml = xml->next;
	return xml;
}

//+ ezxml.library/ezxml_attr
/****** ezxml.library/ezxml_attr ***********************************************
* NAME
*  ezxml_attr() - reads value of attribute (V8)
*
* SYNOPSIS
* ezxml_attr(xml, attr);
* CONST_STRPTR ezxml_attr(ezxml_t, CONST_STRPTR);
*
* FUNCTION
*  Reads value of the requested tag attribute.
*
* INPUTS
*  xml  - ezxml_t tag structure
*  attr - name of attribute
*
* RESULT
*	Return a pointer to string contains value of requested tag attribute.
*
* NOTES
*  Return NULL if not found.
*
* SEE ALSO
*  ezxml_get()
********************************************************************************
*
*/
//-
CONST_STRPTR ezxml_attr(ezxml_t xml, CONST_STRPTR attr)
{
	ULONG i = 0, j = 1;
	ezxml_root_t root = (ezxml_root_t)xml;

	if(!xml || !xml->attr) return NULL;
	while(xml->attr[i] && strcmp(attr, xml->attr[i])) i += 2;
	if(xml->attr[i]) return xml->attr[i + 1];  // found attribute

	while(root->xml.parent) root = (ezxml_root_t)root->xml.parent;  // root tag
	for(i = 0; root->attr[i] && strcmp(xml->name, root->attr[i][0]); i++);
	if(!root->attr[i]) return NULL;  // no matching default attributes
	while(root->attr[i][j] && strcmp(attr, root->attr[i][j])) j += 3;
	return (root->attr[i][j]) ? root->attr[i][j + 1] : NULL; // found default
}

// same as ezxml_get but takes an already initialized va_list
ezxml_t ezxml_vget(ezxml_t xml, va_list ap)
{
	STRPTR name = va_arg(ap, STRPTR);
	LONG idx = -1;

	if(name && *name)
	{
		idx = va_arg(ap, int);
		xml = ezxml_child(xml, name);
	}
	return (idx < 0) ? xml : ezxml_vget(ezxml_idx(xml, idx), ap);
}

//+ ezxml.library/ezxml_get
/****** ezxml.library/ezxml_get ***********************************************
* NAME
*  ezxml_get() - traverses the ezxml sturcture to retrieve a specific subtag (V8)
*
* SYNOPSIS
*  ezxml_get(xml, ...);
*  ezxml_t ezxml_get(ezxml_t, ...);
*
* FUNCTION
*  Traverses the ezxml sturcture to retrieve a specific subtag. Takes a
*  variable length list of tag names and indexes. The argument list must be
*  terminated by either an index of -1 or an empty string tag name.
*
* INPUTS
*  xml - ezxml_t tag structure
*  ... - variable length list of tag names and indexes
*
* RESULT
*	Return a pointer to ezxml_t contains requested tag.
*
* NOTES
*  Don't forget to end list of parameters with -1!
*  Returns NULL if not found.
*
* SEE ALSO
*  ezxml_attr()
*
* EXAMPLE
*  title = ezxml_get(library, "shelf", 0, "book", 2, "title", -1);
*  This retrieves the title of the 3rd book on the 1st shelf of library.
********************************************************************************
*
*/
//-
ezxml_t ezxml_get(ezxml_t xml, ...)
{
	va_list ap;
	ezxml_t r;

	va_start(ap, xml);
	r = ezxml_vget(xml, ap);
	va_end(ap);
	return r;
}

//+ ezxml.library/ezxml_pi
/****** ezxml.library/ezxml_pi **************************************************
* NAME
*  ezxml_pi() - returns a NULL terminated array of processing instructions for the given target (V8)
*
* SYNOPSIS
*  ezxml_pi(xml, target);
*  CONST_STRPTR *ezxml_pi(ezxml_t, CONST_STRPTR);
*
* INPUTS
*  xml    - ezxml_t tag structure
*  target - pointer to string obtaining target
*
* RESULT
*	Returns a NULL terminated array of processing instructions for the given target
********************************************************************************
*
*/
//-
CONST_STRPTR *ezxml_pi(ezxml_t xml, CONST_STRPTR target)
{
	ezxml_root_t root = (ezxml_root_t)xml;
	int i = 0;

	if(!root) return (const char **)EZXML_NIL;
	while(root->xml.parent) root = (ezxml_root_t)root->xml.parent;  // root tag
	while(root->pi[i] && strcmp(target, root->pi[i][0])) i++;  // find target
	return (const char **)((root->pi[i]) ? root->pi[i] + 1 : EZXML_NIL);
}

// set an error string and return root
ezxml_t ezxml_err(ezxml_root_t root, STRPTR s, CONST_STRPTR err, ...)
{
	va_list ap;
	int line = 1;
	STRPTR *t;
	BYTE fmt[EZXML_ERRL];

	for(t = root->s; t < s; t++) if(*t == '\n') line++;
	snprintf(fmt, EZXML_ERRL, "[error near line %d]: %s", line, err);

	va_start(ap, err);
	vsnprintf(root->err, EZXML_ERRL, fmt, ap);
	va_end(ap);

	return &root->xml;
}

// Recursively decodes entity and character references and normalizes new lines
// ent is a null terminated array of alternating entity names and values. set t
// to '&' for general entity decoding, '%' for parameter entity decoding, 'c'
// for cdata sections, ' ' for attribute normalization, or '*' for non-cdata
// attribute normalization. Returns s, or if the decoded string is longer than
// s, returns a malloced string that must be freed.
STRPTR ezxml_decode(STRPTR s, STRPTR *ent, BYTE t, struct LibBase *MyLibBase)
{
	STRPTR e, r = s, m = s;
	long b, c, d, l;

	for(; *s; s++)    // normalize line endings
	{
		while(*s == '\r')
		{
			*(s++) = '\n';
			if(*s == '\n') memmove(s, (s + 1), strlen(s));
		}
	}

	for(s = r; ;)
	{
		while(*s && *s != '&' && (*s != '%' || t != '%') && !isspace(*s)) s++;

		if(!*s) break;
		else if(t != 'c' && !strncmp(s, "&#", 2))    // character reference
		{
			if(s[2] == 'x') c = strtol(s + 3, &e, 16);  // base 16
			else c = strtol(s + 2, &e, 10); // base 10
			if(!c || *e != ';')
			{
				s++;    // not a character ref
				continue;
			}
			if(c < 0x80) *(s++) = c;  // US-ASCII subset
			else   // multi-byte UTF-8 sequence
			{
				for(b = 0, d = c; d; d /= 2) b++;  // number of bits in c
				b = (b - 2) / 5; // number of bytes in payload
				*(s++) = (0xFF << (7 - b)) | (c >> (6 * b)); // head
				while(b) *(s++) = 0x80 | ((c >> (6 * --b)) & 0x3F);  // payload
			}

			memmove(s, strchr(s, ';') + 1, strlen(strchr(s, ';')));
		}
		else if((*s == '&' && (t == '&' || t == ' ' || t == '*')) ||
		        (*s == '%' && t == '%'))   // entity reference
		{
			for(b = 0; ent[b] && strncmp(s + 1, ent[b], strlen(ent[b]));
			        b += 2); // find entity in entity list

			if(ent[b++])    // found a match
			{
				if((c = strlen(ent[b])) - 1 > (e = strchr(s, ';')) - s)
				{
					l = (d = (s - r)) + c + strlen(e); // new length
					r = (r == m) ? strcpy(malloc(l), r) : realloc(r, l);
					e = strchr((s = r + d), ';'); // fix up pointers
				}

				memmove(s + c, e + 1, strlen(e)); // shift rest of string
				strncpy(s, ent[b], c); // copy in replacement text
			}
			else s++; // not a known entity
		}
		else if((t == ' ' || t == '*') && isspace(*s)) *(s++) = ' ';
		else s++; // no decoding needed
	}

	if(t == '*')    // normalize spaces for non-cdata attributes
	{
		for(s = r; *s; s++)
		{
			if((l = strspn(s, " "))) memmove(s, s + l, strlen(s + l) + 1);
			while(*s && *s != ' ') s++;
		}
		if(--s >= r && *s == ' ') *s = '\0';  // trim any trailing space
	}
	return r;
}

// called when parser finds start of new tag
VOID ezxml_open_tag(ezxml_root_t root, STRPTR name, STRPTR *attr, struct LibBase *MyLibBase)
{
	ezxml_t xml = root->cur;

	if(xml->name) xml = ezxml_add_child(xml, name, strlen(xml->txt), MyLibBase);
	else xml->name = name; // first open tag

	xml->attr = attr;
	root->cur = xml; // update tag insertion point
}

// called when parser finds character content between open and closing tag
VOID ezxml_char_content(ezxml_root_t root, STRPTR s, ULONG len, BYTE t, struct LibBase *MyLibBase)
{
	ezxml_t xml = root->cur;
	char *m = s;
	ULONG l;

	if(!xml || !xml->name || !len) return;  // sanity check

	s[len] = '\0'; // null terminate text (calling functions anticipate this)
	len = strlen(s = ezxml_decode(s, root->ent, t, MyLibBase)) + 1;

	if(!*(xml->txt)) xml->txt = s;  // initial character content
	else   // allocate our own memory and make a copy
	{
		xml->txt = (xml->flags & EZXML_TXTM) // allocate some space
		           ? realloc(xml->txt, (l = strlen(xml->txt)) + len)
		           : strcpy(malloc((l = strlen(xml->txt)) + len), xml->txt);
		strcpy(xml->txt + l, s); // add new char content
		if(s != m) free(s);  // free s if it was malloced by ezxml_decode()
	}

	if(xml->txt != m) ezxml_set_flag(xml, EZXML_TXTM);
}

// called when parser finds closing tag
ezxml_t ezxml_close_tag(ezxml_root_t root, STRPTR name, STRPTR s)
{
	if(!root->cur || !root->cur->name || strcmp(name, root->cur->name))
		return ezxml_err(root, s, "unexpected closing tag </%s>", name);

	root->cur = root->cur->parent;
	return NULL;
}

// checks for circular entity references, returns non-zero if no circular
// references are found, zero otherwise
ULONG ezxml_ent_ok(STRPTR name, STRPTR s, STRPTR *ent)
{
	int i;

	for(; ; s++)
	{
		while(*s && *s != '&') s++;  // find next entity reference
		if(!*s) return 1;
		if(!strncmp(s + 1, name, strlen(name))) return 0;  // circular ref.
		for(i = 0; ent[i] && strncmp(ent[i], s + 1, strlen(ent[i])); i += 2);
		if(ent[i] && !ezxml_ent_ok(name, ent[i + 1], ent)) return 0;
	}
}

// called when the parser finds a processing instruction
VOID ezxml_proc_inst(ezxml_root_t root, STRPTR s, ULONG len, struct LibBase *MyLibBase)
{
	int i = 0, j = 1;
	char *target = s;

	s[len] = '\0'; // null terminate instruction
	if(*(s += strcspn(s, EZXML_WS)))
	{
		*s = '\0'; // null terminate target
		s += strspn(s + 1, EZXML_WS) + 1; // skip whitespace after target
	}

	if(!strcmp(target, "xml"))    // <?xml ... ?>
	{
		if((s = strstr(s, "standalone")) && !strncmp(s + strspn(s + 10,
		        EZXML_WS "='\"") + 10, "yes", 3)) root->standalone = 1;
		return;
	}

	if(!root->pi[0]) *(root->pi = malloc(sizeof(char **))) = NULL;  //first pi

	while(root->pi[i] && strcmp(target, root->pi[i][0])) i++;  // find target
	if(!root->pi[i])    // new target
	{
		root->pi = realloc(root->pi, sizeof(char **) * (i + 2));
		root->pi[i] = malloc(sizeof(char *) * 3);
		root->pi[i][0] = target;
		root->pi[i][1] = (char *)(root->pi[i + 1] = NULL); // terminate pi list
		root->pi[i][2] = strdup(""); // empty document position list
	}

	while(root->pi[i][j]) j++;  // find end of instruction list for this target
	root->pi[i] = realloc(root->pi[i], sizeof(char *) * (j + 3));
	root->pi[i][j + 2] = realloc(root->pi[i][j + 1], j + 1);
	strcpy(root->pi[i][j + 2] + j - 1, (root->xml.name) ? ">" : "<");
	root->pi[i][j + 1] = NULL; // null terminate pi list for this target
	root->pi[i][j] = s; // set instruction
}

// called when the parser finds an internal doctype subset
SHORT ezxml_internal_dtd(ezxml_root_t root, STRPTR s, ULONG len, struct LibBase *MyLibBase)
{
	BYTE q;
	STRPTR c, t, n = NULL, v, *ent, *pe;
	int i, j;

	pe = memcpy(malloc(sizeof(EZXML_NIL)), EZXML_NIL, sizeof(EZXML_NIL));

	for(s[len] = '\0'; s;)
	{
		while(*s && *s != '<' && *s != '%') s++;  // find next declaration

		if(!*s) break;
		else if(!strncmp(s, "<!ENTITY", 8))    // parse entity definitions
		{
			c = s += strspn(s + 8, EZXML_WS) + 8; // skip white space separator
			n = s + strspn(s, EZXML_WS "%"); // find name
			*(s = n + strcspn(n, EZXML_WS)) = ';'; // append ; to name

			v = s + strspn(s + 1, EZXML_WS) + 1; // find value
			if((q = *(v++)) != '"' && q != '\'')    // skip externals
			{
				s = strchr(s, '>');
				continue;
			}

			for(i = 0, ent = (*c == '%') ? pe : root->ent; ent[i]; i++);
			ent = realloc(ent, (i + 3) * sizeof(char *)); // space for next ent
			if(*c == '%') pe = ent;
			else root->ent = ent;

			*(++s) = '\0'; // null terminate name
			if((s = strchr(v, q))) * (s++) = '\0'; // null terminate value
			ent[i + 1] = ezxml_decode(v, pe, '%', MyLibBase); // set value
			ent[i + 2] = NULL; // null terminate entity list
			if(!ezxml_ent_ok(n, ent[i + 1], ent))    // circular reference
			{
				if(ent[i + 1] != v) free(ent[i + 1]);
				ezxml_err(root, v, "circular entity declaration &%s", n);
				break;
			}
			else ent[i] = n; // set entity name
		}
		else if(!strncmp(s, "<!ATTLIST", 9))    // parse default attributes
		{
			t = s + strspn(s + 9, EZXML_WS) + 9; // skip whitespace separator
			if(!*t)
			{
				ezxml_err(root, t, "unclosed <!ATTLIST");
				break;
			}
			if(*(s = t + strcspn(t, EZXML_WS ">")) == '>') continue;
			else *s = '\0'; // null terminate tag name
			for(i = 0; root->attr[i] && strcmp(n, root->attr[i][0]); i++);

			while(*(n = ++s + strspn(s, EZXML_WS)) && *n != '>')
			{
				if(*(s = n + strcspn(n, EZXML_WS))) * s = '\0'; // attr name
				else
				{
					ezxml_err(root, t, "malformed <!ATTLIST");
					break;
				}

				s += strspn(s + 1, EZXML_WS) + 1; // find next token
				c = (strncmp(s, "CDATA", 5)) ? "*" : " "; // is it cdata?
				if(!strncmp(s, "NOTATION", 8))
					s += strspn(s + 8, EZXML_WS) + 8;
				s = (*s == '(') ? strchr(s, ')') : s + strcspn(s, EZXML_WS);
				if(!s)
				{
					ezxml_err(root, t, "malformed <!ATTLIST");
					break;
				}

				s += strspn(s, EZXML_WS ")"); // skip white space separator
				if(!strncmp(s, "#FIXED", 6))
					s += strspn(s + 6, EZXML_WS) + 6;
				if(*s == '#')    // no default value
				{
					s += strcspn(s, EZXML_WS ">") - 1;
					if(*c == ' ') continue;  // cdata is default, nothing to do
					v = NULL;
				}
				else if((*s == '"' || *s == '\'')  &&   // default value
				        (s = strchr(v = s + 1, *s))) * s = '\0';
				else
				{
					ezxml_err(root, t, "malformed <!ATTLIST");
					break;
				}

				if(!root->attr[i])    // new tag name
				{
					root->attr = (!i) ? malloc(2 * sizeof(char **))
					             : realloc(root->attr,
					                       (i + 2) * sizeof(char **));
					root->attr[i] = malloc(2 * sizeof(char *));
					root->attr[i][0] = t; // set tag name
					root->attr[i][1] = (char *)(root->attr[i + 1] = NULL);
				}

				for(j = 1; root->attr[i][j]; j += 3);  // find end of list
				root->attr[i] = realloc(root->attr[i],
				                        (j + 4) * sizeof(char *));

				root->attr[i][j + 3] = NULL; // null terminate list
				root->attr[i][j + 2] = c; // is it cdata?
				root->attr[i][j + 1] = (v) ? ezxml_decode(v, root->ent, *c, MyLibBase)
				                       : NULL;
				root->attr[i][j] = n; // attribute name
			}
		}
		else if(!strncmp(s, "<!--", 4)) s = strstr(s + 4, "-->");  // comments
		else if(!strncmp(s, "<?", 2))    // processing instructions
		{
			if((s = strstr(c = s + 2, "?>")))
				ezxml_proc_inst(root, c, s++ - c, MyLibBase);
		}
		else if(*s == '<') s = strchr(s, '>');  // skip other declarations
		else if(*(s++) == '%' && !root->standalone) break;
	}

	free(pe);
	return !*root->err;
}

// Converts a UTF-16 string to UTF-8. Returns a new string that must be freed
// or NULL if no conversion was needed.
STRPTR ezxml_str2utf8(STRPTR *s, ULONG *len, struct LibBase *MyLibBase)
{
	char *u;
	ULONG l = 0, sl, max = *len;
	long c, d;
	int b, be = (**s == '\xFE') ? 1 : (**s == '\xFF') ? 0 : -1;

	if(be == -1) return NULL;  // not UTF-16

	u = malloc(max);
	for(sl = 2; sl < *len - 1; sl += 2)
	{
		c = (be) ? (((*s)[sl] & 0xFF) << 8) | ((*s)[sl + 1] & 0xFF)  //UTF-16BE
		    : (((*s)[sl + 1] & 0xFF) << 8) | ((*s)[sl] & 0xFF); //UTF-16LE
		if(c >= 0xD800 && c <= 0xDFFF && (sl += 2) < *len - 1)    // high-half
		{
			d = (be) ? (((*s)[sl] & 0xFF) << 8) | ((*s)[sl + 1] & 0xFF)
			    : (((*s)[sl + 1] & 0xFF) << 8) | ((*s)[sl] & 0xFF);
			c = (((c & 0x3FF) << 10) | (d & 0x3FF)) + 0x10000;
		}

		while(l + 6 > max) u = realloc(u, max += EZXML_BUFSIZE);
		if(c < 0x80) u[l++] = c;  // US-ASCII subset
		else   // multi-byte UTF-8 sequence
		{
			for(b = 0, d = c; d; d /= 2) b++;  // bits in c
			b = (b - 2) / 5; // bytes in payload
			u[l++] = (0xFF << (7 - b)) | (c >> (6 * b)); // head
			while(b) u[l++] = 0x80 | ((c >> (6 * --b)) & 0x3F);  // payload
		}
	}
	return *s = realloc(u, *len = l);
}

// frees a tag attribute list
VOID ezxml_free_attr(STRPTR *attr, struct LibBase *MyLibBase)
{
	int i = 0;
	char *m;

	if(!attr || attr == EZXML_NIL || attr[0] == NULL) return;  // nothing to free
	while(attr[i]) i += 2;  // find end of attribute list
	m = attr[i + 1]; // list of which names and values are malloced
	if(m)
	{
		for(i = 0; m[i]; i++)
		{
			if((m[i] & EZXML_NAMEM) || (m[i] & EZXML_TXTM) && m[i]);
			if((m[i] & EZXML_NAMEM) && attr[i * 2]) free(attr[i * 2]);
			if((m[i] & EZXML_TXTM) && attr[(i * 2) + 1]) free(attr[(i * 2) + 1]);
		}
		free(m);
	}
	if(attr) free(attr);
}
//+ ezxml.library/ezxml_parse_str
/****** ezxml.library/ezxml_parse_str *****************************************
* NAME
*  ezxml_parse_str - parses string and creates ezxml structure. (V8)
*
* SYNOPSIS
*  ezxml_parse_str(string, size);
*  ezxml_t ezxml_parse_str(STRPTR, ULONG);
*
* FUNCTION
*  Given a string of xml data and its length, parses it and creates an ezxml
*  structure. For efficiency, modifies the data by adding null terminators
*  and decoding ampersand sequences. If you don't want this, copy the data and
*  pass in the copy.
*
* INPUTS
*  string - pointer to string with xml data
*  size   - size of string without 0x00 char
*
* RESULT
*	Returns ezxml_t structure or NULL on failure.
*
* NOTES
*  Notice that original data will be modified.
*  Don't forget to free allocated memory with ezxml_free()
*
* SEE ALSO
*  ezxml_parse_fd() ezxml_parse_file() ezxml_free()
********************************************************************************
*
*/
//-
ezxml_t ezxml_parse_str(STRPTR s, ULONG len, struct LibBase *MyLibBase)
{
	ezxml_root_t root = (ezxml_root_t)ezxml_new(NULL, MyLibBase);
	BYTE q, e;
	STRPTR d, *attr, *a = NULL; // initialize a to avoid compile warning
	int l, i, j;

	root->m = s;
	if(!len) return ezxml_err(root, NULL, "root tag missing");
	root->u = ezxml_str2utf8(&s, &len, MyLibBase); // convert utf-16 to utf-8
	root->e = (root->s = s) + len; // record start and end of work area

	e = s[len - 1]; // save end char
	s[len - 1] = '\0'; // turn end char into null terminator

	while(*s && *s != '<') s++;  // find first tag
	if(!*s) return ezxml_err(root, s, "root tag missing");

	for(; ;)
	{
		attr = (char **)EZXML_NIL;
		d = ++s;

		if(isalpha(*s) || *s == '_' || *s == ':' || *s < '\0')    // new tag
		{
			if(!root->cur)
				return ezxml_err(root, d, "markup outside of root element");

			s += strcspn(s, EZXML_WS "/>");
			while(isspace(*s)) *(s++) = '\0';  // null terminate tag name

			if(*s && *s != '/' && *s != '>')  // find tag in default attr list
				for(i = 0; (a = root->attr[i]) && strcmp(a[0], d); i++);

			for(l = 0; *s && *s != '/' && *s != '>'; l += 2)    // new attrib
			{
				attr = (l) ? realloc(attr, (l + 4) * sizeof(char *))
				       : malloc(4 * sizeof(char *)); // allocate space
				attr[l + 3] = (l) ? realloc(attr[l + 1], (l / 2) + 2)
				              : malloc(2); // mem for list of maloced vals
				strcpy(attr[l + 3] + (l / 2), " "); // value is not malloced
				attr[l + 2] = NULL; // null terminate list
				attr[l + 1] = ""; // temporary attribute value
				attr[l] = s; // set attribute name

				s += strcspn(s, EZXML_WS "=/>");
				if(*s == '=' || isspace(*s))
				{
					*(s++) = '\0'; // null terminate tag attribute name
					q = *(s += strspn(s, EZXML_WS "="));
					if(q == '"' || q == '\'')    // attribute value
					{
						attr[l + 1] = ++s;
						while(*s && *s != q) s++;
						if(*s) *(s++) = '\0';  // null terminate attribute val
						else
						{
							ezxml_free_attr(attr, MyLibBase);
							return ezxml_err(root, d, "missing %c", q);
						}

						for(j = 1; a && a[j] && strcmp(a[j], attr[l]); j += 3);
						attr[l + 1] = ezxml_decode(attr[l + 1], root->ent, (a
						                           && a[j]) ? *a[j + 2] : ' ', MyLibBase);
						if(attr[l + 1] < d || attr[l + 1] > s)
							attr[l + 3][l / 2] = EZXML_TXTM; // value malloced
					}
				}
				while(isspace(*s)) s++;
			}

			if(*s == '/')    // self closing tag
			{
				*(s++) = '\0';
				if((*s && *s != '>') || (!*s && e != '>'))
				{
					if(l) ezxml_free_attr(attr, MyLibBase);
					return ezxml_err(root, d, "missing >");
				}
				ezxml_open_tag(root, d, attr, MyLibBase);
				ezxml_close_tag(root, d, s);
			}
			else if((q = *s) == '>' || (!*s && e == '>'))    // open tag
			{
				*s = '\0'; // temporarily null terminate tag name
				ezxml_open_tag(root, d, attr, MyLibBase);
				*s = q;
			}
			else
			{
				if(l) ezxml_free_attr(attr, MyLibBase);
				return ezxml_err(root, d, "missing >");
			}
		}
		else if(*s == '/')    // close tag
		{
			s += strcspn(d = s + 1, EZXML_WS ">") + 1;
			if(!(q = *s) && e != '>') return ezxml_err(root, d, "missing >");
			*s = '\0'; // temporarily null terminate tag name
			if(ezxml_close_tag(root, d, s)) return &root->xml;
			if(isspace(*s = q)) s += strspn(s, EZXML_WS);
		}
		else if(!strncmp(s, "!--", 3))    // xml comment
		{
			if(!(s = strstr(s + 3, "--")) || (*(s += 2) != '>' && *s) ||
			        (!*s && e != '>')) return ezxml_err(root, d, "unclosed <!--");
		}
		else if(!strncmp(s, "![CDATA[", 8))    // cdata
		{
			if((s = strstr(s, "]]>")))
				ezxml_char_content(root, d + 8, (s += 2) - d - 10, 'c', MyLibBase);
			else return ezxml_err(root, d, "unclosed <![CDATA[");
		}
		else if(!strncmp(s, "!DOCTYPE", 8))    // dtd
		{
			for(l = 0; *s && ((!l && *s != '>') || (l && (*s != ']' ||
			                  *(s + strspn(s + 1, EZXML_WS) + 1) != '>')));
			        l = (*s == '[') ? 1 : l) s += strcspn(s + 1, "[]>") + 1;
			if(!*s && e != '>')
				return ezxml_err(root, d, "unclosed <!DOCTYPE");
			d = (l) ? strchr(d, '[') + 1 : d;
			if(l && !ezxml_internal_dtd(root, d, s++ - d, MyLibBase)) return &root->xml;
		}
		else if(*s == '?')    // <?...?> processing instructions
		{
			do
			{
				s = strchr(s, '?');
			}
			while(s && *(++s) && *s != '>');
			if(!s || (!*s && e != '>'))
				return ezxml_err(root, d, (STRPTR)"unclosed <?");
			else ezxml_proc_inst(root, d + 1, s - d - 2, MyLibBase);
		}
		else return ezxml_err(root, d, "unexpected <");

		if(!s || !*s) break;
		*s = '\0';
		d = ++s;
		if(*s && *s != '<')    // tag character content
		{
			while(*s && *s != '<') s++;
			if(*s) ezxml_char_content(root, d, s - d, '&', MyLibBase);
			else break;
		}
		else if(!*s) break;
	}

	if(!root->cur) return &root->xml;
	else if(!root->cur->name) return ezxml_err(root, d, "root tag missing");
	else return ezxml_err(root, d, "unclosed tag <%s>", root->cur->name);
}

// Wrapper for ezxml_parse_str() that accepts a file stream. Reads the entire
// stream into memory and then parses it. For xml files, use ezxml_parse_file()
// or ezxml_parse_fd()
ezxml_t ezxml_parse_fp(BPTR fp, struct LibBase *MyLibBase)
{
	/* ezxml_root_t root;
	 ULONG l, len = 0;
	 char *s;

	 if (!(s = malloc(EZXML_BUFSIZE))) return NULL;
	 do {
	     len += (l = FRead(fp, (s + len), EZXML_BUFSIZE, 1));
	     if (l == EZXML_BUFSIZE) s = realloc(s, len + EZXML_BUFSIZE);   // I think that it's no needed for MorphOS ver. ;-)
	 } while (s && l == EZXML_BUFSIZE);

	 if (!s) return NULL;
	 root = (ezxml_root_t)ezxml_parse_str(s, len, MyLibBase);
	 root->len = -1; // so we know to free s in ezxml_free()
	 return &root->xml;*/
}

//+ ezxml.library/ezxml_parse_fd
/****** ezxml.library/ezxml_parse_fd ************************************************
* NAME
*  ezxml_parse_fd() - wrapper for ezxml_parse_str() that accepts a file handle (V8)
*
* SYNOPSIS
*  ezxml_parse_fd(fd);
*  ezxml_t ezxml_parse_fd(BPTR);
*
* FUNCTION
*  Reads a file handle and parses read data with ezxml_parse_str().
*
* INPUTS
*  fd - file handle returned by dos.library/Open()
*
* RESULT
*	Returns ezxml_t structure or NULL on failure.
*
* NOTES
*  Don't forget to free allocated memory with ezxml_free()
*
* SEE ALSO
*  ezxml_parse_str() ezxml_parse_file() ezxml_free()
**************************************************************************************
*
*/
//-
ezxml_t ezxml_parse_fd(BPTR fd, struct LibBase *MyLibBase)
{
	ezxml_root_t root;
	struct FileInfoBlock st;
	ULONG l;
	void *m;

	if(fd == NULL) return NULL;

	ExamineFH(fd, &st);

#ifndef EZXML_NOMMAP
	l = (st.st_size + sysconf(_SC_PAGESIZE) - 1) & ~(sysconf(_SC_PAGESIZE) - 1);
	if((m = mmap(NULL, l, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0)) !=
	        MAP_FAILED)
	{
		madvise(m, l, MADV_SEQUENTIAL); // optimize for sequential access
		root = (ezxml_root_t)ezxml_parse_str(m, st.st_size);
		madvise(m, root->len = l, MADV_NORMAL); // put it back to normal
	}
	else   // mmap failed, read file into memory
	{
#endif // EZXML_NOMMAP
		l = Read(fd, m = malloc(st.fib_Size), st.fib_Size);
		root = (ezxml_root_t)ezxml_parse_str(m, l, MyLibBase);
		root->len = -1; // so we know to free s in ezxml_free()
#ifndef EZXML_NOMMAP
	}
#endif // EZXML_NOMMAP
	return &root->xml;
}

//+ ezxml.library/ezxml_parse_file
/****** ezxml.library/ezxml_parse_file *******************************************
* NAME
*  ezxml_parse_file() - wrapper for ezxml_parse_fd() that accepts a file name (V8)
*
* SYNOPSIS
*  ezxml_parse_file(filename);
*  ezxml_t ezxml_parse_file(CONST_STRPTR file);
*
* FUNCTION
*  Opens a file with given filename and passes file pointer to ezxml_parse_fd().
*
* INPUTS
*  filename - standard file name (as usually given for dos.library/Open())
*
* RESULT
*	Returns ezxml_t structure or NULL on failure.
*
* NOTES
*  Don't forget to free allocated memory with ezxml_free()
*
* SEE ALSO
*  ezxml_parse_str() ezxml_parse_fd() ezxml_free()
********************************************************************************
*
*/
//-
ezxml_t ezxml_parse_file(CONST_STRPTR file, struct LibBase *MyLibBase)
{
	int fd = Open(file, MODE_OLDFILE);
	ezxml_t xml = NULL;
	if(fd != NULL)
	{
		xml = ezxml_parse_fd(fd, MyLibBase);
		Close(fd);
	}
	return xml;
}

// Encodes ampersand sequences appending the results to *dst, reallocating *dst
// if length excedes max. a is non-zero for attribute encoding. Returns *dst
STRPTR ezxml_ampencode(CONST_STRPTR s, ULONG len, STRPTR *dst, ULONG *dlen,
                       ULONG *max, SHORT a, struct LibBase *MyLibBase)
{
	const char *e = s + len;

	for(; s != e; s++)
	{
		while(*dlen + 10 > *max) *dst = realloc(*dst, *max += EZXML_BUFSIZE);

		switch(*s)
		{
		case '\0':
			return *dst;
		case '&':
			*dlen += sprintf(*dst + *dlen, "&amp;");
			break;
		case '<':
			*dlen += sprintf(*dst + *dlen, "&lt;");
			break;
		case '>':
			*dlen += sprintf(*dst + *dlen, "&gt;");
			break;
		case '"':
			*dlen += sprintf(*dst + *dlen, (a) ? "&quot;" : "\"");
			break;
		case '\n':
			*dlen += sprintf(*dst + *dlen, (a) ? "&#xA;" : "\n");
			break;
		case '\t':
			*dlen += sprintf(*dst + *dlen, (a) ? "&#x9;" : "\t");
			break;
		case '\r':
			*dlen += sprintf(*dst + *dlen, "&#xD;");
			break;
		default:
			(*dst)[(*dlen)++] = *s;
		}
	}
	return *dst;
}

// Recursively converts each tag to xml appending it to *s. Reallocates *s if
// its length excedes max. start is the location of the previous tag in the
// parent tag's character content. Returns *s.
STRPTR ezxml_toxml_r(ezxml_t xml, STRPTR *s, ULONG *len, ULONG *max,
                     ULONG start, STRPTR **attr, struct LibBase *MyLibBase)
{
	int i, j;
	char *txt = (xml->parent) ? xml->parent->txt : "";
	ULONG off = 0;

	// parent character content up to this tag
	*s = ezxml_ampencode(txt + start, xml->off - start, s, len, max, 0, MyLibBase);

	while(*len + strlen(xml->name) + 4 > *max)  // reallocate s
		*s = realloc(*s, *max += EZXML_BUFSIZE);

	*len += sprintf(*s + *len, "<%s", xml->name); // open tag
	for(i = 0; xml->attr[i]; i += 2)    // tag attributes
	{
		if(ezxml_attr(xml, xml->attr[i]) != xml->attr[i + 1]) continue;
		while(*len + strlen(xml->attr[i]) + 7 > *max)  // reallocate s
			*s = realloc(*s, *max += EZXML_BUFSIZE);

		*len += sprintf(*s + *len, " %s=\"", xml->attr[i]);
		ezxml_ampencode(xml->attr[i + 1], -1, s, len, max, 1, MyLibBase);
		*len += sprintf(*s + *len, "\"");
	}

	for(i = 0; attr[i] && strcmp(attr[i][0], xml->name); i++);
	for(j = 1; attr[i] && attr[i][j]; j += 3)    // default attributes
	{
		if(!attr[i][j + 1] || ezxml_attr(xml, attr[i][j]) != attr[i][j + 1])
			continue; // skip duplicates and non-values
		while(*len + strlen(attr[i][j]) + 7 > *max)  // reallocate s
			*s = realloc(*s, *max += EZXML_BUFSIZE);

		*len += sprintf(*s + *len, " %s=\"", attr[i][j]);
		ezxml_ampencode(attr[i][j + 1], -1, s, len, max, 1, MyLibBase);
		*len += sprintf(*s + *len, "\"");
	}
	*len += sprintf(*s + *len, ">");

	*s = (xml->child) ? ezxml_toxml_r(xml->child, s, len, max, 0, attr, MyLibBase) //child
	     : ezxml_ampencode(xml->txt, -1, s, len, max, 0, MyLibBase);  //data

	while(*len + strlen(xml->name) + 4 > *max)  // reallocate s
		*s = realloc(*s, *max += EZXML_BUFSIZE);

	*len += sprintf(*s + *len, "</%s>", xml->name); // close tag

	while(txt[off] && off < xml->off) off++;  // make sure off is within bounds
	return (xml->ordered) ? ezxml_toxml_r(xml->ordered, s, len, max, off, attr, MyLibBase)
	       : ezxml_ampencode(txt + off, -1, s, len, max, 0, MyLibBase);
}

//+ ezxml.library/ezxml_toxml
/****** ezxml.library/ezxml_toxml *********************************************
* NAME
*  ezxml_toxml() - converts an ezxml structure back to xml (V8)
*
* SYNOPSIS
*  ezxml_toxml(xml);
*  STRPTR ezxml_toxml(ezxml_t);
*
* FUNCTION
*  Reads a ezxml_t structure and converts it's to string contains xml data.
*
* INPUTS
*  xml - ezxml_t structure
*
* RESULT
*	Return a pointer to string contains requested tag.
*
* NOTES
*  String have to be freed by calling FreeVec()!
********************************************************************************
*
*/
//-
STRPTR ezxml_toxml(ezxml_t xml, struct LibBase *MyLibBase)
{
	ezxml_t p = (xml) ? xml->parent : NULL, o = (xml) ? xml->ordered : NULL;
	ezxml_root_t root = (ezxml_root_t)xml;
	ULONG len = 0, max = EZXML_BUFSIZE;
	char *s = strcpy(malloc(max), ""), *t, *n;
	int i, j, k;

	if(!xml || !xml->name) return realloc(s, len + 1);
	while(root->xml.parent) root = (ezxml_root_t)root->xml.parent;  // root tag

	for(i = 0; !p && root->pi[i]; i++)    // pre-root processing instructions
	{
		for(k = 2; root->pi[i][k - 1]; k++);
		for(j = 1; (n = root->pi[i][j]); j++)
		{
			if(root->pi[i][k][j - 1] == '>') continue;  // not pre-root
			while(len + strlen(t = root->pi[i][0]) + strlen(n) + 7 > max)
				s = realloc(s, max += EZXML_BUFSIZE);
			len += sprintf(s + len, "<?%s%s%s?>\n", t, *n ? " " : "", n);
		}
	}

	xml->parent = xml->ordered = NULL;
	s = ezxml_toxml_r(xml, &s, &len, &max, 0, root->attr, MyLibBase);
	xml->parent = p;
	xml->ordered = o;

	for(i = 0; !p && root->pi[i]; i++)    // post-root processing instructions
	{
		for(k = 2; root->pi[i][k - 1]; k++);
		for(j = 1; (n = root->pi[i][j]); j++)
		{
			if(root->pi[i][k][j - 1] == '<') continue;  // not post-root
			while(len + strlen(t = root->pi[i][0]) + strlen(n) + 7 > max)
				s = realloc(s, max += EZXML_BUFSIZE);
			len += sprintf(s + len, "\n<?%s%s%s?>", t, *n ? " " : "", n);
		}
	}

	return realloc(s, len + 1);
}

//+ ezxml.library/ezxml_free
/****** ezxml.library/ezxml_free ***********************************************
* NAME
*  ezxml_free() - frees the memory allocated for an ezxml structure (V8)
*
* SYNOPSIS
*  ezxml_free(xml);
*  VOID ezxml_free(ezxml_t);
*
* FUNCTION
*  Frees the memory allocated for an ezxml structure. When you call it for structure,
*  which have subtags, etc. it will automatically free any child.
*
* INPUTS
*  xml - ezxml_t tag structure
*
* SEE ALSO
*  ezxml_parse_str() ezxml_parse_fd() ezxml_parse_file() ezxml_new()
********************************************************************************
*
*/
//-
VOID ezxml_free(ezxml_t xml, struct LibBase *MyLibBase)
{
	ezxml_root_t root = (ezxml_root_t)xml;
	int i, j;
	char **a, *s;

	if(!xml) return;
	ezxml_free(xml->child, MyLibBase);
	ezxml_free(xml->ordered, MyLibBase);

	if(!xml->parent)    // free root tag allocations
	{
		for(i = 10; root->ent[i]; i += 2)  // 0 - 9 are default entites (<>&"')
			if(((s = root->ent[i + 1]) < root->s || s > root->e) && s) free(s);
		if(root->ent)free(root->ent); // free list of general entities

		for(i = 0; (a = root->attr[i]); i++)
		{
			for(j = 1; a[j++]; j += 2)  // free malloced attribute values
				if(a[j] && (a[j] < root->s || a[j] > root->e)) free(a[j]);
			if(a) free(a);
		}
		if(root->attr[0] && root->attr) free(root->attr);  // free default attribute list

		for(i = 0; root->pi[i]; i++)
		{
			for(j = 1; root->pi[i][j]; j++);
			if(root->pi[i][j + 1])free(root->pi[i][j + 1]);
			if(root->pi[i]) free(root->pi[i]);
		}
		if(root->pi[0] && root->pi) free(root->pi);  // free processing instructions

		if(root->len == -1 && root->m) free(root->m);  // malloced xml data
#ifndef EZXML_NOMMAP
		else if(root->len) munmap(root->m, root->len);  // mem mapped xml data
#endif /* EZXML_NOMMAP */
		if(root->u) free(root->u);  // utf8 conversion
	}

	ezxml_free_attr(xml->attr, MyLibBase); // tag attributes
	if((xml->flags & EZXML_TXTM) && xml->txt) free(xml->txt);  // character content
	if((xml->flags & EZXML_NAMEM) && xml->name) free(xml->name);  // tag name
	if(xml) free(xml);
}

//+ ezxml.library/ezxml_error
/****** ezxml.library/ezxml_error ***********************************************
* NAME
*  ezxml_error() - reads error message (V8)
*
* SYNOPSIS
*  ezxml_error(xml);
*  CONST_STRPTR ezxml_error(ezxml_t);
*
* FUNCTION
*  Reads error messages from ezxml_t structure.
*
* INPUTS
*  xml - ezxml_t tag structure
*
* RESULT
*  Returns pointer to string contains error message.
*
* SEE ALSO
*  ezxml_parse_str() ezxml_parse_fd() ezxml_parse_file()
********************************************************************************
*
*/
//-
CONST_STRPTR ezxml_error(ezxml_t xml)
{
	while(xml && xml->parent) xml = xml->parent;  // find root tag
	return (xml) ? ((ezxml_root_t)xml)->err : "";
}

//+ ezxml.library/ezxml_new
/****** ezxml.library/ezxml_new *************************************************
* NAME
*  ezxml_new() - creates a new ezxml_t structure (V8)
*
* SYNOPSIS
*  ezxml_new(name);
*  ezxml_t ezxml_new(CONST_STRPTR);
*
* FUNCTION
*  Creates a new ezxml_t structure with given root tag name.
*
* INPUTS
*  name - name of the root tag
*
* RESULT
*  Returns new ezxml_t structure with given root tag name.
*
* NOTES
*  Name won't be copied. Please pay attention for not freeing it to early or use ezxml_new_d().
*  For full compatibility with ezxml_free() please use AllocVec() to alloc memory for
*  name string.
*
* SEE ALSO
*  ezxml_free() ezxml_new_d()
********************************************************************************
*
*/
//-
ezxml_t ezxml_new(CONST_STRPTR name, struct LibBase *MyLibBase)
{
	static char *ent[] = { "lt;", "&#60;", "gt;", "&#62;", "quot;", "&#34;",
	                       "apos;", "&#39;", "amp;", "&#38;", NULL
	                     };
	ezxml_root_t root = (ezxml_root_t)memset(malloc(sizeof(struct ezxml_root)),
	                    '\0', sizeof(struct ezxml_root));
	root->xml.name = (char *)name;
	root->cur = &root->xml;
	strcpy(root->err, root->xml.txt = "");
	root->ent = memcpy(malloc(sizeof(ent)), ent, sizeof(ent));
	root->attr = root->pi = (char ***)(root->xml.attr = EZXML_NIL);
	return &root->xml;
}

//+ ezxml.library/ezxml_insert
/****** ezxml.library/ezxml_insert **********************************************
* NAME
*  ezxml_insert() - inserts an existing tag (V8)
*
* SYNOPSIS
*  ezxml_insert(xml, dest, off);
*  ezxml_t ezxml_insert(ezxml_t, ezxml_t, ULONG);
*
* FUNCTION
*  Inserts an exisiting tag into ezxml_t structure.
*
* INPUTS
*  xml  - source ezxml_t structure
*  dest - destination ezxml_structure
*  off  - offset
*
* RESULT
*  Returns the tag.
*
* SEE ALSO
*  ezxml_move()
********************************************************************************
*
*/
//-
ezxml_t ezxml_insert(ezxml_t xml, ezxml_t dest, ULONG off)
{
	ezxml_t cur, prev, head;

	xml->next = xml->sibling = xml->ordered = NULL;
	xml->off = off;
	xml->parent = dest;

	if((head = dest->child))    // already have sub tags
	{
		if(head->off <= off)    // not first subtag
		{
			for(cur = head; cur->ordered && cur->ordered->off <= off;
			        cur = cur->ordered);
			xml->ordered = cur->ordered;
			cur->ordered = xml;
		}
		else   // first subtag
		{
			xml->ordered = head;
			dest->child = xml;
		}

		for(cur = head, prev = NULL; cur && strcmp(cur->name, xml->name);
		        prev = cur, cur = cur->sibling); // find tag type
		if(cur && cur->off <= off)    // not first of type
		{
			while(cur->next && cur->next->off <= off) cur = cur->next;
			xml->next = cur->next;
			cur->next = xml;
		}
		else   // first tag of this type
		{
			if(prev && cur) prev->sibling = cur->sibling;  // remove old first
			xml->next = cur; // old first tag is now next
			for(cur = head, prev = NULL; cur && cur->off <= off;
			        prev = cur, cur = cur->sibling); // new sibling insert point
			xml->sibling = cur;
			if(prev) prev->sibling = xml;
		}
	}
	else dest->child = xml; // only sub tag

	return xml;
}

//+ ezxml.library/ezxml_add_child
/****** ezxml.library/ezxml_add_child *****************************************
* NAME
*  ezxml_add_child() - adds a child tag (V8)
*
* SYNOPSIS
*  ezxml_add_child(xml, name, off);
*  ezxml_t ezxml_add_child(ezxml_t, CONST_STRPTR, ULONG);
*
* FUNCTION
*  Adds a child to given tag with specified name. Off is the offset of the child tag
*  relative to the start of the parent tag's character content.
*
* INPUTS
*  xml  - ezxml_t structure of parents' tag
*  name - name of the child tag
*  off  - offset of the child tag relative to the start of the parent tag's character content
*
* RESULT
*  Returns the child tag.
*
* NOTES
*  Name won't be copied. Please pay attention for not freeing it to early or use ezxml_add_child_d().
*  For full compatibility with ezxml_free() please use AllocVec() to alloc memory for name string.
*
* SEE ALSO
*  ezxml_free() ezxml_new() ezxml_add_child_d()
********************************************************************************
*
*/
//-
ezxml_t ezxml_add_child(ezxml_t xml, CONST_STRPTR name, ULONG off, struct LibBase *MyLibBase)
{
	ezxml_t child;

	if(!xml) return NULL;
	child = (ezxml_t)memset(malloc(sizeof(struct ezxml)), '\0',
	                        sizeof(struct ezxml));
	child->name = (char *)name;
	child->attr = EZXML_NIL;
	child->txt = "";

	return ezxml_insert(child, xml, off);
}

//+ ezxml.library/ezxml_set_text
/****** ezxml.library/ezxml_set_text ********************************************
* NAME
*  ezxml_set_text() - sets the character content for the given tag (V8)
*
* SYNOPSIS
*  ezxml_set_txt(xml, txt);
*  ezxml_t ezxml_set_txt(ezxml_t, CONST_STRPTR);
*
* FUNCTION
*  Sets the content of the given tag.
*
* INPUTS
*  xml - ezxml_t structure of tag
*  txt - contents to be placed in tag
*
* RESULT
*  Returns the tag.
*
* NOTES
*  Txt won't be copied. Please pay attention for not freeing it to early or use ezxml_set_text_d().
*  For full compatibility with ezxml_free() please use AllocVec() to alloc memory for txt string.
*
* SEE ALSO
*  ezxml_free() ezxml_set_text_d() ezxml_set_attr() ezxml_set_flag()
********************************************************************************
*
*/
//-
ezxml_t ezxml_set_txt(ezxml_t xml, CONST_STRPTR txt, struct LibBase *MyLibBase)
{
	if(!xml) return NULL;
	if(xml->flags & EZXML_TXTM) free(xml->txt);  // existing txt was malloced
	xml->flags &= ~EZXML_TXTM;
	xml->txt = (char *)txt;
	return xml;
}

//+ ezxml.library/ezxml_set_attr
/****** ezxml.library/ezxml_set_attr ********************************************
* NAME
*  ezxml_set_attr() - sets the given tag attribute (V8)
*
* SYNOPSIS
*  ezxml_set_attr(xml, name, value);
*  ezxml_t ezxml_set_attr(ezxml_t, CONST_STRPTR, CONST_STRPTR);
*
* FUNCTION
*  Sets the given tag attribute or creates a new attribute if not found.
*  A value of NULL will remove the specified attribute.
*
* INPUTS
*  xml - ezxml_t structure of tag
*  name - name of attribute
*  value - value of attribute
*
* RESULT
*  Returns the tag given.
*
* NOTES
*  Name and value won't be copied. So pay attention for not freeing it too early.
*  For full compatibility with ezxml_free() please use AllocVec() to alloc memory for strings.
*
* SEE ALSO
*  ezxml_free() ezxml_set_text() ezxml_set_attr_d() ezxml_set_flag()
********************************************************************************
*
*/
//-
ezxml_t ezxml_set_attr(ezxml_t xml, CONST_STRPTR name, CONST_STRPTR value, struct LibBase* MyLibBase)
{
	int l = 0, c;

	if(!xml) return NULL;

	while(xml->attr[l] && strcmp(xml->attr[l], name)) l += 2;
	if(!xml->attr[l])    // not found, add as new attribute
	{
		if(!value) return xml;  // nothing to do
		if(xml->attr == EZXML_NIL)    // first attribute
		{
			xml->attr = malloc(4 * sizeof(char *));
			xml->attr[1] = strdup(""); // empty list of malloced names/vals
		}
		else xml->attr = realloc(xml->attr, (l + 4) * sizeof(char *));

		xml->attr[l] = (char *)name; // set attribute name
		xml->attr[l + 2] = NULL; // null terminate attribute list
		if(xml->attr[l + 1]) xml->attr[l + 3] = realloc(xml->attr[l + 1], (c = strlen(xml->attr[l + 1])) + 2);
	}
	else if(xml->flags & EZXML_DUP) free((char *)name);  // name was strduped

	for(c = l; xml->attr[c]; c += 2);  // find end of attribute list
	if(xml->attr[l + 1] && xml->attr[c + 1][l / 2] && xml->attr[c + 1][l / 2] & EZXML_TXTM) free(xml->attr[l + 1]); //old val
	if(!(xml->flags & EZXML_DUP))
	{
		xml->attr[c + 1][l / 2] &= ~EZXML_TXTM;
	}

	if(value)
	{
		xml->attr[l + 1] = (char *)value; // set attribute value
	}
	else   // remove attribute
	{
		if(xml->attr[c + 1][l / 2] & EZXML_NAMEM) free(xml->attr[l]);
		memmove(xml->attr + l, xml->attr + l + 2, (c - l + 2) * sizeof(char*));
		xml->attr = realloc(xml->attr, (c + 2) * sizeof(char *));
	}
	xml->flags &= ~EZXML_DUP; // clear strdup() flag
	return xml;
}

//+ ezxml.library/ezxml_set_flag
/****** ezxml.library/ezxml_set_flag *****************************************
* NAME
*  ezxml_set_flag() - sets the flag for the given tag (V8)
*
* SYNOPSIS
*  ezxml_set_flag(xml, flag);
*  ezxml_t ezxml_set_flag(ezxml_t, SHORT);
*
* FUNCTION
*  Sets a flag for the given tag.
*
* INPUTS
*  xml  - ezxml_t structure of tag
*  flag - flag to be set
*
* RESULT
*  Returns the tag given.
*
* SEE ALSO
*  ezxml_set_text() ezxml_set_attr_d() ezxml_set_flag()
********************************************************************************
*
*/
//-
ezxml_t ezxml_set_flag(ezxml_t xml, SHORT flag)
{
	if(xml) xml->flags |= flag;
	return xml;
}

//+ ezxml.library/ezxml_cut
/****** ezxml.library/ezxml_cut ***********************************************
* NAME
*  ezxml_cut() - removes a tag along with its subtags (V8)
*
* SYNOPSIS
*  ezxml_cut(xml);
*  ezxml_t ezxml_cut(ezxml_t);
*
* FUNCTION
*  Removes a tag along with its subtags.
*
* INPUTS
*  xml - ezxml_t structure of tag to be cut
*
* RESULT
*  Returns the tag given.
*
* NOTES
*  This function does not freeing memory!
*
* SEE ALSO
*  ezxml_remove() ezxml_free()
********************************************************************************
*
*/
//-
ezxml_t ezxml_cut(ezxml_t xml)
{
	ezxml_t cur;

	if(!xml) return NULL;  // nothing to do
	if(xml->next) xml->next->sibling = xml->sibling;  // patch sibling list

	if(xml->parent)    // not root tag
	{
		cur = xml->parent->child; // find head of subtag list
		if(cur == xml) xml->parent->child = xml->ordered;  // first subtag
		else   // not first subtag
		{
			while(cur->ordered != xml) cur = cur->ordered;
			cur->ordered = cur->ordered->ordered; // patch ordered list

			cur = xml->parent->child; // go back to head of subtag list
			if(strcmp(cur->name, xml->name))    // not in first sibling list
			{
				while(strcmp(cur->sibling->name, xml->name))
					cur = cur->sibling;
				if(cur->sibling == xml)    // first of a sibling list
				{
					cur->sibling = (xml->next) ? xml->next
					               : cur->sibling->sibling;
				}
				else cur = cur->sibling; // not first of a sibling list
			}

			while(cur->next && cur->next != xml) cur = cur->next;
			if(cur->next) cur->next = cur->next->next;  // patch next list
		}
	}
	xml->ordered = xml->sibling = xml->next = NULL;
	return xml;
}

//+ ezxml.library/ezxml_next
/****** ezxml.library/ezxml_next ***********************************************
* NAME
*  ezxml_next() - searches for next tag with this same name (V8)
*
* SYNOPSIS
*  ezxml_next(xml)
*  ezxml_t ezxml_next(ezxml_t)
*
* FUNCTION
*  Searches for the next tag of the same name in the same section and depth.
*
* INPUTS
*  xml - ezxml_t structure
*
* RESULT
*	Return a ezxml_t structure of found tag or NULL on failure.
*
* SEE ALSO
*  ezxml_child() ezxml_idx()
********************************************************************************
*
*/
//-
ezxml_t ezxml_next(ezxml_t xml)
{
	return ((xml) ? xml->next : NULL);
}

//+ ezxml.library/ezxml_name
/****** ezxml.library/ezxml_name ***********************************************
* NAME
*  ezxml_name() - reads name of given tag (V8)
*
* SYNOPSIS
*  ezxml_name(xml)
*  CONST_STRPTR ezxml_name(ezxml_t)
*
* FUNCTION
*  Reads name of given tag from its ezxml_t structure.
*
* INPUTS
*  xml - ezxml_t structure
*
* RESULT
*	Return a pointer to string contains name of given tag.
*
* NOTES
*  Return NULL on failure.
*
* SEE ALSO
*  ezxml_txt()
********************************************************************************
*
*/
//-
STRPTR ezxml_name(ezxml_t xml)
{
	return ((xml) ? xml->name : NULL);
}

//+ ezxml.library/ezxml_txt
/****** ezxml.library/ezxml_txt ***********************************************
* NAME
*  ezxml_txt() - reads contents of given tag (V8)
*
* SYNOPSIS
*  ezxml_txt(xml)
*  CONST_STRPTR ezxml_txt(ezxml_t)
*
* FUNCTION
*  Reads contents of given tag from its ezxml_t structure.
*
* INPUTS
*  xml - ezxml_t structure
*
* RESULT
*	Return a pointer to string contains content of given tag.
*
* NOTES
*  Return empty string ("") on failure.
*
* SEE ALSO
*  ezxml_name()
********************************************************************************
*
*/
//-
STRPTR ezxml_txt(ezxml_t xml)
{
	return ((xml) ? xml->txt : "");
}

//+ ezxml.library/ezxml_new_d
/****** ezxml.library/ezxml_new_d **********************************************
* NAME
*  ezxml_new_d() - wrapper for ezxml_new that duplicate name string (V8)
*
* SYNOPSIS
*  ezxml_new_d(name);
*  ezxml_t ezxml_new_d(CONST_STRPTR);
*
* FUNCTION
*  Creates a new ezxml_t structure with given root tag name. Name string will be duplicated.
*
* INPUTS
*  name - name of the root tag
*
* RESULT
*  Returns new ezxml_t structure with given root tag name.
*
* NOTES
*  Name will be copied, so you don't have to pay attention for not freeing it too early.
*
* SEE ALSO
*  ezxml_free() ezxml_new()
********************************************************************************
*
*/
//-
ezxml_t ezxml_new_d(CONST_STRPTR name, struct LibBase *MyLibBase)
{
	return ezxml_set_flag(ezxml_new(strdup(name), MyLibBase), EZXML_NAMEM);
}
//+ ezxml.library/ezxml_add_child_d
/****** ezxml.library/ezxml_add_child_d *****************************************
* NAME
*  ezxml_add_child_d() - wrapper for ezxml_add_child that duplicate name string (V8)
*
* SYNOPSIS
*  ezxml_add_child_d(xml, name, off);
*  ezxml_t ezxml_add_child_d(ezxml_t, CONST_STRPTR, ULONG);
*
* FUNCTION
*  Adds a child to given tag with specified name. Off is the offset of the child tag
*  relative to the start of the parent tag's character content.
*
* INPUTS
*  xml  - ezxml_t structure of parents' tag
*  name - name of the child tag
*  off  - offset of the child tag relative to the start of the parent tag's character content
*
* RESULT
*  Returns the child tag.
*
* NOTES
*  Name will be copied, so you don't have to pay attention for not freeing it too early.
*
* SEE ALSO
*  ezxml_free() ezxml_new() ezxml_add_child()
********************************************************************************
*
*/
//-
ezxml_t ezxml_add_child_d(ezxml_t xml, CONST_STRPTR name, ULONG off, struct LibBase *MyLibBase)
{
	return ezxml_set_flag(ezxml_add_child(xml, strdup(name), off, MyLibBase), EZXML_NAMEM);
}
//+ ezxml.library/ezxml_set_text_d
/****** ezxml.library/ezxml_set_text_d ********************************************
* NAME
*  ezxml_set_text_d() - wrapper for ezxml_set_text() that duplicate content string (V8)
*
* SYNOPSIS
*  ezxml_set_txt_d(xml, txt);
*  ezxml_t ezxml_set_txt_d(ezxml_t, CONST_STRPTR);
*
* FUNCTION
*  Sets the content of the given tag with copy the txt string.
*
* INPUTS
*  xml - ezxml_t structure of tag
*  txt - contents to be placed in tag
*
* RESULT
*  Returns the tag.
*
* NOTES
*  Txt will be copied, so you don't have to pay attention for not freeing it too early.
*
* SEE ALSO
*  ezxml_free() ezxml_set_text() ezxml_set_attr() ezxml_set_flag()
********************************************************************************
*
*/
//-
ezxml_t ezxml_set_txt_d(ezxml_t xml, CONST_STRPTR txt, struct LibBase *MyLibBase)
{
	return ezxml_set_flag(ezxml_set_txt(xml, strdup(txt), MyLibBase), EZXML_TXTM);
}
//+ ezxml.library/ezxml_set_attr_d
/****** ezxml.library/ezxml_set_attr_d *****************************************
* NAME
*  ezxml_set_attr_d() - wrapper for ezxml_set_attr() that duplicate name and value (V8)
*
* SYNOPSIS
*  ezxml_set_attr_d(xml, name, value);
*  ezxml_t ezxml_set_attr_d(ezxml_t, CONST_STRPTR, CONST_STRPTR);
*
* FUNCTION
*  Sets the given tag attribute or creates a new attribute if not found.
*  Name and value will be duplicated.
*
* INPUTS
*  xml - ezxml_t structure of tag
*  name - name of attribute
*  value - value of attribute
*
* RESULT
*  Returns the tag given.
*
* NOTES
*  Value cannot be NULL, so for deleting attribute you have to use ezxml_set_attr().
*
* SEE ALSO
*  ezxml_free() ezxml_set_text() ezxml_set_attr() ezxml_set_flag()
********************************************************************************
*
*/
//-
ezxml_t ezxml_set_attr_d(ezxml_t xml, CONST_STRPTR name, CONST_STRPTR value, struct LibBase *MyLibBase)
{
	return ezxml_set_attr(ezxml_set_flag(xml, EZXML_DUP), strdup(name), strdup(value), MyLibBase);
}
//+ ezxml.library/ezxml_move
/****** ezxml.library/ezxml_move **********************************************
* NAME
*  ezxml_move() - moves an existing tag (V8)
*
* SYNOPSIS
*  ezxml_move(xml, dest, off);
*  ezxml_t ezxml_move(ezxml_t, ezxml_t, ULONG);
*
* FUNCTION
*  Moves an existing tag to become a subtag of dest tag at the
*  given offset from the start of dest's character content.
*
* INPUTS
*  xml  - source ezxml_t structure
*  dest - destination ezxml_structure
*  off  - offset
*
* RESULT
*  Returns the moved tag.
*
* SEE ALSO
*  ezxml_insert()
********************************************************************************
*
*/
//-
ezxml_t ezxml_move(ezxml_t xml, ezxml_t dest, ULONG off)
{
	return ezxml_insert(ezxml_cut(xml), dest, off);
}
//+ ezxml.library/ezxml_remove
/****** ezxml.library/ezxml_remove **********************************************
* NAME
*  ezxml_remove() - wrapper for ezxml_cut() and ezxml_free() (V8)
*
* SYNOPSIS
*  ezxml_remove(xml);
*  VOID ezxml_move(ezxml_t);
*
* FUNCTION
*  Removes a tag along with its subtag with freeing the memory.
*
* INPUTS
*  xml - ezxml_t structure do be removed
*
* SEE ALSO
*  ezxml_free() ezxml_cut()
********************************************************************************
*
*/
//-
void ezxml_remove(ezxml_t xml, struct LibBase *MyLibBase)
{
	ezxml_free(ezxml_cut(xml), MyLibBase);
}