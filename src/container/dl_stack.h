#ifndef CONTAINER_STACK_H_INCLUDED
#define CONTAINER_STACK_H_INCLUDED

#include "dl_array.h"

/*
	Class: CStackStatic
		Implementation of a stack containing its own static memory.

	Parameters:
		T         - Type of elements in stack
		SIZE      - Number of elements in stack
		ALIGNMENT - Alignment of elements in stack, if 0 natural alignment of T is used.
*/
template <typename T, int SIZE, int ALIGNMENT=0>
class CStackStatic
{
	M_DECLARE_ARGTYPES_CLASS(T, TArg, TArgConst);
	CArrayStatic<T, SIZE, ALIGNMENT> m_Data;
public:
	/*
		Function: Push
	*/
	void Push(TArgConst _Elem) { m_Data.Add(_Elem); }
	
	/*
		Function: Pop

		Note: 
			A call to top on an empty stack is invalid! 
	*/
	void Pop() { m_Data.Decr(); }
	
	/* 
		Function: Top
			Return a reference to the current top-element on the stack. 

		Note: 
			A call to top on an empty stack is invalid! 
	*/
	TArg Top() { M_ASSERT( !Empty() && "Stack is empty" ); return m_Data[m_Data.Len() - 1]; }
	
	/*
		Function: Empty
	*/
	bool Empty() { return Len() == 0; }
	
	/*
		Function: Len
			Return the number of elements currently in the stack.
	*/
	pint Len() { return m_Data.Len(); }
};

#endif // CONTAINER_STACK_H_INCLUDED
