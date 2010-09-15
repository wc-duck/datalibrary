/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef CONTAINER_TRAITS_H_INCLUDED
#define CONTAINER_TRAITS_H_INCLUDED

/*
	Struct: TSelect
		Select between two types at compile time, depending on a boolean template parameter
*/
template<bool TCondition, typename TIf, typename TElse>
struct TSelect
{
	typedef TIf TResult;
};
template<typename TIf, typename TElse>
struct TSelect<false, TIf, TElse>
{
	typedef TElse TResult;
};

/*
	Struct: TArgTrait
		Argument trait to automatically decide if a type should
		be passed by value or reference. It defaults to pass types larger
		than pointer size by reference.
*/
template<typename T>
struct TArgTrait
{
	typedef typename TSelect<(sizeof(T) <= sizeof(void*)), T, T&>::TResult TArg;
	typedef typename TSelect<(sizeof(T) <= sizeof(void*)), const T, const T&>::TResult TArgConst;
};

/*
	Macro: M_PASS_BY_VALUE
		Declares a class to be passed by value, overriding automatic detection.
*/
#define M_PASS_BY_VALUE(T)		\
template<>						\
struct TArgTrait<T>				\
{								\
	typedef T TArg;				\
	typedef const T TArgConst;	\
};

/*
	Macro: M_PASS_BY_REFERENCE
		Declares a class to be passed by reference, overriding automatic detection.
*/
#define M_PASS_BY_REFERENCE(T)	\
template<>						\
struct TArgTrait<T>				\
{								\
	typedef T& TArg;			\
	typedef const T& TArgConst;	\
};

/*
	Macro: M_DECLARE_ARGTYPES
		Declares regular and const argument types for the type specified using the names specified.
		For use outside classes.

	Parameters:
		T - name of the type
		N - name of the regular argument type to be declared
		CN - name of the const argument type to be declared
*/
#define M_DECLARE_ARGTYPES_CLASS(T, N, CN)	\
typedef typename TArgTrait<T>::TArg N;		\
typedef typename TArgTrait<T>::TArgConst CN;

/*
	Macro: M_DECLARE_ARGTYPES_CLASS
		Declares regular and const argument types for the type specified using the names specified.
		For use inside classes.

	Parameters:
		T - name of the type
		N - name of the regular argument type to be declared
		CN - name of the const argument type to be declared
*/
#define M_DECLARE_ARGTYPES(T, N, CN)\
typedef TArgTrait<T>::TArg N;		\
typedef TArgTrait<T>::TArgConst CN;

#endif // CONTAINER_TRAITS_H_INCLUDED
