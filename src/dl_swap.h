/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_SWAP_H_INCLUDED
#define DL_DL_SWAP_H_INCLUDED

#include "dl_types.h"

DL_FORCEINLINE int8  dl_swap_endian_int8 ( int8  val ) { return val; }
DL_FORCEINLINE int16 dl_swap_endian_int16( int16 val ) { return ( ( val & 0x00FF ) << 8 )  | ( ( val & 0xFF00 ) >> 8 ); }
DL_FORCEINLINE int32 dl_swap_endian_int32( int32 val ) { return ( ( val & 0x00FF ) << 24 ) | ( ( val & 0xFF00 ) << 8) | ( ( val >> 8 ) & 0xFF00 ) | ( ( val >> 24 ) & 0x00FF ); }
DL_FORCEINLINE int64 dl_swap_endian_int64( int64 val )
{
	union { int32 m_i32[2]; int64 m_i64; } conv;
	conv.m_i64 = val;
	int32 tmp     = dl_swap_endian_int32( conv.m_i32[0] );
	conv.m_i32[0] = dl_swap_endian_int32( conv.m_i32[1] );
	conv.m_i32[1] = tmp;
	return conv.m_i64;
}
DL_FORCEINLINE uint8  dl_swap_endian_uint8 ( uint8  val ) { return val; }
DL_FORCEINLINE uint16 dl_swap_endian_uint16( uint16 val ) { return ( ( val & 0x00FF ) << 8 )  | ( ( val & 0xFF00 ) >> 8 ); }
DL_FORCEINLINE uint32 dl_swap_endian_uint32( uint32 val ) { return ( ( val & 0x00FF ) << 24 ) | ( ( val & 0xFF00 ) << 8) | ( ( val >> 8 ) & 0xFF00 ) | ( ( val >> 24 ) & 0x00FF ); }
DL_FORCEINLINE uint64 dl_swap_endian_uint64( uint64 val )
{
	union { uint32 m_u32[2]; uint64 m_u64; } conv;
	conv.m_u64 = val;
	uint32 tmp    = dl_swap_endian_uint32( conv.m_u32[0] );
	conv.m_u32[0] = dl_swap_endian_uint32( conv.m_u32[1] );
	conv.m_u32[1] = tmp;
	return conv.m_u64;
}

// DL_FORCEINLINE int8  DLSwapEndian(int8  _SwapMe) { return _SwapMe; }
// DL_FORCEINLINE int16 DLSwapEndian(int16 _SwapMe) { return ((_SwapMe & 0xff) << 8)  | ((_SwapMe & 0xff00) >> 8); }
// DL_FORCEINLINE int32 DLSwapEndian(int32 _SwapMe) { return ((_SwapMe & 0xff) << 24) | ((_SwapMe & 0xff00) << 8) | ((_SwapMe >> 8) & 0xff00) | ((_SwapMe >> 24) & 0xff); }
/*DL_FORCEINLINE int64 DLSwapEndian(int64 _SwapMe)
{
	union { int32 m_i32[2]; int64 m_i64; } conv;
	conv.m_i64 = _SwapMe;
	int32 tmp     = DLSwapEndian(conv.m_i32[0]);
	conv.m_i32[0] = DLSwapEndian(conv.m_i32[1]);
	conv.m_i32[1] = tmp;
	return conv.m_i64;
}
DL_FORCEINLINE uint8  DLSwapEndian(uint8  _SwapMe) { return _SwapMe; }
DL_FORCEINLINE uint16 DLSwapEndian(uint16 _SwapMe) { return ((_SwapMe & 0xff) << 8)  | ((_SwapMe & 0xff00) >> 8); }
DL_FORCEINLINE uint32 DLSwapEndian(uint32 _SwapMe) { return ((_SwapMe & 0xff) << 24) | ((_SwapMe & 0xff00) << 8) | ((_SwapMe >> 8) & 0xff00) | ((_SwapMe >> 24) & 0xff); }
DL_FORCEINLINE uint64 DLSwapEndian(uint64 _SwapMe)
{
	union { uint32 m_u32[2]; uint64 m_u64; } conv;
	conv.m_u64 = _SwapMe;
	uint32 tmp    = DLSwapEndian(conv.m_u32[0]);
	conv.m_u32[0] = DLSwapEndian(conv.m_u32[1]);
	conv.m_u32[1] = tmp;
	return conv.m_u64;
}*/

DL_FORCEINLINE pint dl_swap_endian_pint( pint p )
{
#ifdef DL_PTR_SIZE32
	return dl_swap_endian_uint32( p );
#else
	return dl_swap_endian_uint64( p );
#endif
}

DL_FORCEINLINE void* dl_swap_endian_ptr( void* ptr ) { return (void*)dl_swap_endian_pint( (pint)ptr ); }

/*
DL_FORCEINLINE void* DLSwapEndian(void* _SwapMe)
{
#ifdef DL_PTR_SIZE32
	return (void*)DLSwapEndian(uint32(_SwapMe));
#else
	return (void*)DLSwapEndian(uint64(_SwapMe));
#endif
}
*/

DL_FORCEINLINE float dl_swap_endian_fp32( float f )
// DL_FORCEINLINE float DLSwapEndian(float _SwapMe)
{
	union { uint32 m_u32; float m_fp32; } conv;
	conv.m_fp32 = f;
	conv.m_u32  = dl_swap_endian_uint32( conv.m_u32 );
	return conv.m_fp32;
}

DL_FORCEINLINE double dl_swap_endian_fp64( double f )
// DL_FORCEINLINE double DLSwapEndian(double _SwapMe)
{
	union { uint64 m_u64; double m_fp64; } conv;
	conv.m_fp64 = f;
	conv.m_u64  = dl_swap_endian_uint64( conv.m_u64 );
	return conv.m_fp64;
}

DL_STATIC_ASSERT(sizeof(dl_type_t) == 4, dl_type_need_to_be_4_bytes);
// DL_FORCEINLINE dl_type_t DLSwapEndian(dl_type_t _SwapMe) { return (dl_type_t)DLSwapEndian((uint32)_SwapMe); }
DL_FORCEINLINE dl_type_t dl_swap_endian_dl_type( dl_type_t val ) { return (dl_type_t)dl_swap_endian_uint32( val ); }


#endif // DL_DL_SWAP_H_INCLUDED
