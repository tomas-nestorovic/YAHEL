#include "stdafx.h"
#include "Instance.h"

namespace Yahel
{
namespace Gui
{
	LPCWSTR WINAPI GetDefaultEnglishMessage(TMsg id){
		switch (id){
			case ERROR_KOSHER:
				return L"Rabi approves";
			case ERROR_ITEM_DEF_BYTE_COUNT_UNKNOWN:
				return L"Missing item length in Bytes";
			case ERROR_ITEM_DEF_BYTE_COUNT:
				return L"Incorrect item length in Bytes (too long or too short)";
			case ERROR_ITEM_DEF_PATTERN_TOO_LONG:
				return L"Item pattern too long";
			case ERROR_ITEM_DEF_PATTERN_FORBIDDEN:
				return L"Item pattern contains forbidden characters";
			case ERROR_ITEM_DEF_PATTERN_NOT_PRINTABLE:
				return L"Item pattern contains non-printable characters";
			case ERROR_ITEM_DEF_PATTERN_INDEX:
				return L"Item pattern indices out of bounds";
			case ERROR_ITEM_DEF_PATTERN_NO_PLACEHOLDER:
				return L"Item pattern doesn't address the stream";
			case ERROR_ITEM_DEF_PATTERN_INSUFFICIENT_BUFFER:
				return L"Item pattern doesn't fit in caller's buffer";
			case ERROR_ITEM_COUNT_INVALID:
				return L"Invalid count of items per row";
			case ERROR_PASTE_FAILED:
				return L"Can't paste content";
			case ERROR_SELECTION_CANT_SAVE:
				return L"Can't save selection";

			case MSG_READY:
				return L"Ready";
			case MSG_LIMIT_UPPER:
				return L"The content reached its upper limit";
			case MSG_SELECTION_SAVED_PARTIALLY:
				return L"Selection saved only partially";
			case MSG_SELECTION_RESET_PARTIALLY:
				return L"Selection reset only partially";
			case MSG_PASTED_PARTIALLY:
				return L"Pasted only partially";
			case MSG_PADDED_TO_MINIMUM_SIZE:
				return L"To preserve the minimum size, the content has been padded with zeros";

			case QUERY_DELETE_SELECTED_BOOKMARKS:
				return L"Delete selected bookmarks?";
			case QUERY_DELETE_ALL_BOOKMARKS:
				return L"Delete all bookmarks?";

			default:
				return	id<MSG_READY ? L"General error" : nullptr;
		}
	}

	#define BUDDY_ID	ID_YAHEL_EDIT_RESET
	#define BUDDY_MARGIN 1

	bool WINAPI IsWindowIntHexa(HWND hEditBox){
		// True <=> EditBox content is to be interpreted in hexa-decimal form
		return ::IsDlgButtonChecked( hEditBox, BUDDY_ID )==BST_CHECKED;
	}

	bool WINAPI IsDlgItemIntHexa(HWND hDlg,UINT idEditBox){
		// True <=> EditBox content is to be interpreted in hexa-decimal form
		return IsWindowIntHexa( ::GetDlgItem(hDlg,idEditBox) );
	}

	#define DecimalCharsMax 32
		// enough to accommodate the largest/smallest 64-bit number

	static INT64 GetWindowInt(HWND hEditBox){
		WCHAR buf[DecimalCharsMax];
		::GetWindowTextW( hEditBox, buf, ARRAYSIZE(buf) );
		INT64 i;
		::StrToInt64ExW( buf, STIF_SUPPORT_HEX, &i );
		return i;
	}

	TPosition WINAPI GetDlgItemInt(HWND hDlg,UINT idEditBox){
		return GetWindowInt( ::GetDlgItem(hDlg,idEditBox) );
	}

	static void SetWindowInt(HWND hEditBox,INT64 i){
		WCHAR buf[DecimalCharsMax];
		::wsprintfW( buf, L"%I64i", i );
		::SetWindowTextW( hEditBox, buf );
		::SendMessageW( ::GetParent(hEditBox), WM_COMMAND, MAKELONG(::GetWindowLongW(hEditBox,GWL_ID),EN_CHANGE), (LPARAM)hEditBox );
	}

	void WINAPI SetDlgItemInt(HWND hDlg,UINT idEditBox,TPosition value){
		SetWindowInt( ::GetDlgItem(hDlg,idEditBox), value );
	}

	#define TEXT_BUDDY	L"hex"
	#define TEXT_HEXA	L"0x"

	void WINAPI SetWindowIntBuddyW(HWND hEditBox,TPosition defaultValue,TNotation defaultNotation,bool protrudeEditBox){
		// creates a default Buddy checkbox inside an existing EditBox
		if (!hEditBox)
			return;
		const long style=::GetWindowLongW( hEditBox, GWL_STYLE );
		if ((style&ES_MULTILINE)==0)
			return; // must be created with the Multiline style for better visual experience on Win10 and later
		// - create the Buddy
		const HWND hBuddy=::CreateWindowExW( 0, WC_BUTTONW, TEXT_BUDDY,
			WS_CHILD|WS_VISIBLE|BS_PUSHLIKE|BS_CHECKBOX,
			0,0, 1,1, // positioned and sized in WM_SIZE
			hEditBox, (HMENU)BUDDY_ID, 0, nullptr
		);
		::CheckDlgButton( hEditBox, BUDDY_ID, BST_CHECKED*(defaultNotation==TNotation::Hexa) );
		// - derive from EditBox
		struct TParams sealed{
			const WNDPROC wndProcOrg;
			const Utils::CYahelFont font;
			const int buddyWidth;

			TParams(WNDPROC wndProcOrg,const Utils::CYahelFont::TLogFont &lf)
				: wndProcOrg(wndProcOrg) , font(lf)
				, buddyWidth( font.GetTextSize(TEXT_BUDDY).cx*ARRAYSIZE(TEXT_BUDDY)/(ARRAYSIZE(TEXT_BUDDY)-1) ) { // aka. "N+1/N characters"
			}
		};
		struct F{
			static void SelectAll(HWND hEditBox){
				::SendMessageW( hEditBox, EM_SETSEL, 0, -1 );
			}

			static HRESULT CALLBACK WndProcW(HWND hEditBox,UINT msg,WPARAM wParam,LPARAM lParam){
				const HWND hBuddy=::GetDlgItem( hEditBox, BUDDY_ID );
				const TParams &p=*(TParams *)::GetWindowLongW( hBuddy, GWL_USERDATA );
				switch (msg){
					case WM_SIZE:{
						// window size changed
						::CallWindowProcW( p.wndProcOrg, hEditBox, msg, wParam, lParam );
						RECT rc;
						Edit_GetRect( hEditBox, &rc );
						rc.right-=p.buddyWidth+BUDDY_MARGIN;
						Edit_SetRect( hEditBox, &rc ); // make space to accommodate the Buddy
						::SetWindowPos( // position the Buddy
							hBuddy, 0,
							LOWORD(lParam)-p.buddyWidth-BUDDY_MARGIN, BUDDY_MARGIN, p.buddyWidth, HIWORD(lParam)-2*BUDDY_MARGIN,
							SWP_NOZORDER
						);
						return 0;
					}
					case WM_KEYDOWN:
						// a key has been pressed
						if (::GetKeyState(VK_CONTROL)<0)
							switch (wParam){
								case 'A': // want select all text
									SelectAll(hEditBox);
									break;
								case VK_UP: // want increment value by magnitude
									SetWindowInt( hEditBox, GetWindowInt(hEditBox)+(10+6*IsWindowIntHexa(hEditBox)) );
									break;
								case VK_DOWN: // want decrement value by magnitude
									SetWindowInt( hEditBox, GetWindowInt(hEditBox)-(10+6*IsWindowIntHexa(hEditBox)) );
									break;
							}
						else
							switch (wParam){
								case VK_UP: // want increment value
									SetWindowInt( hEditBox, GetWindowInt(hEditBox)+1 );
									break;
								case VK_DOWN: // want decrement value
									SetWindowInt( hEditBox, GetWindowInt(hEditBox)-1 );
									break;
							}
						break;
					case WM_CHAR:
						// character
						if (::StrChrIW( L"\x8\x3\x16", wParam ))
							break; // pass through backspace, ETX (Ctrl+C), and SYN (Ctrl+V) chars
						if (wParam=='-'){ // want negative number ?
							if (IsWindowIntHexa(hEditBox))
								return 0; // can't enter minus sign in Hexa Notation
							WCHAR buf[2];
							if (::GetWindowTextW(hEditBox,buf,ARRAYSIZE(buf)) && *buf=='-')
								return 0; // block insertion of second minus sign
							Edit_SetSel( hEditBox, 0, 0 );
							break; // prepend an existing Decimal number with a minus sign
						}
						if ('0'<=wParam && wParam<='9')
							break; // pass through all digits
						if ('a'<=wParam && wParam<='z')
							wParam-=32; // convert to capitals
						if (wParam=='X') // want toggle Notation ?
							return ::SendMessageW( hEditBox, WM_COMMAND, BUDDY_ID, (LPARAM)::GetDlgItem(hEditBox,BUDDY_ID) ); // don't use "::CheckDlgButton" as it doesn't send BN_CLICKED
						if (wParam<'A' || 'F'<wParam)
							return 0; // block all non-hexa chars
						if (IsWindowIntHexa(hEditBox))
							break; // do nothing extra if already in hexa-mode
						::CheckDlgButton( hEditBox, BUDDY_ID, BST_CHECKED ); // upgrade notation without sending BN_CLICKED
						break; // enter character
					case WM_COMMAND:
						switch (wParam){
							case MAKELONG(BUDDY_ID,BN_CLICKED):{
								// toggle number base, Decimal vs Hexadecimal
								// . toggle
								WCHAR buf[DecimalCharsMax];
								::GetWindowTextW( hEditBox, buf, ARRAYSIZE(buf) );
								::CheckDlgButton( hEditBox, BUDDY_ID, BST_CHECKED*!IsWindowIntHexa(hEditBox) ); // toggle, see '!'
								::SetWindowTextW( hEditBox, buf );
								// . focus EditBox (focus may be set to the Buddy instead)
								::SetFocus(hEditBox);
								// . select all text
								::SendMessage( hEditBox, EM_SETSEL, 0, -1 );
								break;
							}
						}
						break;
					case WM_GETTEXT:{
						// want Decimal notation
						WCHAR buf[2*ARRAYSIZE(TEXT_HEXA)+DecimalCharsMax];
						static_assert( (ARRAYSIZE(TEXT_HEXA)-1)*sizeof(*buf)==sizeof(int), "" ); // see 'PINT' below
						*(PINT)buf=MAKELONG( ' ', ' ' );
						::CallWindowProcW(
							p.wndProcOrg, hEditBox, msg, ARRAYSIZE(buf),
							(LPARAM)(buf+ARRAYSIZE(TEXT_HEXA)-1)
						);
						if (IsWindowIntHexa(hEditBox))
							if (!::StrChrIW(buf,'x')) // missing the '0x' prefix ?
								*(PINT)buf=*(PINT)TEXT_HEXA;
						INT64 i;
						return	::StrToInt64ExW( buf, STIF_SUPPORT_HEX, &i )
								? ::wsprintfW( (LPWSTR)lParam, L"%I64i", i )
								: ( *(LPWSTR)lParam='\0' );
					}
					case WM_GETTEXTLENGTH:
						// want Decimal Notation length
						return 0; //TODO
					case WM_SETTEXT:{
						// providing Hexa/Decimal notation
						INT64 i=0;
						::StrToInt64ExW( (LPCWSTR)lParam, STIF_SUPPORT_HEX, &i );
						if (IsWindowIntHexa(hEditBox))
							::wsprintfW( (LPWSTR)lParam, TEXT_HEXA L"%I64X", i );
						else
							::wsprintfW( (LPWSTR)lParam, L"%I64i", i );
						break;
					}
					case WM_COPY:{
						// copy to the clipboard in current Notation
						WCHAR buf[ARRAYSIZE(TEXT_HEXA)+DecimalCharsMax];
						::GetWindowTextW( hEditBox, buf, ARRAYSIZE(buf) );
						::SetWindowTextW( hEditBox, buf ); // assure number in the correct Notation
						SelectAll(hEditBox); // indicate whole number copied
						break;
					}
					case WM_PASTE:{
						// paste from the clipboard in provided Notation
						::CallWindowProcW( // paste
							p.wndProcOrg, hEditBox, msg, wParam, lParam
						);
						WCHAR buf[ARRAYSIZE(TEXT_HEXA)+DecimalCharsMax];
						::CallWindowProcW( // get what's been pasted without processing it (aka. without 'GetWindowText')
							p.wndProcOrg, hEditBox, WM_GETTEXT, ARRAYSIZE(buf), (LPARAM)buf
						);
						::SetWindowTextW( hEditBox, buf ); // assure number in the correct Notation
						Edit_SetSel( hEditBox, DecimalCharsMax, -1 ); // caret to the end
						return 0;
					}
					case WM_DESTROY:
						// window destroyed
						// . revert subclassing (and avoid WM_NCDESTROY that would cause a crash)
						::SetWindowLongW( hEditBox, GWL_WNDPROC, (LONG)p.wndProcOrg );
						// . base
						::CallWindowProcW( p.wndProcOrg, hEditBox, msg, wParam, lParam );
						// . cleanup and done
						delete &p;
						return 0;
				}
				return ::CallWindowProcW( p.wndProcOrg, hEditBox, msg, wParam, lParam );
			}
		};
		::SetWindowLongW( hEditBox, GWL_STYLE, style&~ES_NUMBER ); // remove ES_NUMBER (want hexa 'a-f' chars)
		Utils::CYahelFont::TLogFont lf( (HFONT)::SendMessageW(hEditBox,WM_GETFONT,0,0) );
			::lstrcpy( lf.lfFaceName, FONT_COURIER_NEW );
		TParams *const pParams=new TParams(
			(WNDPROC)::SetWindowLongW( hEditBox, GWL_WNDPROC, (LONG)F::WndProcW ),
			lf
		);
		::SetWindowLongW( hBuddy, GWL_USERDATA, (LONG)pParams );
		::SendMessageW( hBuddy, WM_SETFONT, pParams->font, 0 );
		// - want preserve the current size of the EditBox ?
		RECT rc;
		if (protrudeEditBox){
			::GetWindowRect( hEditBox, &rc );
			::SetWindowPos( hEditBox, // also puts Buddy into correct place
				0, 0,0, rc.right-rc.left+2*BUDDY_MARGIN+pParams->buddyWidth, rc.bottom-rc.top, SWP_NOZORDER|SWP_NOMOVE
			);
		}else{
			::GetClientRect( hEditBox, &rc );
			::SendMessageW( hEditBox, // must send dummy sizing to invoke Buddy positioning
				WM_SIZE, 0, MAKELONG(rc.right,rc.bottom)
			);
		}
		// - set DefaultValue
		SetWindowInt( hEditBox, defaultValue );
	}

	void WINAPI SetDlgItemIntBuddyW(HWND hDlg,UINT idEditBox,TPosition defaultValue,TNotation defaultNotation,bool protrudeEditBox){
		// creates a default Buddy checkbox inside an existing EditBox
		SetWindowIntBuddyW( ::GetDlgItem(hDlg,idEditBox), defaultValue, defaultNotation, protrudeEditBox );
	}

	bool YAHEL_DECLSPEC WINAPI QuerySingleIntA(LPCSTR caption,LPCSTR label,const TPosInterval &range,TPosition &inOutValue,TNotation defaultNotation,HWND hParent){
		WCHAR captionW[128], labelW[256];
		::wsprintfW( captionW, L"%S", caption );
		::wsprintfW( labelW, L"%S", label );
		return QuerySingleIntW( captionW, labelW, range, inOutValue, defaultNotation, hParent );
	}

	bool WINAPI QuerySingleIntW(LPCWSTR caption,LPCWSTR label,const TPosInterval &rangeIncl,TPosition &inOutValue,TNotation defaultNotation,HWND hParent){
		// - must be able to set at least two options
		if (rangeIncl.GetLength()<1) // inclusive!
			return false;
		// - initial value must be within Range
		if (!rangeIncl.Contains(inOutValue) && inOutValue!=rangeIncl.z) // inclusive!
			return false;
		// - defining the Dialog
		class CSingleNumberDialog sealed:public Utils::CYahelDialog{
			const LPCWSTR caption,label;
			const TPosInterval rangeIncl;
			const TNotation defaultNotation;

			bool InitDialog() override{
				// dialog initialization
				::SetWindowText( hDlg, caption );
				SetDlgItemIntBuddyW( hDlg, IDC_NUMBER, Value, defaultNotation, false );
				ReformulateInstructions();
				return true; // set the keyboard focus to the default control
			}

			void ReformulateInstructions(){
				// reformulates instructions using current Notation
				TCHAR buf[200], strMin[32], strMax[32];
				if (IsDlgItemIntHexa( hDlg, IDC_NUMBER )){
					TCHAR format[32];
					::wsprintf( format, _T("0x%%0%dX"), ::wsprintf(strMax,_T("%x"),rangeIncl.a|rangeIncl.z) );
					::wsprintf( strMin, format, rangeIncl.a );
					::wsprintf( strMax, format, rangeIncl.z );
				}else{
					::wsprintf( strMin, _T("%d"), rangeIncl.a );
					::wsprintf( strMax, _T("%d"), rangeIncl.z );
				}
				const int nLabelChars=::lstrlen(label);
				if (label[nLabelChars-1]==')') // Label finishes with text enclosed in brackets
					::wsprintf( ::lstrcpy(buf,label)+nLabelChars-1, _T("; %s - %s):"), strMin, strMax );
				else
					::wsprintf( buf, _T("%s (%s - %s):"), label, strMin, strMax );
				SetDlgItemText( IDC_INFO1, buf );
			}

			bool ValidateDialog() override{
				// True <=> Dialog inputs are acceptable, otherwise False
				Value=GetDlgItemInt(IDC_NUMBER);
				return	rangeIncl.Contains(Value) || Value==rangeIncl.z;
			}

			bool OnCommand(WPARAM wParam,LPARAM lParam) override{
				// command processing
				switch (wParam){
					case MAKELONG(IDC_NUMBER,EN_CHANGE):{
						ReformulateInstructions();
						EnableDlgItem( IDOK, ValidateDialog() ); // indicate validity
						return true;
					}
				}
				return false;
			}
		public:
			TPosition Value;

			CSingleNumberDialog(LPCWSTR caption,LPCWSTR label,const TPosInterval &rangeIncl,TPosition initValue,TNotation defaultNotation)
				// ctor
				: caption(caption) , label(label) , rangeIncl(rangeIncl) , defaultNotation(defaultNotation)
				, Value(initValue) {
			}
		} d( caption, label, rangeIncl, inOutValue, defaultNotation );
		// - showing the Dialog and processing its result
		const bool confirmed=d.DoModal( IDR_YAHEL_SINGLE_NUMBER, hParent )==IDOK;
		if (hParent)
			::SetFocus(hParent);
		if (confirmed)
			inOutValue=d.Value;
		return confirmed;
	}
}





namespace Stream
{
	TPosition WINAPI GetMaximumLength(){
		return GetErrorPosition()-1;
	}

	TPosition WINAPI GetErrorPosition(){
		return LLONG_MAX;
	}

	TPosition WINAPI GetLength(IStream *s){
		ULARGE_INTEGER uli;
		return	SUCCEEDED(IStream_Size( s, &uli ))
				? uli.QuadPart
				: GetErrorPosition();
	}






	class CBufferStream:public IStream{
		volatile ULONG nReferences;
		const PBYTE buffer;
		TPosition bufferLength;
		TPosition position;
	public:
		CBufferStream(PVOID buffer,TPosition bufferLength)
			// ctor
			: nReferences(1)
			, buffer((PBYTE)buffer) , bufferLength(bufferLength) , position(0) {
		}

		// IUnknown methods
		ULONG STDMETHODCALLTYPE AddRef() override{
			return ::InterlockedIncrement(&nReferences);
		}

		ULONG STDMETHODCALLTYPE Release() override{
			if (const auto n=::InterlockedDecrement(&nReferences))
				return n;
			delete this;
			return 0;
		}

		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,PVOID *ppvObject) override{
			return E_NOINTERFACE;
		}

		// IStream methods
		HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER *plibNewPosition) override{
			TPosition offset=dlibMove.QuadPart;
			switch (dwOrigin){
				case STREAM_SEEK_CUR:
					offset+=position;
					//fallthrough
				case STREAM_SEEK_SET:
					position=std::min( offset, bufferLength );
					break;
				case STREAM_SEEK_END:
					position=std::max( bufferLength-offset, 0LL );
					break;
				default:
					assert(FALSE);
					return E_INVALIDARG;
			}
			if (plibNewPosition)
				plibNewPosition->QuadPart=position;
			return S_OK;
		}

		HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER libNewSize) override{
			if (libNewSize.QuadPart>bufferLength)
				::ZeroMemory( buffer+bufferLength, libNewSize.QuadPart-bufferLength );
			bufferLength=libNewSize.QuadPart;
			return S_OK;
		}

		HRESULT STDMETHODCALLTYPE CopyTo(IStream *pstm,ULARGE_INTEGER cb,ULARGE_INTEGER *pcbRead,ULARGE_INTEGER *pcbWritten) override{
			return E_NOTIMPL;
		}

		HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags) override{
			return E_NOTIMPL;
		}

		HRESULT STDMETHODCALLTYPE Revert() override{
			return E_NOTIMPL;
		}

		HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER libOffset,ULARGE_INTEGER cb,DWORD dwLockType) override{
			return E_NOTIMPL;
		}

		HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER libOffset,ULARGE_INTEGER cb,DWORD dwLockType) override{
			return E_NOTIMPL;
		}

		HRESULT STDMETHODCALLTYPE Stat(STATSTG *pstatstg,DWORD grfStatFlag) override{
			if (!pstatstg)
				return E_INVALIDARG;
			::ZeroMemory( pstatstg, sizeof(*pstatstg) );
			pstatstg->type=STGTY_STREAM;
			pstatstg->cbSize.QuadPart=bufferLength;
			return S_OK;
		}

		HRESULT STDMETHODCALLTYPE Clone(IStream **ppstm) override{
			if (ppstm){
				*ppstm=FromBuffer( buffer, bufferLength );
				return S_OK;
			}else
				return E_INVALIDARG;
		}

		// ISequentialStream methods
		HRESULT STDMETHODCALLTYPE Read(PVOID target,ULONG nCount,PULONG pcbRead) override{
			// tries to read given NumberOfBytes into the Buffer, starting with current Position; returns the number of Bytes actually read (increments the Position by this actually read number of Bytes)
			if (!target)
				return E_INVALIDARG;
			nCount=std::min( nCount, (ULONG)(bufferLength-position) );
			::memcpy( target, buffer+position, nCount );
			position+=nCount;
			if (pcbRead)
				*pcbRead=nCount;
			return S_OK;
		}

		HRESULT STDMETHODCALLTYPE Write(LPCVOID data,ULONG dataLength,PULONG pcbWritten) override{
			// tries to write given NumberOfBytes from the Buffer to the current Position (increments the Position by the number of Bytes actually written)
			if (!data)
				return E_INVALIDARG;
			::memcpy( buffer+position, data, dataLength );
			bufferLength=std::max( bufferLength, position+=dataLength );
			if (pcbWritten)
				*pcbWritten=dataLength;
			return S_OK;
		}
	};

	IStream * WINAPI FromBuffer(PVOID pBuffer,TPosition length){
		return new CBufferStream(pBuffer,length);
	}






	IStream * WINAPI FromFileForSharedReading(LPCWSTR fileName,DWORD dwFlagsAndAttributes){
		IStream *s;
		return	SUCCEEDED(::SHCreateStreamOnFileEx( fileName, STGM_READ|STGM_SHARE_DENY_WRITE, dwFlagsAndAttributes, FALSE, nullptr, &s ))
				? s
				: nullptr;
	}






	// see also https://github.com/Microsoft/Windows-classic-samples/blob/main/Samples/Win7Samples/winui/shell/appplatform/DragDropVisuals/DataObject.cpp
	class COleDataObject:public IDataObject{
		volatile ULONG nReferences;
		//const UINT cfPasteSucceeded;
		const CInstance::TContent f;
		const TPosInterval range;
		struct{
			ULONG n;
			FORMATETC list[2];
			UINT tymedMask;

			void Add(UINT tymed){
				assert( n<ARRAYSIZE(list) );
				assert( (tymedMask&tymed)==0 );
				FORMATETC &r=list[n++];
					r.cfFormat=IInstance::GetClipboardFormat();
					r.dwAspect=DVASPECT_CONTENT;
					r.lindex=-1;
					r.tymed=tymed;
				tymedMask|=tymed;
			}
		} supportedFormats;
	public:
		COleDataObject(IStream &s,const TPosInterval &range)
			: nReferences(1)
			//, cfPasteSucceeded( ::RegisterClipboardFormat(CFSTR_PASTESUCCEEDED) )
			, f(&s) , range(range) {
			::ZeroMemory( &supportedFormats, sizeof(supportedFormats) );
			supportedFormats.Add(TYMED_HGLOBAL);
		}
		virtual ~COleDataObject(){
		}

		// IUnknown methods
		ULONG STDMETHODCALLTYPE AddRef() override{
			return ::InterlockedIncrement(&nReferences);
		}

		ULONG STDMETHODCALLTYPE Release() override{
			if (const auto n=::InterlockedDecrement(&nReferences))
				return n;
			delete this;
			return 0;
		}

		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,PVOID *ppvObject) override{
			static const QITAB qit[]={
				QITABENT( COleDataObject, IDataObject ),
				{ 0 },
			};
			return ::QISearch( this, qit, riid, ppvObject );
		}

		// IDataObject methods
        HRESULT STDMETHODCALLTYPE GetData(LPFORMATETC pFormatEtc,LPSTGMEDIUM outMedium) override{
			if (outMedium->tymed!=TYMED_NULL)
				return GetDataHere( pFormatEtc, outMedium );
			outMedium->pUnkForRelease=nullptr;
			const auto nBytesToRead=range.GetLength();
			switch ( outMedium->tymed=pFormatEtc->tymed ){
				case TYMED_HGLOBAL:
					if (outMedium->hGlobal=::GlobalAlloc( GPTR, nBytesToRead )){
						const HRESULT hr=GetDataHere( pFormatEtc, outMedium );
						if (SUCCEEDED(hr)) // some Bytes written ...
							if (const auto nMissing=range.z-f.GetPosition()) // ... but not all of them?
								if (const HGLOBAL hRealloc=::GlobalReAlloc( outMedium->hGlobal, nBytesToRead-nMissing, 0 ))
									outMedium->hGlobal=hRealloc;
								else{
									::GlobalFree( outMedium->hGlobal );
									return E_OUTOFMEMORY;
								}
						return hr;
					}
					return E_OUTOFMEMORY;
			}
			return CO_E_NOT_SUPPORTED; // we shouldn't end up here!
		}

        HRESULT STDMETHODCALLTYPE GetDataHere(LPFORMATETC pFormatEtc,LPSTGMEDIUM pMedium) override{
			const HRESULT hResult=QueryGetData(pFormatEtc);
			if (FAILED(hResult))
				return hResult;
			if (!pMedium)
				return E_INVALIDARG;
			auto nBytesToRead=range.GetLength();
			switch (pMedium->tymed){
				case TYMED_HGLOBAL:
					if (::GlobalSize(pMedium->hGlobal)<range.GetLength())
						return E_NOT_SUFFICIENT_BUFFER;
					if (const auto &&data=Utils::CGlobalMemLocker(pMedium->hGlobal)){
						PBYTE p=data; f.Seek(range.a);
						while (nBytesToRead>0)
							if (const auto nBytesRead=f.Read( p, nBytesToRead, IgnoreIoResult ))
								p+=nBytesRead, nBytesToRead-=nBytesRead;
							else
								break;
						return S_OK;
					}else
						return STG_E_LOCKVIOLATION;
			}
			return CO_E_NOT_SUPPORTED; // we shouldn't end up here!
		}

        HRESULT STDMETHODCALLTYPE QueryGetData(LPFORMATETC pFormatEtc) override{
			if (!pFormatEtc)
				return DV_E_FORMATETC;
			if (pFormatEtc->lindex!=-1)
				return DV_E_LINDEX;
			if (pFormatEtc->cfFormat!=IInstance::GetClipboardFormat())
				return DV_E_FORMATETC;
			if ((pFormatEtc->tymed&supportedFormats.tymedMask)==0)
				return DV_E_TYMED;
			return S_OK;
		}

        HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(LPFORMATETC pFormatEtcIn,LPFORMATETC pFormatEtcOut) override{
			const HRESULT hResult=QueryGetData(pFormatEtcIn);
			if (FAILED(hResult))
				return hResult;
			if (!pFormatEtcOut)
				return E_INVALIDARG;
			( *pFormatEtcOut=*pFormatEtcIn ).ptd=nullptr; // target-device (ptd) for server metafile format is not supported
			return DATA_S_SAMEFORMATETC;
		}

        HRESULT STDMETHODCALLTYPE SetData(LPFORMATETC pFormatEtc,LPSTGMEDIUM pMedium,BOOL fRelease) override{
			return E_UNEXPECTED; // construct another IDataObject to change the source Stream!
		}

        HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD dwDirection,IEnumFORMATETC **ppEnumFormatEtc) override{
			if (dwDirection!=DATADIR_GET)
				return E_NOTIMPL;
			if (!ppEnumFormatEtc)
				return E_INVALIDARG;
			return ::SHCreateStdEnumFmtEtc( supportedFormats.n, supportedFormats.list, ppEnumFormatEtc );
		}

		HRESULT STDMETHODCALLTYPE DAdvise(LPFORMATETC pFormatEtc,DWORD advf,LPADVISESINK pAdvSink,DWORD *pdwConnection) override{
			return OLE_E_ADVISENOTSUPPORTED;
		}

        HRESULT STDMETHODCALLTYPE DUnadvise(DWORD dwConnection) override{
			return OLE_E_ADVISENOTSUPPORTED;
		}

        HRESULT STDMETHODCALLTYPE EnumDAdvise(IEnumSTATDATA **ppenumAdvise) override{
			return OLE_E_ADVISENOTSUPPORTED;
		}
	};

	IDataObject * WINAPI CreateDataObject(IStream *s,const TPosInterval &range){
		//
		return	s!=nullptr && range.IsValidNonempty()
				? new COleDataObject( *s, range )
				: nullptr;
	}






	TPosition IAdvisor::GetMaximumRecordLength(){
		return 0x7fffff00;
	}
}





	LPCTSTR IInstance::GetVersionString(){
		return DLL_VERSION;
	}

	void IInstance::ShowModalAboutDialog(HWND hParent){
		SYSTEMTIME st;
		::GetLocalTime(&st);
		TCHAR buf[80];
		::wsprintf( buf, DLL_FULLNAME _T("\n\nVersion ") DLL_VERSION _T("\n\n\ntomascz, 2023-%d"), st.wYear );
		::MessageBox(
			hParent,
			buf,
			_T("About ") DLL_ABBREVIATION,
			MB_OK
		);
	}

	LPCSTR IInstance::GetBaseClassNameA(HINSTANCE hInstance){
		return WC_EDITA;
	}

	LPCWSTR IInstance::GetBaseClassNameW(HINSTANCE hInstance){
		return WC_EDITW;
	}

	UINT IInstance::GetClipboardFormat(){
		return ::RegisterClipboardFormat(CLIPFORMAT_YAHEL_BINARY);
	}

	IInstance *IInstance::Create(HINSTANCE hInstance,POwner pOwner,PVOID lpParam,HFONT hFont){
		if (pOwner)
			return new CInstance( hInstance, pOwner, lpParam, hFont );
		else
			return nullptr;
	}

	LPCWSTR IInstance::GetDefaultByteItemDefinition(){
		return L"1;Aa ";
	}

	bool IInstance::DefineItemUsingDefaultEnglishDialog(PWCHAR definitionBuffer,BYTE bufferCapacity,HWND hParent,HFONT hFont){
		//
		// - the PatternBuffer must accommodate at least one char
		static_assert( ITEM_STREAM_BYTES_MAX<=99, "" ); // must otherwise revise the below BufferCapacity
		if (!definitionBuffer || bufferCapacity<ITEM_STREAM_BYTES_SPEC_LEN+ITEM_PATTERN_LENGTH_MIN+1) // capacity of "27;Aa ", see Assert above
			return false;
		// - defining the Dialog
		class CItemDefinitionDialog sealed:public Utils::CYahelDialog{
			const PWCHAR definitionBuffer;
			const BYTE bufferCapacity, patternLengthMax;
			CInstance hexaEditor;
			BYTE sampleStreamBytes[ITEM_STREAM_BYTES_MAX];
			bool processNotifications; // notifications on user's interaction with Dialog

			void AddPresetItem(LPCWSTR name,LPCWSTR pattern){
				WCHAR buf[80];
				::wsprintfW( buf, L"%s  [%s]", name, pattern );
				const HWND hPresets=GetDlgItemHwnd(IDC_PRESETS);
				ComboBox_SetItemData( hPresets, ::SendMessage(hPresets,CB_ADDSTRING,0,(LPARAM)buf), pattern );
			}
			void UpdateStreamLength(){
				if (const auto &s=hexaEditor.GetCurrentStream()){
					ULARGE_INTEGER uli;
						uli.QuadPart=item.nStreamBytes;
					s->SetSize(uli);
				}
				hexaEditor.RepaintData();
			}
			void ShowItem(){
				const Utils::CVarTempReset<bool> pn0( processNotifications, false ); // don't recurrently trigger notifications
				::SetDlgItemInt( hDlg, IDC_NUMBER, item.nStreamBytes, FALSE );
				UpdateStreamLength();
				WCHAR pattern[ARRAYSIZE(item.pattern)+1];
				SetDlgItemText( IDC_PATTERN, item.GetDefinition(pattern) );
			}
			bool InitDialog() override{
				// Dialog initialization
				// . Presets
				AddPresetItem( L"Byte", GetDefaultByteItemDefinition() );
				AddPresetItem( L"Word", L"2;AaBb " );
				AddPresetItem( L"Custom", nullptr );
				// . labels
				InitDlgItemText( IDC_INFO1, ITEM_STREAM_BYTES_MAX );
				InitDlgItemText( IDC_INFO2, ITEM_PATTERN_LENGTH_MIN, patternLengthMax );
				// . seeing if initial Item is one of Presets
				ShowItem();
				RecognizePresetItem();
				// . (!!) creation and initial populating of the HexaEditor; MUST BE THE LAST IN THIS METHOD ...
				Gui::SetDlgItemIntBuddyW( hDlg, IDC_NUMBER, item.nStreamBytes, Gui::Decimal, true );
				hexaEditor.SubclassAndAttach( GetDlgItemHwnd(IDC_FILE) );
				processNotifications=true; // ... BECAUSE OF THIS
				SendCommand( MAKELONG(IDC_NUMBER,EN_CHANGE) );
				return true; // set the keyboard focus to the default control
			}
			TError TrySaveDefinition(){
				// attempts to redefine an Item from current inputs; returns a DWORD-encoded error
				const int nStreamBytes=GetDlgItemInt(IDC_NUMBER);
				TError err=ERROR_KOSHER; // assumption
				if (nStreamBytes<=0 || ITEM_STREAM_BYTES_MAX<nStreamBytes)
					err=ERROR_ITEM_DEF_BYTE_COUNT;
				else if (patternLengthMax<GetDlgItemTextLength(IDC_PATTERN))
					err=ERROR_ITEM_DEF_PATTERN_INSUFFICIENT_BUFFER;
				else{
					const int i=::wsprintfW( definitionBuffer, L"%d;", nStreamBytes );
					::GetDlgItemText( hDlg, IDC_PATTERN, definitionBuffer+i, bufferCapacity-i-1 );
					err=item.Redefine(definitionBuffer);
				}
				if (EnableDlgItem( IDOK, !err ))
					SetDlgItemText( IDC_ERROR, nullptr );
				else
					SetDlgItemText( IDC_ERROR, Gui::GetDefaultEnglishMessage(err.code) );
				return err;
			}
			void RecognizePresetItem(){
				// selects the predefined Item that matches current inputs
				const HWND hPresets=GetDlgItemHwnd(IDC_PRESETS);
				const auto nItems=ComboBox_GetCount(hPresets);
				if (TrySaveDefinition()==ERROR_KOSHER)
					for( auto i=nItems; i>0; )
						if (!::lstrcmpW( definitionBuffer, (LPCWSTR)ComboBox_GetItemData(hPresets,--i) )){
							ComboBox_SetCurSel( hPresets, i );
							return;
						}
				ComboBox_SetCurSel( hPresets, nItems-1 ); // "Custom"
				ValidateDialog(); // show eventual errors in custom Item definition
			}
			bool ValidateDialog(){
				// True <=> Dialog inputs are acceptable, otherwise False
				return TrySaveDefinition()==ERROR_KOSHER;
			}
			bool OnCommand(WPARAM wParam,LPARAM lParam){
				// command processing
				if (processNotifications)
					switch (wParam){
						case MAKELONG(IDC_PRESETS,CBN_SELCHANGE):{
							const HWND hPresets=GetDlgItemHwnd(IDC_PRESETS);
							if (const LPCWSTR def=(LPCWSTR)ComboBox_GetItemData(hPresets,ComboBox_GetCurSel(hPresets))){ // one of Presets?
								item.Redefine(def);
								ShowItem();
							}
							break;
						}
						case MAKELONG(IDC_NUMBER,EN_CHANGE):
							if (TrySaveDefinition()==ERROR_KOSHER)
								UpdateStreamLength();
							//fallthrough
						case MAKELONG(IDC_PATTERN,EN_CHANGE):
							if (TrySaveDefinition()==ERROR_KOSHER)
								RecognizePresetItem();
							break;
					}
				return false;
			}
		public:
			CInstance::TItem item;

			CItemDefinitionDialog(PWCHAR definitionBuffer,BYTE bufferCapacity,HFONT hFont)
				// ctor
				// . parsing the input Item Pattern
				: definitionBuffer(definitionBuffer) , bufferCapacity(std::min(bufferCapacity,(BYTE)ARRAYSIZE(item.pattern)))
				, patternLengthMax( bufferCapacity-1-ITEM_STREAM_BYTES_SPEC_LEN )
				, hexaEditor( (HINSTANCE)&__ImageBase, &hexaEditor, MARK_RECURRENT_USE, hFont )
				, processNotifications(false) {
				static_assert( ARRAYSIZE(item.pattern)<=99, "must revise ::wsprintf calls in InitDialog!" );
				definitionBuffer[bufferCapacity-1]='\0'; // mustn't overrun!
				if (item.Redefine( definitionBuffer )) // invalid input Pattern?
					item.Redefine( GetDefaultByteItemDefinition() );
				// . creating the Stream for the the HexaEditor
				if (IStream *const s=Stream::FromBuffer( sampleStreamBytes, ITEM_STREAM_BYTES_MAX )){
					for( BYTE i=0; i<ITEM_STREAM_BYTES_MAX; i++ )
						sampleStreamBytes[i]=i;
					hexaEditor.Reset( s, nullptr, TPosInterval(1,ITEM_STREAM_BYTES_MAX) );
					s->Release();
				}
				hexaEditor.ShowColumns(TColumn::VIEW);
			}
		} d( definitionBuffer, bufferCapacity, hFont );
		// - showing the Dialog and processing its result
		return d.DoModal( IDR_YAHEL_ITEM_DEFINITION, hParent )==IDOK;
	}

}