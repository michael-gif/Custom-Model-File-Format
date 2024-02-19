#if !defined(AFX_STDAFX_H__AFD56823_0B43_11D4_8B0F_0050BAC83302__INCLUDED_)
#define AFX_STDAFX_H__AFD56823_0B43_11D4_8B0F_0050BAC83302__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Stdlib.h>
#include <Stdio.h>
#include <String.h>
#include <iostream>

typedef signed char			sbyte;
typedef signed short		sword;
typedef signed int			sdword;
typedef signed __int64		sqword;
typedef unsigned __int64	uqword;
typedef float				sfloat;

#define	null NULL
#define RELEASE(x) { if (x != null) delete x; x = null; }
#define RELEASEARRAY(x)	{ if (x != null) delete []x; x = null; }

inline void ZeroMemory(void* addr, uint32_t size)
{
	memset(addr, 0, size);
}

inline void CopyMemory(void* dest, const void* src, uint32_t size)
{
	memcpy(dest, src, size);
}

inline void FillMemory(void* dest, uint32_t size, uint8_t val)
{
	memset(dest, val, size);
}

#include "RevisitedRadix.h"
#include "CustomArray.h"
#include "Adjacency.h"
#include "Striper.h"

#endif
