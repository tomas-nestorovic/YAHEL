#ifndef STDAFX_H
#define STDAFX_H

#include <cstdint>
#define _USE_MATH_DEFINES // for M_PI to be defined
#include <cmath>
#ifdef RELEASE_MFC42	// preventing from usage of C++ exceptions in releases compiled against legacy MFC 4.2
	#define _HAS_EXCEPTIONS 0
	#include <xstddef>
	#define _RAISE(x)
	#define _RERAISE
	#define _THROW(x,y)
	#define _THROW_NCEE(x,y)
	#define _HAS_EXCEPTIONS 1
	#include <exception>
	#define _HAS_EXCEPTIONS 0
	#include <typeinfo>
	#define bad_cast(x)
	#define bad_typeid(x)
	#define _CRTIMP2_PURE
	#include <xutility>
	#define _Xout_of_range(x)
	#define _Xlength_error(x)
	#define _Xbad_alloc()
#endif
#include <algorithm>
#include <memory>
#include <map>
#include <set>

// use Common-Controls library version 6
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <atlbase.h>
#include <Windows.h>
#include <WindowsX.h>
#include <CommCtrl.h>

#undef min
#undef max
#undef INFINITY

#if _MSC_VER<=1600
	#define constexpr const
#endif

#pragma warning( disable : 4228 ) // non-standard language extension
#pragma warning( disable : 4341 ) // pre-C++14 enums shouldn't be signed

#define DLL_FULLNAME	_T("Yet Another Hexa-Editor Library")
#define DLL_ABBREVIATION _T("YAHEL")
#define DLL_VERSION		_T("1.6")
#define DLL_CLASSNAME	_T("Afx:tomascz.") DLL_ABBREVIATION

#define FONT_SYMBOL			L"Symbol"
#define FONT_WEBDINGS		L"Webdings"
#define FONT_WINGDINGS		L"Wingdings"
#define FONT_COURIER_NEW	L"Courier New"

#define GITHUB_HTTPS_NAME		_T("https://github.com")
#define GITHUB_API_SERVER_NAME	_T("api.github.com")
#define GITHUB_VERSION_TAG_NAME	"\"tag_name\""

#define COLOR_WHITE	0xffffff
#define COLOR_BLACK	0
#define COLOR_YELLOW 0xffff
#define COLOR_RED	0xff

#ifdef RELEASE_MFC42
	#if _MSC_VER<=1600
		#define noexcept
	#endif

	void __cdecl operator delete(PVOID ptr, UINT sz) noexcept;
#endif

typedef const BYTE *PCBYTE;
typedef const WORD *PCWORD;

#ifndef _delayimp_h 
	extern "C" IMAGE_DOS_HEADER __ImageBase; 
#endif

#include "..\res\resource.h"
#include "api.h"
#include "Utils.h"

#define EXCLUSIVELY_LOCK_THIS_OBJECT()	const Utils::CExclusivelyLocked x12345(locker)

extern const WNDCLASS EditClass;
extern HRESULT IgnoreIoResult;

SHSTDAPI SHCreateStdEnumFmtEtc(UINT cfmt, const FORMATETC *afmt,IEnumFORMATETC **ppenumFormatEtc);

#endif STDAFX_H
