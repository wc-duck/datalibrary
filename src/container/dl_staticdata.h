/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef CONTAINER_STATICDATA_H_INCLUDED
#define CONTAINER_STATICDATA_H_INCLUDED

#include "dl_aligned_type.h"

template <typename T, int ELEMENTS, int ALIGNMENT=0>
class TStaticData
{
	enum { RESULT_ALIGNMENT = DL_ALIGNMENTOF(T) > ALIGNMENT ? DL_ALIGNMENTOF(T) : ALIGNMENT };
	typedef TAlignedType<RESULT_ALIGNMENT> TDataType;
public:
	TDataType m_pStorage[(sizeof(T) * ELEMENTS) / sizeof(TDataType)];
	DL_FORCEINLINE pint AllocSize(){return ELEMENTS;}
	DL_FORCEINLINE T* Base(){return (T*)m_pStorage;}
};

#endif // CONTAINER_STATICDATA_H_INCLUDED
