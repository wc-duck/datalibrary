/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef CONTAINER_ALIGNED_TYPE_H_INCLUDED
#define CONTAINER_ALIGNED_TYPE_H_INCLUDED

#include <dl/dl_defines.h>
#include "../dl_types.h" // TODO: Ugly fugly for type

template<pint N> struct TAlignedType
{
	DL_STATIC_ASSERT(N > 0, no_negative_alignment);
	DL_STATIC_ASSERT(N <= 256, not_implemented_yet);
	DL_STATIC_ASSERT(N % 2 == 0, power_of_2_needed);
};

template<> struct TAlignedType<1>   { DL_ALIGN(1)   char m_Dummy[1]; };
template<> struct TAlignedType<2>   { DL_ALIGN(2)   char m_Dummy[2]; };
template<> struct TAlignedType<4>   { DL_ALIGN(4)   char m_Dummy[4]; };
template<> struct TAlignedType<8>   { DL_ALIGN(8)   char m_Dummy[8]; };
template<> struct TAlignedType<16>  { DL_ALIGN(16)  char m_Dummy[16]; };
template<> struct TAlignedType<32>  { DL_ALIGN(32)  char m_Dummy[32]; };
template<> struct TAlignedType<64>  { DL_ALIGN(64)  char m_Dummy[64]; };
template<> struct TAlignedType<128> { DL_ALIGN(128) char m_Dummy[128]; };
template<> struct TAlignedType<256> { DL_ALIGN(256) char m_Dummy[256]; };

#endif // CONTAINER_ALIGNED_TYPE_H_INCLUDED
