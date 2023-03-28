#include "stdafx.h"
#include "Instance.h"

namespace Yahel{

	TGoToParams::TGoToParams(const TPosInterval &addressRange,TPosition caretPos)
		// ctor
		: addressRange(addressRange) , address(caretPos) {
	}

	bool TGoToParams::IsValid() const{
		return addressRange.Contains(address);
	}

	bool TGoToParams::EditModalWithDefaultEnglishDialog(HWND hParent){
		//
		// - can't edit invalid Params
		if (!IsValid())
			return false;
		// - showing the Dialog and processing its result
		Utils::CSingleNumberDialog d( _T("Go to"), _T("&Address"), addressRange, address, true );
		if (d.DoModal(hParent)==IDOK){
			address=d.Value;
			return true;
		}else
			return false;
	}

}
