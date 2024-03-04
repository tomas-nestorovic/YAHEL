#include "stdafx.h"
#include "Instance.h"

namespace Yahel{

	TChecksumParams::TChecksumParams()
		// ctor
		: type(Add) , initValue(GetDefaultInitValue()) , range(0) {
	}

	bool TChecksumParams::IsValid() const{
		return	type<Last
				&&
				initValue!=GetErrorChecksumValue()
				&&
				range.IsValidNonempty();
	}

	int TChecksumParams::GetDefaultInitValue(){
		return 0;
	}

	int TChecksumParams::GetErrorChecksumValue(){
		return -1;
	}

	bool TChecksumParams::EditModalWithDefaultEnglishDialog(HWND hParent){
		//
		// - can't edit invalid Params
		if (!IsValid())
			return false;
		// - showing the Dialog and processing its result
		Utils::CSingleNumberDialog d( _T("Checksum"), _T("&Initial value"), TPosInterval(0,INT_MAX), initValue, true );
		if (d.DoModal(hParent)==IDOK){
			initValue=d.Value;
			return true;
		}else
			return false;
	}






	typedef int (* FnAppender)(int seed,BYTE b);

	static int Add(int seed,BYTE b){
		return seed+b;
	}

	static int Xor(int seed,BYTE b){
		return seed^b;
	}

	static int Crc16Ccitt(int seed,BYTE b){
		WORD tmp= (LOBYTE(seed)<<8) + HIBYTE(seed); // big endian
			BYTE x = tmp>>8 ^ b;
			x ^= x>>4;
			tmp = (tmp<<8) ^ (WORD)(x<<12) ^ (WORD)(x<<5) ^ (WORD)x;
		return (LOBYTE(tmp)<<8) + HIBYTE(tmp); // little endian
	}

	int CInstance::GetChecksum(const TChecksumParams &params,volatile const bool &cancel) const{
		//
		// - can't compute Checksum by invalid Params
		if (!params.IsValid())
			return TChecksumParams::GetErrorChecksumValue();
		// - determine computation function
		FnAppender fn;
		switch (params.type){
			case TChecksumParams::Add:
				fn=Add;
				break;
			case TChecksumParams::Xor:
				fn=Xor;
				break;
			case TChecksumParams::Ccitt16:
				fn=Crc16Ccitt;
				break;
			default:
				assert(false);
				return TChecksumParams::GetErrorChecksumValue();
		}
		// - computation
		struct TContent sealed:public CInstance::TContent{
			TPosition posOrg;

			TContent()
				: posOrg(Stream::GetErrorPosition()) {
			}
			~TContent(){
				if (posOrg!=Stream::GetErrorPosition())
					Seek(posOrg);
			}
		} f;
		const HRESULT hr=this->f.stream->Clone( &f.stream.p );
		if (hr==E_NOTIMPL){ // have to reuse existing Stream?
			assert(false); // using here the same Stream always requires attention; YAHEL is fine with that - is also the client app fine with that?
			f.stream=this->f.stream, f.posOrg=f.GetPosition();
		}else if (FAILED(hr))
			return TChecksumParams::GetErrorChecksumValue();
		auto result=params.initValue;
		f.Seek( params.range.a );
		for( BYTE b; f.GetPosition()<params.range.z; )
			if (cancel)
				return TChecksumParams::GetErrorChecksumValue();
			else if (f.Read( &b, 1, IgnoreIoResult ))
				result=fn( result, b );
			else
				f.Seek( 1, STREAM_SEEK_CUR ); // skipping irrecoverable portion of data
		return result;
	}

}
