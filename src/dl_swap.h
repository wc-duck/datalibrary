/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_SWAP_H_INCLUDED
#define DL_DL_SWAP_H_INCLUDED

#include "dl_types.h"

// #include <platform/defines.h>

DL_FORCEINLINE int8  DLSwapEndian(int8 _SwapMe)  { return _SwapMe; }
DL_FORCEINLINE int16 DLSwapEndian(int16 _SwapMe) { return ((_SwapMe & 0xff) << 8)  | ((_SwapMe & 0xff00) >> 8); }
DL_FORCEINLINE int32 DLSwapEndian(int32 _SwapMe) { return ((_SwapMe & 0xff) << 24) | ((_SwapMe & 0xff00) << 8) | ((_SwapMe >> 8) & 0xff00) | ((_SwapMe >> 24) & 0xff); }
DL_FORCEINLINE int64 DLSwapEndian(int64 _SwapMe)
{
	union { int32 m_i32[2]; int64 m_i64; } conv;
	conv.m_i64 = _SwapMe;
	int32 tmp     = DLSwapEndian(conv.m_i32[0]);
	conv.m_i32[0] = DLSwapEndian(conv.m_i32[1]);
	conv.m_i32[1] = tmp;
	return conv.m_i64;
}
DL_FORCEINLINE uint8  DLSwapEndian(uint8 _SwapMe)  { return _SwapMe; }
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
}

/*
#if !defined(M_PLATFORM_PS3) && !defined(M_PLATFORM_WINX64) // compiler complains about redefinition on these platforms????
DL_FORCEINLINE pint DLSwapEndian(pint _SwapMe)
{
	switch(sizeof(pint))
	{
		case 4: return (pint)DLSwapEndian((uint32)_SwapMe);
		case 8: return (pint)DLSwapEndian((uint64)_SwapMe);
		default: 
			return 0;
	}
}
#endif // !defined(M_PLATFORM_PS3) && !defined(M_PLATFORM_WINX64)
*/

DL_FORCEINLINE void* DLSwapEndian(void* _SwapMe)
{
#ifdef DL_PTR_SIZE32
	return (void*)DLSwapEndian(uint32(_SwapMe));
#else
	return (void*)DLSwapEndian(uint64(_SwapMe));
#endif
}

DL_FORCEINLINE float DLSwapEndian(float _SwapMe)
{
	union { uint32 m_u32; float m_fp32; } conv;
	conv.m_fp32 = _SwapMe;
	conv.m_u32 = DLSwapEndian(conv.m_u32);
	return conv.m_fp32;
}

DL_FORCEINLINE double DLSwapEndian(double _SwapMe)
{
	union { uint64 m_u64; double m_fp64; } conv;
	conv.m_fp64 = _SwapMe;
	conv.m_u64 = DLSwapEndian(conv.m_u64);
	return conv.m_fp64;
}

DL_STATIC_ASSERT(sizeof(EDLType) == 4, dl_type_need_to_be_4_bytes);
DL_FORCEINLINE EDLType DLSwapEndian(EDLType _SwapMe) { return (EDLType)DLSwapEndian((uint32)_SwapMe); }

#endif // DL_DL_SWAP_H_INCLUDED
