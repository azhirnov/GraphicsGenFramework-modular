// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "Array.h"

namespace GX_STL
{
namespace GXTypes
{

	//
	// Dynamic Stack
	//

	template <	typename T,
				typename S = typename AutoDetectCopyStrategy<T>::type,
				typename MC = MemoryContainer<T>
			 >
	struct Stack : public CompileTime::CopyQualifiers< Array<T,S,MC> >
	{
	// types
	public:
		using Strategy_t		= S;
		using MemoryContainer_t	= MC;
		using Value_t			= T;
		using Self				= Stack<T,S,MC>;
		using Container_t		= Array<T,S,MC>;
		using const_iterator	= typename Container_t::const_iterator;


	// variables
	private:
		Container_t		_memory;


	// methods
	public:
		Stack (GX_DEFCTOR)										{ _memory.Resize( 1 ); }
		
		Stack (const Self &other): _memory( other._memory )		{}

		Stack (Self &&other): _memory( RVREF(other._memory) )	{}

		~Stack ()												{}

		void SetDefault (const T &value)						{ _memory[0] = value; }
		void SetDefault (T &&value)								{ _memory[0] = RVREF( value ); }

		void Reserve (usize size)								{ _memory.Reserve( size ); }
		
		bool Pop ();
		
		void Push ();
		void Push (const T &value)								{ _memory.PushBack( value ); }
		void Push (T &&value)									{ _memory.PushBack( RVREF( value ) ); }

		ND_ T &			Get ()									{ return _memory.Back(); }
		ND_ const T &	Get ()	const							{ return _memory.Back(); }
		
		void Set (const T &value)								{ _memory.Set( _memory.LastIndex(), value ); }
		void Set (T &&value)									{ _memory.Set( _memory.LastIndex(), RVREF( value ) ); }

		ND_ usize			Capacity ()		const				{ return _memory.Capacity(); }
		ND_ constexpr usize MaxCapacity ()	const				{ return _memory.MaxSize(); }	// max available for allocation count of elements
		ND_ usize			Count ()		const				{ return _memory.Count(); }
		ND_ bool			Empty ()		const				{ return Count() <= 1; }

		void				Clear ()							{ _memory.Resize(1); }

		ND_ bool	operator == (ArrayCRef<T> right) const		{ return ArrayCRef<T>(*this) == right; }
		ND_ bool	operator != (ArrayCRef<T> right) const		{ return not ((*this) == right); }
		
		Self &		operator =  (Self &&right)		= default;
		Self &		operator =  (const Self &right)	= default;
		
		ND_ T const& operator [] (usize index) const			{ return _memory[index+1]; }
		
		ND_ const_iterator		begin ()			const		{ return _memory.begin() + 1; }	// skip default
		ND_ const_iterator		end ()				const		{ return _memory.end(); }

		static constexpr bool	IsLinearMemory ()				{ return Container_t::IsLinearMemory(); }
		constexpr bool			IsStaticMemory () const			{ return _memory.IsStatic(); }


		friend void SwapValues (INOUT Self &left, INOUT Self &right)
		{
			SwapValues( left._memory, right._memory );
		}
	};



	template <typename T, usize Count>
	using FixedSizeStack = Stack<T, typename AutoDetectCopyStrategy<T>::type, StaticMemoryContainer<T, Count> >;

	
/*
=================================================
	Pop
=================================================
*/
	template <typename T, typename S, typename MC>
	inline bool Stack<T,S,MC>::Pop ()
	{
		if ( _memory.Count() > 1 ) {
			_memory.PopBack();
			return true;
		}
		
		WARNING("stack is empty");
		return false;
	}
	
/*
=================================================
	Push
=================================================
*/
	template <typename T, typename S, typename MC>
	inline void Stack<T,S,MC>::Push ()
	{
		T	temp = _memory.Back();
		_memory.PushBack( RVREF( temp ) );
	}

	
/*
=================================================
	Hash
=================================================
*/
	template <typename T, typename S, typename MC>
	struct Hash< Stack<T,S,MC> >
	{
		ND_ HashResult  operator () (const Stack<T,S,MC> &x) const noexcept
		{
			return HashOf( ArrayCRef<T>( x ) );
		}
	};

}	// GXTypes
}	// GX_STL
