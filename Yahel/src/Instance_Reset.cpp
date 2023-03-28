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
		Utils::CSingleNumberDialog d( _T("Reset selection"), _T("&Value"), TPosInterval(0,256), byteValue, true );
		if (d.DoModal(hParent)==IDOK){
			byteValue=d.Value;
			return true;
		}else
			return false;
	}

}
