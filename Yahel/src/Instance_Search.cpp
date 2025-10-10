#include "stdafx.h"
#include "Instance.h"

namespace Yahel{

	TSearchParams::TSearchParams()
		// ctor
		: type(ASCII_ANY_CASE) , patternLength(0) , searchForward(true) , selectFinding(true) {
	}

	bool TSearchParams::IsValid() const{
		return	0<=type && type<LAST
				&&
				searchForward //TODO: can't now search backwards
				&&
				patternLength<sizeof(pattern);
	}

	bool TSearchParams::IsUsable() const{
		return	IsValid()
				&&
				patternLength>0;
	}

	bool TSearchParams::EditModalWithDefaultEnglishDialog(HWND hParent,HFONT hFont){
		//
		// - can't edit invalid Params
		if (!IsValid())
			return false;
		// - defining the Dialog
		class CParamsDialog sealed:public Utils::CYahelDialog{
			CInstance hexaEditor;

			bool InitDialog() override{
				hexaEditor.SubclassAndAttach( GetDlgItemHwnd(IDC_FILE) );
					char patternText[sizeof(params.pattern.chars)+1];
					::SetDlgItemTextA( hDlg, ID_YAHEL_TEXT, ::lstrcpynA(patternText,params.pattern.chars,params.patternLength+1) );
				static constexpr WORD EditControls[]={ ID_YAHEL_TEXT, IDC_FILE, ID_YAHEL_NUMBER };
				const HWND hEdit=GetDlgItemHwnd( EditControls[params.type] );
				::SetFocus(hEdit);
				::SendMessage( hEdit, EM_SETSEL, 0, params.patternLength );
				Gui::SetDlgItemIntBuddyW( hDlg, ID_YAHEL_NUMBER, 0, Gui::Hexa, true );
				return false; // focus already set manually
			}

			bool ValidateDialog() override{
				static_assert( sizeof(params.type)==sizeof(int), "" );
				static constexpr WORD SearchButtons[]={ ID_YAHEL_FIND_PREV, ID_YAHEL_FIND_NEXT, 0 };
				EnableDlgItems( SearchButtons, false ); // assumption (the Dialog has invalid configuration)
				if (IsDlgButtonChecked(ID_YAHEL_DEFAULT1)){
					params.type=ASCII_ANY_CASE;
					params.patternLength=::GetDlgItemTextA( hDlg, ID_YAHEL_TEXT, params.pattern.chars, sizeof(params.pattern.chars) );
					if (IsDlgButtonChecked(ID_YAHEL_ACCURACY))
						params.type=ASCII_MATCH_CASE;
				}else if (IsDlgButtonChecked(ID_YAHEL_DEFAULT2)){
					params.type=HEXA;
					params.patternLength=hexaEditor.f.GetLength();
				}else if (IsDlgButtonChecked(ID_YAHEL_DEFAULT3)){
					params.type=NOT_BYTE;
					const auto i=GetDlgItemInt(ID_YAHEL_NUMBER);
					if (i>UCHAR_MAX)
						return false;
					*params.pattern.bytes=i, params.patternLength=1;
				}else
					return false;
				return EnableDlgItems( SearchButtons, params.patternLength>0 );
			}

			bool OnCommand(WPARAM wParam,LPARAM lParam) override{
				switch (wParam){
					case ID_YAHEL_FIND_PREV:
					case ID_YAHEL_FIND_NEXT:
						if (ValidateDialog())
							EndDialog( hDlg, wParam );
						return true;
				// ASCII search events
					case MAKELONG(ID_YAHEL_DEFAULT1,BN_CLICKED):
					case MAKELONG(ID_YAHEL_ACCURACY,BN_CLICKED):
						FocusDlgItem(ID_YAHEL_TEXT);
						//fallthrough
					case MAKELONG(ID_YAHEL_TEXT,EN_SETFOCUS):
						::CheckRadioButton( hDlg, ID_YAHEL_DEFAULT1, ID_YAHEL_DEFAULT4, ID_YAHEL_DEFAULT1 );
						//fallthrough
					case MAKELONG(ID_YAHEL_TEXT,EN_CHANGE):
						ValidateDialog();
						hexaEditor.SetStreamLogicalSize( params.patternLength );
						hexaEditor.f.SetLength( params.patternLength ); // zeros new space
						::GetDlgItemTextA( hDlg, ID_YAHEL_TEXT, params.pattern.chars, sizeof(params.pattern.chars) ); // already in ValidateDialog, here once more as the Stream misses the tail of the text
						hexaEditor.RepaintData();
						return true;
				// View search events
					case MAKELONG(ID_YAHEL_DEFAULT2,BN_CLICKED):
						FocusDlgItem(IDC_FILE);
						//fallthrough
					case MAKELONG(IDC_FILE,EN_SETFOCUS):
						::CheckRadioButton( hDlg, ID_YAHEL_DEFAULT1, ID_YAHEL_DEFAULT4, ID_YAHEL_DEFAULT2 );
						//fallthrough
					case MAKELONG(IDC_FILE,EN_CHANGE):
						ValidateDialog();
						params.pattern.chars[params.patternLength]='\0';
						::SetDlgItemTextA( hDlg, ID_YAHEL_TEXT, params.pattern.chars );
						return true;
				// "Not-Byte" search events
					case MAKELONG(ID_YAHEL_DEFAULT3,BN_CLICKED):
						FocusDlgItem(ID_YAHEL_NUMBER);
						//fallthrough
					case MAKELONG(ID_YAHEL_NUMBER,EN_SETFOCUS):
						::CheckRadioButton( hDlg, ID_YAHEL_DEFAULT1, ID_YAHEL_DEFAULT4, ID_YAHEL_DEFAULT3 );
						//fallthrough
					case MAKELONG(ID_YAHEL_NUMBER,EN_CHANGE):{
						const Utils::CVarBackup<BYTE> b0(*params.pattern.bytes); // "ValidateDialog" below overwrites the first Byte in the Stream, affecting HexaEditor and subsequently Ascii, giving bad impression to the user
						ValidateDialog();
						return true;
					}
				}
				return false; // unrecognized command
			}
		public:
			TSearchParams params;

			CParamsDialog(const TSearchParams &params0,HFONT hFont)
				: params(params0)
				, hexaEditor( (HINSTANCE)&__ImageBase, &hexaEditor, MARK_RECURRENT_USE, hFont ) {
				IStream *const s=Stream::FromBuffer( params.pattern.bytes, params.patternLength );
					hexaEditor.Reset( s, nullptr, TPosInterval(1,sizeof(params.pattern.bytes)) );
				s->Release();
				hexaEditor.ShowColumns( IInstance::TColumn::MINIMAL );
				hexaEditor.SetEditable(true);
			}
		} d(*this,hFont);
		// - showing the Dialog and processing its result
		switch (d.DoModal( IDR_YAHEL_FIND, hParent )){
			case ID_YAHEL_FIND_PREV:
				assert(false); //TODO
				return false;
			case ID_YAHEL_FIND_NEXT:
				*this=d.params;
				searchForward=true;
				return true;
		}
		return false;
	}





	typedef bool (* FnComparer)(BYTE b1,BYTE b2);

	static bool EqualBytes(BYTE b1,BYTE b2){
		return b1==b2;
	}

	static bool EqualChars(BYTE b1,BYTE b2){
		return ::strnicmp( (LPCSTR)&b1, (LPCSTR)&b2, 1 )==0;
	}

	static bool InequalBytes(BYTE b1,BYTE b2){
		return b1!=b2;
	}

	#define params searchParams

	TPosition CInstance::FindNextOccurence(const TPosInterval &range,volatile const bool &cancel) const{
		// search by specified Params
		// - can't search by unusable Params
		if (!params.IsUsable())
			return Stream::GetErrorPosition();
		// - determining Comparer function
		FnComparer cmp;
		switch (params.type){
			case TSearchParams::HEXA:
			case TSearchParams::ASCII_MATCH_CASE:
				cmp=EqualBytes;
				break;
			case TSearchParams::ASCII_ANY_CASE:
				cmp=EqualChars;
				break;
			case TSearchParams::NOT_BYTE:
				cmp=InequalBytes;
				break;
			default:
				assert(false);
				return Stream::GetErrorPosition();
		}
		// - preparation for KMP search (Knuth-Morris-Pratt)
		static_assert( sizeof(*params.pattern.bytes)==sizeof(*params.pattern.chars), "" ); // musn't mix Bytes and (for instance) Unicode
		BYTE pie[sizeof(params.pattern.bytes)];
		*pie=0;
		for( BYTE i=1,k=0; i<params.patternLength; i++ ){
			while (k>0 && params.pattern.bytes[k]!=params.pattern.bytes[i])
				k=pie[k-1];
			k+=params.pattern.bytes[k]==params.pattern.bytes[i];
			pie[i]=k;
		}
		// - search for next match of the Pattern
		struct TContent sealed:public CInstance::TContent{
			TPosition posOrg;

			TContent()
				: posOrg(Stream::GetErrorPosition()) {
			}
			~TContent(){
				if (posOrg!=Stream::GetErrorPosition())
					Seek(posOrg);
			}
		} f;
		const HRESULT hr=this->f.stream->Clone( &f.stream.p );
		if (hr==E_NOTIMPL){ // have to reuse existing Stream?
			assert(false); // using here the same Stream always requires attention; YAHEL is fine with that - is also the client app fine with that?
			( f.stream=this->f.stream )->AddRef(), f.posOrg=f.GetPosition();
		}else if (FAILED(hr))
			return Stream::GetErrorPosition();
		f.Seek( range.a );
		for( BYTE posMatched=0,b; f.GetPosition()<range.z; )
			if (cancel)
				break;
			else if (!f.Read( &b, 1, IgnoreIoResult )){
				f.Seek( 1, STREAM_SEEK_CUR ); // skipping irrecoverable portion of data ...
				posMatched=0; // ... and beginning with Pattern comparison from scratch
			}else{
				while (posMatched>0 && !cmp(b,params.pattern.bytes[posMatched]) )
					posMatched=pie[posMatched-1];
				posMatched+=cmp( b, params.pattern.bytes[posMatched] );
				if (posMatched==params.patternLength)
					return f.GetPosition()-params.patternLength;
			}
		// - no match found
		return Stream::GetErrorPosition();
	}

}
