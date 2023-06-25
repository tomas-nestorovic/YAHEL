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


	


	struct TLogBrush:public LOGBRUSH{
		TLogBrush(int stockObjectId){
			::GetObject( ::GetStockObject(stockObjectId), sizeof(*this), this );
		}
		TLogBrush(COLORREF solidColor){
			::ZeroMemory( this, sizeof(*this) );
			lbStyle=BS_SOLID;
			lbColor=solidColor;
		}
	};
	CYahelBrush::CYahelBrush(int stockObjectId)
		// ctor
		: LOGBRUSH( TLogBrush(stockObjectId) )
		, handle( (HBRUSH)::GetStockObject(stockObjectId) ) {
	}

	CYahelBrush::CYahelBrush(COLORREF solidColor,bool sysColor)
		// ctor
		: LOGBRUSH(  TLogBrush( sysColor?::GetSysColor(solidColor):solidColor )  )
		, handle( ::CreateBrushIndirect(this) ) {
	}

	CYahelBrush::~CYahelBrush(){
		// dtor
		::DeleteObject(handle);
	}

	const CYahelBrush CYahelBrush::None=NULL_BRUSH;
	const CYahelBrush CYahelBrush::Black=BLACK_BRUSH;
	const CYahelBrush CYahelBrush::White=WHITE_BRUSH;




	CYahelFont::TLogFont::TLogFont(LPCTSTR face,int pointHeight){
		// ctor
		::ZeroMemory( this, sizeof(*this) );
		lfHeight=(10*-pointHeight)/72;
		lfCharSet=DEFAULT_CHARSET;
		lfQuality=ANTIALIASED_QUALITY;
		::lstrcpyn( lfFaceName, face, ARRAYSIZE(lfFaceName) );
	}

	CYahelFont::TLogFont::TLogFont(HFONT hFont){
		// ctor
		::GetObject( hFont, sizeof(*this), this );
	}

	CYahelFont::CYahelFont(const LOGFONT &lf)
		// ctor
		// - initialization
		: handle( ::CreateFontIndirect(&lf) ) {
		// - determining the AvgWidth and Height of Font characters
		const HDC screen=::GetDC(0);
			const HGDIOBJ hFont0=::SelectObject( screen, handle );
				TEXTMETRIC tm;
				::GetTextMetrics( screen, &tm );
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
								subA=subZ+(*subZ!='\0');
							}
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
		return	::DialogBoxParam(
					(HINSTANCE)&__ImageBase, MAKEINTRESOURCE(nIDTemplate),
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





	CSingleNumberDialog::CSingleNumberDialog(LPCTSTR caption,LPCTSTR label,const TPosInterval &range,TPosition initValue,bool hexa)
		// ctor
		: caption(caption) , label(label) , range(range) , hexa(hexa)
		, Value(initValue) {
	}

	bool CSingleNumberDialog::InitDialog(){
		// dialog initialization
		::SetWindowText( hDlg, caption );
		// - instructions
		TCHAR buf[200], strMin[32], strMax[32];
		if (hexa){
			TCHAR format[32];
			::wsprintf( format, _T("0x%%0%dX"), ::wsprintf(strMax,_T("%x"),range.a|range.z) );
			::wsprintf( strMin, format, range.a );
			::wsprintf( strMax, format, range.z );
		}else{
			::wsprintf( strMin, _T("%d"), range.a );
			::wsprintf( strMax, _T("%d"), range.z );
		}
		const int nLabelChars=::lstrlen(label);
		if (label[nLabelChars-1]==')') // Label finishes with text enclosed in brackets
			::wsprintf( ::lstrcpy(buf,label)+nLabelChars-1, _T("; %s - %s):"), strMin, strMax );
		else
			::wsprintf( buf, _T("%s (%s - %s):"), label, strMin, strMax );
		SetDlgItemText( IDC_INFO1, buf );
		// - initial Value
		const HWND hValue=GetDlgItemHwnd(IDC_NUMBER);
		if (hexa){
			::SetWindowLong( hValue, GWL_STYLE, ::GetWindowLong(hValue,GWL_STYLE)&~ES_NUMBER );
			TCHAR buf[32];
			::wsprintf( buf, _T("0x%X"), Value );
			SetDlgItemText( IDC_NUMBER, buf );
		}else{
			::SetWindowLong( hValue, GWL_STYLE, ::GetWindowLong(hValue,GWL_STYLE)|ES_NUMBER );
			SetDlgItemInt( IDC_NUMBER, Value );
		}
		// - value format
		if (EnableDlgItem( IDC_HEXA, (range.a|range.z)>=0 ))
			CheckDlgButton( IDC_HEXA, hexa );
		else
			CheckDlgButton( IDC_HEXA, false );
		return true; // set the keyboard focus to the default control
	}

	bool CSingleNumberDialog::GetCurrentValue(TPosition &outValue) const{
		// True <=> input value successfully parsed, otherwise False
		TCHAR buf[32], *p=buf;
		auto nChars=GetDlgItemText( IDC_NUMBER, buf, ARRAYSIZE(buf) );
		if (hexa){
			if (nChars>2 && *buf=='0' && buf[1]=='x')
				p+=2, nChars-=2;
			else if (nChars>1 && (*buf=='$'||*buf=='#'||*buf=='%'))
				p++, nChars--;
			if (nChars>sizeof(Value)*2)
				return false;
			return	_stscanf( ::CharLower(p), _T("%x"), &outValue )>0;
		}else
			return	_stscanf( p, _T("%d"), &outValue )>0;
	}

	bool CSingleNumberDialog::ValidateDialog(){
		// True <=> Dialog inputs are acceptable, otherwise False
		return	GetCurrentValue(Value) ? range.Contains(Value) : false;
	}

	bool CSingleNumberDialog::OnCommand(WPARAM wParam,LPARAM lParam){
		// command processing
		switch (wParam){
			case MAKELONG(IDC_HEXA,BN_CLICKED):
				ValidateDialog();
				hexa=IsDlgButtonChecked(IDC_HEXA)!=BST_UNCHECKED;
				InitDialog();
				FocusDlgItem(IDC_NUMBER);
				Edit_SetSel( GetDlgItemHwnd(IDC_NUMBER), 0, -1 ); // selecting full content
				return true;
		}
		return false;
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
