// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "Core/STL/Containers/ArrayRef.h"

namespace GX_STL
{
namespace GXTypes
{
	namespace _types_hidden_
	{
		template <typename T>
		ND_ forceinline usize StrLength (const T *str) noexcept;

	}	// _types_hidden_

	typedef TStringRef< char >			StringRef;
	typedef TStringRef< wchar >			WStringRef;

	typedef TStringRef< const char >	StringCRef;
	typedef TStringRef< const wchar >	WStringCRef;



	//
	// String Wrapper
	//

	template <typename T>
	struct TStringRef : public CompileTime::FastCopyable
	{
	// types
	public:
		using Self		= TStringRef<T>;
		using Value_t	= T;
		using C			= typename TypeTraits::RemoveConst<T>;


	// variables
	private:
		union {
			T *					_memory;
			TMemoryViewer<T>	_memView;
		};
		usize					_count;


	// methods
	public:
		TStringRef (GX_DEFCTOR);
		TStringRef (T *pString);
		TStringRef (T *pString, usize length);
		TStringRef (void *pBegin, void *pEnd);

		TStringRef (const std::basic_string< C, std::char_traits<C>, std::allocator<C> > &str);

		std::basic_string< C, std::char_traits<C>, std::allocator<C> > ToStdString () const;

		TStringRef (Self &&other) = default;
		TStringRef (const Self &other) = default;

		ND_ T		&	operator [] (usize i);
		ND_ T const&	operator [] (usize i) const;
		
		Self &			operator =  (Self &&right)		= default;
		Self &			operator =  (const Self &right)	= default;

		ND_ bool		operator == (TStringRef<const T> right) const;
		ND_ bool		operator != (TStringRef<const T> right) const;
		ND_ bool		operator == (const T *right) const;
		ND_ bool		operator != (const T *right) const;
		
		ND_ bool		operator <  (TStringRef<const T> right) const;
		ND_ bool		operator >  (TStringRef<const T> right) const;
		ND_ bool		operator <= (TStringRef<const T> right) const;
		ND_ bool		operator >= (TStringRef<const T> right) const;

		ND_ friend bool	operator == (const T * left, const Self &right)	{ return right == left; }
		ND_ friend bool	operator != (const T * left, const Self &right)	{ return right != left; }
		
		ND_ friend bool	operator <  (const T * left, const Self &right)	{ return Self(left) < right; }
		ND_ friend bool	operator >  (const T * left, const Self &right)	{ return Self(left) > right; }
		ND_ friend bool	operator <= (const T * left, const Self &right)	{ return Self(left) <= right; }
		ND_ friend bool	operator >= (const T * left, const Self &right)	{ return Self(left) >= right; }

		ND_ operator	ArrayRef<T> ()					{ return ArrayRef<T>( _memory, Length() ); }
		ND_ operator	ArrayCRef<T> ()			const	{ return ArrayCRef<T>( _memory, Length() ); }
		ND_ operator	TStringRef<const T> ()	const	{ return TStringRef<const T>( _memory, Length() ); }

		ND_ T		*	ptr ();
		ND_ T const*	ptr ()					const;
		ND_ T const*	cstr ()					const	{ ASSERT( IsNullTerminated() ); return ptr(); }

		ND_ T const*	RawPtr ()				const	{ return _memory; }
		
		ND_ T		&	Back ()							{ return (*this)[ Length()-1 ]; }
		ND_ T const&	Back ()					const	{ return (*this)[ Length()-1 ]; }
		ND_ T		&	Front ()						{ return *ptr(); }
		ND_ T const&	Front ()				const	{ return *ptr(); }

		ND_ usize		Length ()				const	{ return _count - usize(_count > 0); }
		ND_ usize		Count ()				const	{ return Length(); }
		ND_ BytesU		Size ()					const	{ return SizeOf<T> * _count; }
		ND_ BytesU		LengthInBytes ()		const	{ return SizeOf<T> * Length(); }

		ND_ bool		Empty ()				const	{ return _count <= 1; }
		
		ND_ T		*	begin ()						{ return ptr(); }
		ND_ T const *	begin ()				const	{ return ptr(); }
		ND_ T		*	end ()							{ return ptr() + Length(); }
		ND_ T const *	end ()					const	{ return ptr() + Length(); }


		// TODO: rename
		ND_ bool  IsInString (const T &valueRef)const	{ return ArrayCRef<T>(*this).IsInArray( valueRef ); }
		ND_ usize GetIndex (const T &valueRef)	const	{ return ArrayCRef<T>(*this).GetIndex( valueRef ); }

		ND_ bool EqualsIC (TStringRef<const T> right)	const;

		ND_ bool StartsWith (TStringRef<const T> right) const;
		ND_ bool StartsWithIC (TStringRef<const T> right) const;

		ND_ bool EndsWith (TStringRef<const T> right) const;
		ND_ bool EndsWithIC (TStringRef<const T> right) const;

		ND_ bool HasChar (T right) const;
		ND_ bool HasCharIC (T right) const;

		ND_ bool HasSubString (TStringRef<const T> right) const;
		ND_ bool HasSubStringIC (TStringRef<const T> right) const;

		//bool NumericLess (TStringRef<const T> right) const;

		ND_ bool LessThan (TStringRef<const T> right) const;
		ND_ bool LessThanIC (TStringRef<const T> right) const;
		
		ND_ bool GreaterThan (TStringRef<const T> right) const;
		ND_ bool GreaterThanIC (TStringRef<const T> right) const;
			  
		ND_ bool EqualsInRange (TStringRef<const T> right, usize begin, usize end) const;
		ND_ bool EqualsInRangeIC (TStringRef<const T> right, usize begin, usize end) const;

		bool Find (TStringRef<const T> value, OUT usize &pos, usize start = 0) const;
		bool Find (const T value, OUT usize &pos, usize start = 0) const;

		bool FindIC (TStringRef<const T> value, OUT usize &pos, usize start = 0) const;
		bool FindIC (const T value, OUT usize &pos, usize start = 0) const;

		ND_ bool IsNullTerminated () const;

		// get range
		ND_ TStringRef<T>		SubString (usize pos, usize count = UMax);
		ND_ TStringRef<const T>	SubString (usize pos, usize count = UMax) const;
		
		ND_ TStringRef<T>		GetInterval (usize begin, usize end);
		ND_ TStringRef<const T>	GetInterval (usize begin, usize end) const;

		ND_ TStringRef<T>		GetInterval (const T* begin, const T* end);
		ND_ TStringRef<const T>	GetInterval (const T* begin, const T* end) const;


		ND_ bool				Intersects (TStringRef<const T> other) const;
		

		static constexpr bool	IsLinearMemory ()	{ return true; }
		static constexpr bool	IsStaticMemory ()	{ return false; }


		template <typename B, typename S, typename MC>
		ND_ static TStringRef<const T> From (const Array<B,S,MC> &arr);

		template <typename B>	ND_ static TStringRef<const T> From (ArrayRef<B> arr);
		template <typename B>	ND_ static TStringRef<const T> From (ArrayCRef<B> arr);

	private:
		static bool _Equals (const T *left, const T *right, usize length);
		static bool _EqualsIC (const T *left, const T *right, usize length);
		static T    _ToLower (T c);
	};
	
	
	
/*
=================================================
	operator "" _ref
=================================================
*/
	ND_ inline TStringRef<const char>		operator "" _ref (const char *str, size_t len)		{ return TStringRef<const char>( str, len ); }
	ND_ inline TStringRef<const wchar_t>	operator "" _ref (const wchar_t *str, size_t len)	{ return TStringRef<const wchar_t>( str, len ); }
	//inline TStringRef<const char16_t>	operator "" _ref (const char16_t *str, size_t len)	{ return TStringRef<const char16_t>( str, len ); }
	//inline TStringRef<const char32_t>	operator "" _ref (const char32_t *str, size_t len)	{ return TStringRef<const char32_t>( str, len ); }
	
/*
=================================================
	constructor
=================================================
*/
	template <typename T>
	inline TStringRef<T>::TStringRef (UninitializedType) :
		_memory(null), _count(0)
	{}
	
/*
=================================================
	constructor
=================================================
*/
	template <typename T>
	inline TStringRef<T>::TStringRef (T *pString) :
		_memory(pString), _count( pString == null ? 0 : _types_hidden_::StrLength(pString)+1 )
	{}
	
/*
=================================================
	constructor
=================================================
*/
	template <typename T>
	inline TStringRef<T>::TStringRef (T *pString, const usize length) :
		_memory(pString), _count( pString == null ? 0 : length+1 )
	{}
	
/*
=================================================
	constructor
=================================================
*/
	template <typename T>
	inline TStringRef<T>::TStringRef (void *pBegin, void *pEnd) :
		_memory( Cast<T *>(pBegin) ), _count( ( usize(pEnd) - usize(pBegin) ) / sizeof(T) )
	{}
	
/*
=================================================
	constructor
=================================================
*/
	template <typename T>
	inline TStringRef<T>::TStringRef (const std::basic_string< C, std::char_traits<C>, std::allocator<C> > &str) :
		_memory( Cast<T *>(str.data()) ), _count( str.empty() ? 0 : str.length()+1 )
	{}
	
/*
=================================================
	ToStdString
=================================================
*/
	template <typename T>
	inline std::basic_string< typename TStringRef<T>::C, std::char_traits< typename TStringRef<T>::C >, std::allocator< typename TStringRef<T>::C > >
		TStringRef<T>::ToStdString () const
	{
		return std::basic_string< C, std::char_traits<C>, std::allocator<C> >( ptr(), Length() );
	}
	
/*
=================================================
	operator []
=================================================
*/
	template <typename T>
	inline T & TStringRef<T>::operator [] (const usize i)
	{
		ASSUME( i < _count );
		return ptr()[i];
	}
	
	template <typename T>
	inline T const & TStringRef<T>::operator [] (const usize i) const
	{
		ASSUME( i < _count );
		return ptr()[i];
	}
		
/*
=================================================
	operator ==
=================================================
*/
	template <typename T>
	inline bool TStringRef<T>::operator == (TStringRef<const T> right) const
	{
		if ( Length() != right.Length() )
			return false;

		if ( _count == 0 or right.ptr() == _memory )
			return true;

		return _Equals( _memory, right.ptr(), Length() );
	}

	template <typename T>
	inline bool TStringRef<T>::operator == (const T *pStr) const
	{
		return ( *this == Self(pStr) );
	}
	
/*
=================================================
	operator !=
=================================================
*/
	template <typename T>
	inline bool TStringRef<T>::operator != (TStringRef<const T> right) const
	{
		return not ( *this == right );
	}

	template <typename T>
	inline bool TStringRef<T>::operator != (const T *pStr) const
	{
		return not ( *this == pStr );
	}
		
/*
=================================================
	operator <
=================================================
*/
	template <typename T>
	inline bool TStringRef<T>::operator < (TStringRef<const T> right) const
	{
		return (right > *this);
	}
		
/*
=================================================
	operator >
=================================================
*/
	template <typename T>
	inline bool TStringRef<T>::operator > (TStringRef<const T> right) const
	{
		typedef typename CompileTime::NearUInt::FromType<T>		UType;

		if ( Length() != right.Length() )
			return Length() > right.Length();

		for (usize i = 0; i < right.Length(); ++i)
		{
			if ( (*this)[i] != right[i] )
				return UType( (*this)[i] ) > UType( right[i] );
		}
		return false;
	}
	
/*
=================================================
	operator <=
=================================================
*/
	template <typename T>
	inline bool TStringRef<T>::operator <= (TStringRef<const T> right) const
	{
		return not ( *this > right );
	}
		
/*
=================================================
	operator >=
=================================================
*/
	template <typename T>
	inline bool TStringRef<T>::operator >= (TStringRef<const T> right) const
	{
		return not ( *this < right );
	}

/*
=================================================
	ptr
=================================================
*/
	template <typename T>
	inline T * TStringRef<T>::ptr ()
	{
		ASSUME( _memory != null );
		return _memory;
	}
	
	template <typename T>
	inline T const * TStringRef<T>::ptr () const
	{
		ASSUME( _memory != null );
		return _memory;
	}
	
/*
=================================================
	EqualsIC
=================================================
*/
	template <typename T>
	inline bool TStringRef<T>::EqualsIC (TStringRef<const T> right) const
	{
		if ( Length() != right.Length() )
			return false;

		return _EqualsIC( ptr(), right.ptr(), Length() );
	}
	
/*
=================================================
	StartsWith
=================================================
*/
	template <typename T>
	inline bool TStringRef<T>::StartsWith (TStringRef<const T> right) const
	{
		if ( Length() < right.Length() )
			return false;

		return _Equals( ptr(), right.ptr(), right.Length() );
	}
	
/*
=================================================
	StartsWithIC
=================================================
*/
	template <typename T>
	inline bool TStringRef<T>::StartsWithIC (TStringRef<const T> right) const
	{
		if ( Length() < right.Length() )
			return false;

		return _EqualsIC( ptr(), right.ptr(), right.Length() );
	}
	
/*
=================================================
	EndsWith
=================================================
*/
	template <typename T>
	inline bool TStringRef<T>::EndsWith (TStringRef<const T> right) const
	{
		if ( Length() < right.Length() )
			return false;

		return _Equals( ptr() + (Length() - right.Length()), right.ptr(), right.Length() );
	}
	
/*
=================================================
	EndsWithIC
=================================================
*/
	template <typename T>
	inline bool TStringRef<T>::EndsWithIC (TStringRef<const T> right) const
	{
		if ( Length() < right.Length() )
			return false;

		return _EqualsIC( ptr() + (Length() - right.Length()), right.ptr(), right.Length() );
	}
	
/*
=================================================
	HasChar
=================================================
*/
	template <typename T>
	inline bool TStringRef<T>::HasChar (const T right) const
	{
		usize	pos;
		return Find( right, OUT pos );
	}
	
/*
=================================================
	HasCharIC
=================================================
*/
	template <typename T>
	inline bool TStringRef<T>::HasCharIC (const T right) const
	{
		usize	pos;
		return FindIC( right, OUT pos );
	}
	
/*
=================================================
	HasSubString
=================================================
*/
	template <typename T>
	inline bool TStringRef<T>::HasSubString (TStringRef<const T> right) const
	{
		usize	pos;
		return Find( right, OUT pos );
	}
	
/*
=================================================
	HasSubStringIC
=================================================
*/
	template <typename T>
	inline bool TStringRef<T>::HasSubStringIC (TStringRef<const T> right) const
	{
		usize	pos;
		return FindIC( right, OUT pos );
	}
	
/*
=================================================
	_Equals
=================================================
*/
	template <typename T>
	inline bool TStringRef<T>::_Equals (const T *left, const T *right, const usize length)
	{
		for (usize i = 0; i < length; ++i) {
			if ( left[i] != right[i] )
				return false;
		}
		return true;
	}
	
/*
=================================================
	_EqualsIC
=================================================
*/
	template <typename T>
	inline bool TStringRef<T>::_EqualsIC (const T *left, const T *right, const usize length)
	{
		for (usize i = 0; i < length; ++i) {
			if ( _ToLower( left[i] ) != _ToLower( right[i] ) )
				return false;
		}
		return true;
	}
	
/*
=================================================
	_ToLower
=================================================
*/
	template <typename T>
	inline T TStringRef<T>::_ToLower (const T c)
	{
		return (c >= T('A') and c <= T('Z')) ? c + T('a' - 'A') : c;
	}

/*
=================================================
	Find
=================================================
*/
	template <typename T>
	inline bool  TStringRef<T>::Find (TStringRef<const T> value, OUT usize &pos, const usize start) const
	{
		if ( Empty() or value.Empty() )
			return false;

		usize	j = 0;

		for (usize i = start; i < Length(); ++i)
		{
			while ( value[j] == _memory[i+j] and i+j < _count-1 and j < value.Length() )
			{
				++j;
				if ( j >= value.Length() ) {
					pos = i;
					return true;
				}
			}
			j = 0;
		}
		return false;
	}
	
/*
=================================================
	Find
=================================================
*/
	template <typename T>
	inline bool  TStringRef<T>::Find (const T tValue, OUT usize &pos, const usize start) const
	{
		if ( Empty() )
			return false;

		for (usize i = start; i < Length(); ++i)
		{
			if ( _memory[i] == tValue ) {
				pos = i;
				return true;
			}
		}
		return false;
	}
	
/*
=================================================
	FindIC
=================================================
*/
	template <typename T>
	inline bool  TStringRef<T>::FindIC (TStringRef<const T> value, OUT usize &pos, const usize start) const
	{
		if ( Empty() or value.Empty() )
			return false;

		usize	j = 0;

		for (usize i = start; i < Length(); ++i)
		{
			while ( _ToLower( value[j] ) == _ToLower( _memory[i+j] ) and
					i+j < _count-1 and j < value.Length() )
			{
				++j;
				if ( j >= value.Length() ) {
					pos = i;
					return true;
				}
			}
			j = 0;
		}
		return false;
	}
	
/*
=================================================
	FindIC
=================================================
*/
	template <typename T>
	inline bool  TStringRef<T>::FindIC (const T tValue, OUT usize &pos, const usize start) const
	{
		if ( Empty() )
			return false;

		const T	val = _ToLower( tValue );

		for (usize i = start; i < Length(); ++i)
		{
			if ( _ToLower( _memory[i] ) == val ) {
				pos = i;
				return true;
			}
		}
		return false;
	}
	
/*
=================================================
	IsNullTerminated
=================================================
*/
	template <typename T>
	inline bool  TStringRef<T>::IsNullTerminated () const
	{
		return _memory == null or ( _count > 0 and (*this)[ _count-1 ] == T(0));
	}

/*
=================================================
	SubString
=================================================
*/
	template <typename T>
	inline TStringRef<T>  TStringRef<T>::SubString (const usize pos, usize count)
	{
		if ( pos >= _count )
			return TStringRef<T>();

		if ( (count >= _count) or (count + pos >= _count) )
			count = _count - pos - 1;

		return ( TStringRef<T>( _memory + pos, count ) );
	}
	
/*
=================================================
	SubString
=================================================
*/
	template <typename T>
	inline TStringRef<const T>  TStringRef<T>::SubString (const usize pos, usize count) const
	{
		if ( pos >= _count )
			return TStringRef<const T>();
		
		if ( (count >= _count) or (count + pos >= _count) )
			count = _count - pos - 1;

		return ( TStringRef<const T>( _memory + pos, count ) );
	}
	
/*
=================================================
	GetInterval
=================================================
*/
	template <typename T>
	inline TStringRef<T>  TStringRef<T>::GetInterval (const usize begin, const usize end)
	{
		ASSERT( begin < end );
		return SubString( begin, end - begin );
	}

	template <typename T>
	inline TStringRef<const T>  TStringRef<T>::GetInterval (const usize begin, const usize end) const
	{
		ASSERT( begin < end );
		return SubString( begin, end - begin );
	}
	
/*
=================================================
	GetInterval
=================================================
*/
	template <typename T>
	inline TStringRef<T>  TStringRef<T>::GetInterval (const T* begin, const T* end)
	{
		return GetInterval( GetIndex(*begin), GetIndex(*end) );
	}

	template <typename T>
	inline TStringRef<const T>  TStringRef<T>::GetInterval (const T* begin, const T* end) const
	{
		return GetInterval( GetIndex(*begin), GetIndex(*end) );
	}
	
/*
=================================================
	LessThan
=================================================
*/
	template <typename T>
	inline bool  TStringRef<T>::LessThan (TStringRef<const T> right) const
	{
		typedef typename CompileTime::NearUInt::FromType<T>		UType;

		const usize	min_length = GXMath::Min( Length(), right.Length() );

		for (usize i = 0; i < min_length; ++i)
		{
			const UType a = UType( (*this)[i] );
			const UType b = UType( right[i] );

			if ( a != b )
				return a < b;
		}
		return Length() < right.Length();
	}
	
/*
=================================================
	GreaterThan
=================================================
*/
	template <typename T>
	inline bool  TStringRef<T>::GreaterThan (TStringRef<const T> right) const
	{
		typedef typename CompileTime::NearUInt::FromType<T>		UType;

		const usize	min_length = GXMath::Min( Length(), right.Length() );

		for (usize i = 0; i < min_length; ++i)
		{
			const UType a = UType( (*this)[i] );
			const UType b = UType( right[i] );

			if ( a != b )
				return a > b;
		}
		return Length() > right.Length();
	}
	
/*
=================================================
	LessThanIC
=================================================
*/
	template <typename T>
	inline bool  TStringRef<T>::LessThanIC (TStringRef<const T> right) const
	{
		typedef typename CompileTime::NearUInt::FromType<T>		UType;

		const usize	min_length = GXMath::Min( Length(), right.Length() );

		for (usize i = 0; i < min_length; ++i)
		{
			const UType a = UType( _ToLower( (*this)[i] ) );
			const UType b = UType( _ToLower( right[i] ) );

			if ( a != b )
				return a < b;
		}
		return Length() < right.Length();
	}
	
/*
=================================================
	GreaterThanIC
=================================================
*/
	template <typename T>
	inline bool  TStringRef<T>::GreaterThanIC (TStringRef<const T> right) const
	{
		typedef typename CompileTime::NearUInt::FromType<T>		UType;

		const usize	min_length = GXMath::Min( Length(), right.Length() );

		for (usize i = 0; i < min_length; ++i)
		{
			const UType a = UType( _ToLower( (*this)[i] ) );
			const UType b = UType( _ToLower( right[i] ) );

			if ( a != b )
				return a > b;
		}
		return Length() > right.Length();
	}
	
/*
=================================================
	EqualsInRange
=================================================
*/
	template <typename T>
	inline bool  TStringRef<T>::EqualsInRange (TStringRef<const T> right, const usize begin, const usize end) const
	{
		if ( begin < end or Length() <= end or right.Length() <= end ) {
			WARNING( "invalid args" );
			return false;
		}

		return _Equals( ptr() + begin, right.ptr(), end - begin );
	}
	
/*
=================================================
	EqualsInRangeIC
=================================================
*/
	template <typename T>
	inline bool  TStringRef<T>::EqualsInRangeIC (TStringRef<const T> right, const usize begin, const usize end) const
	{
		if ( begin < end or Length() <= end or right.Length() <= end ) {
			WARNING( "invalid args" );
			return false;
		}

		return _EqualsIC( ptr() + begin, right.ptr(), end - begin );
	}
	
/*
=================================================
	Intersects
=================================================
*/
	template <typename T>
	inline bool  TStringRef<T>::Intersects (TStringRef<const T> other) const
	{
		ASSERT( begin() <= end() );
		ASSERT( other.begin() <= other.end() );
		return begin() > other.end() or end() < other.begin();
	}
	
/*
=================================================
	ArrayRef::From
=================================================
*/
	template <typename T>
	template <typename B>
	inline ArrayRef<T>  ArrayRef<T>::From (TStringRef<B> str)
	{
		return From( ArrayRef<B>( str ) );
	}
	
	template <typename T>
	template <typename B>
	inline ArrayRef<T>  ArrayRef<T>::FromStd (const std::basic_string< B, std::char_traits<B>, std::allocator<B> > &str)
	{
		return From( TStringRef<const B>( str ) );
	}
	
	template <typename T>
	template <typename B>
	inline ArrayRef<T>  ArrayRef<T>::FromStd (std::basic_string< B, std::char_traits<B>, std::allocator<B> > &str)
	{
		return From( TStringRef<B>( str ) );
	}
	
/*
=================================================
	From
=================================================
*/
	template <typename T>
	template <typename B>
	inline TStringRef<const T>  TStringRef<T>::From (ArrayRef<B> arr)
	{
		return From( ArrayCRef<B>(arr) );
	}

	template <typename T>
	template <typename B>
	inline TStringRef<const T>  TStringRef<T>::From (ArrayCRef<B> arr)
	{
		if ( not arr.Empty() )
			return TStringRef<const T>{ Cast<const T*>(arr.RawPtr()), usize(arr.Size() / SizeOf<T>) };
		else
			return TStringRef<const T>{};
	}

/*
=================================================
	StrLength
=================================================
*/
	namespace _types_hidden_
	{
		template <typename T>
		forceinline usize StrLength (const T *str) noexcept
		{
			usize	i = 0;

			while ( *str != 0 )
			{
				++str;
				++i;
			}
			return i;
		}

		template <>
		forceinline usize StrLength (const char *str) noexcept
		{
            return ::strlen( str );
		}

	}	// _types_hidden_

	
/*
=================================================
	Hash
=================================================
*/
	template <typename T>
	struct Hash< TStringRef<T> >
	{
		ND_ HashResult  operator () (const TStringRef<T> &x) const noexcept
		{
			return HashOf( ArrayCRef<T>( x ) );
		}
	};

}	// GXTypes
}	// GX_STL
