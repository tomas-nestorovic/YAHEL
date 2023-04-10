#include "stdafx.h"
#include "Instance.h"

namespace Yahel
{
	LPCWSTR IOwner::GetDefaultEnglishMessage(TMsg id){
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
				return L"The content has reached its upper limit";
			case MSG_SELECTION_SAVED_PARTIALLY:
				return L"Selection saved only partially";
			case MSG_SELECTION_RESET_PARTIALLY:
				return L"Selection only partially reset";
			case MSG_PASTED_PARTIALLY:
				return L"Pasted only partially";
			case MSG_PADDED_TO_MINIMUM_SIZE:
				return L"To preserve the minimum size, the content has been padded with zeros";

			case QUERY_DELETE_SELECTED_BOOKMARKS:
				return L"Sure to delete selected bookmarks?";
			case QUERY_DELETE_ALL_BOOKMARKS:
				return L"Sure to delete all bookmarks?";

			default:
				return	id<MSG_READY ? L"General error" : nullptr;
		}
	}





namespace Stream
{
	TPosition WINAPI GetMaximumLength(){
		return GetErrorPosition()-1;
	}

	TPosition WINAPI GetErrorPosition(){
		return LONG_MAX;
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
					position=std::max( bufferLength-offset, 0L );
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
			nCount=std::min<ULONG>( nCount, bufferLength-position );
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
		IStream &s;
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
			, s(s) , range(range) {
			s.AddRef();
			::ZeroMemory( &supportedFormats, sizeof(supportedFormats) );
			supportedFormats.Add(TYMED_HGLOBAL);
		}
		virtual ~COleDataObject(){
			s.Release();
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
			static constexpr QITAB qit[]={
				QITABENT( COleDataObject, IDataObject ),
				{ 0 },
			};
			return ::QISearch( this, qit, riid, ppvObject );
		}

		// IDataObject methods
        HRESULT STDMETHODCALLTYPE GetData(LPFORMATETC pFormatEtc,LPSTGMEDIUM outMedium) override{
			const HRESULT hResult=QueryGetData(pFormatEtc);
			if (FAILED(hResult))
				return hResult;
			if (!outMedium)
				return E_INVALIDARG;
			::ZeroMemory( outMedium, sizeof(*outMedium) );
			auto nBytesToRead=range.GetLength();
			switch ( outMedium->tymed=pFormatEtc->tymed ){
				case TYMED_HGLOBAL:
					if (outMedium->hGlobal!=nullptr)
						break;
					if (HGLOBAL hMem=::GlobalAlloc( GPTR, nBytesToRead )){
						if (const PBYTE pData=(PBYTE)::GlobalLock(hMem)){
							const CInstance::TContent f=&s;
							PBYTE p=pData; f.Seek(range.a);
							while (nBytesToRead>0)
								if (const auto nBytesRead=f.Read( p, nBytesToRead, IgnoreIoResult ))
									p+=nBytesRead, nBytesToRead-=nBytesRead;
								else
									break;
							::GlobalUnlock(hMem);
							if (nBytesToRead>0) // failure during reading?
								if (const HGLOBAL hRealloc=::GlobalReAlloc( hMem, p-pData, 0 ))
									hMem=hRealloc;
								else{
									::GlobalFree( hMem );
									return E_OUTOFMEMORY;
								}
							outMedium->hGlobal=hMem;
							return S_OK;
						}
						break;
					}else
						return E_OUTOFMEMORY;
			}
			return E_UNEXPECTED; // we shouldn't end up here!
		}

        HRESULT STDMETHODCALLTYPE GetDataHere(LPFORMATETC pformatetc,LPSTGMEDIUM pmedium) override{
			return E_NOTIMPL;
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

	FORMATETC WINAPI GetFormatEtc(){
		const FORMATETC tmp={ IInstance::GetClipboardFormat(), nullptr, 0, -1, TYMED_ISTREAM };
		return tmp;
	}

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





	void IInstance::ShowModalAboutDialog(){
		::MessageBox( 0,
			DLL_FULLNAME _T("\n\nVersion ") DLL_VERSION _T("\n\n\ntomascz, 2023"),
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

	LPCSTR IInstance::GetDefaultByteItemDefinition(){
		return "1;Aa ";
	}

	bool IInstance::DefineItemUsingDefaultEnglishDialog(char *definitionBuffer,BYTE bufferCapacity,HWND hParent,HFONT hFont){
		//
		// - the PatternBuffer must accommodate at least one char
		static_assert( ITEM_STREAM_BYTES_MAX<=99, "" ); // must otherwise revise the below BufferCapacity
		if (!definitionBuffer || bufferCapacity<ITEM_STREAM_BYTES_SPEC_LEN+ITEM_PATTERN_LENGTH_MIN+1) // capacity of "27;Aa ", see Assert above
			return false;
		// - defining the Dialog
		class CItemDefinitionDialog sealed:public Utils::CYahelDialog{
			const PCHAR definitionBuffer;
			const BYTE bufferCapacity, patternLengthMax;
			CInstance hexaEditor;
			BYTE sampleStreamBytes[ITEM_STREAM_BYTES_MAX];
			bool processNotifications; // notifications on user's interaction with Dialog

			void AddPresetItem(LPCSTR name,LPCSTR pattern){
				char buf[80];
				::wsprintfA( buf, "%s  [%s]", name, pattern );
				const HWND hPresets=GetDlgItemHwnd(IDC_PRESETS);
				ComboBox_SetItemData( hPresets, ComboBox_AddString(hPresets,buf), pattern );
			}
			void ShowItem(){
				const Utils::CVarTempReset<bool> pn0( processNotifications, false ); // don't recurrently trigger notifications
				SetDlgItemInt( IDC_NUMBER, item.nStreamBytes );
				char pattern[ARRAYSIZE(item.pattern)+1];
				SetDlgItemText( IDC_PATTERN, item.GetDefinition(pattern) );
			}
			bool InitDialog() override{
				// Dialog initialization
				// . Presets
				AddPresetItem( "Byte", GetDefaultByteItemDefinition() );
				AddPresetItem( "Word", "2;AaBb " );
				AddPresetItem( "Custom", nullptr );
				// . labels
				TCHAR buf[80];
				GetDlgItemText( IDC_INFO1, buf, ARRAYSIZE(buf) );
					::wsprintf( buf, buf, ITEM_STREAM_BYTES_MAX );
				SetDlgItemText( IDC_INFO1, buf );
				GetDlgItemText( IDC_INFO2, buf, ARRAYSIZE(buf) );
					::wsprintf( buf, buf, ITEM_PATTERN_LENGTH_MIN, patternLengthMax );
				SetDlgItemText( IDC_INFO2, buf );
				// . seeing if initial Item is one of Presets
				ShowItem();
				RecognizePresetItem();
				// . (!!) creation and initial populating of the HexaEditor; MUST BE THE LAST IN THIS METHOD ...
				hexaEditor.SubclassAndAttach( GetDlgItemHwnd(IDC_FILE) );
				processNotifications=true; // ... BECAUSE OF THIS
				SendCommand( MAKELONG(IDC_NUMBER,EN_CHANGE) );
				return true; // set the keyboard focus to the default control
			}
			TError TrySaveDefinition(){
				// attempts to redefine an Item from current inputs; returns a DWORD-encoded error
				auto i=GetDlgItemText( IDC_NUMBER, definitionBuffer, bufferCapacity );
				TError err=ERROR_KOSHER; // assumption
				if (ITEM_STREAM_BYTES_MAX<i)
					err=ERROR_ITEM_DEF_BYTE_COUNT;
				else if (patternLengthMax<GetDlgItemTextLength(IDC_PATTERN))
					err=ERROR_ITEM_DEF_PATTERN_INSUFFICIENT_BUFFER;
				else{
					definitionBuffer[i++]=';';
					GetDlgItemText( IDC_PATTERN, definitionBuffer+i, bufferCapacity-i-1 );
					err=item.Redefine(definitionBuffer);
				}
				if (EnableDlgItem( IDOK, !err ))
					SetDlgItemText( IDC_ERROR, nullptr );
				else
					::SetDlgItemTextW( hDlg, IDC_ERROR, IOwner::GetDefaultEnglishMessage(err.code) );
				return err;
			}
			void RecognizePresetItem(){
				// selects the predefined Item that matches current inputs
				const HWND hPresets=GetDlgItemHwnd(IDC_PRESETS);
				const auto nItems=ComboBox_GetCount(hPresets);
				if (TrySaveDefinition()==ERROR_KOSHER)
					for( auto i=nItems; i>0; )
						if (!::lstrcmpA( definitionBuffer, (LPCSTR)ComboBox_GetItemData(hPresets,--i) )){
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
							if (const LPCSTR def=(LPCSTR)ComboBox_GetItemData(hPresets,ComboBox_GetCurSel(hPresets))){ // one of Presets?
								item.Redefine(def);
								ShowItem();
							}
							break;
						}
						case MAKELONG(IDC_NUMBER,EN_CHANGE):
							if (TrySaveDefinition()==ERROR_KOSHER){
								ULARGE_INTEGER uli;
									uli.QuadPart=item.nStreamBytes;
								if (IStream *const s=hexaEditor.GetCurrentStream()){
									s->SetSize(uli);
									s->Release();
								}
								hexaEditor.RepaintData();
							}
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

			CItemDefinitionDialog(PCHAR definitionBuffer,BYTE bufferCapacity,HFONT hFont)
				// ctor
				// . parsing the input Item Pattern
				: definitionBuffer(definitionBuffer) , bufferCapacity(std::min<BYTE>(bufferCapacity,ARRAYSIZE(item.pattern)))
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