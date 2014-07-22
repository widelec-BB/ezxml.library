/* test.c
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

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/ezxml.h>
#include <exec/libraries.h>
#include <string.h>

struct Library *EzxmlBase;

int	main(int argc, char* argv[])
{
	int i;

	if((EzxmlBase = OpenLibrary("ezxml.library", 8)))
	{
		ezxml_t xml;
		char *s;

		if (argc != 2) return Printf("usage: %s xmlfile\n", argv[0]);

		xml = ezxml_parse_file(argv[1]);
		Printf("%s\n", (s = ezxml_toxml(xml)));
		FreeVec(s);
		i = Printf("%s", ezxml_error(xml));

		ezxml_free(xml);

		CloseLibrary(EzxmlBase);
	}
	else
		PutStr("Error: Could not open ezxml.library\n");

	return (i) ? 1 : 0;
}

