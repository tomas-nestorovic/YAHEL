#include "stdafx.h"
#include "Instance.h"

namespace Yahel{

	TResetSelectionParams::TResetSelectionParams()
		// ctor
		: type(VALUE) , value(0) {
	}

	bool TResetSelectionParams::IsValid() const{
		return type<LAST;
	}

	bool TResetSelectionParams::EditModalWithDefaultEnglishDialog(HWND hParent){
		//
		// - can't edit invalid Params
		if (!IsValid())
			return false;
		// - showing the Dialog and processing its result
		return Gui::QuerySingleIntA(
			"Reset selection", "&Value", Byte, value, Gui::Hexa, hParent
		);
	}

}
