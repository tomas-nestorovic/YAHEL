#include "stdafx.h"
#include "Instance.h"

namespace Yahel{

	TGoToParams::TGoToParams(const TPosInterval &addressRange,TPosition caretPos)
		// ctor
		: addressRange(addressRange) , address(caretPos) {
	}

	bool TGoToParams::IsValid() const{
		return addressRange.Contains(address) || address==addressRange.z; // A|B, A = can navigate inside a Stream, B = can go to the end of Stream
	}

	bool TGoToParams::EditModalWithDefaultEnglishDialog(HWND hParent){
		//
		// - can't edit invalid Params
		if (!IsValid())
			return false;
		// - showing the Dialog and processing its result
		return Gui::QuerySingleIntA(
			"Go to", "&Address", addressRange, address, false, Gui::Hexa, hParent
		);
	}

}
