#include "stdafx.h"
#include "Instance.h"

namespace Yahel{

	bool CInstance::ProcessMessage(UINT msg,WPARAM wParam,LPARAM lParam,LRESULT &outResult){
		// True <=> message fully processed and no need for the caller to process it further, otherwise False
		static LPARAM cursorPos0;
		int i;
		const auto caretStreamPos0=caret.streamPosition;
		outResult=0;
		switch (msg){
			case WM_MOUSEACTIVATE:
				// preventing the focus from being stolen by the parent
				outResult=MA_ACTIVATE;
				return true;
			case WM_KEYDOWN:{
				// key pressed
				if (!f) // there's nothing to do without an underlying Content
					return true;
				const bool ctrl=::GetKeyState(VK_CONTROL)<0;
				switch (wParam){
					case VK_LEFT:
						if (caret.IsInStream()) // in Stream column
							caret.streamPosition--;
						else{ // in View column
							const auto iRow=__logicalPositionToRow__(caret.streamPosition);
							const auto currRowStart=__firstByteInRowToLogicalPosition__(iRow);
							if (caret.streamPosition==currRowStart && caret.iViewHalfbyte==item.iFirstPlaceholder) // about to move Caret "before" current line?
								caret.streamPosition=currRowStart-1, caret.iViewHalfbyte=item.patternLength; // move Caret to previous line
							do{
								if (--caret.iViewHalfbyte<0) // Caret into previous Item?
									caret.streamPosition-=item.nStreamBytes, caret.iViewHalfbyte=item.iLastPlaceholder;
							}while (!item.IsPlaceholder(caret.iViewHalfbyte)); // skip anything in Item pattern that isn't a Halfbyte "placeholder"
						}
						//fallthrough
					case VK_JUNJA:{
caretCorrectlyMoveTo:	// . adjusting the Caret's Position (aligning towards the beginning of File)
						if (!f) // there's nothing to do without an underlying Content
							return true;
						if (caret.streamPosition<0){
							caret.streamPosition=0;
							if (!caret.IsInStream()) // in View column
								caret.iViewHalfbyte=item.iFirstPlaceholder;
						}else if (caret.streamPosition>f.GetLength())
							caret.streamPosition=f.GetLength();
						const auto iRow=__logicalPositionToRow__(caret.streamPosition);
						const auto currRowStart=__firstByteInRowToLogicalPosition__(iRow);
						const auto nextRowStart=__firstByteInRowToLogicalPosition__(iRow+1);
						const auto itemPosition=currRowStart+(caret.streamPosition-currRowStart)/item.nStreamBytes*item.nStreamBytes; // align to the beginning of current Item
						if (caret.IsInStream()){ // in Stream column
							if (std::min(nextRowStart,f.GetLength())-itemPosition<item.nStreamBytes) // Item under Caret incomplete?
								caret.iViewHalfbyte=-item.patternLength;
						}else{ // in View column
							caret.streamPosition = itemPosition;
							if (std::min(nextRowStart,f.GetLength())-itemPosition<item.nStreamBytes) // Item under Caret incomplete?
								/*if (caret.streamPosition>currRowStart) // can go to previous Item in the same line?
									caret.streamPosition-=item.nStreamBytes, caret.iViewHalfbyte=item.iLastPlaceholder;
								else*/
									caret.iViewHalfbyte=0;
						}
						// . adjusting an existing Selection if Shift pressed
						if (mouseDragged || ShiftPressedAsync()){ // wanted to adjust Selection? (by dragging the mouse or having the Shift key pressed)
							if (caret.streamPosition!=caretStreamPos0) // did the Caret actually move?
								RepaintData();
						}else{
							if (caret.streamSelectionA!=caretStreamPos0) // if there has been a Selection before ...
								RepaintData(); // ... invalidating the content as the Selection is about to be cancelled
							caret.CancelSelection(); // cancelling any Selection
						}
						//fallthrough
					}
					case VK_KANJI:{
caretRefresh:			// refresh of Caret display
						// . avoid visual artifacts by hiding the Caret first
						::HideCaret(hWnd);
						// . scrolling if Caret has been moved to an invisible part of the File content
						const TRow iRow=__logicalPositionToRow__(caret.streamPosition), iScrollY=GetVertScrollPos();
						if (iRow<iScrollY) __scrollToRow__(iRow);
						else if (iRow>=iScrollY+nRowsOnPage) __scrollToRow__(iRow-nRowsOnPage+1);
						// . displaying the Caret
						if (::GetFocus()==hWnd){
							__refreshCaretDisplay__();
							ShowCaret();
						}
						return true;
					}
					case VK_RIGHT:
						if (caret.IsInStream()) // in Stream column
							caret.streamPosition++;
						else{ // in View column
							const auto iRow=__logicalPositionToRow__(caret.streamPosition);
							const auto nextRowStart=__firstByteInRowToLogicalPosition__(iRow+1);
							if (nextRowStart-caret.streamPosition<item.nStreamBytes) // Item under Caret incomplete?
								caret.streamPosition=nextRowStart, caret.iViewHalfbyte=-1; // move Caret to next line
							do{
								if (++caret.iViewHalfbyte>=item.patternLength) // Caret into next Item?
									caret.streamPosition+=item.nStreamBytes, caret.iViewHalfbyte=item.iFirstPlaceholder;
							}while (!item.IsPlaceholder(caret.iViewHalfbyte)); // skip anything in Item pattern that isn't a Halfbyte "placeholder"
						}
						goto caretCorrectlyMoveTo;
					case VK_UP:{
						i=1; // move Caret one row up
moveCaretUp:			const auto iRow=__logicalPositionToRow__(caret.streamPosition);
						if (ctrl){
							const auto iScrollY=__scrollToRow__(GetVertScrollPos()-i);
							if (iRow<iScrollY+nRowsOnPage) goto caretRefresh;
						}
						const auto currRowStart=__firstByteInRowToLogicalPosition__(iRow);
						const auto targetRowStart=__firstByteInRowToLogicalPosition__(iRow-i);
						const auto caseA=caret.streamPosition-__firstByteInRowToLogicalPosition__(iRow-i+1)+1;
							// Case A example:
							// ..........			Target row
							// .................
							// ..........
							// .............C...	Current row with Caret
						const auto caseB=currRowStart-targetRowStart;
							// Case B example:
							// .................	..........			Target row
							// ..........			................
							// .................	..........
							// .......C..			.......C........	Current row with Caret
						if (caseA>caseB)
							caret.streamPosition-=caseA, caret.iViewHalfbyte=item.iLastPlaceholder;
						else
							caret.streamPosition-=caseB;
						goto caretCorrectlyMoveTo;
					}
					case VK_DOWN:{
						i=1; // move Caret one row down
moveCaretDown:			const auto iRow=__logicalPositionToRow__(caret.streamPosition);
						if (ctrl){
							const auto iScrollY=__scrollToRow__(GetVertScrollPos()+i);
							if (iRow>=iScrollY) goto caretRefresh;
						}
						const auto currRowStart=__firstByteInRowToLogicalPosition__(iRow);
						const auto targetRowStart=__firstByteInRowToLogicalPosition__(iRow+i);
						const auto caseA=__firstByteInRowToLogicalPosition__(iRow+i+1)-caret.streamPosition-1;
							// Case A example:
							// .............C...	Current row with Caret
							// ..........
							// .................
							// ..........			Target row
						const auto caseB=targetRowStart-currRowStart;
							// Case B example:
							// .......C..			.......C........	Current row with Caret
							// .................	..........
							// ..........			................
							// .................	..........			Target row
						if (caseA<caseB){
							caret.streamPosition+=caseA;
							caret.iViewHalfbyte=item.iLastPlaceholder - caret.IsInStream()*item.patternLength;
						}else
							caret.streamPosition+=caseB;
						goto caretCorrectlyMoveTo;
					}
					case VK_PRIOR:	// page up
						i=nRowsOnPage-ctrl; // move Caret N rows up
						goto moveCaretUp;
					case VK_NEXT:	// page down
						i=nRowsOnPage-ctrl; // move Caret N rows down
						goto moveCaretDown;
					case VK_HOME:
						caret.streamPosition=( ctrl ? 0 : __firstByteInRowToLogicalPosition__(__logicalPositionToRow__(caret.streamPosition)) );
						if (!caret.IsInStream()) // in View column
							caret.iViewHalfbyte=item.iFirstPlaceholder;
						goto caretCorrectlyMoveTo;
					case VK_END:
						caret.streamPosition=( ctrl ? f.GetLength() : __firstByteInRowToLogicalPosition__(__logicalPositionToRow__(caret.streamPosition)+1)-1 );
						if (!caret.IsInStream()) // in View column
							caret.iViewHalfbyte=item.iLastPlaceholder;
						goto caretCorrectlyMoveTo;
					case VK_TAB:{
						const bool shiftPressed=ShiftPressedAsync();
						if (shiftPressed ^ caret.IsInStream()  ||  (columns&TColumn::MINIMAL)!=TColumn::MINIMAL){
							// leaving the HexaEditor control if (a) Tab alone pressed while in Stream part, or (b) Shift+Tab pressed while in the hexa-View part, or (c) there is nowhere to switch to
							::SetFocus(  ::GetNextDlgTabItem( ::GetTopWindow(hWnd), hWnd, shiftPressed )  );
							break;
						}else{
							// switching between the Stream and hexa-View parts
							if (caret.IsInStream()) // in Stream column
								caret.iViewHalfbyte+=item.patternLength; // move to remembered Halfbyte in View column
							else // in View column
								caret.iViewHalfbyte-=item.patternLength; // remember Halfbyte in View column
							const bool selected=caret.SelectionExists();
							const Utils::CVarTempReset<bool> md0( mouseDragged, true ); // don't change Selection (if any) by pretending mouse button being pressed
							SendMessage( WM_KEYDOWN, VK_JUNJA ); // caretCorrectlyMoveTo
							if (!selected) // nothing selected before we switched columns
								caret.CancelSelection(); // a Selection may be created while holding the Shift key pressed during this command (Shift+Tab)
							return true;
						}
					}
					case VK_BACK:
						// deleting the Byte before Caret, or deleting the Selection
						if (!editable) return true; // can't edit content of a disabled window
						// . if Selection not set, setting it as the Byte immediately before Caret
						if (!caret.SelectionExists())
							if (caret.IsInStream()){ // in Stream column
								if (caret.streamPosition<=0) return true; // can actually delete anything before the Caret?
								caret.streamSelectionA=caret.streamPosition-1;
							}else{ // in View column - nothing actually deleted yet, merely created a Selection to delete with another press of Backspace
								const Utils::CVarTempReset<bool> md0( mouseDragged, true ); // adjust Selection by pretending mouse button being pressed
								caret.iViewHalfbyte=item.iFirstPlaceholder;
								SendMessage( WM_KEYDOWN, VK_LEFT ); // select current Item
								std::swap( caret.streamPosition, caret.streamSelectionA );
								goto caretRefresh;
							}
						// . moving the content "after" Selection "to" the position of the Selection
						//fallthrough
					case VK_DELETE:{
						// deleting the Byte after Caret, or deleting the Selection
editDelete:				if (!editable) return true; // can't edit content of a disabled window
						if (!f) return true; // there's nothing to do without an underlying Content
						// . if Selection not set, setting it as the Byte immediately after Caret
						const auto caret0=caret;
						if (!caret.SelectionExists())
							if (caret.IsInStream()){ // in Stream column
								if (caret.streamPosition>=f.GetLength()) return true; // can actually delete anything after the Caret?
								caret.streamSelectionA=caret.streamPosition++;
							}else{ // in View column - nothing actually deleted yet, merely created a Selection to delete with another press of Backspace
								const Utils::CVarTempReset<bool> md0( mouseDragged, true ); // adjust Selection by pretending mouse button being pressed
								caret.iViewHalfbyte=item.iLastPlaceholder;
								SendMessage( WM_KEYDOWN, VK_RIGHT ); // select current Item
								goto caretRefresh;
							}
						// . checking if there are any Bookmarks selected
						const auto selection=caret.GetSelectionAsc();
						auto posSrc=selection.z, posDst=selection.a;
						auto itBm=bookmarks.lower_bound(posDst);
						if (itBm!=bookmarks.end() && *itBm<posSrc){
							if (pOwner->ShowQuestionYesNo( QUERY_DELETE_SELECTED_BOOKMARKS, MB_DEFBUTTON2 )){
								bookmarks.erase( itBm, bookmarks.lower_bound(posSrc) );
								caret=caret0;
								RepaintData();
							}
							SetFocus();
							return true;
						}
						// . removing/trimming Emphases in Selection
						AddHighlight( selection, CLR_DEFAULT );
						// . moving the content "after" Selection "to" the position of the Selection
						caret.streamSelectionA = caret.streamPosition = posDst; // moving the Caret and cancelling any Selection
						auto nBytesToMove=f.GetLength()-posSrc;
						itBm=bookmarks.lower_bound(posSrc);
						auto itEmp=emphases.lower_bound( TEmphasis(posSrc,0) );
						for( BYTE buf[65536]; const auto nBytesRequested=std::min<TPosition>(nBytesToMove,sizeof(buf)); ){
							// : move Bytes
							f.Seek( posSrc );
							UINT nBytesBuffered=0;
							while (const auto nBytesRead=f.Read( buf+nBytesBuffered, nBytesRequested-nBytesBuffered, IgnoreIoResult ))
								nBytesBuffered+=nBytesRead;
							f.Seek( posDst );
							const auto nBytesWritten=f.Write( buf, nBytesBuffered, IgnoreIoResult );
							const auto nBytesSuccessfullyMoved=std::min( std::min<TPosition>(nBytesRequested,nBytesBuffered), nBytesWritten );
							// : move Bookmarks (WARNING: there may be Bookmarks in areas with no data, hence we BREAK below)
							const auto posDelta=posSrc-posDst;
							const TPosInterval movedRegion( posSrc, posSrc+nBytesSuccessfullyMoved );
							while (itBm!=bookmarks.end() && *itBm<movedRegion.z)
								const_cast<TPosition &>(*itBm++)-=posDelta; // this is nasty but does the job - direct modification of std::set
							// : move Emphases (WARNING: there may be Emphases in areas with no data, hence we BREAK below)
							if (movedRegion.Contains(itEmp->a)) // shall we attempt to merge this Emphasis with the previous one?
								if (itEmp!=emphases.begin()){ // is there actually any previous one?
									TEmphasis eTmp=*itEmp;
									eTmp.a-=posDelta; // imagine the beginning of the current Emphasis is already shifted
									auto itEmpPrev=itEmp;
									TEmphasis &ePrev=const_cast<TEmphasis &>(*--itEmpPrev); // this is nasty but does the job - direct modification of std::set
									if (ePrev.IsSameColorNext(eTmp)){
										ePrev.z=itEmp->z;
										emphases.erase(itEmp);
										itEmp=itEmpPrev;
									}
								}
							while (itEmp!=emphases.end()){
								TEmphasis &e=const_cast<TEmphasis &>(*itEmp); // this is nasty but does the job - direct modification of std::set
								if (movedRegion.Contains(e.a))
									e.a-=posDelta;
								if (e.z<=movedRegion.z){
									e.z-=posDelta;
									itEmp++;
								}else
									break;
							}
							// : seeing if we reached an area with no data, in which case we BREAK
							nBytesToMove-=nBytesSuccessfullyMoved, posSrc+=nBytesSuccessfullyMoved, posDst+=nBytesSuccessfullyMoved;
							if (nBytesSuccessfullyMoved!=nBytesRequested)
								break;
						}
						// . the "source-destination" difference filled up with zeros
						if (!nBytesToMove) // successfully moved all Bytes?
							posSrc = logicalSize = std::max( logicalSize+posDst-posSrc, logicalSizeLimits.a );
						if (posDst<posSrc){
							for( static constexpr BYTE Zero=0; posDst++<posSrc; f.Write(&Zero,1,IgnoreIoResult) );
							if (!nBytesToMove) // successfully moved all Bytes?
								ShowMessage( MSG_PADDED_TO_MINIMUM_SIZE );
						}
						// . refreshing the scrollbar
						f.SetLength( logicalSize );
						SetStreamLogicalSize( logicalSize );
						SendEditNotification( EN_CHANGE );
						RepaintData();
						goto caretCorrectlyMoveTo;
					}
					case VK_RETURN:
						// refocusing the window that has previously lost the focus in favor of this HexaEditor
						::SetFocus(hPreviouslyFocusedWnd);
						break;
					case VK_F5:
						// redrawing
						Invalidate();
						break;
					default:
						if (!editable) return true; // can't edit content of a disabled window
						if (ctrl) return true; // a shortcut other than Caret positioning
						if (!caret.IsInStream()){ // here View column; the Stream column handled in WM_CHAR
							// View modification
							if (wParam>='0' && wParam<='9')
								wParam-='0';
							else if (wParam>=VK_NUMPAD0 && wParam<=VK_NUMPAD9)
								wParam-=VK_NUMPAD0;
							else if (wParam>='A' && wParam<='F')
								wParam-='A'-10;
							else
								break;
							const auto iRow=__logicalPositionToRow__(caret.streamPosition);
							const auto nextRowStart=__firstByteInRowToLogicalPosition__(iRow+1);
							if (std::min(nextRowStart,f.GetLength())-caret.streamPosition<item.nStreamBytes) // Item under Caret incomplete?
								if (iRow<nLogicalRows) // incomplete Item in the middle of the File?
									break;
								else if (caret.streamPosition+item.nStreamBytes<=logicalSizeLimits.z){ // can append new Item to the end of File?
									const auto newLength=caret.streamPosition+item.nStreamBytes;
									f.SetLength( newLength );
									SetStreamLogicalSize( newLength );
								}else{
									ShowMessage(MSG_LIMIT_UPPER);
									break;
								}
							BYTE b=0;
							f.Seek( caret.streamPosition+item.GetByteIndex(caret.iViewHalfbyte) );
							f.Read( &b, sizeof(b), IgnoreIoResult );
							if (item.IsLowerHalfbyte(caret.iViewHalfbyte))
								b = b&0xf0 | (BYTE)wParam;
							else
								b = b&0xf | (BYTE)(wParam<<4);
							f.Seek( -1, STREAM_SEEK_CUR );
							f.Write( &b, sizeof(b), IgnoreIoResult );
							caret.CancelSelection(); // cancelling any Selection
							SetStreamLogicalSize(  std::max( logicalSize, f.GetLength() )  );
							SendEditNotification( EN_CHANGE );
							RepaintData();
							SendMessage( WM_KEYDOWN, VK_RIGHT ); // advance Caret
							caret.CancelSelection();
							return true;
						}
						break;
				}
				break;
			}
			case WM_CHAR:
				// character
				if (!editable) return true; // can't edit content of a disabled window
				if (!f) // there's nothing to do without an underlying Content
					return true;
				if (caret.IsInStream()) // here Stream mode; hexa-View mode handled in WM_KEYDOWN
					// Stream modification
					if (::GetAsyncKeyState(VK_CONTROL)>=0 && ::isprint(wParam)) // Ctrl not pressed, thus character printable
						if (caret.streamPosition+(TPosition)sizeof(BYTE)<logicalSizeLimits.z){
							f.Seek( caret.streamPosition );
							f.Write( &wParam, sizeof(BYTE), IgnoreIoResult );
							caret.streamSelectionA = caret.streamPosition+=sizeof(BYTE);
							SetStreamLogicalSize(  std::max( logicalSize, f.GetLength() )  );
							SendEditNotification( EN_CHANGE );
							RepaintData();
							goto caretRefresh;
						}else
							ShowMessage(MSG_LIMIT_UPPER);
				return true;
			case WM_CONTEXTMENU:{
				// context menu invocation
				if (IS_RECURRENT_USE)
					return true;
				contextMenu.UpdateUi( *this ); // default update ...
				contextMenu.UpdateUi( *pOwner ); // ... which the real Owner may override
				POINT pt=Utils::MakePoint(lParam);
				if (pt.x==-1){ // occurs if the context menu invoked using Shift+F10
					::GetCaretPos(&pt);
					::ClientToScreen(hWnd,&pt);
					pt.x+=font.GetCharAvgWidth(), pt.y+=font.GetCharHeight();
				}
				if (const WORD cmd=::TrackPopupMenu( contextMenu, TPM_RETURNCMD, pt.x,pt.y, 0, hWnd, nullptr ))
					SendMessage( WM_COMMAND, cmd );
				return true; // to suppress CEdit's standard context menu
			}
			case WM_COMMAND:
				// processing a command
				if (!f) // there's nothing to do without an underlying Content
					break;
				switch (LOWORD(wParam)){
					case ID_YAHEL_BOOKMARK_TOGGLE:
						// toggling a Bookmark at Caret's Position
						ToggleBookmarkAt(caret.streamPosition);
						RepaintData();
						goto caretRefresh;
					case ID_YAHEL_BOOKMARK_PREV:{
						// navigating the Caret to the previous Bookmark
						auto it=bookmarks.lower_bound(caret.streamPosition);
						if (it!=bookmarks.begin()){ // just to be sure
							caret.streamSelectionA = caret.streamPosition = *--it; // moving the Caret and cancelling any Selection
							RepaintData();
							goto caretCorrectlyMoveTo;
						}else
							break;
					}
					case ID_YAHEL_BOOKMARK_NEXT:{
						// navigating the Caret to the next Bookmark
						const auto it=bookmarks.lower_bound(
							caret.streamPosition
							+
							( caret.IsInStream() ? 1 : item.nStreamBytes )
						);
						if (it!=bookmarks.end()){ // just to be sure
							caret.streamSelectionA = caret.streamPosition = *it; // moving the Caret and cancelling any Selection
							RepaintData();
							goto caretCorrectlyMoveTo;
						}else
							break;
					}
					case ID_YAHEL_BOOKMARK_DELETEALL:
						// deleting all Bookmarks
						if (pOwner->ShowQuestionYesNo( QUERY_DELETE_ALL_BOOKMARKS, MB_DEFBUTTON2 )){
							RemoveAllBookmarks();
							RepaintData();
							goto caretRefresh;
						}else
							break;
					case ID_YAHEL_SELECT_ALL:{
						// Selecting everything
						caret.streamSelectionA=0, caret.streamPosition=f.GetLength();
						RepaintData();
						const Utils::CVarTempReset<bool> md0( mouseDragged, true ); // don't change Selection (if any) by pretending mouse button being pressed
						SendMessage( WM_KEYDOWN, VK_JUNJA ); // caretCorrectlyMoveTo
						return true;
					}
					case ID_YAHEL_SELECT_NONE:
						// removing current selection
						caret.CancelSelection(); // cancelling any Selection
						RepaintData();
						goto caretRefresh;
					case ID_YAHEL_SELECT_CURRENT:{
						// selecting the whole Record under the Caret
						TPosition recordLength=0;
						pStreamAdvisor->GetRecordInfo( caret.streamPosition, &caret.streamSelectionA, &recordLength, nullptr );
						caret.streamPosition=caret.streamSelectionA+recordLength;
						caret.iViewHalfbyte=item.iFirstPlaceholder;
						RepaintData();
						const Utils::CVarTempReset<bool> md0( mouseDragged, true ); // adjust Selection by pretending mouse button being pressed
						outResult=SendMessage( WM_KEYDOWN, VK_JUNJA ); // caretCorrectlyMoveTo
						return true;
					}
					case ID_YAHEL_FILE_SAVE_COPY_AS:{
						// saving Selection as
						WCHAR fileName[MAX_PATH];
						if (pOwner->ShowSaveFileDialog( nullptr, 0, ::lstrcpyW(fileName,L"selection.bin"), ARRAYSIZE(fileName) )){
							const HANDLE hDest=::CreateFileW( fileName, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0 );
							if (hDest!=INVALID_HANDLE_VALUE){
								const auto selection=caret.GetSelectionAsc();
								for( TPosition nBytesToSave=selection.z-f.Seek(selection.a),n; nBytesToSave; nBytesToSave-=n ){
									BYTE buf[65536];
									n=f.Read(  buf,  std::min<TPosition>( nBytesToSave, sizeof(buf) ),  IgnoreIoResult  );
									if (n==Stream::GetErrorPosition()){
										pOwner->ShowInformation( MSG_SELECTION_SAVED_PARTIALLY, ERROR_READ_FAULT );
										break;
									}
									DWORD nWritten;
									::WriteFile( hDest, buf, n, &nWritten, nullptr );
								}
								::CloseHandle(hDest);
							}else
								pOwner->ShowInformation( ERROR_SELECTION_CANT_SAVE, ::GetLastError() );
						}
						SetFocus(); // restoring focus lost by displaying the "Save as" dialog
						return true;
					}
					case ID_YAHEL_EDIT_PASTE_SPECIAL:{
						// pasting content of a file at current Caret Position
						if (!editable) return true; // can't edit content of a disabled window
						WCHAR fileName[MAX_PATH];
						if (pOwner->ShowOpenFileDialog( nullptr, OFN_FILEMUSTEXIST, ::lstrcpyW(fileName,L""), ARRAYSIZE(fileName) )){
							CComPtr<IStream> fSrc;
							fSrc.Attach( Stream::FromFileForSharedReading(fileName) );
							PasteStreamAtCaretAndShowError(fSrc);
						}
						SetFocus(); // restoring focus lost by displaying the "Open" dialog
						return true;
					}
					case ID_YAHEL_EDIT_RESET_ZERO:{
						// resetting Selection with zeros
						if (!editable) return true; // can't edit content of a disabled window
						i=0;
resetSelectionWithValue:BYTE buf[65535];
						if (i>=0) // some constant value
							::memset( buf, i, sizeof(buf) );
						const auto selection=caret.GetSelectionAsc();
						for( TPosition nBytesToReset=selection.z-f.Seek(selection.a),n; nBytesToReset; nBytesToReset-=n ){
							n=std::min<TPosition>( nBytesToReset, sizeof(buf) );
							if (i<0) // Gaussian noise
								Utils::RandomizeData( buf, n );
							if (!f.Write( buf, n, IgnoreIoResult )){
								pOwner->ShowInformation( MSG_SELECTION_RESET_PARTIALLY, ERROR_WRITE_FAULT );
								break;
							}
						}
						SendEditNotification( EN_CHANGE );
						RepaintData();
						goto caretCorrectlyMoveTo;
					}
					case ID_YAHEL_EDIT_RESET:{
						// resetting Selection with user-defined value
						if (!editable) return true; // can't edit content of a disabled window
						TResetSelectionParams rsp;
						if (pOwner->QueryByteToResetSelectionWith(rsp) && rsp.IsValid())
							switch (rsp.type){
								case TResetSelectionParams::Byte:
									i=rsp.byteValue;
									goto resetSelectionWithValue;
								case TResetSelectionParams::GaussianNoise:
									i=-1;
									goto resetSelectionWithValue;
								default:
									assert(false);
									break;
							}
						return true;
					}
					case ID_YAHEL_EDIT_COPY:
						// copying the Selection into clipboard
						if (!caret.SelectionExists())
							return true;
						if (const LPDATAOBJECT pdo=Stream::CreateDataObject( f, caret.GetSelectionAsc() )){
							::OleSetClipboard(pdo);
							if (editable) // if content Editable ...
								::OleFlushClipboard(); // ... render data immediately
							else
								delayedDataInClipboard=pdo;
							if (lParam) // want pointer to the data now placed into the clipboard?
								*((LPDATAOBJECT *)lParam)=pdo;
							else
								pdo->Release();
						}
						return true;
					case ID_YAHEL_EDIT_PASTE:{
						// pasting binary data from clipboard at the Position of Caret
						if (!editable) return true; // can't edit content of a disabled window
						CComPtr<IDataObject> pdo;
						if (FAILED(::OleGetClipboard(&pdo.p)))
							return true;
						CComPtr<IEnumFORMATETC> pefe;
						if (FAILED(pdo->EnumFormatEtc( DATADIR_GET, &pefe.p )))
							return true;
						CComPtr<IStream> ps;
						STGMEDIUM stg={};
						for( FORMATETC fmt; pefe->Next(1,&fmt,nullptr)==S_OK; )
							if (fmt.cfFormat==GetClipboardFormat())
								if (fmt.tymed==TYMED_HGLOBAL && SUCCEEDED(pdo->GetData(&fmt,&stg))){
									if (const PVOID pData=::GlobalLock(stg.hGlobal)){
										ps.Attach( Stream::FromBuffer(pData,::GlobalSize(stg.hGlobal)) );
										PasteStreamAtCaretAndShowError(ps);
										::GlobalUnlock(stg.hGlobal);
									}
									break;
								}
						return true;
						SendEditNotification( EN_CHANGE );
						RepaintData();
						goto caretCorrectlyMoveTo;
					}
					case ID_YAHEL_EDIT_DELETE:
						// deleting content of the current selection
						goto editDelete;
					case ID_YAHEL_FIND_NEXT:
						// find next occurence of Pattern in the Content
						if (!searchParams.IsUsable()) // no Pattern specified yet
							wParam=ID_YAHEL_FIND;
						//fallthrough
					case ID_YAHEL_FIND:{
						// find a Pattern in the Content
						// . ignoring disabled command (e.g. can't search within the "Find" dialog)
						if (IS_RECURRENT_USE)
							return true;
						// . requesting the Owner to begin/continue with a Search session
						if (LOWORD(wParam)==ID_YAHEL_FIND){
							TSearchParams tmp;
							if (pOwner->QueryNewSearchParams(tmp) && tmp.IsUsable())
								searchParams=tmp;
							else
								return true;
						}
						const auto posFound=pOwner->ContinueSearching(searchParams);
						if (posFound!=Stream::GetErrorPosition()){ // found a match?
							SetFocus();
							caret.streamPosition=( caret.streamSelectionA=posFound )+searchParams.patternLength;
							if (!caret.IsInStream()) // in View column
								caret.iViewHalfbyte-=item.patternLength; // switch to Stream column
							RepaintData();
							goto caretRefresh;
						}else
							return true;
					}
					case ID_YAHEL_GOTO_RECORD_NEXT:{
						// navigating to the next Record
						TPosition currRecordLength=0;
						pStreamAdvisor->GetRecordInfo( caret.streamPosition, &caret.streamPosition, &currRecordLength, nullptr );
						caret.streamPosition+=currRecordLength;
						goto caretCorrectlyMoveTo;
					}
					case ID_YAHEL_GOTO_RECORD_PREV:
						// navigating to the previous Record (or the beginning of current Record, if Caret not already there)
						pStreamAdvisor->GetRecordInfo( --caret.streamPosition, &caret.streamPosition, nullptr, nullptr );
						goto caretCorrectlyMoveTo;
					case ID_YAHEL_GOTO_ADDRESS:{
						// navigating to an address input by the user
						TGoToParams gtp( TPosInterval(0,f.GetLength()), caret.streamPosition );
						const bool valid=pOwner->QueryAddressToGoTo(gtp) && gtp.IsValid();
						::SetFocus(hWnd);
						if (valid){
							caret.streamPosition=gtp.address;
							goto caretCorrectlyMoveTo;
						}else
							return true;
					}
				}
				break;
			case WM_LBUTTONDOWN:
				// left mouse button pressed
				mouseDragged=false;
				goto leftMouseDragged; // "as if it's already been Dragged"
			case WM_RBUTTONDOWN:{
				// right mouse button pressed
				mouseDragged=false;
				const auto logPos=__logicalPositionFromPoint__( Utils::MakePoint(lParam), nullptr );
				if (logPos>=0){
					// in either View or Stream column
					if (caret.GetSelectionAsc().Contains(logPos))
						// right-clicked on the Selection
						break; // showing context menu at the place of Caret
					else
						// right-clicked outside the Selection - unselecting everything and moving the Caret
						goto leftMouseDragged; // "as if it's already been Dragged"
				}else
					// outside any interactive column - ignoring the right-click
					return true;
			}
			case WM_LBUTTONUP:
				// left mouse button released
				mouseDragged=false;
				break;
			case WM_MOUSEMOVE:{
				// mouse moved
				if (!( mouseDragged=::GetKeyState(VK_LBUTTON)<0 )) return true; // if mouse button not pressed, current Selection cannot be modified
				if (cursorPos0==lParam) return true; // actually no Cursor movement occured
leftMouseDragged:
				cursorPos0=lParam;
				const auto logPos=__logicalPositionFromPoint__( Utils::MakePoint(lParam), &caret.iViewHalfbyte );
				if (logPos>=0)
					// in either View or Stream column
					caret.streamPosition=logPos;
				else{
					// outside any interactive column
					if (!mouseDragged){ // if right now mouse button pressed ...
						caret.CancelSelection(); // ... cancelling any Selection ...
						RepaintData(); // ... and painting the result
					}
					break;
				}
				::CallWindowProc( EditClass.lpfnWndProc, hWnd, msg, wParam, lParam ); // to set focus and accept WM_KEY* messages
				ShowCaret();
				goto caretCorrectlyMoveTo;
			}
			case WM_LBUTTONDBLCLK:{
				// left mouse button double-clicked
				const auto logPos=__logicalPositionFromPoint__( Utils::MakePoint(lParam), &caret.iViewHalfbyte );
				if (logPos>=0){
					// in either View or Stream columns
					caret.streamPosition=logPos;
					outResult=SendMessage( WM_COMMAND, ID_YAHEL_SELECT_CURRENT );
					return true;
				}
				break;
			}
			case WM_SETFOCUS:
				// window has received focus
				hPreviouslyFocusedWnd=(HWND)wParam; // the window that is losing the focus (may be refocused later when Enter is pressed)
				SendEditNotification( EN_SETFOCUS );
				ShowCaret();
				goto caretRefresh;
			case WM_KILLFOCUS:
				// window has lost focus
		{		EXCLUSIVELY_LOCK_THIS_OBJECT();
				mouseInNcArea=false;
		}		::DestroyCaret();
				if (const HWND hParent=::GetParent(hWnd))
					::InvalidateRect( hParent, nullptr, FALSE );
				hPreviouslyFocusedWnd=0;
				break;
			case WM_MOUSEWHEEL:{
				// mouse wheel was rotated
				int nLinesToScroll=1;
				::SystemParametersInfo( SPI_GETWHEELSCROLLLINES, 0, &nLinesToScroll, 0 );
				const short zDelta=(short)HIWORD(wParam);
				if (nLinesToScroll==WHEEL_PAGESCROLL)
					SendMessage( WM_VSCROLL, zDelta>0?SB_PAGEUP:SB_PAGEDOWN, 0 );
				else
					__scrollToRow__( GetVertScrollPos()-zDelta*nLinesToScroll/WHEEL_DELTA );
				return true;
			}
			/*case WM_HSCROLL:{
				// scrolling horizontally
				// . determining the char to scroll to
				SCROLLINFO si={ sizeof(si) };
				GetScrollInfo( SB_HORZ, &si, SIF_POS|SIF_RANGE|SIF_TRACKPOS ); // getting 32-bit position
				int c=si.nPos;
				switch (LOWORD(wParam)){
					case SB_PAGEUP:		// clicked into the gap above "thumb"
						c-=nRowsOnPage;	break;
					case SB_PAGEDOWN:	// clicked into the gap below "thumb"
						c+=nRowsOnPage; break;
					case SB_LINEUP:		// clicked on arrow up
						c--; break;
					case SB_LINEDOWN:	// clicked on arrow down
						c++; break;
					case SB_THUMBPOSITION: // "thumb" released
						break;
					case SB_THUMBTRACK:	// "thumb" dragged
						c = si.nTrackPos;	break;
				}
				// . redrawing HexaEditor's client and non-client areas
				RepaintData();
				break;
			}*/
			case WM_VSCROLL:{
				// scrolling vertically
				// . determining the Row to scroll to
				SCROLLINFO si={ sizeof(si), SIF_POS|SIF_RANGE|SIF_TRACKPOS };
				::GetScrollInfo( hWnd, SB_VERT, &si ); // getting 32-bit position
				TRow row=si.nPos;
				switch (LOWORD(wParam)){
					case SB_PAGEUP:		// clicked into the gap above "thumb"
						row-=nRowsOnPage;	break;
					case SB_PAGEDOWN:	// clicked into the gap below "thumb"
						row+=nRowsOnPage; break;
					case SB_LINEUP:		// clicked on arrow up
						row--; break;
					case SB_LINEDOWN:	// clicked on arrow down
						row++; break;
					case SB_THUMBPOSITION: // "thumb" released
						break;
					case SB_THUMBTRACK:	// "thumb" dragged
						row = si.nTrackPos;	break;
				}
				// . redrawing HexaEditor's client and non-client areas
				__scrollToRow__(row);
				//fallthrough (the "thumb" might have been released outside the scrollbar area)
			}
			case WM_NCMOUSEMOVE:{
				// mouse moved in non-client area
		{		EXCLUSIVELY_LOCK_THIS_OBJECT();
				mouseInNcArea=true;
		}		TRACKMOUSEEVENT tme={ sizeof(tme), TME_NONCLIENT|TME_LEAVE, hWnd, 0 };
				::TrackMouseEvent(&tme);
				break;
			}
			case WM_NCMOUSELEAVE:{
				// mouse left non-client area
				EXCLUSIVELY_LOCK_THIS_OBJECT();
				if (*static_cast<TState *>(this)!=update){
					*static_cast<TState *>(this)=update;
					RepaintData();
				}
				mouseInNcArea=false;
				break;
			}
			case WM_SIZE:
				// window size has changed
				item.nInRow=item.nInRowLimits.Clip(
					(	std::min( LOWORD(lParam)/font.GetCharAvgWidth(), STREAM_BYTES_IN_ROW_MAX )
						-
						addrLength - ADDRESS_SPACE_LENGTH
						-
						IsColumnShown(TColumn::VIEW)*VIEW_SPACE_LENGTH
					)
					/
					( IsColumnShown(TColumn::VIEW)*item.patternLength + IsColumnShown(TColumn::STREAM)*item.nStreamBytes )
				);
				__refreshVertically__(); // to guarantee that the actual data is always drawn
				SendMessage( WM_KEYDOWN, VK_KANJI ); // scroll to refreshed Caret
				break;
			case WM_ERASEBKGND:
				// drawing the background
				outResult=TRUE;
				return true; // nop (always painting over existing Content)
			case WM_PAINT:{
				// drawing
				if (!f) // there's nothing to paint without an underlying Content
					return false;
				class CHexaPaintDC sealed{
					const CInstance &he;
					const HDC handle;
					PAINTSTRUCT ps;
				public:
					const COLORREF SelectionColor;
					const COLORREF LabelColor;
					const Utils::CYahelPen RecordDelimitingHairline;
					const Utils::CYahelBrush BtnFaceBrush;
				private:
					const HGDIOBJ hFont0, hPen0;
					BYTE currContentFlags;
					COLORREF currEmphasisColor;
					WCHAR charBuffer[4096];
					WORD nCharsBuffered;
					LPRECT pRect;
				public:
					enum TContentFlags:BYTE{
						Normal		=0,
						Selected	=1,
						Erroneous	=2,
						Unknown		=4 // = currently fetching data
					};

					CHexaPaintDC(CInstance &hexaEditor)
						: he(hexaEditor)
						, handle( ::BeginPaint(he.hWnd,&ps) )
						, hFont0( ::SelectObject(*this,he.font) )
						, hPen0( ::SelectObject(*this,RecordDelimitingHairline) )
						, SelectionColor( ::GetSysColor(COLOR_HIGHLIGHT) )
						, LabelColor( Utils::GetSaturatedColor(::GetSysColor(COLOR_GRAYTEXT),1.7f) )
						, RecordDelimitingHairline( 0, LabelColor )
						, BtnFaceBrush( COLOR_BTNFACE, true )
						, currContentFlags(-1) , currEmphasisColor(::GetBkColor(handle)) , nCharsBuffered(0) , pRect(nullptr) {
					}

					~CHexaPaintDC(){
						assert( !nCharsBuffered ); // call FlushPrintBuffer first!
						::SelectObject( *this, hFont0 );
						::SelectObject( *this, hPen0 );
						::EndPaint( he.hWnd, &ps );
					}

					inline operator HDC() const{ return handle; }
					inline const RECT GetPaintRect() const{ return ps.rcPaint; }

					void SetPrintRect(RECT &rc){
						assert( !nCharsBuffered ); // call FlushPrintBuffer first!
						pRect=&rc;
					}

					void SetContentPrintState(BYTE newContentFlags,COLORREF newEmphasisColor){
						if (newContentFlags==currContentFlags && newEmphasisColor==currEmphasisColor) // no change in settings?
							return;
						FlushPrintBuffer();
						if (newContentFlags&Erroneous){
							// Erroneous content
							// : TextColor is (some tone of) Red
							if (!(currContentFlags&Erroneous)) // "if previously not Erroneous"
								::SetTextColor( handle, !he.editable*0x777700+COLOR_RED );
							// : BackgroundColor is the EmphasisColor overlayed with SelectionColor
blendEmphasisAndSelection:	if (newEmphasisColor!=currEmphasisColor || newContentFlags!=currContentFlags) // "if EmphasisColor or the application of SelectionColor have changed"
								::SetBkColor( handle,
									newContentFlags&Selected
									? Utils::GetBlendedColor(newEmphasisColor,SelectionColor,.6f) // need to overlay EmphasisColor with SelectionColor
									: newEmphasisColor // EmphasisColor only
								);
						}else if (newContentFlags&Unknown){
							// content not yet known (e.g. floppy drive head is currently seeking to requested cylinder, etc.)
							// : TextColor is (some tone of) Yellow
							if (!(currContentFlags&Unknown)) // "if previously not Unknown"
								::SetTextColor( handle, !he.editable*0x1122+0x66cc99 );
							// : BackgroundColor is the EmphasisColor
							goto blendEmphasisAndSelection;
						}else if (!newContentFlags){
							// Normal (not Selected) content
							// : TextColor is (some tone of) Black
							if (currContentFlags) // "if previously not Normal"
								::SetTextColor( handle, ::GetSysColor(he.editable?COLOR_WINDOWTEXT:COLOR_GRAYTEXT) );
							// : BackgroundColor is the EmphasisColor
							goto blendEmphasisAndSelection;
						}else
							// Selected content
							if (newEmphasisColor!=currEmphasisColor || newContentFlags!=currContentFlags){ // "if EmphasisColor or the application of SelectionColor have changed"
								// : TextColor is either Black or White, whichever is in higher contrast to CurrentBackgroundColor
								const COLORREF bgColor=Utils::GetBlendedColor(newEmphasisColor,SelectionColor,.6f); // need to overlay EmphasisColor with SelectionColor
								const WORD rgbSum = *(PCBYTE)&bgColor + ((PCBYTE)&bgColor)[1] + ((PCBYTE)&bgColor)[2];
								::SetTextColor( handle, rgbSum>0x180 ? COLOR_BLACK : COLOR_WHITE );
								// : BackgroundColor is the EmphasisColor overlayed with SelectionColor
								::SetBkColor( handle, bgColor );
							}
						currContentFlags=newContentFlags, currEmphasisColor=newEmphasisColor;
					}

					void FlushPrintBuffer(){
						if (nCharsBuffered>0){ // anything to flush?
							assert( pRect!=nullptr ); // call SetPrintRect first!
							::DrawTextW( *this, charBuffer, nCharsBuffered, pRect, DT_LEFT|DT_TOP|DT_NOPREFIX );
							pRect->left+=nCharsBuffered*he.font.GetCharAvgWidth();
							nCharsBuffered=0;
						}
					}

					inline void PrintChar(WCHAR c){
						assert( pRect!=nullptr ); // call SetPrintRect first!
						charBuffer[nCharsBuffered++]=c;
					}

					void PrintChars(LPCWSTR c,int n){
						assert( pRect!=nullptr ); // call SetPrintRect first!
						::memcpy( charBuffer+nCharsBuffered, c, sizeof(WCHAR)*n );
						nCharsBuffered+=n;
					}

					void PrintBkSpace(int n){
						assert( pRect!=nullptr ); // call SetPrintRect first!
						if (currEmphasisColor!=COLOR_WHITE || (currContentFlags&Selected)!=0) // front color is irrelevant, what counts is only the background
							SetContentPrintState( currContentFlags&~Selected, COLOR_WHITE );
						if (!n)
							n=(GetPaintRect().right-pRect->left)/he.font.GetCharAvgWidth()+1 - nCharsBuffered;
						while (n-->0)
							PrintChar(' ');
					}
				} dc(*this);
				// . drawing Header
				const char patternLength=std::max( ITEM_PATTERN_LENGTH_MIN, item.patternLength );
				const int itemWidth=patternLength*font.GetCharAvgWidth();
				const auto charLayout=GetCharLayout();
				RECT rcClip=GetClientRect();
					rcClip.top=dc.GetPaintRect().top, rcClip.bottom=dc.GetPaintRect().bottom;
				if (rcClip.top<HEADER_HEIGHT){ // Header drawn only if its region invalid
					RECT rcHeader={ 0, 0, rcClip.right, HEADER_HEIGHT };
					::FillRect( dc, &rcHeader, dc.BtnFaceBrush );
					if (IsColumnShown(TColumn::VIEW)){
						TCHAR buf[16];
						dc.SetContentPrintState( CHexaPaintDC::Normal, ::GetSysColor(COLOR_BTNFACE) );
						rcHeader.left=charLayout.view.a*font.GetCharAvgWidth();
						for( WORD n=0; n<item.nInRow&&rcHeader.left<rcHeader.right; rcHeader.left+=itemWidth )
							::DrawText( dc, buf, ::wsprintf(buf,_T("%02X"),n++*item.nStreamBytes), &rcHeader, DT_LEFT|DT_TOP );
					}
				}
				// . determining the visible part of the File content
				const TRow iRowFirstToPaint=std::max( (std::max(rcClip.top,dc.GetPaintRect().top)-HEADER_HEIGHT)/font.GetCharHeight(), 0L );
				TRow iRowA= GetVertScrollPos() + iRowFirstToPaint;
				const TRow iRowLastToPaint= GetVertScrollPos() + std::min(rcClip.bottom,dc.GetPaintRect().bottom)/font.GetCharHeight() + 1;
				const TRow iRowZ=std::min( std::min(nLogicalRows,iRowLastToPaint), iRowA+nRowsOnPage );
				// . drawing Addresses and data (both View and Stream parts)
				auto address=__firstByteInRowToLogicalPosition__(iRowA);
				auto y=HEADER_HEIGHT+iRowFirstToPaint*font.GetCharHeight();
				const auto selection=caret.GetSelectionAsc();
				auto itEmp=emphases.lower_bound( TEmphasis(address,0) );
				if (itEmp!=emphases.begin() && address<itEmp->a) // potentially skipped a relevant Emphasis? (e.g. at Address 48 we have skipped the Emphasis <42;70> by discovering the next Emphasis <72;80>)
					itEmp--;
				f.Seek( address );
				for( WCHAR buf[16]; iRowA<=iRowZ; iRowA++,y+=font.GetCharHeight() ){
					RECT rcRow={ 0, y, rcClip.right, y+font.GetCharHeight() };
					while (itEmp->z<address) itEmp++; // choosing the first visible Emphasis
					// : address
					dc.SetPrintRect(rcRow);
					if (addrLength){
						dc.SetContentPrintState( CHexaPaintDC::Normal, ::GetSysColor(COLOR_BTNFACE) );
						dc.PrintChars(
							buf,
							::wsprintfW( buf, ADDRESS_FORMAT, HIWORD(address), LOWORD(address) )
						);
					}
					dc.PrintBkSpace( ADDRESS_SPACE_LENGTH );
					// : File content
					const bool isEof=f.GetPosition()==f.GetLength();
					auto nBytesExpected=std::min( __firstByteInRowToLogicalPosition__(iRowA+1), f.GetLength() )-address;
					bool dataReady=false; // assumption
					pStreamAdvisor->GetRecordInfo( address, nullptr, nullptr, &dataReady );
					if (dataReady){
						// Record's data are known (there are some, some with error, or none)
						BYTE bytes[STREAM_BYTES_IN_ROW_MAX]; TPosition nBytesRead=0;
						enum:BYTE{ Good, Bad, Fuzzy } byteStates[STREAM_BYTES_IN_ROW_MAX];
						while (const auto nMissing=nBytesExpected-nBytesRead){
							HRESULT hIoResult;
							const auto nNewBytesRead=f.Read( bytes+nBytesRead, nMissing, hIoResult );
							if (FAILED(hIoResult)) // no Bytes are available
								break;
							if (nNewBytesRead>0){ // some more data read - Good or Bad
								::memset( byteStates+nBytesRead, hIoResult!=S_OK, nNewBytesRead );
								nBytesRead+=nNewBytesRead;
							}else if (f.GetPosition()<f.GetLength()){ // no data read - probably because none could have been determined (e.g. fuzzy bits)
								byteStates[nBytesRead++]=Fuzzy;
								f.Seek( 1, STREAM_SEEK_CUR );
							}else{
								nBytesExpected=nBytesRead;
								break;
							}
						}
						if (nBytesRead==nBytesExpected){
							// entire Row available
							static constexpr WCHAR NonprintableChar=L'\x2219'; // if original character not really printable, displaying a substitute one
							static constexpr WCHAR FuzzyChar=L'\x2592';
							const auto itNearestBookmark=bookmarks.lower_bound(address);
							// | View column
							if (IsColumnShown(TColumn::VIEW)){
								auto itEmpView=itEmp;
								auto itNearestBm=itNearestBookmark;
								auto aView=address; auto aNearestBm=itNearestBm!=bookmarks.end()?*itNearestBm:Stream::GetErrorPosition();
								const auto d=div( nBytesRead, (TPosition)item.nStreamBytes );
								const bool readIncompleteItem=d.rem!=0;
								const WORD nCompleteItems=std::min<WORD>( item.nInRow, d.quot );
								for( WORD n=0; n<nCompleteItems+readIncompleteItem; n++ ){
									const BYTE nStreamBytes=readIncompleteItem && n==nCompleteItems // drawing an incomplete last Item in the Row?
															? d.rem
															: item.nStreamBytes;
									const BYTE printFlags =	selection.Contains( aView, nStreamBytes )
															? CHexaPaintDC::Selected
															: CHexaPaintDC::Normal;
									for( char i=0; i<item.patternLength; i++ ){
										COLORREF emphasisColor=	itEmpView->Contains( aView, nStreamBytes ) // whole Item contained in a single Emphasis?
																? itEmpView->color
																: COLOR_WHITE; // assumption (no Emphasis)
										WCHAR c=item.GetPrintableChar(i);
										if (nStreamBytes!=item.nStreamBytes) // drawing an incomplete last Item in the Row?
											c=L'\x2026', i=CHAR_MAX-1; // when the ellipsis printed, terminate this cycle
										if (c){
											dc.SetContentPrintState( printFlags, emphasisColor );
											dc.PrintChar(c);
										}else{
											if (emphasisColor==COLOR_WHITE){
												const auto a=aView+item.GetByteIndex(i);
												for( auto it=itEmpView; it->a<=a; it++ )
													if (it->Contains(a)){
														emphasisColor=it->color;
														break;
													}
											}
											const BYTE iByte=n*nStreamBytes+item.GetByteIndex(i);
											if (byteStates[iByte]==Good)
												dc.SetContentPrintState( printFlags, emphasisColor );
											else
												dc.SetContentPrintState( printFlags|CHexaPaintDC::Erroneous, emphasisColor );
											if (byteStates[iByte]!=Fuzzy){
												::wsprintfW( buf, VIEW_HALFBYTE_FORMAT, item.IsLowerHalfbyte(i)?bytes[iByte]&0xf:bytes[iByte]>>4 );
												dc.PrintChar(*buf);
											}else
												dc.PrintChar(FuzzyChar);
										}
										while (itEmpView->z<aView)
											itEmpView++;
									}
									aView+=nStreamBytes;
									if (aNearestBm<aView){
										dc.FlushPrintBuffer(); // to print the Bookmark over the File content and update the rcView!
										const LONG x=charLayout.view.a*font.GetCharAvgWidth()+n*itemWidth;
										const RECT rcBookmark={ x, y, x+itemWidth, y+font.GetCharHeight() };
										::FrameRect( dc, &rcBookmark, Utils::CYahelBrush::Black );
										aNearestBm=	( itNearestBm=bookmarks.lower_bound(aView) )!=bookmarks.end()
													? *itNearestBm
													: Stream::GetErrorPosition();
									}
								}
								dc.PrintBkSpace(
									charLayout.view.GetLength()-nCompleteItems*item.patternLength-readIncompleteItem // blank space caused by lack of Items
									+
									VIEW_SPACE_LENGTH
								);
							}
							// | Stream column
							if (IsColumnShown(TColumn::STREAM)){
								auto itEmpStream=itEmp;
								auto itNearestBm=itNearestBookmark;
								auto aStream=address; auto aNearestBm=itNearestBm!=bookmarks.end()?*itNearestBm:Stream::GetErrorPosition();
								for( WORD i=0; i<nBytesRead; i++ ){
									// > choose colors
									if (aStream==itEmpStream->z)
										itEmpStream++;
									const COLORREF emphasisColor= itEmpStream->Contains(aStream) ? itEmpStream->color : COLOR_WHITE;
									BYTE printFlags= byteStates[i]==Good ? CHexaPaintDC::Normal : CHexaPaintDC::Erroneous;
									if (selection.Contains(aStream))
										printFlags|=CHexaPaintDC::Selected;
									dc.SetContentPrintState( printFlags, emphasisColor );
									// > print
									if (byteStates[i]!=Fuzzy){
										const WCHAR wByte=bytes[i];
										dc.PrintChar( ::isprint(wByte) ? wByte : NonprintableChar ); // if original character not printable, displaying a substitute one
									}else
										dc.PrintChar(FuzzyChar);
									if (aStream++==aNearestBm){
										dc.FlushPrintBuffer(); // to print the Bookmark over the File content and update the rcStream!
										const RECT rcBookmark={ rcRow.left-font.GetCharAvgWidth(), y, rcRow.left, y+font.GetCharHeight() };
										::FrameRect( dc, &rcBookmark, Utils::CYahelBrush::Black );
										aNearestBm= ++itNearestBm!=bookmarks.end() ? *itNearestBm : Stream::GetErrorPosition();
									}
								}
								dc.PrintBkSpace( charLayout.stream.GetLength()-nBytesRead );
							}
							address+=nBytesRead;
						}else if (!isEof){
							// content not available (e.g. irrecoverable Sector read error)
							f.Seek( address+=nBytesExpected );
							#define ERR_MSG	L"» No data «"
							dc.SetContentPrintState( CHexaPaintDC::Erroneous, COLOR_WHITE );
							dc.PrintChars( ERR_MSG, ARRAYSIZE(ERR_MSG)-1 );
						}
					}else if (!isEof){
						// Record's data are not yet known - caller will refresh the HexaEditor when data for this Record are known
						f.Seek( address+=nBytesExpected );
						#define STATUS_MSG	L"» Fetching data ... «"
						dc.SetContentPrintState( CHexaPaintDC::Unknown, COLOR_WHITE );
						dc.PrintChars( STATUS_MSG, ARRAYSIZE(STATUS_MSG)-1 );
					}
					// : filling the rest of the Row with background color (e.g. the last Row in a Record may not span up to the end)
					dc.PrintBkSpace(0);
					dc.FlushPrintBuffer();
					// : drawing the Record label if the just drawn Row is the Record's first Row
					if (!isEof && IsColumnShown(TColumn::LABEL)){ // yes, a new Record can potentially start at the Row
						WCHAR buf[80];
						if (const LPCWSTR recordLabel=pStreamAdvisor->GetRecordLabelW( __firstByteInRowToLogicalPosition__(iRowA), buf, ARRAYSIZE(buf), param )){
							RECT rc={ (charLayout.stream.z+2)*font.GetCharAvgWidth(), y, rcClip.right, rcClip.bottom };
							const COLORREF textColor0=::SetTextColor(dc,dc.LabelColor), bgColor0=::SetBkColor(dc,COLOR_WHITE);
								::DrawTextW( dc, recordLabel, -1, &rc, DT_LEFT|DT_TOP );
								::MoveToEx( dc, addrLength*font.GetCharAvgWidth(), y, nullptr );
								::LineTo( dc, rcClip.right, y );
							::SetTextColor(dc,textColor0), ::SetBkColor(dc,bgColor0);
						}
					}
				}
				// . filling the rest of HexaEditor with background color
				rcClip.top=y;
				::FillRect( dc, &rcClip, Utils::CYahelBrush::White );
				return true;
			}
			case WM_HEXA_PAINTSCROLLBARS:{
				// repainting the scrollbars
				// . vertical scrollbar
				EXCLUSIVELY_LOCK_THIS_OBJECT();
				SCROLLINFO si={ sizeof(si), SIF_RANGE|SIF_PAGE, 0,nLogicalRows-1, nRowsOnPage };
				::SetScrollInfo( hWnd, SB_VERT, &si, TRUE );
				if (mouseInNcArea){
					const BOOL vertScrollbarNecessary=nRowsOnPage<nLogicalRows;
					::ShowScrollBar( hWnd, SB_VERT, vertScrollbarNecessary );
				}
				// . horizontal scrollbar
			/*	CRect rc;
				GetClientRect(&rc);
				const LONG nCharsInRow=GetLayout().stream.z/font.GetCharAvgWidth();
				const int nCharsInRowDisplayed=std::max( 0, rc.Width()/font.GetCharAvgWidth() );
				si.nMax=nCharsInRow-1, si.nPage=nCharsInRowDisplayed-1;
				SetScrollInfo( SB_HORZ, &si, TRUE );
				if (mouseInNcArea){
					const BOOL horzScrollbarNecessary=si.nPage<nCharsInRow;
					ShowScrollBar( SB_HORZ, horzScrollbarNecessary );
				}*/
				return true;
			}
			case WM_DESTROY:
				// window destroyed
				logPosScrolledTo=GetVisiblePart().a;
				Detach();
				break;
		}
		return false;
	}

}
