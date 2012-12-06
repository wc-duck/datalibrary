/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef CONTAINER_ARRAY_H_INCLUDED
#define CONTAINER_ARRAY_H_INCLUDED

#include <dl/dl_defines.h>

#include "dl_staticdata.h"
#include "dl_traits.h"

#include "../dl_types.h" // TODO: Ugly fugly for types

#include <new>

/*
Class: CArrayNoResizeBase
An implementation of an array that cannot grow. Requires the user to provide a storage class. Alignment should be taken care of by supplied storage class. The array has an allocated size and an actual size.
*/

template <typename T, typename TSTORAGE>
class CArrayNoReAllocBase
{
	TSTORAGE m_Storage;
	size_t m_nElements;
	M_DECLARE_ARGTYPES_CLASS(T, TArg, TArgConst);
public:
	/*
	Constructor: CArrayNoResizeBase
	Constructs an array
	*/
	CArrayNoReAllocBase(){m_nElements = 0;}

	/*
	Destructor: CArrayNoResizeBase
	Constructs an array that is 
	*/
	~CArrayNoReAllocBase(){Reset();}

	/*
	Function: Reset()
	Reset used size to 0.
	*/
	inline void Reset() {SetSize(0);}

	/*
	Function: SetSize()
	Set used size.

	Parameters:
	_NewSize - New used size.
	*/
	inline void SetSize(size_t _NewSize)
	{
		DL_ASSERT(_NewSize <= m_Storage.AllocSize());
		if(_NewSize < m_nElements) // destroy the rest
		{
			for(size_t i=_NewSize; i<m_nElements; i++)
				m_Storage.Base()[i].~T();
		}
		else if(_NewSize > m_nElements) // create new objects
		{
			for(size_t i=m_nElements; i<_NewSize; i++)
				new(&(m_Storage.Base()[i])) T;
		}
		m_nElements = _NewSize;
	}
	/*
	Function: Len()
	Get used size	

	Returns:
	Returns Return used length;
	*/
	inline size_t Len(){return m_nElements;}

	/*
	Function: Full()
	Returns true if the array is full
	*/
	inline bool Full()  { return m_nElements == m_Storage.AllocSize(); }

	/*
	Function: Empty()
	Returns true if the array is empty
	*/
	inline bool Empty() { return m_nElements == 0; }

	/*
	Function: Capacity()
	Returns the amount of items that fit in the array.
	*/
	inline size_t Capacity() { return m_Storage.AllocSize(); }

	/*
	Function: Add()
	Add an element to the array.

	Parameters:
	_Element - Element to add
	*/
	void Add(TArgConst _Element)
	{
		DL_ASSERT(!Full() && "Array is full");
		new(&(m_Storage.Base()[m_nElements])) T(_Element);
		m_nElements++;
	}

	/*
	Function: Decr()
	Removes the last element from the stack.
	*/
	void Decr()
	{
		DL_ASSERT(m_nElements != 0 && "Array is empty");
		m_Storage.Base()[m_nElements--].~T();
	}

	/*
	Function: operator[]
	Get element.
	Parameters:
	_iEl - Index of wanted element.

	Returns:
	Returns reference to wanted element.
	*/
	T& operator[](size_t _iEl)
	{
		DL_ASSERT(_iEl < m_nElements && "Index out of bound");		
		return m_Storage.Base()[_iEl];
	}

	/*
	Function: operator[]
	Get element.
	Parameters:
	_iEl - Index of wanted element. Const version.

	Returns:
	Returns const reference to wanted element.
	*/
	const T& operator[](size_t _iEl) const
	{
		DL_ASSERT(_iEl < m_nElements && "Index out of bound");		
		return m_Storage.Base()[_iEl];
	}

	/*
	Function: GetBasePtr
	Get the array base pointer.
	
	Returns:
	Returns array base pointer.
	*/
	inline T* GetBasePtr() { return m_Storage.Base(); }
};


/*
Class: CArrayStatic
Specialization of <CArrayNoResizeBase> providing its own storage within itself.

Parameters:
	SIZE	- Size in number of elements of the array.
	T		- Type to store.
*/

template <typename T, int SIZE, int ALIGNMENT=0>
class CArrayStatic : public CArrayNoReAllocBase<T, TStaticData<T, SIZE, ALIGNMENT> >
{
};

#endif //CONTAINER_ARRAY_H_INCLUDED
