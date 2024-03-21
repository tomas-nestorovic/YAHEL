#ifndef YAHEL_INSTANCE_H
#define YAHEL_INSTANCE_H

namespace Yahel{

	#define HEADER_LINES_COUNT		1
	#define HEADER_HEIGHT			(HEADER_LINES_COUNT*font.GetCharHeight())

	#define ADDRESS_FORMAT			L" %04X-%04X"
	#define ADDRESS_FORMAT_LENGTH	10
	#define ADDRESS_SPACE_LENGTH	1

	#define ITEM_PATTERN_LENGTH_MIN	'\x3'
	#define ITEM_STREAM_BYTES_MAX	('Z'+1-'A')
	#define ITEM_STREAM_BYTES_SPEC_LEN (2+1)

	#define VIEW_HALFBYTE_FORMAT	L"%X"
	#define VIEW_SPACE_LENGTH		2

	#define STREAM_BYTES_IN_ROW_MAX	512

	#define LABEL_SPACE_LENGTH		2

	#define WM_HEXA_PAINTSCROLLBARS	WM_USER+1

	#define MARK_RECURRENT_USE	(&CInstance::mouseDragged)
	#define IS_RECURRENT_USE	(param==MARK_RECURRENT_USE)

	#define CLIPFORMAT_YAHEL_BINARY	_T("tomascz:cfYahelBinary")



	struct TState{
		TPosInterval logicalSizeLimits;
		TPosition logicalSize; // zero by default

		inline TState()
			: logicalSizeLimits(0,0)
			, logicalSize(0) {
		}

		inline bool operator!=(const TState &r) const{
			return ::memcmp( this, &r, sizeof(*this) )!=0;
		}
	};



	typedef class CInstance:public IInstance, public IOwner, TState{
		static LRESULT WINAPI WndProc(HWND hYahel,UINT msg,WPARAM wParam,LPARAM lParam);
	protected:
		struct TCaretPosition{
			static const TCaretPosition Invalid;

			TPosition streamPosition;
			char iViewHalfbyte; // negative <=> Caret in Stream column, otherwise positive or zero

			inline TCaretPosition(TPosition streamPosition=0,char iViewHalfbyte=0) // by default in View column
				: streamPosition(streamPosition)
				, iViewHalfbyte(iViewHalfbyte) {
			}

			inline bool operator<(const TCaretPosition &r) const{ return streamPosition<r.streamPosition; }
			inline operator bool() const{ return streamPosition>=0; }
			inline operator TPosition() const{ return streamPosition; }
			inline bool IsInStream() const{ return iViewHalfbyte<0; }
		};

		static bool ShiftPressedAsync();

		volatile ULONG nReferences;
		const HINSTANCE hInstance;
		mutable CRITICAL_SECTION locker;

		const PVOID param;
		const HACCEL hDefaultAccelerators;
		const Utils::CYahelContextMenu contextMenu;
		Utils::CYahelFont font;
		HWND hWnd;
		TRow nLogicalRows;
		TRow nRowsDisplayed;
		TRow nRowsOnPage;
		TPosition logPosScrolledTo; // Position in the upper left corner of the hexa-editor
		HWND hPreviouslyFocusedWnd;
		struct TCaret sealed:public TCaretPosition{
			TCaretPosition selectionInit;
			TPosInterval streamSelection;
			
			TCaret(TPosition position); // ctor

			inline bool SelectionExists() const{ return streamSelection.IsValidNonempty(); }
			void CancelSelection();
		} caret;

		std::set<TPosition> bookmarks;

		struct TEmphasis:public TPosInterval{
			COLORREF color;

			TEmphasis(const TPosInterval &range,COLORREF color);	

			inline bool operator<(const TEmphasis &r) const{ return a<r.a; }
			inline bool IsSameColorNext(const TEmphasis &r) const{ return z==r.a && color==r.color; }
		};
		std::set<TEmphasis> emphases;

		struct TLabelColumn{
			char nCharsMax; // negative = Label Column always visible without scrolling; zero = Column hidden; positive = Label Column always visible with eventual scrolling
			Utils::CYahelBrush bgBrush;

			inline TLabelColumn()
				: nCharsMax(16)
				, bgBrush(NULL_BRUSH) {
			}
		} label;
		
		const POwner pOwner;
		Stream::IAdvisor *pStreamAdvisor;
		BYTE columns; // TColumn flags
		TCol addrLength; // Address format length (see ADDRESS_FORMAT); modified in ShowColumns
		bool editable; // True <=> content can be edited, otherwise False
		TState update;
		CComPtr<IDataObject> delayedDataInClipboard; // an IDataObject whose data were not yet rendered
	public:
		struct TItem sealed{
			BYTE nStreamBytes;
			char patternLength;
			char iFirstPlaceholder, iLastPlaceholder; // inclusive!
			struct{
				WCHAR printableChar; // '\0' = a placeholder for a Halfbyte
				BYTE iStreamHalfByte;
			} pattern[80]; // e.g. "Hh ABba* 4CcDdEe" where {a...z} and {A...Z} are lower/upper halfbyte references, and the rest characters to print out
			WORD nInRow; // current # of Items in a full Row
			TInterval<WORD> nInRowLimits;

			TError Redefine(LPCWSTR newDef);
			TError SetRowLimits(const TInterval<WORD> limits);
			LPCWSTR GetDefinition(PWCHAR def) const;
			inline WCHAR GetPrintableChar(BYTE iPattern) const{ return pattern[iPattern].printableChar; }
			inline void SetPrintableChar(BYTE iPattern,WCHAR c){ pattern[iPattern].printableChar=c; }
			inline bool IsPlaceholder(BYTE iPattern) const{ return !GetPrintableChar(iPattern); }
			inline void SetPlaceholder(BYTE iPattern,BYTE iByte,bool lowerHalfByte){ pattern[iPattern].iStreamHalfByte=iByte<<1|(BYTE)lowerHalfByte; }
			inline BYTE GetByteIndex(BYTE iPattern) const{ return pattern[iPattern].iStreamHalfByte>>1; }
			inline bool IsLowerHalfbyte(BYTE iPattern) const{ return (pattern[iPattern].iStreamHalfByte&1)!=0; }
		} item;
	protected:
		TSearchParams searchParams;

		TPosition __firstByteInRowToLogicalPosition__(TRow row) const;
		TRow __logicalPositionToRow__(TPosition logPos) const;
		TCaretPosition CaretPositionFromPoint(const POINT &pt) const;
		TPosInterval GetItemAt(const TCaretPosition &caretPos) const;
		TRow __scrollToRow__(TRow row);
		void ScrollToCaretAsync();
		void RefreshScrollInfo();
		void RefreshCaretDisplay() const;
		void ShowMessage(TMsg id) const;
		BYTE ReadByteUnderCaret(HRESULT &outResult) const;
		void SendEditNotification(WORD en) const;
		void PasteStreamAtCaretAndShowError(IStream *s,BYTE nIgnoredTailBytes=0);
		void PasteAtCaretAndShowError(const STGMEDIUM &stg,BYTE nIgnoredTailBytes=0);
		struct TCharLayout sealed{ TInterval<TCol> address,view,stream,label; } GetCharLayout() const;
		struct TMatrix sealed{ TInterval<TRow> rows; TInterval<TCol> cols; } GetVisibleMatrix() const;
		RECT GetClientRect() const;

		inline HWND SetFocus() const{ assert(hWnd); return ::SetFocus(hWnd); }
		inline void ShowCaret() const{ assert(hWnd); ::ShowCaret(hWnd); }
		inline void Invalidate(bool erase=true) const{ assert(hWnd); ::InvalidateRect(hWnd,nullptr,erase); }
		inline TCol GetHorzScrollPos() const{ assert(hWnd); return ::GetScrollPos(hWnd,SB_HORZ); }
		inline TRow GetVertScrollPos() const{ assert(hWnd); return ::GetScrollPos(hWnd,SB_VERT); }
		inline LRESULT SendMessage(UINT msg,WPARAM wParam=0,LPARAM lParam=0) const{ assert(hWnd); return ::SendMessage(hWnd,msg,wParam,lParam); }
		inline bool PostMessage(UINT msg,WPARAM wParam=0,LPARAM lParam=0) const{ assert(hWnd); return ::PostMessage(hWnd,msg,wParam,lParam)!=FALSE; }

		// default IOwner implementation
		bool QueryNewSearchParams(TSearchParams &outSp) const override;
		TPosition ContinueSearching(const TSearchParams &sp) const override;
		bool QueryAddressToGoTo(TGoToParams &outGtp) const override;
		bool QueryChecksumParams(TChecksumParams &outCp) const override;
		int ComputeChecksum(const TChecksumParams &cp) const override;
		bool QueryByteToResetSelectionWith(TResetSelectionParams &outRsp) const override;
		int GetCustomCommandMenuFlags(WORD cmd) const override;
		bool ShowOpenFileDialog(LPCWSTR singleFilter,DWORD ofnFlags,PWCHAR lpszFileNameBuffer,WORD bufferCapacity) const override;
		bool ShowSaveFileDialog(LPCWSTR singleFilter,DWORD ofnFlags,PWCHAR lpszFileNameBuffer,WORD bufferCapacity) const override;
		void ShowInformation(TMsg id,UINT errorCode=ERROR_SUCCESS) const override;
		bool ShowQuestionYesNo(TMsg id,UINT defaultButton=MB_DEFBUTTON2) const override;

	public:
		static bool mouseDragged,mouseInNcArea;

		struct TContent{
			CComPtr<IStream> stream;

			inline TContent(IStream *s=nullptr) : stream(s) {}

			inline operator IStream *() const{ return stream; }

			inline TPosition GetLength() const{ return Stream::GetLength(stream); }
			void SetLength(TPosition length) const;
			TPosition GetPosition() const;
			TPosition Seek(TPosition pos,STREAM_SEEK dwMoveMethod=STREAM_SEEK_SET) const;
			TPosition Read(PVOID buffer,TPosition bufferCapacity,HRESULT &outResult) const;
			TPosition Write(LPCVOID data,TPosition dataLength,HRESULT &outResult) const;
		} f;

		CInstance(HINSTANCE hInstance,POwner pOwner,PVOID lpParam,HFONT hFont);
		~CInstance();

		WNDPROC SubclassAndAttach(HWND hBaseClassWnd);

		// IUnknown
		ULONG STDMETHODCALLTYPE AddRef() override;
		ULONG STDMETHODCALLTYPE Release() override;
		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,PVOID *ppvObject) override;

		// creation (destroy by simply calling ::DestroyWindow)
		bool Attach(HWND hWnd) override;
		HWND Detach() override;
		bool ProcessMessage(UINT msg,WPARAM wParam,LPARAM lParam,LRESULT &outResult) override;
		void RepaintData() const override;
		int GetDefaultCommandMenuFlags(WORD cmd) const override;
		HMENU GetContextMenu() const override;
		HACCEL GetAcceleratorTable() const override;

		// general
		void SetEditable(bool editable) override;
		bool IsEditable() const override;
		void ShowColumns(BYTE columns=TColumn::ALL) override;
		bool IsColumnShown(TColumn c) const override;
		IStream *GetCurrentStream() const override;
		void Update(IStream *f,Stream::IAdvisor *sa) override;
		bool Update(IStream *f,Stream::IAdvisor *sa,const TPosInterval &fileLogicalSizeLimits) override;
		bool Reset(IStream *f,Stream::IAdvisor *sa,const TPosInterval &fileLogicalSizeLimits) override;
		TPosition GetCaretPosition() const override;
		TPosInterval GetSelectionAsc() const override;
		void SetSelection(TPosition selStart,TPosition selEnd) override;
		TPosInterval GetVisiblePart() const override;
		void ScrollTo(TPosition logicalPos,bool moveAlsoCaret=false) override;
		void ScrollToColumn(TCol col) override;
		void ScrollToRow(TRow iRow) override;

		// "Address" column
		int GetAddressColumnWidth() const override;

		// "View" column
		TError RedefineItem(LPCWSTR newItemDef=nullptr,WORD nItemsInRowMin=1,WORD nItemsInRowMax=-1) override;
		TError SetItemCountPerRow(WORD nItemsInRowMin=1,WORD nItemsInRowMax=-1) override;

		// "Stream" column
		bool SetStreamLogicalSizeLimits(const TPosInterval &limits) override;
		TPosition GetStreamLogicalSize() const override;
		void SetStreamLogicalSize(TPosition logicalSize) override;
		WORD GetStreamBytesCountPerRow() const override;
		int GetChecksum(const TChecksumParams &cp,volatile const bool &cancel) const override;

		// "Label" column
		TError SetLabelColumnParams(char nCharsSpace,COLORREF bgColor) override;

		// searching
		TPosition FindNextOccurence(const TPosInterval &range,volatile const bool &cancel) const override;

		// bookmarks
		bool ToggleBookmarkAt(TPosition pos) override;
		void RemoveAllBookmarks() override;

		// highlights
		bool AddHighlight(const TPosInterval &range,COLORREF color) override;
		void RemoveAllHighlights() override;

	} *PInstance;

}

#endif // YAHEL_INSTANCE_H
