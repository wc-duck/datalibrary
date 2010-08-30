#ifndef DL_DL_BINARY_WRITER_H_INCLUDED
#define DL_DL_BINARY_WRITER_H_INCLUDED

#include "dl_temp.h"

#include <stdio.h>

#if 0 // BinaryWriterVerbose
	#define DL_LOG_BIN_WRITER_VERBOSE(_Fmt, ...)
#else
	#define DL_LOG_BIN_WRITER_VERBOSE(_Fmt, ...) printf("DL:" _Fmt, ##__VA_ARGS__)
#endif

class CDLBinaryWriter
{
public:
	CDLBinaryWriter(uint8* _pOutData, pint _OutDataSize, bool _Dummy, ECpuEndian _SourceEndian, ECpuEndian _TargetEndian, EDLPtrSize _TargetPtrSize) 
		: m_Dummy(_Dummy)
		, m_SourceEndian(_SourceEndian)
		, m_TargetEndian(_TargetEndian)
		, m_PtrSize(_TargetPtrSize)
		, m_Pos(0)
		, m_NeededSize(0)
		, m_Data(_pOutData)
		, m_DataSize(_OutDataSize)
		{}

	void SeekSet (pint _pPos) { DL_LOG_BIN_WRITER_VERBOSE("Seek: %u", _pPos); m_Pos  = _pPos; }
	void SeekEnd ()           { DL_LOG_BIN_WRITER_VERBOSE("Seek End: %u", m_NeededSize); m_Pos  = m_NeededSize; }
	pint Tell()               { return m_Pos; }
	pint NeededSize()         { return m_NeededSize; }

	void Write(const void* _pData, pint _Size)
	{
		if(!m_Dummy)
		{
			DL_LOG_BIN_WRITER_VERBOSE("Write: %u + %u (%u)", m_Pos, _Size, *(pint*)_pData);
			M_ASSERT(m_Pos + _Size <= m_DataSize && "To small buffer!");
			memcpy(m_Data + m_Pos, _pData, _Size);
		}

		m_Pos += _Size;
		UpdateNeededSize();
	}

	void WriteSwap(const void* _pData, pint _Size)
	{
		switch(_Size)
		{
			case sizeof(uint8):  Write( *(uint8*)_pData); break;
			case sizeof(uint16): Write(*(uint16*)_pData); break;
			case sizeof(uint32): Write(*(uint32*)_pData); break;
			case sizeof(uint64): Write(*(uint64*)_pData); break;
		}
	}

	template<typename T> void Write(T _Val)
	{
		if(m_SourceEndian != m_TargetEndian)
			_Val = DLSwapEndian(_Val);
		
		Write(&_Val, sizeof(T));
	}

	template<typename T> void WriteArray(const T* _pArray, pint _Count)
	{
		if(!m_Dummy)
		{
			DL_LOG_BIN_WRITER_VERBOSE("Write Array: %u + %u (%u)", m_Pos, sizeof(T), _Count);
			for (pint i = 0; i < _Count; ++i)
				DL_LOG_BIN_WRITER_VERBOSE("%u = %u", i, (uint32)_pArray[i]);
		}

		if(m_SourceEndian != m_TargetEndian)
		{
			for(pint i = 0; i < _Count; ++i)
				Write(_pArray[i]);
		}
		else
			Write(_pArray, sizeof(T) * _Count);
	}

	// _Val is expected to be in host-endian!!!
	void WritePtr(pint _Val)
	{
		if(m_TargetEndian != ENDIAN_HOST)
		{
			switch(m_PtrSize)
			{
				case DL_PTR_SIZE_32BIT: Write((uint32)_Val); break;
				case DL_PTR_SIZE_64BIT: Write((uint64)_Val); break;
				default:
					M_ASSERT(false && "Bad ptr-size!?!");
			}
		}
		else
		{
			switch(m_PtrSize)
			{
				case DL_PTR_SIZE_32BIT: { uint32 u = (uint32)_Val; Write(&u, 4); } break;
				case DL_PTR_SIZE_64BIT: { uint64 u = (uint64)_Val; Write(&u, 8); } break;
				default:
					M_ASSERT(false && "Bad ptr-size!?!");
			}
		}
	}

	void WriteZero(pint _Bytes)
	{
		if(!m_Dummy)
		{
			DL_LOG_BIN_WRITER_VERBOSE("Write zero: %u + %u", m_Pos, _Bytes);
			M_ASSERT(m_Pos + _Bytes <= m_DataSize && "To small buffer!");
			memset(m_Data + m_Pos, 0x0, _Bytes);
		}

		m_Pos += _Bytes;
		UpdateNeededSize();
	}

	void Reserve(pint _Bytes)
	{
		m_NeededSize = Max(m_NeededSize, m_Pos + _Bytes);
	}

	// these feel a bit hackish, need to look over!
	template <typename T> T  Read()
	{
		if(m_Dummy) return 0;
		return *(T*)(m_Data + m_Pos);
	}

	void Align(pint _Alignment)
	{
		pint Alignment = AlignUp(m_Pos, _Alignment);
		if(!m_Dummy && Alignment != m_Pos) 
		{
			DL_LOG_BIN_WRITER_VERBOSE("Align: %u + %u (%u)", m_Pos, Alignment - m_Pos, _Alignment);
			memset(m_Data + m_Pos, 0x0, Alignment - m_Pos);
		}
		m_Pos = Alignment;
		UpdateNeededSize();
	};

private:
	void UpdateNeededSize() { m_NeededSize = Max(m_NeededSize, m_Pos); }

	bool       m_Dummy;
	ECpuEndian m_SourceEndian;
	ECpuEndian m_TargetEndian;
	EDLPtrSize m_PtrSize;
	pint       m_Pos;
	pint       m_NeededSize;		
	uint8*     m_Data;
	pint       m_DataSize;
};

#endif // DL_DL_BINARY_WRITER_H_INCLUDED
