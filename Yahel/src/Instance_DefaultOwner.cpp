#include "stdafx.h"
#include "Instance.h"

namespace Yahel{

	// searching
	bool CInstance::QueryNewSearchParams(TSearchParams &outSp) const{
		return false;
	}

	TPosition CInstance::ContinueSearching(const TSearchParams &sp) const{
		return Stream::GetErrorPosition();
	}



	// navigation
	bool CInstance::QueryAddressToGoTo(TGoToParams &outGtp) const{
		return false;
	}



	// resetting
	bool CInstance::QueryByteToResetSelectionWith(TResetSelectionParams &outRsp) const{
		return false;
	}



	// GUI
	int CInstance::GetCustomCommandMenuFlags(WORD cmd) const{
		// custom command GUI update
		int flags=0; // enabled and unchecked
		switch (cmd){
			case ID_YAHEL_EDIT_COPY:
			case ID_YAHEL_FILE_SAVE_COPY_AS:
				flags|=MF_GRAYED*!( caret.SelectionExists() );
				return flags;
			case ID_YAHEL_EDIT_PASTE:
				flags|=MF_GRAYED*!( editable && ::IsClipboardFormatAvailable(GetClipboardFormat()) );
				return flags;
			case ID_YAHEL_BOOKMARK_TOGGLE:
				flags|=MF_GRAYED*!(
					!caret.SelectionExists() // no selection
					&&
					caret.streamPosition<f.GetLength() // can't set a Bookmark beyond the content
				);
				flags|=MF_CHECKED*( bookmarks.find(caret.streamPosition)!=bookmarks.end() );
				return flags;
			case ID_YAHEL_BOOKMARK_DELETEALL:
			case ID_YAHEL_SELECT_ALL:
			case ID_YAHEL_SELECT_NONE:
			case ID_YAHEL_SELECT_CURRENT:
			case ID_YAHEL_GOTO_RECORD_NEXT:
			case ID_YAHEL_GOTO_RECORD_PREV:
			case ID_YAHEL_GOTO_ADDRESS:
				return flags;
			case ID_YAHEL_FIND:
			case ID_YAHEL_FIND_NEXT:
				flags|=MF_GRAYED*!( SEARCH_ENABLED );
				return flags;
			case ID_YAHEL_EDIT_PASTE_SPECIAL:
			case ID_YAHEL_EDIT_DELETE:
			case ID_YAHEL_EDIT_RESET_ZERO:
			case ID_YAHEL_EDIT_RESET:
				flags|=MF_GRAYED*!( editable );
				return flags;
			case ID_YAHEL_BOOKMARK_PREV:
				flags|=MF_GRAYED*!( bookmarks.size()>0 && *bookmarks.cbegin()<caret.streamPosition );
				return flags;
			case ID_YAHEL_BOOKMARK_NEXT:
				flags|=MF_GRAYED*!( bookmarks.size()>0 && caret.streamPosition<*bookmarks.crbegin() );
				return flags;
		}
		return -1; // unknown command
	}
		
	bool CInstance::ShowOpenFileDialog(LPCWSTR singleFilter,DWORD ofnFlags,PWCHAR lpszFileNameBuffer,WORD bufferCapacity) const{
		return false;
	}

	bool CInstance::ShowSaveFileDialog(LPCWSTR singleFilter,DWORD ofnFlags,PWCHAR lpszFileNameBuffer,WORD bufferCapacity) const{
		return false;
	}

	void CInstance::ShowInformation(TMsg id,UINT errorCode) const{
	}

	bool CInstance::ShowQuestionYesNo(TMsg id,UINT defaultButton) const{
		return false;
	}

}
