// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "Core/STL/Containers/Array.h"
#include "Core/STL/Containers/IndexedIterator.h"

namespace GX_STL
{
namespace GXTypes
{
	
	#define RET_ERROR( _ret_ )	{ WARNING("error in CircularQueue");  return _ret_; }
	#define RET_FALSE			RET_ERROR( false )
	#define RET_VOID			RET_ERROR( ; )



	//
	// Circular Queue
	//

	template <	typename T,
				typename S = typename AutoDetectCopyStrategy<T>::type,
				typename MC = MemoryContainer<T>
			 >
	struct CircularQueue : public CompileTime::CopyQualifiers< CompileTime::FastCopyable, MC >
	{
	// types
	public:
		using Strategy_t		= S;
		using MemoryContainer_t	= MC;
		using Value_t			= T;
		using Self				= CircularQueue<T,S,MC>;
		using iterator			= IndexedIterator<T>;
		using const_iterator	= IndexedIterator<const T>;


	// variables
	private:
		MemoryContainer_t	_memory;
		usize				_first,
							_end,		// last + 1
							_size;
		
		//  _____________ __________ _____________
		// | used memory | reserved | used memory |
		// 0           _end      _first        _size


	// methods
	private:
		void  _Reallocate (usize newSize, bool allowReserve);
		usize _WrapIndex (isize i) const;
		void  _Move (Self &&other);

		template <typename B>
		void _AppendFront (B* pArray, usize count);
		
		template <typename B>
		void _AppendBack (B* pArray, usize count);
		
		static bool  _CheckIntersection (const void *leftBegin, const void *leftEnd,
										 const void *rightBegin, const void *rightEnd);


	public:
		CircularQueue (GX_DEFCTOR);
		
		CircularQueue (const Self &other);

		CircularQueue (Self &&other);

		CircularQueue (ArrayCRef<T> other);

		~CircularQueue ()												{ Free(); }
		
		ND_ T		&	Front ();
		ND_ T const	&	Front () const;

		ND_ T		&	Back ();
		ND_ T const	&	Back () const;
		
		ND_ T		&	operator [] (usize i);
		ND_ T const	&	operator [] (usize i) const;
		
			Self &		operator =  (ArrayCRef<T> right)				{ Copy( right );					return *this; }
			Self &		operator =  (const Self &right)					{ Copy( right );					return *this; }
			Self &		operator =  (Self &&right)						{ Free();  _Move( RVREF(right) );	return *this; }

			Self &		operator << (const T& right)					{ PushBack( right );				return *this; }
			Self &		operator << (Self &&right)						{ AppendBack( RVREF( right ) );		return *this; }
			Self &		operator << (const Self &right)					{ AppendBack( right );				return *this; }
			Self &		operator << (ArrayCRef<T> right)				{ AppendBack( right );				return *this; }

		friend Self &	operator >> (const T& left, Self &right)		{ right.PushFront( left );				return right; }
		friend Self &	operator >> (Self &&left, Self &right)			{ right.AppendFront( RVREF( left ) );	return right; }
		friend Self &	operator >> (const Self &left, Self &right)		{ right.AppendFront( left );			return right; }
		friend Self &	operator >> (ArrayCRef<T> left, Self &right)	{ right.AppendFront( left );			return right; }
		
		ND_ bool		operator == (ArrayCRef<T> right)	const;
		ND_ bool		operator == (const Self &right)		const;
		ND_ bool		operator != (ArrayCRef<T> right)	const		{ return not ( (*this) == right ); }
		ND_ bool		operator != (const Self &right)		const		{ return not ( (*this) == right ); }

		void PushBack (const T &value);
		void PushBack (T&& value);

		void PushFront (const T &value);
		void PushFront (T&& value);
		
		void PopBack ();
		void PopFront ();
		
		void AppendFront (ArrayCRef<T> value);
		void AppendFront (const Self &value);
		void AppendFront (Self &&value);

		void AppendBack (ArrayCRef<T> value);
		void AppendBack (const Self &value);
		void AppendBack (Self &&value);

		void Copy (ArrayCRef<T> other);
		void Copy (const Self &other);
		
		void Resize (usize newSize, bool allowReserve = true);
		void Reserve (usize size);

		void Free ();
		void Clear ();
		
		void GetParts (OUT ArrayRef<T> &part0, OUT ArrayRef<T> &part1);
		void GetParts (OUT ArrayCRef<T> &part0, OUT ArrayCRef<T> &part1) const;

		ND_ usize			Count ()		const;
		ND_ BytesU			Size ()			const				{ return SizeOf<T> * Count(); }
		ND_ usize			Capacity ()		const				{ return _size-1; }
		ND_ constexpr usize MaxCapacity ()	const				{ return _memory.MaxSize(); }	// max available for allocation count of elements
		ND_ BytesU			FullSize ()		const				{ return SizeOf<T> * Capacity(); }
		ND_ bool			Empty ()		const				{ return _first == _end; }
		ND_ usize			LastIndex ()	const				{ return Count()-1; }
		
		ND_ auto			begin ()							{ return IndexedIterator<T>{ *this, 0 }; }
		ND_ auto			begin ()		const				{ return IndexedIterator<const T>{ *this, 0 }; }
		ND_ auto			end ()								{ return IndexedIterator<T>{ *this, Count() }; }
		ND_ auto			end ()			const				{ return IndexedIterator<const T>{ *this, Count() }; }


		static constexpr bool	IsLinearMemory ()				{ return false; }
		constexpr bool			IsStaticMemory ()	const		{ return _memory.IsStatic(); }

		friend void SwapValues (INOUT Self &left, INOUT Self &right)
		{
			left._memory.SwapMemory( right._memory );
		
			SwapValues( left._first,	right._first );
			SwapValues( left._end,		right._end );
			SwapValues( left._size,		right._size );
		}
	};
	
	

	template <typename T, usize Size>
	using FixedSizeCircularQueue = CircularQueue< T, typename AutoDetectCopyStrategy<T>::type, StaticMemoryContainer<T, Size> >;

	
/*
=================================================
	constructor
=================================================
*/
	template <typename T, typename S, typename MC>
	inline CircularQueue<T,S,MC>::CircularQueue (UninitializedType) :
		_first(0), _end(0), _size(0)
	{}
	
/*
=================================================
	constructor
=================================================
*/
	template <typename T, typename S, typename MC>
	inline CircularQueue<T,S,MC>::CircularQueue (const Self &other) :
		_first(0), _end(0), _size(0)
	{
		Copy( other );
	}
	
/*
=================================================
	constructor
=================================================
*/
	template <typename T, typename S, typename MC>
	inline CircularQueue<T,S,MC>::CircularQueue (Self &&other)
	{
		_Move( RVREF( other ) );
	}
	
/*
=================================================
	constructor
=================================================
*/
	template <typename T, typename S, typename MC>
	inline CircularQueue<T,S,MC>::CircularQueue (ArrayCRef<T> other) :
		_first(0), _end(0), _size(0)
	{
		Copy( other );
	}
	
/*
=================================================
	_Move
=================================================
*/
	template <typename T, typename S, typename MC>
	inline void CircularQueue<T,S,MC>::_Move (Self &&other)
	{
		_memory	= RVREF( other._memory );
		_first	= other._first;
		_end	= other._end;
		_size	= other._size;

		other._first = other._end = other._size = 0;
	}
	
/*
=================================================
	_Reallocate
=================================================
*/
	template <typename T, typename S, typename MC>
	inline void  CircularQueue<T,S,MC>::_Reallocate (const usize newSize, const bool allowReserve)
	{
		if ( newSize <= _size )
			return;
		
		// save old values
		const usize			old_size	= _size;
		usize				old_count	= Count();
		MemoryContainer_t	old_memcont;	old_memcont.MoveFrom( INOUT _memory );

		_size = newSize;

		CHECK( _memory.Allocate( INOUT _size, allowReserve ) );
		old_count = GXMath::Min( old_count, _size );

		if ( old_memcont.Pointer() == null or old_count == 0 )
		{
			_first = _end = 0;
			return;
		}

		// replace
		if ( _first > _end )
		{
			usize	off = old_size - _first;
			Strategy_t::Replace( _memory.Pointer(), old_memcont.Pointer() + _first, off );
			Strategy_t::Replace( _memory.Pointer() + off, old_memcont.Pointer(), _end );
		}
		else
		{
			Strategy_t::Replace( _memory.Pointer(), old_memcont.Pointer() + _first, _end - _first );
		}
		
		_first = 0;
		_end   = old_count;
	}
	
/*
=================================================
	_CheckIntersection
=================================================
*/
	template <typename T, typename S, typename MC>
	inline bool CircularQueue<T,S,MC>::_CheckIntersection (const void *leftBegin, const void *leftEnd,
															const void *rightBegin, const void *rightEnd)
	{
		return CheckPointersAliasing( leftBegin, leftEnd, rightBegin, rightEnd );
	}
	
/*
=================================================
	_WrapIndex
=================================================
*/
	template <typename T, typename S, typename MC>
	inline usize CircularQueue<T,S,MC>::_WrapIndex (const isize i) const
	{
		return ( i < 0 ? _size + i : ( i >= isize(_size) ? i - _size : i ) );
	}
	
/*
=================================================
	Count
=================================================
*/
	template <typename T, typename S, typename MC>
	inline usize  CircularQueue<T,S,MC>::Count () const
	{
		return	_first <= _end ?
				( _end - _first ) :
				( _size - _first + _end );
	}
	
/*
=================================================
	Front
=================================================
*/
	template <typename T, typename S, typename MC>
	inline T & CircularQueue<T,S,MC>::Front ()
	{
		ASSERT( not Empty() );
		return _memory.Pointer()[ _first ];
	}
	
	template <typename T, typename S, typename MC>
	inline T const & CircularQueue<T,S,MC>::Front () const
	{
		ASSERT( not Empty() );
		return _memory.Pointer()[ _first ];
	}
	
/*
=================================================
	Back
=================================================
*/
	template <typename T, typename S, typename MC>
	inline T & CircularQueue<T,S,MC>::Back ()
	{
		ASSERT( not Empty() );
		return _memory.Pointer()[ _WrapIndex( _end - 1 ) ];
	}
	
	template <typename T, typename S, typename MC>
	inline T const & CircularQueue<T,S,MC>::Back () const
	{
		ASSERT( not Empty() );
		return _memory.Pointer()[ _WrapIndex( _end - 1 ) ];
	}
		
/*
=================================================
	operator []
=================================================
*/
	template <typename T, typename S, typename MC>
	inline T & CircularQueue<T,S,MC>::operator [] (const usize i)
	{
		ASSERT( i < Count() );
		return _memory.Pointer()[ _WrapIndex( i + _first ) ];
	}
	
	template <typename T, typename S, typename MC>
	inline T const & CircularQueue<T,S,MC>::operator [] (const usize i) const
	{
		ASSERT( i < Count() );
		return _memory.Pointer()[ _WrapIndex( i + _first ) ];
	}
		
/*
=================================================
	PushBack
=================================================
*/
	template <typename T, typename S, typename MC>
	inline void CircularQueue<T,S,MC>::PushBack (const T &value)
	{
		usize cnt = Count();

		if ( cnt+2 >= _size )
			_Reallocate( _size+1, true );
		
		Strategy_t::Copy( _memory.Pointer() + _end, AddressOf(value), 1 );
		_end = _WrapIndex( _end + 1 );
	}
	
/*
=================================================
	PushBack
=================================================
*/
	template <typename T, typename S, typename MC>
	inline void CircularQueue<T,S,MC>::PushBack (T&& value)
	{
		if ( Count()+2 >= _size )
			_Reallocate( _size+1, true );
		
		Strategy_t::Move( _memory.Pointer() + _end, AddressOf(value), 1 );
		_end = _WrapIndex( _end + 1 );
	}
	
/*
=================================================
	PushFront
=================================================
*/
	template <typename T, typename S, typename MC>
	inline void CircularQueue<T,S,MC>::PushFront (const T &value)
	{
		if ( Count()+2 >= _size )
			_Reallocate( _size+1, true );
		
		_first = _WrapIndex( _first - 1 );
		Strategy_t::Copy( _memory.Pointer() + _first, AddressOf(value), 1 );
	}
	
/*
=================================================
	PushFront
=================================================
*/
	template <typename T, typename S, typename MC>
	inline void CircularQueue<T,S,MC>::PushFront (T&& value)
	{
		if ( Count()+2 >= _size )
			_Reallocate( _size+1, true );

		_first = _WrapIndex( _first - 1 );
		Strategy_t::Move( _memory.Pointer() + _first, AddressOf(value), 1 );
	}
		
/*
=================================================
	PopBack
=================================================
*/
	template <typename T, typename S, typename MC>
	inline void CircularQueue<T,S,MC>::PopBack ()
	{
		if ( not Empty() )
		{
			_end = _WrapIndex( _end - 1 );
			Strategy_t::Destroy( _memory.Pointer() + _end, 1 );
		}
	}
	
/*
=================================================
	PopFront
=================================================
*/
	template <typename T, typename S, typename MC>
	inline void CircularQueue<T,S,MC>::PopFront ()
	{
		if ( not Empty() )
		{
			Strategy_t::Destroy( _memory.Pointer() + _first, 1 );
			_first = _WrapIndex( _first + 1 );
		}
	}
	
/*
=================================================
	_AppendFront
=================================================
*/
	template <typename T, typename S, typename MC>
	template <typename B>
	inline void CircularQueue<T,S,MC>::_AppendFront (B *pArray, const usize count)
	{
		if ( pArray == null or count == 0 )
			return;

		if ( not Empty() and _CheckIntersection( _memory.Pointer(), _memory.Pointer() + _size, pArray, pArray + count ) )
			RET_VOID;

		if ( Count() + count + 1 >= _size )
			_Reallocate( _size + count + 1, true );

		if ( Empty() )
		{
			if_constexpr( TypeTraits::IsConst<B> )	Strategy_t::Copy( _memory.Pointer(), pArray, count );
			else									Strategy_t::Move( _memory.Pointer(), const_cast<T*>(pArray), count );

			_first = 0;
			_end   = count;
		}
		else
		if ( _first < _end )
		{
			const usize	cnt = GXMath::Min( count, _first );

			if_constexpr( TypeTraits::IsConst<B> )	Strategy_t::Copy( _memory.Pointer() + _first - cnt, pArray + count - cnt, cnt );
			else									Strategy_t::Move( _memory.Pointer() + _first - cnt, const_cast<T*>(pArray) + count - cnt, cnt );

			_first -= cnt;
			
			if ( cnt != count )
			{
				_first = _size - (count - cnt);

				if_constexpr( TypeTraits::IsConst<B> )	Strategy_t::Copy( _memory.Pointer() + _first, pArray, count - cnt );
				else									Strategy_t::Move( _memory.Pointer() + _first, const_cast<T*>(pArray), count - cnt );
			}
		}
		else
		{
			if_constexpr( TypeTraits::IsConst<B> )	Strategy_t::Copy( _memory.Pointer() + _first - count, pArray, count );
			else									Strategy_t::Move( _memory.Pointer() + _first - count, const_cast<T*>(pArray), count );

			_first -= count;
		}
	}
	
/*
=================================================
	_AppendBack
=================================================
*/
	template <typename T, typename S, typename MC>
	template <typename B>
	inline void CircularQueue<T,S,MC>::_AppendBack (B *pArray, const usize count)
	{
		if ( pArray == null or count == 0 )
			return;

		if ( not Empty() and _CheckIntersection( _memory.Pointer(), _memory.Pointer() + _size, pArray, pArray + count ) )
			RET_VOID;

		if ( Count() + count + 1 >= _size )
			_Reallocate( _size + count + 1, true );

		if ( Empty() )
		{
			if_constexpr( TypeTraits::IsConst<B> )	Strategy_t::Copy( _memory.Pointer(), pArray, count );
			else									Strategy_t::Move( _memory.Pointer(), const_cast<T*>(pArray), count );

			_first = 0;
			_end   = count;
		}
		else
		if ( _first < _end )
		{
			const usize	cnt = GXMath::Min( count, _size - _end );

			if_constexpr( TypeTraits::IsConst<B> )	Strategy_t::Copy( _memory.Pointer() + _end, pArray, cnt );
			else									Strategy_t::Move( _memory.Pointer() + _end, const_cast<T*>(pArray), cnt );

			_end += cnt;
			
			if ( cnt != count )
			{
				_end = count - cnt;

				if_constexpr( TypeTraits::IsConst<B> )	Strategy_t::Copy( _memory.Pointer(), pArray + cnt, count - cnt );
				else									Strategy_t::Move( _memory.Pointer(), const_cast<T*>(pArray) + cnt, count - cnt );
			}
		}
		else
		{
			if_constexpr( TypeTraits::IsConst<B> )	Strategy_t::Copy( _memory.Pointer() + _end, pArray, count );
			else									Strategy_t::Move( _memory.Pointer() + _end, const_cast<T*>(pArray), count );

			_end += count;
		}
	}
	
/*
=================================================
	AppendFront
=================================================
*/
	template <typename T, typename S, typename MC>
	inline void CircularQueue<T,S,MC>::AppendFront (ArrayCRef<T> value)
	{
		return _AppendFront<const T>( value.RawPtr(), value.Count() );
	}
		
/*
=================================================
	AppendFront
=================================================
*/
	template <typename T, typename S, typename MC>
	inline void CircularQueue<T,S,MC>::AppendFront (const Self &value)
	{
		ArrayCRef<T>	part0;
		ArrayCRef<T>	part1;
		value.GetParts( part0, part1 );

		AppendFront( part1 );
		AppendFront( part0 );
	}
	
/*
=================================================
	AppendFront
=================================================
*/
	template <typename T, typename S, typename MC>
	inline void CircularQueue<T,S,MC>::AppendFront (Self &&value)
	{
		ArrayRef<T>	part0;
		ArrayRef<T>	part1;
		value.GetParts( part0, part1 );

		_AppendFront<T>( part1.RawPtr(), part1.Count() );
		_AppendFront<T>( part0.RawPtr(), part0.Count() );
	}
	
/*
=================================================
	AppendBack
=================================================
*/
	template <typename T, typename S, typename MC>
	inline void CircularQueue<T,S,MC>::AppendBack (ArrayCRef<T> value)
	{
		return _AppendBack<const T>( value.RawPtr(), value.Count() );
	}
	
/*
=================================================
	AppendBack
=================================================
*/
	template <typename T, typename S, typename MC>
	inline void CircularQueue<T,S,MC>::AppendBack (const Self &value)
	{
		ArrayCRef<T>	part0;
		ArrayCRef<T>	part1;
		value.GetParts( part0, part1 );

		AppendBack( part1 );
		AppendBack( part0 );
	}
		
/*
=================================================
	AppendBack
=================================================
*/
	template <typename T, typename S, typename MC>
	inline void CircularQueue<T,S,MC>::AppendBack (Self &&value)
	{
		ArrayRef<T>	part0;
		ArrayRef<T>	part1;
		value.GetParts( part0, part1 );
		
		_AppendBack<T>( part1.RawPtr(), part1.Count() );
		_AppendBack<T>( part0.RawPtr(), part0.Count() );
	}
	
/*
=================================================
	Reserve
=================================================
*/
	template <typename T, typename S, typename MC>
	inline void CircularQueue<T,S,MC>::Reserve (const usize size)
	{
		if ( size <= _size )
			return;

		_Reallocate( size, false );
	}
	
/*
=================================================
	Resize
=================================================
*/
	template <typename T, typename S, typename MC>
	inline void CircularQueue<T,S,MC>::Resize (const usize newSize, const bool allowReserve)
	{
		const usize	old_count = Count();
		
		if ( newSize == old_count )
			return;
		
		if ( newSize == 0 )
		{
			Clear();
			return;
		}
		
		if ( newSize < old_count )
		{
			const usize	first_del = _WrapIndex( _first + newSize );

			if ( first_del > _end )
			{
				Strategy_t::Destroy( _memory.Pointer() + first_del, _size - first_del );
				Strategy_t::Destroy( _memory.Pointer(), _end );
			}
			else
			{
				Strategy_t::Destroy( _memory.Pointer() + first_del, old_count - newSize );
			}

			_end = first_del;
			return;
		}

		_Reallocate( newSize, allowReserve );
		_end = _WrapIndex( _first + newSize );

		const usize	first_uninit = _WrapIndex( _first + old_count );
		
		if ( first_uninit > _end )
		{
			Strategy_t::Create( _memory.Pointer() + first_uninit, _size - first_uninit );
			Strategy_t::Create( _memory.Pointer(), _end );
		}
		else
		{
			Strategy_t::Create( _memory.Pointer() + first_uninit, newSize - old_count );
		}
	}
	
/*
=================================================
	operator ==
=================================================
*/
	template <typename T, typename S, typename MC>
	inline bool CircularQueue<T,S,MC>::operator == (ArrayCRef<T> right) const
	{
		if ( Count() != right.Count() )
			return false;

		FOR( i, right )
		{
			if ( not ( (*this)[i] == right[i] ) )
				return false;
		}
		return true;
	}
	
/*
=================================================
	operator ==
=================================================
*/
	template <typename T, typename S, typename MC>
	inline bool CircularQueue<T,S,MC>::operator == (const Self &right) const
	{
		if ( Count() != right.Count() )
			return false;

		FOR( i, right )
		{
			if ( not ( (*this)[i] == right[i] ) )
				return false;
		}
		return true;
	}
	
/*
=================================================
	Free
=================================================
*/
	template <typename T, typename S, typename MC>
	inline void CircularQueue<T,S,MC>::Free ()
	{
		if ( _memory.Pointer() != null )
		{
			if ( Empty() )
			{}
			else
			if ( _first > _end )
			{
				Strategy_t::Destroy( _memory.Pointer() + _first, _size - _first );
				Strategy_t::Destroy( _memory.Pointer(), _end );
			}
			else
			{
				Strategy_t::Destroy( _memory.Pointer() + _first, _end - _first );
			}
			_memory.Deallocate();
		}
		_first	= _end = 0;
		_size	= 0;
	}
	
/*
=================================================
	Clear
=================================================
*/
	template <typename T, typename S, typename MC>
	inline void CircularQueue<T,S,MC>::Clear ()
	{
		if ( _memory.Pointer() != null )
		{
			if ( Empty() )
			{}
			else
			if ( _first > _end )
			{
				Strategy_t::Destroy( _memory.Pointer() + _first, _size - _first );
				Strategy_t::Destroy( _memory.Pointer(), _end );
			}
			else
			{
				Strategy_t::Destroy( _memory.Pointer() + _first, Count() );
			}
		}
		_first = _end = 0;
	}
	
/*
=================================================
	Copy
=================================================
*/
	template <typename T, typename S, typename MC>
	inline void CircularQueue<T,S,MC>::Copy (const Self &other)
	{
		Clear();
		AppendBack( other );
	}
	
/*
=================================================
	Copy
=================================================
*/
	template <typename T, typename S, typename MC>
	inline void CircularQueue<T,S,MC>::Copy (ArrayCRef<T> other)
	{
		Clear();
		AppendBack( other );
	}
	
/*
=================================================
	GetParts
=================================================
*/
	template <typename T, typename S, typename MC>
	inline void CircularQueue<T,S,MC>::GetParts (OUT ArrayRef<T> &part0, OUT ArrayRef<T> &part1)
	{
		if ( Empty() )
		{
			part0 = ArrayRef<T>();
			part1 = ArrayRef<T>();
		}
		else
		if ( _first > _end )
		{	
			part0 = ArrayRef<T>( _memory.Pointer() + _first, _size - _first );
			part1 = ArrayRef<T>( _memory.Pointer(), _end );
		}
		else
		{
			part0 = ArrayRef<T>( _memory.Pointer() + _first, Count() );
			part1 = ArrayRef<T>();
		}
	}
	
/*
=================================================
	GetParts
=================================================
*/
	template <typename T, typename S, typename MC>
	inline void CircularQueue<T,S,MC>::GetParts (OUT ArrayCRef<T> &part0, OUT ArrayCRef<T> &part1) const
	{
		if ( Empty() )
		{
			part0 = ArrayCRef<T>();
			part1 = ArrayCRef<T>();
		}
		else
		if ( _first > _end )
		{	
			part0 = ArrayCRef<T>( _memory.Pointer() + _first, _size - _first );
			part1 = ArrayCRef<T>( _memory.Pointer(), _end );
		}
		else
		{
			part0 = ArrayCRef<T>( _memory.Pointer() + _first, Count() );
			part1 = ArrayCRef<T>();
		}
	}

	#undef  RET_ERROR
	#undef	RET_FALSE
	#undef  RET_VOID
	
	
/*
=================================================
	Hash
=================================================
*/
	template <typename T, typename S, typename MC>
	struct Hash< CircularQueue<T,S,MC> >
	{
		ND_ HashResult  operator () (const CircularQueue<T,S,MC> &x) const noexcept
		{
			ArrayCRef<T>			part0;
			ArrayCRef<T>			part1;
			Hash< ArrayCRef<T> >    hasher;

			x.GetParts( part0, part1 );

			return hasher( part0 ) + hasher( part1 );
		}
	};

}	// GXTypes
}	// GX_STL
