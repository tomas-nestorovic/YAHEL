#include "stdafx.h"
#include "Instance.h"

namespace Yahel{
namespace Checksum{

	TParams::TParams()
		// ctor
		: type(Add) , initValue(GetDefaultInitValue()) {
	}

	TParams::TParams(TType type,int initValue)
		// ctor
		: type(type) , initValue(initValue) {
	}

	bool TParams::IsValid() const{
		return	type<Last
				&&
				initValue!=GetErrorValue();
	}

	int GetDefaultInitValue(){
		return 0;
	}

	int GetErrorValue(){
		return -1;
	}

	bool TParams::EditModalWithDefaultEnglishDialog(HWND hParent){
		//
		// - can't edit invalid Params
		if (!IsValid())
			return false;
		// - showing the Dialog and processing its result
		return Gui::QuerySingleIntA(
			"Checksum", "&Initial value", TPosInterval(0,UINT_MAX), initValue, Gui::Hexa, hParent, Seeds
		);
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

	int Compute(const TParams &params,LPCVOID bytes,UINT nBytes){
		//
		// - can't compute Checksum by invalid Params
		if (!params.IsValid())
			return GetErrorValue();
		// - determine computation function
		FnAppender fn;
		switch (params.type){
			case TParams::Add:
				fn=Add;
				break;
			case TParams::Xor:
				fn=Xor;
				break;
			case TParams::Ccitt16:
				fn=Crc16Ccitt;
				break;
			default:
				assert(false);
				return GetErrorValue();
		}
		// - computation
		auto result=params.initValue;
		for( const BYTE *pb=(LPBYTE)bytes; nBytes-->0; result=fn(result,*pb++) );
		return result;
	}

	BYTE ComputeXor(LPCVOID bytes,UINT nBytes,BYTE initValue){
		return Compute( TParams(TParams::Xor,initValue), bytes, nBytes );
	}

	int ComputeAdd(LPCVOID bytes,UINT nBytes,int initValue){
		return Compute( TParams(TParams::Add,initValue), bytes, nBytes );
	}
}





	int CInstance::GetChecksum(const Checksum::TParams &params,const TPosInterval &range,volatile const bool &cancel) const{
		//
		// - can't compute Checksum by invalid Params
		if (!params.IsValid() || !range.IsValidNonempty())
			return Checksum::GetErrorValue();
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
			return Checksum::GetErrorValue();
		Checksum::TParams p=params;
		f.Seek( range.a );
		for( BYTE buf[65536]; f.GetPosition()<range.z; )
			if (cancel)
				return Checksum::GetErrorValue();
			else if (const auto nBytesRead=f.Read( buf, std::min((TPosition)sizeof(buf),range.z-f.GetPosition()), IgnoreIoResult )){
				p.initValue=Checksum::Compute( p, buf, nBytesRead );
				if (p.initValue==Checksum::GetErrorValue())
					break;
			}else
				f.Seek( 1, STREAM_SEEK_CUR ); // skipping irrecoverable portion of data
		return p.initValue;
	}

}
