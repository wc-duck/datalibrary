/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_BIT_H_INCLUDED
#define DL_DL_BIT_H_INCLUDED

#define DL_BIT(_Bit)						( 1ULL << (_Bit) )
#define DL_BITMASK(_Bits)                   ( DL_BIT(_Bits) - 1ULL )
#define DL_BITRANGE(_MinBit,_MaxBit)		( ((1ULL << (_MaxBit)) | ((1ULL << (_MaxBit))-1ULL)) ^ ((1ULL << (_MinBit))-1ULL) )

#define DL_ZERO_BITS(_Target, _Start, _Bits)         ( (_Target) & ~DL_BITRANGE(_Start, (_Start) + (_Bits) - 1ULL) )
#define DL_EXTRACT_BITS(_Val, _Start, _Bits)         ( (_Val >> (_Start)) & DL_BITMASK(_Bits) )
#define DL_INSERT_BITS(_Target, _Val, _Start, _Bits) ( DL_ZERO_BITS(_Target, _Start, _Bits) | ( (DL_BITMASK(_Bits) & (_Val) ) << (_Start)) )
#define DL_BITS_IN_TYPE(_Type)                       ( sizeof(_Type) * 8 )

#endif // DL_DL_BIT_H_INCLUDED
