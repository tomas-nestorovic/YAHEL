#include "stdafx.h"
#include "Instance.h"

namespace Yahel{
namespace Checksum{

	TParams::TParams()
		// ctor
		: type(Add) , seed(GetDefaultSeed()) {
	}

	TParams::TParams(TType type,T seed)
		// ctor
		: type(type) , seed(seed) {
	}

	bool TParams::IsValid() const{
		return	type<Last
				&&
				seed!=GetErrorValue();
	}

	T GetDefaultSeed(){
		return 0;
	}

	T GetErrorValue(){
		return -1;
	}

	bool TParams::EditModalWithDefaultEnglishDialog(HWND hParent){
		//
		// - can't edit invalid Params
		if (!IsValid())
			return false;
		// - showing the Dialog and processing its result
		static const Gui::TNamedInt Seeds[]={
			{ 0xE295, "MFM A1A1A1<data>" },
			{ 0xD2F6, "MFM A1A1A1<deleted-data>" }
		};
		return Gui::QuerySingleIntA(
			"Checksum", "&Initial value", UInt32, seed, Gui::Hexa, hParent, Seeds
		);
	}






	typedef T (* FnAppender)(T seed,const BYTE *pBytes,WORD nBytes);

	static T Add(T seed,const BYTE *pBytes,WORD nBytes){
		while (nBytes-->0)
			seed+=*pBytes++;
		return seed;
	}

	static T Xor(T seed,const BYTE *pBytes,WORD nBytes){
		while (nBytes-->0)
			seed^=*pBytes++;
		return seed;
	}

	static T Ibm3740(T seed,const BYTE *pBytes,WORD nBytes){
		while (nBytes-->0){
			BYTE b=*pBytes++;
			b ^= seed>>8;
			b ^= b>>4;
			seed= (WORD)(seed<<8) ^ (WORD)(b<<12) ^ (WORD)(b<<5) ^ b;
		}
		return seed;
	}

	T Compute(const TParams &params,LPCVOID bytes,UINT nBytes){
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
			case TParams::Ibm3740:
				fn=Ibm3740;
				break;
			default:
				assert(false);
				return GetErrorValue();
		}
		// - computation
		constexpr UINT ChunkBytesMax=4096;
		T result=params.seed;
		for( const BYTE *pb=(LPBYTE)bytes; nBytes>0; ){
			const UINT nChunkBytes=std::min( nBytes, ChunkBytesMax );
			result=fn( result, pb, nChunkBytes );
			pb+=nChunkBytes, nBytes-=nChunkBytes;
		}
		return result;
	}

	BYTE ComputeXor(LPCVOID bytes,UINT nBytes,BYTE seed){
		return Compute( TParams(TParams::Xor,seed), bytes, nBytes );
	}

	T ComputeAdd(LPCVOID bytes,UINT nBytes,T seed){
		return Compute( TParams(TParams::Add,seed), bytes, nBytes );
	}
}





	Checksum::T CInstance::GetChecksum(const Checksum::TParams &params,const TPosInterval &range,volatile const bool &cancel) const{
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
				p.seed=Checksum::Compute( p, buf, nBytesRead );
				if (p.seed==Checksum::GetErrorValue())
					break;
			}else
				f.Seek( 1, STREAM_SEEK_CUR ); // skipping irrecoverable portion of data
		return p.seed;
	}

}
