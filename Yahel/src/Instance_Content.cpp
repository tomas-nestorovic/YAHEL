#include "stdafx.h"
#include "Instance.h"

namespace Yahel
{
	CInstance::CContent::CContent(IStream *stream)
		// ctor
		: stream(stream) {
		stream->Release();
	}



	static constexpr ULARGE_INTEGER LargeZero={};

	TPosition CInstance::CContent::GetLength() const{
		const TPosition currPos=GetPosition();
		if (currPos!=GetErrorPosition()){
			ULARGE_INTEGER tmp;
			if (SUCCEEDED(stream->Seek( LargeZero, STREAM_SEEK_END, &tmp ))){
				const TPosition length=tmp.QuadPart;
				tmp.QuadPart=currPos;
				if (SUCCEEDED(stream->Seek( tmp, STREAM_SEEK_SET, nullptr )))
					return length;
			}
		}
		return GetErrorPosition();
	}

	void CInstance::CContent::SetLength(TPosition length){
		ULARGE_INTEGER tmp;
			tmp.QuadPart=length;
		stream->SetSize( tmp );
	}

	TPosition CInstance::CContent::GetPosition() const{
		ULARGE_INTEGER tmp;
		return	SUCCEEDED(stream->Seek( LargeZero, STREAM_SEEK_CUR, &tmp ))
				? tmp.QuadPart
				: GetErrorPosition();
	}

	TPosition CInstance::CContent::Seek(TPosition pos,DWORD nFrom){
		ULARGE_INTEGER tmp,res;
			tmp.QuadPart=pos;
		return	SUCCEEDED(stream->Seek( tmp, nFrom, &res ))
				? res.QuadPart
				: GetErrorPosition();
	}

	TPosition CInstance::CContent::Read(PVOID buffer,TPosition bufferCapacity){
		ULONG nBytesRead;
		return	SUCCEEDED(stream->Read( buffer, bufferCapacity, &nBytesRead ))
				? nBytesRead
				: 0;
	}

	void CInstance::CContent::Write(LPCVOID data,TPosition dataLength){
		stream->Write( data, dataLength, nullptr );
	}
	
}
