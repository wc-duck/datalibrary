/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_BINARY_WRITER_H_INCLUDED
#define DL_DL_BINARY_WRITER_H_INCLUDED

#include "container/dl_stack.h"

#include <stdio.h>

#if 0 // BinaryWriterVerbose
	#define DL_LOG_BIN_WRITER_VERBOSE(_Fmt, ...) printf("DL:" _Fmt "\n", ##__VA_ARGS__)
#else
	#define DL_LOG_BIN_WRITER_VERBOSE(_Fmt, ...)
#endif

class CDLBinaryWriter
{
public:
	CDLBinaryWriter(uint8* _pOutData, pint _OutDataSize, bool _Dummy, dl_endian_t _SourceEndian, dl_endian_t _TargetEndian, dl_ptr_size_t _TargetPtrSize) 
		: m_Dummy(_Dummy)
		, m_SourceEndian(_SourceEndian)
		, m_TargetEndian(_TargetEndian)
		, m_PtrSize(_TargetPtrSize)
		, m_Pos(0)
		, m_NeededSize(0)
		, m_Data(_pOutData)
		, m_DataSize(_OutDataSize)
		, m_BackAllocPos(_OutDataSize)
		{}

	~CDLBinaryWriter() { DL_ASSERT( m_BackAllocPos == m_DataSize ); }

	void SeekSet (pint _pPos) { DL_LOG_BIN_WRITER_VERBOSE("Seek Set: %lu", _pPos); m_Pos  = _pPos; }
	void SeekEnd ()           { DL_LOG_BIN_WRITER_VERBOSE("Seek End: %lu", m_NeededSize); m_Pos  = m_NeededSize; }
	pint Tell()               { return m_Pos; }
	pint NeededSize()         { return m_NeededSize; }

	void Write(const void* _pData, pint _Size)
	{
		if( !m_Dummy && ( m_Pos + _Size <= m_DataSize ) )
		{
			DL_LOG_BIN_WRITER_VERBOSE("Write: %lu + %lu (%u)", pos, _Size, uint32(*(pint*)_pData) );
			DL_ASSERT(m_Pos + _Size <= m_DataSize && "To small buffer!");
			memmove(m_Data + m_Pos, _pData, _Size);
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
			DL_LOG_BIN_WRITER_VERBOSE("Write Array: %lu + %lu (%lu)", pos, sizeof(T), _Count);
			for (pint i = 0; i < _Count; ++i)
				DL_LOG_BIN_WRITER_VERBOSE("%lu = %u", i, (uint32)_pArray[i]);
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
		if(m_TargetEndian != DL_ENDIAN_HOST)
		{
			switch(m_PtrSize)
			{
				case DL_PTR_SIZE_32BIT: Write((uint32)_Val); break;
				case DL_PTR_SIZE_64BIT: Write((uint64)_Val); break;
				default:
					DL_ASSERT(false && "Bad ptr-size!?!");
			}
		}
		else
		{
			switch(m_PtrSize)
			{
				case DL_PTR_SIZE_32BIT: { uint32 u = (uint32)_Val; Write(&u, 4); } break;
				case DL_PTR_SIZE_64BIT: { uint64 u = (uint64)_Val; Write(&u, 8); } break;
				default:
					DL_ASSERT(false && "Bad ptr-size!?!");
			}
		}
	}

	void WriteString( const void* str, pint len )
	{
		Write( str, len );
		Write( char('\0') );
	}

	void WriteZero(pint _Bytes)
	{
		if(!m_Dummy)
		{
			DL_LOG_BIN_WRITER_VERBOSE("Write zero: %lu + %lu", pos, _Bytes);
			DL_ASSERT(m_Pos + _Bytes <= m_DataSize && "To small buffer!");
			memset(m_Data + m_Pos, 0x0, _Bytes);
		}

		m_Pos += _Bytes;
		UpdateNeededSize();
	}

	void Reserve(pint _Bytes)
	{
		DL_LOG_BIN_WRITER_VERBOSE("Reserve: %lu + %lu", pos, _Bytes);
		m_NeededSize = m_NeededSize >= m_Pos + _Bytes ? m_NeededSize : m_Pos + _Bytes;
	}

	pint PushBackAlloc( pint bytes )
	{
		// allocates a chunk of "bytes" bytes from the back of the buffer and return the
		// offset to it. function ignores alignment and tries to pack data as tightly as possible
		// since data that will be stored here is only temporary.

		m_BackAllocPos -= bytes;
		DL_LOG_BIN_WRITER_VERBOSE("push back-alloc: %lu (%lu)\n", m_BackAllocPos, bytes);
		return m_BackAllocPos;
	}

	// return start of new array!
	pint PopBackAlloc(uint32 num_elem, uint32 elem_size, uint32 elem_align)
	{
		DL_ASSERT( m_BackAllocPos != m_DataSize );
		DL_ASSERT( m_BackAllocPos >= m_NeededSize && "back-alloc and front-alloc overlap!" );

		SeekEnd();
		Align( elem_align );
		Reserve( elem_size * num_elem );
		pint arr_pos = Tell();

		pint first_elem = m_BackAllocPos;
		pint last_elem  = first_elem + elem_size * ( num_elem - 1 );

		DL_LOG_BIN_WRITER_VERBOSE("First: %lu last %lu", first_elem, last_elem);

		unsigned char* curr_first = m_Data + first_elem;
		unsigned char* curr_last  = m_Data + last_elem;

		unsigned char swap_buffer[256]; // TODO: Handle bigger elements!

		if( !m_Dummy )
		while( curr_first < curr_last )
		{
			DL_ASSERT( elem_size <= DL_ARRAY_LENGTH(swap_buffer));

			DL_LOG_BIN_WRITER_VERBOSE("Swap: %lu to %lu", curr_first - m_Data, curr_last - m_Data );

			memcpy( swap_buffer, curr_first,  elem_size );
			memcpy( curr_first,  curr_last,   elem_size );
			memcpy( curr_last,   swap_buffer, elem_size );

			curr_first += elem_size;
			curr_last  -= elem_size;
		}

		if( elem_size == elem_align )
			Write( m_Data + first_elem, elem_size + num_elem );
		else
		{
			for( uint32 elem = 0; elem < num_elem; ++elem )
			{
				Align( elem_align );
				Write( m_Data + first_elem + elem * elem_size, elem_size );
			}
		}

		m_BackAllocPos += elem_size * ( num_elem );

		return arr_pos;
	}

	bool InBackAllocArea() { return m_Pos > m_NeededSize; }

	// these feel a bit hackish, need to look over!
	template <typename T> T  Read()
	{
		if(m_Dummy) return 0;
		return *(T*)(m_Data + m_Pos);
	}

	void Align(pint _Alignment)
	{
		pint Alignment = dl_internal_align_up(m_Pos, _Alignment);
		if(!m_Dummy && Alignment != m_Pos) 
		{
			DL_LOG_BIN_WRITER_VERBOSE("Align: %lu + %lu (%lu)", pos, Alignment - pos, _Alignment);
			memset(m_Data + m_Pos, 0x0, Alignment - m_Pos);
		}
		m_Pos = Alignment;
		UpdateNeededSize();
	};

	pint m_BackAllocPos;

private:
	void UpdateNeededSize()
	{
		if( m_BackAllocPos == m_DataSize || m_Pos <= m_BackAllocPos )
			m_NeededSize = m_NeededSize >= m_Pos ? m_NeededSize : m_Pos;
	}

	bool          m_Dummy;
	dl_endian_t   m_SourceEndian;
	dl_endian_t   m_TargetEndian;
	dl_ptr_size_t m_PtrSize;
	pint          m_Pos;
	pint          m_NeededSize;
	uint8*        m_Data;
	pint          m_DataSize;
};

#endif // DL_DL_BINARY_WRITER_H_INCLUDED
