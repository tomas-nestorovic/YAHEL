#include "stdafx.h"

namespace Yahel{
namespace Utils{

	CExclusivelyLocked::CExclusivelyLocked(CRITICAL_SECTION &cs)
		// ctor
		: cs(cs) {
		::EnterCriticalSection(&cs);
	}

	CExclusivelyLocked::~CExclusivelyLocked(){
		// ctor
		::LeaveCriticalSection(&cs);
	}




	CGlobalMemLocker::CGlobalMemLocker(HGLOBAL handle)
		// ctor
		: handle(handle)
		, pMem( ::GlobalLock(handle) ) {
	}

	CGlobalMemLocker::~CGlobalMemLocker(){
		// dtor
		::GlobalUnlock(handle);
	}




	CViewportOrg::CViewportOrg(HDC dc,TRow r,TCol c,const CYahelFont &font)
		: dc(dc) {
		::SetViewportOrgEx( dc, c*font.GetCharAvgWidth(), r*font.GetCharHeight(), &pt0 );
	}

	CViewportOrg::~CViewportOrg(){
		::SetViewportOrgEx( dc, pt0.x, pt0.y, nullptr );
	}




	struct TLogPen:public LOGPEN{
		TLogPen(BYTE thickness,COLORREF color){
			::ZeroMemory( this, sizeof(*this) );
			lopnStyle=PS_SOLID;
			lopnWidth.x = lopnWidth.y = thickness;
			lopnColor=color;
		}
	};

	CYahelPen::CYahelPen(BYTE thickness,COLORREF color)
		// ctor
		: LOGPEN( TLogPen(thickness,color) )
		, handle( ::CreatePenIndirect(this) ) {
	}

	CYahelPen::~CYahelPen(){
		// dtor
		::DeleteObject(handle);
	}


	


	CYahelBrush::CYahelBrush(int stockObjectId)
		// ctor
		: handle( (HBRUSH)::GetStockObject(stockObjectId) ) {
		::GetObjectW( ::GetStockObject(stockObjectId), sizeof(LOGBRUSH), static_cast<LPLOGBRUSH>(this) );
	}

	CYahelBrush::CYahelBrush(COLORREF solidColor,bool sysColor){
		// ctor
		::ZeroMemory( this, sizeof(*this) );
		lbStyle=BS_SOLID;
		lbColor= sysColor ? ::GetSysColor(solidColor) : solidColor;
		handle=::CreateBrushIndirect(this);
	}

	CYahelBrush::~CYahelBrush(){
		// dtor
		::DeleteObject(handle);
	}

	CYahelBrush &CYahelBrush::operator=(CYahelBrush &&r){
		// reset
		::DeleteObject(handle);
		*static_cast<LPLOGBRUSH>(this)=r;
		handle=r.handle;
		r.handle=nullptr;
		return *this;
	}

	const CYahelBrush CYahelBrush::None=NULL_BRUSH;
	const CYahelBrush CYahelBrush::Black=BLACK_BRUSH;
	const CYahelBrush CYahelBrush::White=WHITE_BRUSH;




	CYahelFont::TLogFont::TLogFont(LPCWSTR face,int pointHeight){
		// ctor
		::ZeroMemory( this, sizeof(*this) );
		lfHeight=(10*-pointHeight)/72;
		lfCharSet=DEFAULT_CHARSET;
		lfQuality=ANTIALIASED_QUALITY;
		::lstrcpynW( lfFaceName, face, ARRAYSIZE(lfFaceName) );
	}

	CYahelFont::TLogFont::TLogFont(HFONT hFont){
		// ctor
		::GetObjectW( hFont, sizeof(*this), this );
	}

	CYahelFont::CYahelFont(const LOGFONTW &lf)
		// ctor
		// - initialization
		: handle( ::CreateFontIndirectW(&lf) ) {
		// - determining the AvgWidth and Height of Font characters
		const HDC screen=::GetDC(0);
			const HGDIOBJ hFont0=::SelectObject( screen, handle );
				TEXTMETRICW tm;
				::GetTextMetricsW( screen, &tm );
				charAvgWidth=tm.tmAveCharWidth;
				charHeight=tm.tmHeight;
			::SelectObject(screen,hFont0);
		::ReleaseDC(0,screen);
	}

	CYahelFont::~CYahelFont(){
		// dtor
		::DeleteObject(handle);
	}

	SIZE CYahelFont::GetTextSize(LPCTSTR text,int textLength) const{
		// determines and returns the Size of the specified Text using using this font face
		SIZE result={},tmp;
		const HDC screen=::GetDC(0);
			const HGDIOBJ hFont0=::SelectObject( screen, handle );
				for( LPCTSTR subA=text,subZ=subA; *subA; subZ++ )
					switch (*subZ){
						case '\0':
						case '\n':
							if (::GetTextExtentPoint32( screen, subA, subZ-subA, &tmp )){
								if (tmp.cx>result.cx)
									result.cx=tmp.cx;
								result.cy+=charHeight;
							}
							subA=subZ+(*subZ!='\0');
							break;
					}
			::SelectObject( screen, hFont0 );
		::ReleaseDC(0,screen);
		return result;
	}

	SIZE CYahelFont::GetTextSize(LPCTSTR text) const{
		// determines and returns the Size of the specified Text using using this font face
		return GetTextSize( text, ::lstrlen(text) );
	}





	CYahelContextMenu::CYahelContextMenu(UINT idMenuRes)
		// ctor
		: hResourceMenu( ::LoadMenu((HINSTANCE)&__ImageBase,MAKEINTRESOURCE(idMenuRes)) )
		, handle( ::GetSubMenu(hResourceMenu,0) ) {
	}

	CYahelContextMenu::~CYahelContextMenu(){
		// dtor
		::DestroyMenu(hResourceMenu);
	}

	static void UpdateUi(HMENU hMenu,const IOwner &owner){
		// recurrently adjusting UI of the ContextMenu
		for( auto i=::GetMenuItemCount(hMenu); i>0; )
			switch ( const auto id=::GetMenuItemID(hMenu,--i) ){
				case 0:
					// menu separators and invalid commands are ignored
					continue;
				case UINT_MAX:
					// recurrently updating Submenus
					if (const HMENU hSubMenu=::GetSubMenu(hMenu,i))
						UpdateUi( hSubMenu, owner );
					break;
				default:{
					// normal menu item
					const int flags=owner.GetCustomCommandMenuFlags(id)&~MF_BYPOSITION;
					if (id>=0){
						::EnableMenuItem( hMenu, id, flags );
						::CheckMenuItem( hMenu, id, flags );
					}//else
						//nop (don't auto-disable irresponsive items)
					break;
				}
			}
	}

	void CYahelContextMenu::UpdateUi(const IOwner &owner) const{
		Utils::UpdateUi( handle, owner );
	}





	CYahelDialog::CYahelDialog()
		// ctor
		: hDlg(0) {
	}

	UINT_PTR CYahelDialog::DoModal(UINT nIDTemplate,HWND hParent){
		assert(!hDlg);
		// - adopting Parent's Font or default system Font
		HFONT hFont=(HFONT)::SendMessageW( hParent, WM_GETFONT, 0, 0 );
		if (!hFont)
			hFont=(HFONT)::GetStockObject(DEFAULT_GUI_FONT);
		// - injecting Font information into the Dialog template
		const HRSRC hRes=::FindResource( (HINSTANCE)&__ImageBase, MAKEINTRESOURCE(nIDTemplate), RT_DIALOG );
		const HGLOBAL gRes=::LoadResource( (HINSTANCE)&__ImageBase, hRes );
		const LPCDLGTEMPLATE lpRes=(LPCDLGTEMPLATE)::LockResource( gRes ); // MSDN: doesn't actually lock memory; it is just used to obtain a pointer to the memory containing the resource data
			const auto cbRes=::SizeofResource( (HINSTANCE)&__ImageBase, hRes );
			struct{
				DLGTEMPLATE header;
				WCHAR str[4096];
			} dlgTemplate;
			if (hFont && (lpRes->style&DS_SETFONT)==0){ // just to be sure (Font determined, and the Dialog lacks explicit Font settings)
				// . copying template before Font information
				LPCWSTR p0=(LPCWSTR)(lpRes+1);
				for( BYTE n=3; n>0; n-=*p0++=='\0' );
				auto nBytesBeforeFont=(PBYTE)p0-(PBYTE)lpRes;
				::memcpy( &dlgTemplate, lpRes, nBytesBeforeFont );
				// . injecting Font (adopted from MFC library)
				PWCHAR p=(PWCHAR)((PBYTE)&dlgTemplate+nBytesBeforeFont);
				const CYahelFont::TLogFont lf(hFont);
				const HDC hDC=::GetDC(nullptr);
					*p++=::MulDiv( std::abs(lf.lfHeight), 72, ::GetDeviceCaps(hDC,LOGPIXELSY) );
				::ReleaseDC( nullptr, hDC );
				p+=::lstrlenW( ::lstrcpyW(p,lf.lfFaceName) );
				*(LPDWORD)p++=0;
				dlgTemplate.header.style|=DS_SETFONT;
				// . copying template after Font information (aligned to a DWORD, aka. 4 Bytes)
				nBytesBeforeFont=(nBytesBeforeFont+3)&~3;
				p+=((INT_PTR)p&3)!=0;
				::memcpy( p, (PBYTE)lpRes+nBytesBeforeFont, cbRes-nBytesBeforeFont );
			}else
				::memcpy( &dlgTemplate, lpRes, cbRes );
		::FreeResource(gRes);
		return	::DialogBoxIndirectParamW(
					(HINSTANCE)&__ImageBase, &dlgTemplate.header,
					hParent, DialogProc, (LPARAM)this
				);
	}

	INT_PTR WINAPI CYahelDialog::DialogProc(HWND hDlg,UINT msg,WPARAM wParam,LPARAM lParam){
		// dialog procedure
		switch (msg){
			case WM_INITDIALOG:{
				::SetWindowLong( hDlg, GWL_USERDATA, lParam );
				CYahelDialog *const pd=(CYahelDialog *)lParam;
				pd->hDlg=hDlg;
				return pd->InitDialog();
			}
			case WM_COMMAND:{
				CYahelDialog *const pd=(CYahelDialog *)::GetWindowLongA(hDlg,GWL_USERDATA);
				switch (LOWORD(wParam)){
					case IDOK:
						if (!pd->ValidateDialog())
							return TRUE;
						//fallthrough
					case IDCANCEL:
						::EndDialog( hDlg, LOWORD(wParam) );
						return TRUE;
					default:
						if (pd->OnCommand(wParam,lParam))
							return TRUE;
						break;
				}
				break;
			}
		}
		return FALSE;
	}

	bool CYahelDialog::EnableDlgItem(WORD id,bool enabled) const{
		// enables/disables the specified Dialog control and returns this new state
		::EnableWindow( ::GetDlgItem(hDlg,id), enabled );
		return enabled;
	}

	bool CYahelDialog::EnableDlgItems(PCWORD pIds,bool enabled) const{
		// enables/disables all specified Dialog controls and returns this new state
		while (const WORD id=*pIds++)
			::EnableWindow( ::GetDlgItem(hDlg,id), enabled );
		return enabled;
	}

	void CYahelDialog::InitDlgItemText(WORD id,...) const{
		//
		va_list argList;
		va_start( argList, id );
			WCHAR buf[80];
			GetDlgItemText( id, buf );
				::wvsprintf( buf, buf, argList );
			SetDlgItemText( id, buf );
		va_end(argList);
	}

	int CYahelDialog::GetDlgItemTextLength(WORD id) const{
		return ::GetWindowTextLength( GetDlgItemHwnd(id) );
	}

	HWND CYahelDialog::FocusDlgItem(WORD id) const{
		return ::SetFocus( GetDlgItemHwnd(id) );
	}

	RECT CYahelDialog::MapDlgItemClientRect(WORD id) const{
		//
		const HWND hItem=::GetDlgItem(hDlg,id);
		RECT tmp;
		::GetClientRect( hItem, &tmp );
		::MapWindowPoints( hItem, hDlg, (LPPOINT)&tmp, 2 );
		return tmp;
	}





	POINT MakePoint(LPARAM lParam){
		const POINT pt={ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		return pt;
	}

	COLORREF GetSaturatedColor(COLORREF currentColor,float saturationFactor){
		// saturates input Color by specified SaturationFactor and returns the result
		assert(saturationFactor>=0);
		COLORREF result=0;
		for( BYTE i=sizeof(COLORREF),*pbIn=(PBYTE)&currentColor,*pbOut=(PBYTE)&result; i-->0; ){
			const float f=*pbIn++*saturationFactor;
			*pbOut++=std::min( f, 255.f );
		}
		return result;
	}

	COLORREF GetBlendedColor(COLORREF color1,COLORREF color2,float blendFactor){
		// computes and returns the Color that is the mixture of the two input Colors in specified ratio (BlendFactor=0 <=> only Color1, BlendFactor=1 <=> only Color2
		assert(0.f<=blendFactor && blendFactor<=1.f);
		COLORREF result=0;
		for( BYTE i=sizeof(COLORREF),*pbIn1=(PBYTE)&color1,*pbIn2=(PBYTE)&color2,*pbOut=(PBYTE)&result; i-->0; ){
			const float f = blendFactor**pbIn1++ + (1.f-blendFactor)**pbIn2++;
			*pbOut++=std::min( f, 255.f );
		}
		return result;
	}

	void RandomizeData(PVOID buffer,TPosition nBytes){
		// populates Buffer with given CountOfBytes of random data
		::srand( ::GetTickCount() );
		for( PBYTE p=(PBYTE)buffer; nBytes-->0; *p++=::rand() );
	}

}
}
