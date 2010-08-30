#ifndef CONTAINER_ARRAY_H_INCLUDED
#define CONTAINER_ARRAY_H_INCLUDED

#include <dl/dl_defines.h>

#include "dl_staticdata.h"
#include "dl_traits.h"

#include <new>
#include <stdlib.h>

/*
Class: CArrayNoResizeBase
An implementation of an array that cannot grow. Requires the user to provide a storage class. Alignment should be taken care of by supplied storage class. The array has an allocated size and an actual size.
*/

template <typename T, typename TSTORAGE>
class CArrayNoReAllocBase
{
	TSTORAGE m_Storage;
	pint m_nElements;
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
	inline void SetSize(pint _NewSize)
	{
		M_ASSERT(_NewSize <= m_Storage.AllocSize());
		if(_NewSize < m_nElements) // destroy the rest
		{
			for(pint i=_NewSize; i<m_nElements; i++)
				m_Storage.Base()[i].~T();
		}
		else if(_NewSize > m_nElements) // create new objects
		{
			for(pint i=m_nElements; i<_NewSize; i++)
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
	inline pint Len(){return m_nElements;}

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
	inline pint Capacity() { return m_Storage.AllocSize(); }

	/*
	Function: Add()
	Add an element to the array.

	Parameters:
	_Element - Element to add
	*/
	void Add(TArgConst _Element)
	{
		M_ASSERT(!Full() && "Array is full");
		new(&(m_Storage.Base()[m_nElements])) T(_Element);
		m_nElements++;
	}

	/*
	Function: Decr()
	Removes the last element from the stack.
	*/
	void Decr()
	{
		M_ASSERT(m_nElements != 0 && "Array is empty");
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
	T& operator[](pint _iEl)
	{
		M_ASSERT(_iEl < m_nElements && "Index out of bound");		
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
	const T& operator[](pint _iEl) const
	{
		M_ASSERT(_iEl < m_nElements && "Index out of bound");		
		return m_Storage.Base()[_iEl];
	}

	/*
	Function: GetBasePtr
	Get the array base pointer.
	
	Returns:
	Returns array base pointer.
	*/
	inline T* GetBasePtr(){return (T*)m_Storage.m_pStorage;}
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

/*
Class: TArray
Array class that can reallocate its memory.

Parameters:
	T		- Type to store.
*/
template <typename T> // TODO: alignment
class TArray
{
protected:
	T* m_pData;
	pint m_nElements;
	pint m_Capacity;
	M_DECLARE_ARGTYPES_CLASS(T, TArg, TArgConst);

public:
	/*
	Constructor: TArray
	Constructs an array
	*/
	TArray() : m_pData(0x0), m_nElements(0), m_Capacity(0) { }
	TArray(pint _Capacity) : m_nElements(0), m_Capacity(_Capacity) { m_pData = reinterpret_cast<T*>(malloc(sizeof(T)*m_Capacity)); }

	/*
	Destructor: TArray
	Runs destructor on all elements in array and frees memory.
	*/
	~TArray()
	{
		Reset();
		if(m_pData)
			free(m_pData);
	}

	/*
	Function: Reset()
	Reset used size to 0. Runs destructors.
	*/
	inline void Reset() { SetSize(0); }

	/*
	Function: SetSize()
	Set used size.
	Might allocate memory, will run copy constructors and constructors.

	Parameters:
	_NewSize - New used size.
	*/
	inline void SetSize(pint _NewSize)
	{
		if(_NewSize <= m_Capacity)
		{
			if(_NewSize < m_nElements) // destroy the rest
			{
				for(pint i = _NewSize; i < m_nElements; ++i)
					m_pData[i].~T();
			}
			else if(_NewSize > m_nElements) // create new objects
			{
				for(pint i = m_nElements; i < _NewSize; ++i)
					new(&(m_pData[i])) T;
			}
		}
		else
		{
			T* pOldData = m_pData;
			m_pData = reinterpret_cast<T*>(malloc(sizeof(T)*_NewSize)); // TODO: use policy
			m_Capacity = _NewSize;

			for(pint i = 0; i < m_nElements; ++i) // run copy constructors
				new(&(m_pData[i])) T(pOldData[i]);

			for(pint i = m_nElements; i < _NewSize; ++i) // create new objects for the rest
					new(&(m_pData[i])) T;

			free(pOldData);
		}
		m_nElements = _NewSize;
	}

	/*
	Function: Len()
	Get used size	

	Returns:
	Returns Return used length;
	*/
	inline pint Len() const { return m_nElements; }

	/*
	Function: Full()
	Returns true if the array is full
	*/
	inline bool Full() const { return m_nElements == m_Capacity; }

	/*
	Function: Empty()
	Returns true if the array is empty
	*/
	inline bool Empty() const { return m_nElements == 0; }

	/*
	Function: Capacity()
	Returns the amount of items that fit in the array.
	*/
	inline pint Capacity() const { return m_Capacity; }

	/*
	Function: Add()
	Add an element to the array.

	Parameters:
	_Element - Element to add
	*/
	void Add(TArgConst _Element)
	{
		M_ASSERT(!Full() && "Array is full");
		new(&(m_pData[m_nElements])) T(_Element);
		m_nElements++;
	}

	/*
	Function: Decr()
	Removes the last element from the stack.
	*/
	void Decr()
	{
		M_ASSERT(m_nElements != 0 && "Array is empty");
		m_pData[m_nElements--].~T();
	}

	/*
	Function: operator[]
	Get element.
	Parameters:
	_iEl - Index of wanted element.

	Returns:
	Returns reference to wanted element.
	*/
	TArg operator[](pint _iEl)
	{
		M_ASSERT(_iEl < m_nElements && "Index out of bound");		
		return m_pData[_iEl];
	}

	/*
	Function: operator[]
	Get element.
	Parameters:
	_iEl - Index of wanted element. Const version.

	Returns:
	Returns const reference to wanted element.
	*/
	TArgConst operator[](pint _iEl) const
	{
		M_ASSERT(_iEl < m_nElements && "Index out of bound");		
		return m_pData[_iEl];
	}

	/*
	Function: GetBasePtr
	Get the array base pointer.
	
	Returns:
	Returns array base pointer.
	*/
	inline T* GetBasePtr() { return m_pData; }
};

#endif //CONTAINER_ARRAY_H_INCLUDED