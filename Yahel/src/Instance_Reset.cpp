#include "stdafx.h"
#include "Instance.h"

namespace Yahel{

	TResetSelectionParams::TResetSelectionParams()
		// ctor
		: type(Byte) , byteValue(0) {
	}

	bool TResetSelectionParams::IsValid() const{
		return type<=GaussianNoise;
	}

	bool TResetSelectionParams::EditModalWithDefaultEnglishDialog(HWND hParent){
		//
		// - can't edit invalid Params
		if (!IsValid())
			return false;
		// - showing the Dialog and processing its result
		TPosition tmp=byteValue;
		if (Gui::QuerySingleIntA( "Reset selection", "&Value", TPosInterval(0,UCHAR_MAX), tmp, Gui::Hexa, hParent )){
			byteValue=tmp;
			return true;
		}
		return false;
	}

}
