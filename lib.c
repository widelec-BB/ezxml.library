/* lib.c
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
#include "ezxml.library.h"
#include "debug.h"

extern ULONG LibFuncTable[];
struct Library* LIB_Init(struct LibBase  *MyLibBase, BPTR SegList, struct ExecBase *SBase);

struct LibInitStruct
{
	ULONG LibSize;
	void  *FuncTable;
	void  *DataTable;
	void (*InitFunc)(void);
};

struct LibInitStruct LibInitStruct =
{
	sizeof(struct LibBase),
	LibFuncTable,
	NULL,
	(void (*)(void)) &LIB_Init
};


struct Resident LibResident =
{
	RTC_MATCHWORD,
	&LibResident,
	&LibResident + 1,
	RTF_PPC | RTF_EXTENDED | RTF_AUTOINIT,
	VERSION,
	NT_LIBRARY,
	0,
	"ezxml.library",
	VSTRING,
	&LibInitStruct,
	REVISION,
	NULL
};

ULONG __abox__ = 1;

LONG NoExecute(void)
{
	return -1;
}

__attribute__((section(".text"))) const UBYTE VTag[] = VERSTAG;

struct Library* LIB_Init(struct LibBase  *MyLibBase, BPTR SegList, struct ExecBase *sysBase)
{
	MyLibBase->SegList = SegList;
	MyLibBase->sysBase = sysBase;

	if((DOSBase = OpenLibrary("dos.library", 0)))
	{
		return(&MyLibBase->Lib);
	}

	FreeMem(((UBYTE *)MyLibBase) - MyLibBase->Lib.lib_NegSize,
	        MyLibBase->Lib.lib_NegSize + MyLibBase->Lib.lib_PosSize);

	return NULL;
}

BPTR LibExpunge(struct LibBase *MyLibBase)
{
	BPTR MySegment;

	Forbid();

	if(MyLibBase->Lib.lib_OpenCnt)
	{
		MyLibBase->Lib.lib_Flags |= LIBF_DELEXP;

		Permit();

		return 0;
	}

	Remove(&MyLibBase->Lib.lib_Node);

	Permit();

	MySegment = MyLibBase->SegList;

	FreeMem(((UBYTE *)MyLibBase) - MyLibBase->Lib.lib_NegSize,
	        MyLibBase->Lib.lib_NegSize + MyLibBase->Lib.lib_PosSize);

	return MySegment;
}

BPTR LIB_Expunge(void)
{
	struct LibBase *MyLibBase = (struct LibBase*)REG_A6;

	return LibExpunge(MyLibBase);
}

struct Library*	LIB_Open(void)
{
	struct LibBase *MyLibBase = (struct LibBase*)REG_A6;

	Forbid();

	MyLibBase->Lib.lib_Flags &= ~LIBF_DELEXP;
	MyLibBase->Lib.lib_OpenCnt++;

	Permit();

	return &MyLibBase->Lib;
}

BPTR LIB_Close(void)
{
	struct LibBase *MyLibBase = (struct LibBase*)REG_A6;
#ifdef SysBase
#undef SysBase
#endif /* SysBase */
	struct ExecBase *SysBase = MyLibBase->sysBase;
	BPTR ret = 0;

	Forbid();

	if((--MyLibBase->Lib.lib_OpenCnt) == 0 && (MyLibBase->Lib.lib_Flags & LIBF_DELEXP))
		ret = LibExpunge(MyLibBase);

	Permit();

	return ret;
}

ULONG LIB_Reserved(void)
{
	return 0;
}
