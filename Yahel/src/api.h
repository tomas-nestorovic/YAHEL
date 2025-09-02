#ifndef YAHEL_API_H
#define YAHEL_API_H

#include <cassert>

#ifdef YAHEL_EXPORT
	#define YAHEL_DECLSPEC __declspec(dllexport)
#else
	#define YAHEL_DECLSPEC __declspec(dllimport)
#endif

#define IDR_YAHEL						450
#define IDR_YAHEL_FIND					451
#define IDR_YAHEL_ITEM_DEFINITION		452
#define IDR_YAHEL_SINGLE_NUMBER			453

#define ID_YAHEL_SELECT_NONE			41000
#define ID_YAHEL_BOOKMARK_TOGGLE		41001
#define ID_YAHEL_GOTO_RECORD_NEXT		41002
#define ID_YAHEL_GOTO_RECORD_PREV		41003
#define ID_YAHEL_FIND_PREV				41004
#define ID_YAHEL_FIND_NEXT				41005
#define ID_YAHEL_EDIT_DELETE			41006
#define ID_YAHEL_BOOKMARK_NEXT			41007
#define ID_YAHEL_SELECT_CURRENT			41008
#define ID_YAHEL_BOOKMARK_PREV			41009
#define ID_YAHEL_GOTO_ADDRESS			41010
#define ID_YAHEL_BOOKMARK_DELETEALL		41011
#define ID_YAHEL_EDIT_PASTE_SPECIAL		41012
#define ID_YAHEL_FIND					41013
#define ID_YAHEL_SELECT_ALL				41014
#define ID_YAHEL_EDIT_PASTE				41015
#define ID_YAHEL_FILE_SAVE_COPY_AS		41016
#define ID_YAHEL_EDIT_COPY				41017
#define ID_YAHEL_EDIT_RESET_ZERO		41018
#define ID_YAHEL_EDIT_RESET				41019
#define ID_YAHEL_COLUMN_ADDRESS         41020
#define ID_YAHEL_COLUMN_ITEMS           41021
#define ID_YAHEL_COLUMN_STREAM          41022
#define ID_YAHEL_COLUMN_LABEL           41023
#define ID_YAHEL_EDIT_INCREASE          41024
#define ID_YAHEL_EDIT_DECREASE          41025
#define ID_YAHEL_CHECKSUM_ADD           41026
#define ID_YAHEL_CHECKSUM_XOR           41027
#define ID_YAHEL_CHECKSUM_CCITT16       41028

namespace Yahel
{
	template<typename T>
	struct TInterval{
		T a; // inclusive
		T z; // exclusive

		inline TInterval(){}
		inline TInterval(T single) : a(single) , z(single+1) {}
		inline TInterval(T a,T z) : a(a), z(z) { assert(a<=z); }

		inline bool IsValid() const{ return a<=z; } // an empty Interval is valid
		inline bool IsValidNonempty() const{ return a<z; }
		inline bool Contains(T i) const{ return a<=i && i<z; }
		inline bool Contains(T i,T length) const{ return a<=i && i+length<=z; }
		inline bool Contains(const TInterval &r) const{ return a<=r.a && r.z<=z; }
		inline T GetLength() const{ return z-a; }

		inline operator bool() const{ return IsValidNonempty(); }

		T Clip(T i) const{
			if (i<a) i=a;
			if (i>=z) i=z-1;
			return i;
		}
	};

	typedef int TRow,TCol;
	typedef long TPosition,*PPosition;
	typedef TInterval<TPosition> TPosInterval;



	struct YAHEL_DECLSPEC TSearchParams{
		enum:int{ // in order of radio buttons in the default "Find" dialog
			ASCII_ANY_CASE,
			HEXA,
			NOT_BYTE,
			ASCII_MATCH_CASE,
			LAST
		} type;
		union{
			BYTE bytes[SCHAR_MAX];
			char chars[SCHAR_MAX];
		} pattern; // pattern to find
		BYTE patternLength;
		bool searchForward;
		bool selectFinding;

		TSearchParams(); // ctor

		bool IsValid() const;
		bool IsUsable() const;
		bool EditModalWithDefaultEnglishDialog(HWND hParent,HFONT hFont=0);
	};



	struct YAHEL_DECLSPEC TGoToParams{
		const TPosInterval addressRange;
		TPosition address; // initialized with current caret position in stream

		TGoToParams(const TPosInterval &addressRange,TPosition caretPos);

		bool IsValid() const;
		bool EditModalWithDefaultEnglishDialog(HWND hParent);
	};



	struct YAHEL_DECLSPEC TResetSelectionParams{
		enum:BYTE{
			Byte,
			GaussianNoise
		} type;
		BYTE byteValue;

		TResetSelectionParams();

		bool IsValid() const;
		bool EditModalWithDefaultEnglishDialog(HWND hParent);
	};



	namespace Checksum{
		struct YAHEL_DECLSPEC TParams{
			enum TType:BYTE{
				Add,
				Xor,
				Ccitt16,
				Last
			} type;
			int initValue;

			TParams();
			TParams(TType type,int initValue);

			bool IsValid() const;
			bool EditModalWithDefaultEnglishDialog(HWND hParent);
		};

		int YAHEL_DECLSPEC GetDefaultInitValue();
		int YAHEL_DECLSPEC GetErrorValue();
		int YAHEL_DECLSPEC Compute(const TParams &params,LPCVOID bytes,UINT nBytes);
		int YAHEL_DECLSPEC ComputeAdd(LPCVOID bytes,UINT nBytes,int initValue=0);
		BYTE YAHEL_DECLSPEC ComputeXor(LPCVOID bytes,UINT nBytes,BYTE initValue=0);
	}



	enum TMsg:BYTE{
		ERROR_KOSHER,
		ERROR_ITEM_DEF_BYTE_COUNT_UNKNOWN,
		ERROR_ITEM_DEF_BYTE_COUNT,
		ERROR_ITEM_DEF_PATTERN_TOO_LONG,
		ERROR_ITEM_DEF_PATTERN_FORBIDDEN,
		ERROR_ITEM_DEF_PATTERN_NOT_PRINTABLE,
		ERROR_ITEM_DEF_PATTERN_INDEX,
		ERROR_ITEM_DEF_PATTERN_NO_PLACEHOLDER,
		ERROR_ITEM_DEF_PATTERN_INSUFFICIENT_BUFFER,
		ERROR_ITEM_COUNT_INVALID		=15,
		ERROR_PASTE_FAILED,
		ERROR_SELECTION_CANT_SAVE,

		MSG_READY						=100,
		MSG_LIMIT_UPPER,
		MSG_SELECTION_SAVED_PARTIALLY,
		MSG_SELECTION_RESET_PARTIALLY,
		MSG_PASTED_PARTIALLY,
		MSG_PADDED_TO_MINIMUM_SIZE,

		QUERY_DELETE_SELECTED_BOOKMARKS	=190,
		QUERY_DELETE_ALL_BOOKMARKS
	};

	union TError{
		struct{
			TMsg code;
		};
		DWORD dw;

		inline TError(TMsg code):dw(code){}

		inline operator bool() const{ return code!=ERROR_KOSHER; }
	};



	typedef interface IOwner{
		// searching
		virtual bool QueryNewSearchParams(TSearchParams &outSp) const=0;
		virtual TPosition ContinueSearching(const TSearchParams &sp) const=0;
		// navigation
		virtual bool QueryAddressToGoTo(TGoToParams &outGtp) const=0;
		// resetting
		virtual bool QueryByteToResetSelectionWith(TResetSelectionParams &outRsp) const=0;
		// checksum
		virtual bool QueryChecksumParams(Checksum::TParams &outCp) const=0;
		virtual int ComputeChecksum(const Checksum::TParams &cp,const TPosInterval &range) const=0;
		// GUI
		virtual int GetCustomCommandMenuFlags(WORD cmd) const=0;
		virtual bool ShowOpenFileDialog(LPCWSTR singleFilter,DWORD ofnFlags,PWCHAR lpszFileNameBuffer,WORD bufferCapacity) const=0;
		virtual bool ShowSaveFileDialog(LPCWSTR singleFilter,DWORD ofnFlags,PWCHAR lpszFileNameBuffer,WORD bufferCapacity) const=0;
		virtual void ShowInformation(TMsg id,UINT errorCode=ERROR_SUCCESS) const=0;
		virtual bool ShowQuestionYesNo(TMsg id,UINT defaultButton=MB_DEFBUTTON2) const=0;
	} *POwner;




namespace Gui
{
	enum TDlgItemIntType{
		Unsigned,
		Hexa,
		Signed
	};

	LPCWSTR YAHEL_DECLSPEC WINAPI GetDefaultEnglishMessage(TMsg id);
	bool YAHEL_DECLSPEC WINAPI GetDlgItemInt(HWND hDlg,WORD editBoxId,TPosition &outValue,TDlgItemIntType type);
	bool YAHEL_DECLSPEC WINAPI SetDlgItemInt(HWND hDlg,WORD editBoxId,TPosition value,TDlgItemIntType type);
	inline bool GetDlgItemInt(HWND hDlg,WORD editBoxId,TPosition &outValue,bool hexa){ return GetDlgItemInt( hDlg, editBoxId, outValue, (TDlgItemIntType)hexa ); }
	inline bool SetDlgItemInt(HWND hDlg,WORD editBoxId,TPosition value,bool hexa){ return SetDlgItemInt( hDlg, editBoxId, value, (TDlgItemIntType)hexa ); }
	bool YAHEL_DECLSPEC WINAPI QuerySingleIntA(LPCSTR caption,LPCSTR label,const TPosInterval &rangeIncl,TPosition &inOutValue,TDlgItemIntType type,HWND hParent);
	bool YAHEL_DECLSPEC WINAPI QuerySingleIntW(LPCWSTR caption,LPCWSTR label,const TPosInterval &rangeIncl,TPosition &inOutValue,TDlgItemIntType type,HWND hParent);
	#ifdef UNICODE
		#define QuerySingleInt QuerySingleIntW
	#else
		#define QuerySingleInt QuerySingleIntA
	#endif
}



	namespace Stream{
		interface IAdvisor{
			static TPosition YAHEL_DECLSPEC WINAPI GetMaximumRecordLength();

			virtual void GetRecordInfo(TPosition pos,PPosition pOutRecordStartLogPos,PPosition pOutRecordLength,bool *pOutDataReady)=0;
			virtual TRow LogicalPositionToRow(TPosition pos,WORD nStreamBytesInRow)=0;
			virtual TPosition RowToLogicalPosition(TRow row,WORD nStreamBytesInRow)=0;
			virtual LPCWSTR GetRecordLabelW(TPosition pos,PWCHAR labelBuffer,BYTE labelBufferCharsMax,PVOID param) const=0;
		};

		TPosition YAHEL_DECLSPEC WINAPI GetMaximumLength();
		TPosition YAHEL_DECLSPEC WINAPI GetErrorPosition();
		TPosition YAHEL_DECLSPEC WINAPI GetLength(IStream *s);
		IStream YAHEL_DECLSPEC * WINAPI FromBuffer(PVOID pBuffer,TPosition length);
		IStream YAHEL_DECLSPEC * WINAPI FromFileForSharedReading(LPCWSTR fileName,DWORD dwFlagsAndAttributes=FILE_ATTRIBUTE_NORMAL);
		IDataObject YAHEL_DECLSPEC * WINAPI CreateDataObject(IStream *s,const TPosInterval &range);
	};



	interface IInstance:public IUnknown
	{
		enum TColumn:BYTE{
			ADDRESS	=1,
			VIEW	=2,
			STREAM	=4,
			LABEL	=8,
			DATA	= VIEW | STREAM,
			MINIMAL	= DATA,
			ALL		= ADDRESS | MINIMAL | LABEL
		};

		static LPCTSTR YAHEL_DECLSPEC WINAPI GetVersionString();
		static void YAHEL_DECLSPEC WINAPI ShowModalAboutDialog(HWND hParent);
		static LPCSTR YAHEL_DECLSPEC WINAPI GetBaseClassNameA(HINSTANCE hInstance);
		static LPCWSTR YAHEL_DECLSPEC WINAPI GetBaseClassNameW(HINSTANCE hInstance);
		static UINT YAHEL_DECLSPEC WINAPI GetClipboardFormat();
		static IInstance YAHEL_DECLSPEC * WINAPI Create(HINSTANCE hInstance,POwner pOwner,PVOID lpParam=0,HFONT hFont=0);
		static LPCWSTR YAHEL_DECLSPEC WINAPI GetDefaultByteItemDefinition();
		static bool YAHEL_DECLSPEC WINAPI DefineItemUsingDefaultEnglishDialog(PWCHAR definitionBuffer,BYTE bufferCapacity,HWND hParent=0,HFONT hFont=0);

		// window manipulation (destroy by simply calling ::DestroyWindow)
		virtual bool Attach(HWND hYahel)=0;
		virtual HWND Detach()=0;
		virtual bool ProcessMessage(UINT msg,WPARAM wParam,LPARAM lParam,LRESULT &outResult)=0;
		virtual int GetDefaultCommandMenuFlags(WORD cmd) const=0;
		virtual void RepaintData() const=0; // the preferred way over InvalidateRect
		virtual HMENU GetContextMenu() const=0;
		virtual HACCEL GetAcceleratorTable() const=0;

		// general
		virtual void SetEditable(bool editable)=0;
		virtual bool IsEditable() const=0;
		virtual void ShowColumns(BYTE columns=TColumn::ALL)=0;
		virtual bool IsColumnShown(TColumn c) const=0;
		virtual IStream *GetCurrentStream() const=0;
		virtual void Update(IStream *s,Stream::IAdvisor *sa)=0;
		virtual bool Update(IStream *s,Stream::IAdvisor *sa,const TPosInterval &fileLogicalSizeLimits)=0;
		virtual bool Reset(IStream *s,Stream::IAdvisor *sa,const TPosInterval &fileLogicalSizeLimits)=0;
		virtual TPosition GetCaretPosition() const=0;
		virtual TPosInterval GetSelectionAsc() const=0;
		virtual void SetSelection(TPosition selStart,TPosition selEnd)=0;
		virtual TPosInterval GetVisiblePart() const=0;
		virtual void ScrollTo(TPosition logicalPos,bool moveAlsoCaret=false)=0;
		virtual void ScrollToColumn(TCol col)=0;
		virtual void ScrollToRow(TRow iRow)=0;

		// "Address" column
		virtual int GetAddressColumnWidth() const=0;

		// "View" column
		virtual TError RedefineItem(LPCWSTR newItemDef=nullptr,WORD nItemsInRowMin=1,WORD nItemsInRowMax=-2)=0;
		virtual TError SetItemCountPerRow(WORD nItemsInRowMin=1,WORD nItemsInRowMax=-1)=0;

		// "Stream" column
		virtual bool SetStreamLogicalSizeLimits(const TPosInterval &limits)=0;
		virtual TPosition GetStreamLogicalSize() const=0;
		virtual void SetStreamLogicalSize(TPosition logicalSize)=0;
		virtual WORD GetStreamBytesCountPerRow() const=0;
		virtual int GetChecksum(const Checksum::TParams &cp,const TPosInterval &range,volatile const bool &cancel) const=0;

		// "Label" column
		virtual TError SetLabelColumnParams(char nLabelCharsMax,COLORREF bgColor)=0; // 'zero' = hidden column, 'positive' = shown and must scroll to see the column, 'negative' = shown and always visible

		// searching
		virtual TPosition FindNextOccurence(const TPosInterval &range,volatile const bool &cancel) const=0;

		// bookmarks
		virtual bool ToggleBookmarkAt(TPosition pos)=0;
		virtual void RemoveAllBookmarks()=0;

		// highlights
		virtual bool AddHighlight(const TPosInterval &range,COLORREF color=CLR_DEFAULT)=0; // use alpha channel to "un-highlight" a range
		virtual void RemoveAllHighlights()=0;
	};
}

#endif // YAHEL_API_H
