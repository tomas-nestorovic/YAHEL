#include "stdafx.h"
#include "Instance.h"

struct TWndClass:public WNDCLASS{
	TWndClass(LPCTSTR name){
		::GetClassInfo( nullptr, name, this );
	}
};

const WNDCLASS EditClass=TWndClass(WC_EDIT);

HRESULT IgnoreIoResult;




namespace Yahel{

	void CInstance::TContent::SetLength(TPosition length) const{
		ULARGE_INTEGER uli;
			uli.QuadPart=length;
		stream->SetSize( uli );
	}

	TPosition CInstance::TContent::GetPosition() const{
		return Seek( 0, STREAM_SEEK_CUR );
	}

	TPosition CInstance::TContent::Seek(TPosition pos,STREAM_SEEK dwMoveMethod) const{
		LARGE_INTEGER li;
			li.QuadPart=pos;
		ULARGE_INTEGER uli;
		return	SUCCEEDED(stream->Seek( li, dwMoveMethod, &uli ))
				? uli.QuadPart
				: Stream::GetErrorPosition();
	}

	TPosition CInstance::TContent::Read(PVOID buffer,TPosition bufferCapacity,HRESULT &outResult) const{
		ULONG nBytesRead=0;
		outResult=stream->Read( buffer, bufferCapacity, &nBytesRead );
		return nBytesRead;
	}

	TPosition CInstance::TContent::Write(LPCVOID data,TPosition dataLength,HRESULT &outResult) const{
		ULONG nBytesWritten=0;
		outResult=stream->Write( data, dataLength, &nBytesWritten );
		return nBytesWritten;
	}







	ULONG STDMETHODCALLTYPE CInstance::AddRef(){
		return ::InterlockedIncrement(&nReferences);
	}

	ULONG STDMETHODCALLTYPE CInstance::Release(){
		if (const auto n=::InterlockedDecrement(&nReferences))
			return n;
		delete this;
		return 0;
	}

	HRESULT STDMETHODCALLTYPE CInstance::QueryInterface(REFIID riid,PVOID *ppvObject){
		return E_NOINTERFACE;
	}






	static struct:public Stream::IAdvisor{
		void GetRecordInfo(TPosition logPos,PPosition pOutRecordStartLogPos,PPosition pOutRecordLength,bool *pOutDataReady) override{
			// retrieves the start logical position and length of the Record pointed to by the input LogicalPosition
			if (pOutRecordStartLogPos)
				*pOutRecordStartLogPos=0;
			if (pOutRecordLength)
				*pOutRecordLength=GetMaximumRecordLength();
			if (pOutDataReady)
				*pOutDataReady=true;
		}
		TRow LogicalPositionToRow(TPosition logPos,WORD nStreamBytesInRow) override{
			// computes and returns the row containing the specified LogicalPosition
			return logPos/nStreamBytesInRow;
		}
		TPosition RowToLogicalPosition(TRow row,WORD nStreamBytesInRow) override{
			// converts Row begin (i.e. its first Byte) to corresponding logical position in underlying File and returns the result
			return row*nStreamBytesInRow;
		}
		LPCWSTR GetRecordLabelW(TPosition logPos,PWCHAR labelBuffer,BYTE labelBufferCharsMax,PVOID param) const override{
			// populates the Buffer with label for the Record that STARTS at specified LogicalPosition, and returns the Buffer; returns Null if no Record starts at specified LogicalPosition
			return nullptr;
		}
	} DefaultStreamAdvisor;







	CInstance::CInstance(HINSTANCE hInstance,POwner pOwner,PVOID lpParam,HFONT hFont)
		// ctor
		: nReferences(1) , param(lpParam) , hInstance(hInstance) , pOwner(pOwner) , hWnd(0)
		, font( hFont ? Utils::CYahelFont::TLogFont(hFont) :  Utils::CYahelFont::TLogFont(FONT_COURIER_NEW,105) )
		, contextMenu( IDR_YAHEL )
		, hDefaultAccelerators(::LoadAccelerators((HINSTANCE)&__ImageBase,MAKEINTRESOURCE(IDR_YAHEL)))
		, caret(0) , hPreviouslyFocusedWnd(0)
		, logPosScrolledTo(0)
		, pStreamAdvisor(&DefaultStreamAdvisor)
		, editable(false) {
		RemoveAllHighlights(); // to correctly initialize
		SetLabelColumnParams( 16, CLR_DEFAULT ); // to correctly initialize
		::InitializeCriticalSection(&locker);
		Reset( nullptr, nullptr, TPosInterval(0) );
		ShowColumns(TColumn::ALL);
		const TError err=RedefineItem( nullptr, 8, 16 ); // default View of one Byte
		assert( !err );
		// - comparing requested configuration with HexaEditor's skills
		/*assert(	recordSize>=128 // making sure that entire Rows are either (1) well readable, (2) readable with error, or (3) non-readable; 128 = min Sector length
				? recordSize%128==0 // case: Record spans over entire Sectors
				: 128%recordSize==0 // case: Sector contains integral multiple of Records
			);*/
	}

	CInstance::~CInstance(){
		// dtor
		// - destroying the Accelerator table
		::DestroyAcceleratorTable(hDefaultAccelerators);
		// - destroying the Locker
		::DeleteCriticalSection(&locker);
	}








	bool CInstance::Attach(HWND hWnd){
		if (!hWnd || !::IsWindow(hWnd)) // must be a valid window handle
			return false;
		if (::GetCurrentThreadId()!=::GetWindowThreadProcessId(hWnd,nullptr)) // must be called from the GUI Thread
			return false;
		WCHAR baseClassName[80];
		const auto n=::GetClassNameW( hWnd, baseClassName, ARRAYSIZE(baseClassName) );
		if (!n)
			return false;
		if (::lstrcmpW( baseClassName, GetBaseClassNameW(GetWindowInstance(hWnd)) )) // not the required window class?
			return false;
		EXCLUSIVELY_LOCK_THIS_OBJECT();
		if (this->hWnd && this->hWnd!=hWnd) // has already a different window buddy?
			return false;
		this->hWnd=hWnd;
		SendMessage( WM_SIZE, 0, GetClientRect().right ); // refresh horizontally
		if (f!=nullptr)
			ScrollTo( logPosScrolledTo ); // recovering the scroll position
		return true;
	}

	HWND CInstance::Detach(){
		EXCLUSIVELY_LOCK_THIS_OBJECT();
		const HWND hResult=hWnd;
		hWnd=0;
		return hResult;
	}

	static WNDPROC baseClassWndProc;

	LRESULT WINAPI CInstance::WndProc(HWND hYahel,UINT msg,WPARAM wParam,LPARAM lParam){
		// window procedure
		CInstance *const instance=(CInstance *)::GetWindowLong(hYahel,GWL_USERDATA);
		MSG msgOrg={ hYahel, msg, wParam, lParam };
		if (::TranslateAccelerator( hYahel, instance->hDefaultAccelerators, &msgOrg ))
			return 0;
		LRESULT result;
		if (instance->ProcessMessage( msg, wParam, lParam, result ))
			return result;
		return ::CallWindowProc( baseClassWndProc, hYahel, msg, wParam, lParam );
	}

	WNDPROC CInstance::SubclassAndAttach(HWND hBaseClassWnd){
		if (!Attach(hBaseClassWnd))
			return nullptr;
		::SetWindowLong( hBaseClassWnd, GWL_USERDATA, (LONG)this );
		baseClassWndProc=(WNDPROC)::SetWindowLong( hBaseClassWnd, GWL_WNDPROC, (LONG)WndProc );
		SendMessage( WM_SIZE, 0, GetClientRect().right ); // refresh horizontally
		return WndProc;
	}

	void CInstance::RepaintData() const{
		// invalidates the "data" (the content below the Header)
		if (hWnd){
	{		EXCLUSIVELY_LOCK_THIS_OBJECT();
			if (!mouseInNcArea) // when NOT in the non-client area (e.g. over a scrollbar), repainting normally
				const_cast<PInstance>(this)->RefreshScrollInfo();
	}		RECT rc=GetClientRect();
			rc.top=HEADER_HEIGHT;
			::InvalidateRect( hWnd, &rc, TRUE );
		}
	}

	int CInstance::GetDefaultCommandMenuFlags(WORD cmd) const{
		return GetCustomCommandMenuFlags(cmd);
	}

	HMENU CInstance::GetContextMenu() const{
		return contextMenu;
	}

	HACCEL CInstance::GetAcceleratorTable() const{
		return hDefaultAccelerators;
	}







	void CInstance::SetEditable(bool _editable){
		// enables/disables possibility to edit the content of the File (see the Reset function)
		if (editable=_editable) // if making content Editable ...
			if (::OleIsCurrentClipboard(delayedDataInClipboard)==S_OK){
				::OleFlushClipboard(); // ... render data that we put into clipboard earlier
				delayedDataInClipboard.Release();
			}
		if (::IsWindow(hWnd)){ // may be window-less if the owner is window-less
			if (::GetFocus()==hWnd){
				__refreshCaretDisplay__();
				SetFocus();
				ShowCaret();
			}
			Invalidate(FALSE);
		}
	}

	bool CInstance::IsEditable() const{
		return editable;
	}

	void CInstance::ShowColumns(BYTE columns){
		// shows/hides individual columns
		this->columns=columns;
		addrLength= IsColumnShown(TColumn::ADDRESS) ? ADDRESS_FORMAT_LENGTH : 0;
		if (::IsWindow(hWnd)) // may be window-less if the owner is window-less
			Invalidate(FALSE);
	}

	bool CInstance::IsColumnShown(TColumn c) const{
		return (columns&c)!=0;
	}

	IStream *CInstance::GetCurrentStream() const{
		if (f.stream.p!=nullptr)
			f.stream.p->AddRef();
		return f;
	}

	void CInstance::Update(IStream *f,Stream::IAdvisor *sa){
		// updates the underlying File content
		EXCLUSIVELY_LOCK_THIS_OBJECT();
		pStreamAdvisor= sa!=nullptr ? sa : &DefaultStreamAdvisor;
		this->f.stream.Attach(f);
		if (f){
			SetStreamLogicalSize(this->f.GetLength());
			f->AddRef();
		}
	}

	bool CInstance::Update(IStream *f,Stream::IAdvisor *sa,const TPosInterval &fileLogicalSizeLimits){
		// updates the underlying File content
		if (!fileLogicalSizeLimits.IsValidNonempty()) // invalid or empty Interval
			return false;
		EXCLUSIVELY_LOCK_THIS_OBJECT();
		this->f.stream.Release(); // anything previously set is now invalid
		SetStreamLogicalSizeLimits( fileLogicalSizeLimits );
		Update( f, sa );
		if (::IsWindow(hWnd)){ // may be window-less if the owner is window-less
			RefreshScrollInfo();
			Invalidate(FALSE);
		}
		return true;
	}

	bool CInstance::Reset(IStream *f,Stream::IAdvisor *sa,const TPosInterval &fileLogicalSizeLimits){
		// resets the HexaEditor and supplies it new File content
		EXCLUSIVELY_LOCK_THIS_OBJECT();
		if (!Update( f, sa, fileLogicalSizeLimits ))
			return false;
		if (!f)
			nLogicalRows = nRowsDisplayed = nRowsOnPage = 0;
		caret=TCaret(0); // resetting the Caret and Selection
		logPosScrolledTo=0;
		return true;
	}

	bool CInstance::SetStreamLogicalSizeLimits(const TPosInterval &limits){
		// changes the min and max File size
		if (!limits.IsValidNonempty()) // invalid or empty Interval
			return false;
		EXCLUSIVELY_LOCK_THIS_OBJECT();
		if (mouseInNcArea){
			// when in the non-client area (e.g. over a scrollbar), putting updated values aside
			update.logicalSizeLimits=limits;
		}else{
			// otherwise, updating the values normally
			this->logicalSizeLimits = update.logicalSizeLimits = limits; // setting also Update just in case the cursor is in non-client area
			if (::IsWindow(hWnd)) // may be window-less if the owner is window-less
				if (f)
					RefreshScrollInfo();
		}
		return true;
	}

	TPosition CInstance::GetStreamLogicalSize() const{
		// returns the LogicalSize of File content
		EXCLUSIVELY_LOCK_THIS_OBJECT();
		return	mouseInNcArea
				? update.logicalSize
				: logicalSize;
	}

	void CInstance::SetStreamLogicalSize(TPosition logicalSize){
		// changes the LogicalSize of File content (originally set when Resetting the HexaEditor)
		logicalSize=std::max( 0L, logicalSize ); // mustn't be negative
		EXCLUSIVELY_LOCK_THIS_OBJECT();
		caret.streamSelectionA=std::min( caret.streamSelectionA, logicalSize );
		caret.streamPosition=std::min( caret.streamPosition, logicalSize );
		//caret.iViewHalfbyte=std::min<char>( caret.iViewHalfbyte, 0 ); // don't change the column (View vs Stream)
		if (mouseInNcArea)
			// when in the non-client area (e.g. over a scrollbar), putting updated values aside
			update.logicalSize=logicalSize;
		else{
			// otherwise, updating the values normally
			this->logicalSize = update.logicalSize = logicalSize; // setting also Update just in case the cursor is in non-client area
			if (::IsWindow(hWnd)) // may be window-less if the owner is window-less
				RefreshScrollInfo();
		}
	}

	TPosition CInstance::GetCaretPosition() const{
		return caret.streamPosition;
	}

	TPosInterval CInstance::GetSelectionAsc() const{
		return caret.GetSelectionAsc();
	}

	void CInstance::SetSelection(TPosition selA,TPosition selZ){
		// sets current Selection, moving Caret to the end of the Selection
		if (selA>selZ)
			std::swap( selA, selZ );
		caret.streamSelectionA=std::max( 0L, selA );
		caret.streamPosition=std::min( selZ, logicalSizeLimits.z );
		caret.iViewHalfbyte=item.iFirstPlaceholder;
		RepaintData();
		ScrollToCaretAsync();
	}

	TPosInterval CInstance::GetVisiblePart() const{
		// gets the beginning and end of visible portion of the File content
		const auto i=GetVertScrollPos();
		return	TPosInterval(
					__firstByteInRowToLogicalPosition__(i),
					__firstByteInRowToLogicalPosition__(i+nRowsDisplayed)
				);
	}

	void CInstance::ScrollTo(TPosition logicalPos,bool moveAlsoCaret){
		// independently from Caret, displays specified LogicalPosition
		if (moveAlsoCaret)
			caret.streamSelectionA = caret.streamPosition = logicalPos;
		RefreshScrollInfo();
		__scrollToRow__( __logicalPositionToRow__(logicalPos) );
	}

	void CInstance::ScrollToRow(TRow iRow){
		// independently from Caret, displays specified LogicalPosition
		ScrollTo( __firstByteInRowToLogicalPosition__(iRow), false );
	}






	int CInstance::GetAddressColumnWidth() const{
		return ADDRESS_FORMAT_LENGTH*font.GetCharAvgWidth();
	}






	TError CInstance::RedefineItem(LPCWSTR newItemDef,WORD nItemsInRowMin,WORD nItemsInRowMax){
		//
		if (!newItemDef) // want the default View?
			newItemDef=GetDefaultByteItemDefinition();
		auto newItem=item; // transactional - only if Definition valid shall it be used
			if (const auto err=newItem.Redefine(newItemDef))
				return err;
			if (const auto err=newItem.SetRowLimits( TInterval<WORD>(nItemsInRowMin,nItemsInRowMax+1) ))
				return err;
		item=newItem;
		if (caret.IsInStream())
			caret.iViewHalfbyte=-item.iFirstPlaceholder;
		else
			caret.iViewHalfbyte=item.iFirstPlaceholder;
		return ERROR_KOSHER;
	}

	TError CInstance::SetItemCountPerRow(WORD nItemsInRowMin,WORD nItemsInRowMax){
		//
		return item.SetRowLimits( TInterval<WORD>(nItemsInRowMin,nItemsInRowMax+1) );
	}






	WORD CInstance::GetStreamBytesCountPerRow() const{
		return item.nInRow*item.nStreamBytes;
	}






	TError CInstance::SetLabelColumnParams(char nLabelCharsMax,COLORREF bgColor){
		// sets properties of the Label Column
		if (nLabelCharsMax!=0){ // want the Label Column visible?
			columns|=TColumn::LABEL;
			label.nCharsMax=nLabelCharsMax;
			label.bgBrush=Utils::CYahelBrush(
				bgColor>=CLR_DEFAULT ? Utils::GetBlendedColor(::GetSysColor(COLOR_WINDOW),::GetSysColor(COLOR_BTNFACE),0.85f) : bgColor,
				false
			);
		}else
			columns&=~TColumn::LABEL;
		ShowColumns( columns );
		return ERROR_KOSHER;
	}






	bool CInstance::ToggleBookmarkAt(TPosition pos){
		// - erasing an existing Bookmark at Position
		const auto it=bookmarks.find(pos);
		if (it!=bookmarks.end()){
			bookmarks.erase(it);
			return true;
		}
		// - inserting a new Bookmark at Position
		if (pos<f.GetLength()){
			bookmarks.insert(pos);
			return true;
		}
		// - can't neither erase nor insert a Bookmark
		return false;
	}

	void CInstance::RemoveAllBookmarks(){
		bookmarks.clear();
	}






	CInstance::TEmphasis::TEmphasis(const TPosInterval &range,COLORREF color)
		// ctor
		: TPosInterval(range) , color(color) {
	}

	bool CInstance::AddHighlight(const TPosInterval &range,COLORREF color){
		// adds a new Emphasis into the list and orders the list by beginnings A (and thus also by endings Z; insertsort)
		if (!range.IsValidNonempty())
			return false;
		TEmphasis e( range, color );
		auto it=emphases.lower_bound(e);
		// - does Range.Z overwrite FULLY any later Emphases?
		while (it!=emphases.end())
			if (it->z<=range.z){ // yes, it does
				const auto itErase=it++;
				emphases.erase(itErase); // remove such Emphasis
			}else
				break;
		// - does Range.Z overwrite PARTLY the nearest later Emphasis?
		if (it!=emphases.end())
			if (it->a<range.z){ // yes, it does
				TEmphasis tmp=*it;
					tmp.a=range.z;
				emphases.erase(it), it=emphases.insert(tmp).first; // replace such Emphasis
			}
		// - does Range.A anyhow overwrite the nearest previous Emphasis?
		if (it!=emphases.begin()){
			const auto itPrev=--it;
			if (range.z<itPrev->z){ // yes, it splits the previous Emphasis into two pieces
				TEmphasis tmp=*itPrev;
					tmp.a=range.z;
				it=emphases.insert(tmp).first; // second part
				*const_cast<PPosition>(&itPrev->z)=range.a; // first part always non-empty; doing this is OK because Z doesn't serve as the key for the Emphases std::set
			}else if (range.a<it->z) // yes, only partly the end
				*const_cast<PPosition>(&it++->z)=range.a; // doing this is OK because Z doesn't serve as the key for the Emphases std::set
			else
				it++;
		}
		// - alpha component indicates mere deletion of existing Emphases ...
		if (color>=0x01000000)
			return true; // ... which has just been done
		// - can the new Emphasis be merged with nearest next one?
		bool merged=false;
		if (it->a==e.z && it->color==color){ // yes, it can
			e=*it;
				e.a=range.a;
			emphases.erase(it), it=emphases.insert(e).first; // merge them
			merged=true;
		}
		// - can the new Emphasis be merged with nearest previous one?
		if (it--!=emphases.begin())
			if (it->IsSameColorNext(e)){ // yes, it can
				*const_cast<PPosition>(&it->z)=e.z; // merge them; doing this is OK because Z doesn't serve as the key for the Emphases std::set
				if (merged) // already Merged above?
					emphases.erase(++it);
				else
					merged=true;
			}
		// - adding the new Emphasis
		if (!merged)
			emphases.insert(e);
		return true;
	}

	void CInstance::RemoveAllHighlights(){
		// destroys the list of Emphases
		emphases.clear();
		emphases.insert( TEmphasis(Stream::GetMaximumLength(),CLR_DEFAULT) ); // terminator
	}








	bool CInstance::mouseDragged;
	bool CInstance::mouseInNcArea;

	bool CInstance::ShiftPressedAsync(){
		return ::GetAsyncKeyState(VK_SHIFT)<0;
	}

	CInstance::TCaret::TCaret(TPosition position)
		// ctor
		: iViewHalfbyte(0) // in View column
		, streamSelectionA(position) , streamSelectionZ(position) { // nothing selected
	}

	CInstance::TCaret &CInstance::TCaret::operator=(const TCaret &r){
		// copy assignment operator
		return *(TCaret *)(  ::memcpy( this, &r, sizeof(*this) )  );
	}

	TPosInterval CInstance::TCaret::GetSelectionAsc() const{
		return	TPosInterval(
					std::min( streamSelectionA, streamSelectionZ ),
					std::max( streamSelectionA, streamSelectionZ )
				);
	}







	TError CInstance::TItem::Redefine(LPCWSTR newDef){
		//
		::ZeroMemory( this, sizeof(*this) );
		LPCWSTR p=::StrChrW( newDef, ';' );
		if (!p)
			return ERROR_ITEM_DEF_BYTE_COUNT_UNKNOWN;
		nStreamBytes=::_wtoi( newDef );
		if (nStreamBytes<1 || std::min(STREAM_BYTES_IN_ROW_MAX,ITEM_STREAM_BYTES_MAX)<nStreamBytes)
			return ERROR_ITEM_DEF_BYTE_COUNT;
		for( iFirstPlaceholder=SCHAR_MAX,iLastPlaceholder=-1; const WCHAR c=*++p; patternLength++ ){
			if (patternLength==ARRAYSIZE(pattern)) // would we overrun the buffer?
				return ERROR_ITEM_DEF_PATTERN_TOO_LONG;
			else if ('A'<=c && c<='Z') // upper Halfbyte Placeholder
				SetPlaceholder( patternLength, c-'A', false );
			else if ('a'<=c && c<='z') // lower Halfbyte Placeholder
				SetPlaceholder( patternLength, c-'a', true );
			else if (::StrChrW(L"{}\\~'\"",c)) // one of forbidden chars?
				return ERROR_ITEM_DEF_PATTERN_FORBIDDEN;
			else if (c>=' '){ // literal to be printed exactly
				SetPrintableChar( patternLength, c );
				continue;
			}else
				return ERROR_ITEM_DEF_PATTERN_NOT_PRINTABLE;
			iFirstPlaceholder=std::min( iFirstPlaceholder, patternLength );
			iLastPlaceholder=std::max( iLastPlaceholder, patternLength );
			if (GetByteIndex(patternLength)>=nStreamBytes) // indexed a Byte out of Pattern?
				return ERROR_ITEM_DEF_PATTERN_INDEX;
		}
		if (iLastPlaceholder<0) // no Halfbyte addressed?
			return ERROR_ITEM_DEF_PATTERN_NO_PLACEHOLDER;
		return ERROR_KOSHER;
	}

	TError CInstance::TItem::SetRowLimits(const TInterval<WORD> limits){
		//
		if (!limits.IsValidNonempty()) // invalid or empty Interval
			return ERROR_ITEM_COUNT_INVALID;
		if (limits.a<1 || STREAM_BYTES_IN_ROW_MAX<limits.a*nStreamBytes)
			return ERROR_ITEM_COUNT_INVALID;
		nInRow=( nInRowLimits=limits ).Clip(nInRow);
		return ERROR_KOSHER;
	}

	LPCWSTR CInstance::TItem::GetDefinition(PWCHAR def) const{
		// assuming this Item is valid, composes its string Definition, and returns pointer to the Pattern in the Definition
		def+=::wsprintfW( def, L"%d;", nStreamBytes );
		const LPCWSTR patternBegin=def;
		for( BYTE i=0; i<patternLength; i++ )
			if (const WCHAR c=GetPrintableChar(i))
				*def++=c;
			else
				*def++= 'A'+GetByteIndex(i) + ('a'-'A')*IsLowerHalfbyte(i);
		*def='\0';
		return patternBegin;
	}







	TPosition CInstance::__firstByteInRowToLogicalPosition__(TRow row) const{
		// converts Row begin (i.e. its first Byte) to corresponding position in underlying File and returns the result
		return pStreamAdvisor->RowToLogicalPosition( row, GetStreamBytesCountPerRow() );
	}

	TRow CInstance::__logicalPositionToRow__(TPosition logPos) const{
		// computes and returns at which Row is the specified LogicalPosition
		return pStreamAdvisor->LogicalPositionToRow( logPos, GetStreamBytesCountPerRow() );
	}

	TRow CInstance::__scrollToRow__(TRow row){
		// scrolls the HexaEditor so that the specified Row is shown as the first one from top; returns the Row number which it has been really scrolled to
	{	EXCLUSIVELY_LOCK_THIS_OBJECT();
		row=std::min( std::max(row,0), std::max(nLogicalRows-nRowsOnPage,0) );
	}	if (const auto dr=GetVertScrollPos()-row){
			RECT rcScroll=GetClientRect();
				rcScroll.bottom=( rcScroll.top=HEADER_HEIGHT )+nRowsDisplayed*font.GetCharHeight();
			::ScrollWindow( hWnd, 0, dr*font.GetCharHeight(), nullptr, &rcScroll );
			::SetScrollPos( hWnd, SB_VERT, row, TRUE ); // True = redrawing the scroll-bar, not HexaEditor's canvas!
			SendMessage( WM_VSCROLL, SB_THUMBPOSITION ); // letting descendants of HexaEditor know that a scrolling occured
			::DestroyCaret();
		}
		return GetVertScrollPos();
	}

	void CInstance::ScrollToColumn(TCol col){
		// scrolls the HexaEditor so that the specified Column is shown as the first one from left; returns the Column number which it has been really scrolled to
		SCROLLINFO si={ sizeof(si), SIF_POS|SIF_PAGE|SIF_RANGE|SIF_TRACKPOS };
		::GetScrollInfo( hWnd, SB_HORZ, &si ); // getting 32-bit position
		col=std::min( std::max(col,0), std::max<TCol>(si.nMax+1-si.nPage,0) );
		if (const auto dc=si.nPos-col){
			RECT rcScroll=GetClientRect();
				rcScroll.left=(addrLength+ADDRESS_SPACE_LENGTH)*font.GetCharAvgWidth();
			::ScrollWindow( hWnd, dc*font.GetCharAvgWidth(), 0, nullptr, &rcScroll );
			::SetScrollPos( hWnd, SB_HORZ, col, TRUE ); // True = redrawing the scroll-bar, not HexaEditor's canvas!
			SendMessage( WM_HSCROLL, SB_THUMBPOSITION ); // letting descendants of HexaEditor know that a scrolling occured
			::DestroyCaret();
		}
	}

	void CInstance::RefreshScrollInfo(){
		// refreshes all parameters that relate scrolling content horizontally or vertically
		if (!f)
			return;
		// - determining the total number of Rows
	{	EXCLUSIVELY_LOCK_THIS_OBJECT();
		nLogicalRows=__logicalPositionToRow__( std::max(f.GetLength(),logicalSize) );
	}	// - setting horizontal scroll parameters
		//nop (in WM_SIZE)
		// - setting vertical scroll parameters
		const RECT r=GetClientRect();
		nRowsDisplayed=std::max( 0L, (r.bottom-r.top)/font.GetCharHeight()-HEADER_LINES_COUNT );
		nRowsOnPage=std::max( 0, nRowsDisplayed-1 );
		// - paint both scrollbars
		if (::GetCurrentThreadId()==::GetWindowThreadProcessId(hWnd,nullptr)) // do we own the HexaEditor control?
			SendMessage( WM_HEXA_PAINTSCROLLBARS );
		else
			PostMessage( WM_HEXA_PAINTSCROLLBARS );
	}

	void CInstance::__refreshCaretDisplay__() const{
		// shows Caret on screen at position that corresponds with Caret's actual Position in the underlying File content (e.g. the 12345-th Byte of the File)
		#define CARET_DISABLED_HEIGHT 2
		::CreateCaret(
			hWnd, nullptr, font.GetCharAvgWidth(),
			!editable*CARET_DISABLED_HEIGHT + editable*font.GetCharHeight() // either N (if not Editable) or CharHeight (if Editable)
		);
		TPosition currRecordStart, currRecordLength=1;
		pStreamAdvisor->GetRecordInfo( caret.streamPosition, &currRecordStart, &currRecordLength, nullptr );
		if (currRecordLength<1)
			currRecordLength=1;
		const auto d=div( caret.streamPosition-currRecordStart, currRecordLength );
		const auto iScrollY=GetVertScrollPos();
		//if (d.quot>=iScrollY){ // commented out as always guaranteed
			// Caret "under" the header
			POINT pos={
				d.rem % GetStreamBytesCountPerRow(), // translated below to a particular pixel position
				(HEADER_LINES_COUNT + __logicalPositionToRow__(caret.streamPosition) - iScrollY)*font.GetCharHeight() // already a particular Y pixel position
			};
			if (caret.IsInStream()) // Caret in the Stream ("Ascii") area
				pos.x+=GetCharLayout().stream.a;
			else // Caret in the View area
				pos.x=GetCharLayout().view.a+pos.x/item.nStreamBytes*item.patternLength+caret.iViewHalfbyte;
			pos.x-=GetHorzScrollPos();
			pos.x*=font.GetCharAvgWidth();
			pos.y+=!editable*(font.GetCharHeight()-CARET_DISABLED_HEIGHT);
			::SetCaretPos( pos.x, pos.y );
		/*}else{ // commented out as it never occurs
			// Caret "above" the header
			static constexpr POINT Pos={ -100, -100 };
			SetCaretPos(Pos);
		}*/
	}

	TPosition CInstance::__logicalPositionFromPoint__(const POINT &pt,PCHAR piPlaceholder) const{
		// determines and returns the LogicalPosition pointed to by the input Point (or -1 if not pointing at a particular Byte in both the View and Stream columns)
		int x=pt.x/font.GetCharAvgWidth()-(addrLength+ADDRESS_SPACE_LENGTH)+GetHorzScrollPos();
		const TRow r=pt.y/font.GetCharHeight()-HEADER_LINES_COUNT+GetVertScrollPos();
		const TPosition currLineStart=__firstByteInRowToLogicalPosition__(r);
		const int nCurrLineBytes=__firstByteInRowToLogicalPosition__(r+1)-currLineStart, currLineLastByte=nCurrLineBytes-1;
		if (IsColumnShown(TColumn::VIEW)){
			const int nViewChars=item.nInRow*item.patternLength;
			if (0<=x && x<nViewChars){
				// View area
				const auto d=div( x, item.patternLength );
				if (piPlaceholder)
					if (d.quot>=nCurrLineBytes/item.nStreamBytes)
						*piPlaceholder=item.iLastPlaceholder;
					else
						for( char i=0; i<item.patternLength; i++ ) // find closest Placeholder
							if (d.rem-i>=0 && item.IsPlaceholder(d.rem-i)){
								*piPlaceholder=d.rem-i;
								break;
							}else if (d.rem+i<item.patternLength && item.IsPlaceholder(d.rem+i)){
								*piPlaceholder=d.rem+i;
								break;
							}
				return currLineStart+std::min( d.quot*item.nStreamBytes, currLineLastByte );
			}
			x-=nViewChars+VIEW_SPACE_LENGTH;
		}
		if (IsColumnShown(TColumn::STREAM))
			if (0<=x && x<GetStreamBytesCountPerRow()){
				// Stream area
				if (piPlaceholder) *piPlaceholder=-item.patternLength;
				return currLineStart+std::min( x, currLineLastByte );
			}
		return -1; // outside any area
	}

	void CInstance::ShowMessage(TMsg id) const{
		// shows Message and passes focus back to the HexaEditor
		pOwner->ShowInformation(id);
		::PostMessage( hWnd, WM_LBUTTONDOWN, 0, -1 ); // recovering the focus; "-1" = [x,y] = nonsense value; can't use mere SetFocus because this alone doesn't work
	}

	BYTE CInstance::ReadByteUnderCaret(HRESULT &outResult) const{
		//
		if (caret.IsInStream()) // in Stream column
			f.Seek( caret.streamPosition );
		else
			f.Seek( caret.streamPosition+item.GetByteIndex(caret.iViewHalfbyte) );
		BYTE b=0;
		f.Read( &b, sizeof(b), outResult );
		return b;
	}

	void CInstance::SendEditNotification(WORD en) const{
		::SendMessage( ::GetParent(hWnd), WM_COMMAND, MAKELONG(GetWindowLong(hWnd,GWL_ID),en), 0 );
	}

	void CInstance::PasteStreamAtCaretAndShowError(IStream *s){
		// pastes input Stream at Caret's CurrentPosition, showing also an eventual error
		if (s!=nullptr){
			caret.CancelSelection(); // select what has been pasted
			f.Seek(caret.streamPosition);
			const auto lengthLimit=logicalSizeLimits.z-caret.streamPosition;
			const auto streamLength=Stream::GetLength(s);
			struct{ TMsg msg; DWORD code; } error={};
			TPosition nBytesToRead;
			if (streamLength<=lengthLimit)
				nBytesToRead=streamLength;
			else{
				nBytesToRead=lengthLimit;
				error.msg=MSG_LIMIT_UPPER;
			}
			for( ULONG nRead,nWritten; nBytesToRead>0; nBytesToRead-=nWritten ){
				BYTE buf[65536];
				if (FAILED(s->Read( buf, std::min<ULONG>(nBytesToRead,sizeof(buf)), &nRead ))){
					error.msg=MSG_PASTED_PARTIALLY, error.code=ERROR_READ_FAULT;
					break;
				}
				nWritten=f.Write( buf, nRead, IgnoreIoResult );
				caret.streamPosition+=nWritten; // expand the Selection with what has just been pasted
				if (nWritten!=nRead){
					error.msg=MSG_PASTED_PARTIALLY, error.code=ERROR_WRITE_FAULT;
					break;
				}
			}
			SendEditNotification( EN_CHANGE );
			RepaintData();
			const Utils::CVarTempReset<bool> md0( mouseDragged, true ); // don't change Selection (if any) by pretending mouse button being pressed
			SendMessage( WM_KEYDOWN, VK_JUNJA ); // caretCorrectlyMoveTo
			if (error.msg)
				pOwner->ShowInformation( error.msg, error.code );
		}else
			pOwner->ShowInformation( ERROR_PASTE_FAILED, ::GetLastError() );
	}

	void CInstance::ScrollToCaretAsync(){
		// makes sure the Caret is visible
		if (::GetCurrentThreadId()==::GetWindowThreadProcessId(hWnd,nullptr)) // do we own the HexaEditor control?
			SendMessage( WM_KEYDOWN, VK_KANJI );
		else
			PostMessage( WM_KEYDOWN, VK_KANJI );
	}

	CInstance::TCharLayout CInstance::GetCharLayout() const{
		const int xViewA=addrLength+ADDRESS_SPACE_LENGTH, xViewZ=xViewA+IsColumnShown(TColumn::VIEW)*item.patternLength*item.nInRow;
		const int xStreamA=xViewZ+IsColumnShown(TColumn::VIEW)*VIEW_SPACE_LENGTH, xStreamZ=xStreamA+IsColumnShown(TColumn::STREAM)*GetStreamBytesCountPerRow();
		const int xLabelA=xStreamZ+IsColumnShown(TColumn::STREAM)*LABEL_SPACE_LENGTH, xLabelZ=xLabelA+IsColumnShown(TColumn::LABEL)*std::abs(label.nCharsMax);
		const TCharLayout result={
			TInterval<LONG>( 0, addrLength ),
			TInterval<LONG>( xViewA, xViewZ ),
			TInterval<LONG>( xStreamA, xStreamZ ),
			TInterval<LONG>( xLabelA, xLabelZ )
		};
		return result;
	}

	RECT CInstance::GetClientRect() const{
		assert(hWnd);
		RECT rc;
		::GetClientRect( hWnd, &rc );
		return rc;
	}

}
