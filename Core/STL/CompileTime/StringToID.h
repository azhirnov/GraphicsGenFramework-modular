// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "Core/STL/CompileTime/TemplateMath.h"
#include "Core/STL/Types/Noninstancable.h"
#include "Core/STL/Containers/String.h"

namespace GX_STL
{
namespace CompileTime
{
	
	//
	// String To ID
	//
	namespace _ctime_hidden_
	{
		struct _StringToID_Helper
		{
			using type = ulong;

#			define _ON_ERROR( _msg_ )	(throw std::logic_error(_msg_))
//#			define _ON_ERROR( _msg_ )	(type(GXTypes::UMax))

		// constants
		private:
			static const uint	_MAX_OFFSET		= sizeof(type) * 8;
			static const uint	_SYMBOL_SIZE	= CompileTime::IntLog2< uint, ('Z' - 'A') > + 1;
			static const uint	_COUNT			= CompileTime::SizeOf<type>::bits / _SYMBOL_SIZE;
			static const uint	_OFFSET			= CompileTime::SizeOf<type>::bits % _SYMBOL_SIZE;
			static const type	_NUMERIC		= (1u << _SYMBOL_SIZE) - 1;
			static const uint	_STR_SIZE		= _COUNT + 1;
			static const type	_SYMBOL_MASK	= _NUMERIC;

			
		// methods
		private:
			forceinline static constexpr type _Hash1 (const char c, const size_t shift)
			{
				return
					(c >= 'A' and c <= 'Z')	? type(c - 'A' + 1) << shift				:	// map A-Z
					(c >= 'a' and c <= 'z') ? type(c - 'a' + 1) << shift				:	// map a-z
					(c == '.')				? type(c - '.' + 'Z' - 'A' + 2) << shift	:	// map .
					(c == '-')				? type(c - '-' + 'Z' - 'A' + 3) << shift	:	// map -
					(c == '+')				? type(c - '+' + 'Z' - 'A' + 4) << shift	:	// map +
					(c == '_')				? type(c - '_' + 'Z' - 'A' + 5) << shift	:	// map _
					(c == '\0' or c == ' ')	? 0											:
					type(GXTypes::UMax);
					//_ON_ERROR( "unsupported char" );
			}


			forceinline static constexpr type _Hash2 (const char c, const size_t shift)
			{
				return
					(c >= '0' and c <= '9')	? type(c - '0'  + 1) << shift				:	// map 0-9
					(c == '*')				? type(c - '*'  + '9' - '0' +  2) << shift	:	// map *
					(c == '/')				? type(c - '/'  + '9' - '0' +  3) << shift	:	// map /
					(c == '!')				? type(c - '!'  + '9' - '0' +  4) << shift	:	// map !
					(c == '?')				? type(c - '?'  + '9' - '0' +  5) << shift	:	// map ?
					(c == '~')				? type(c - '~'  + '9' - '0' +  6) << shift	:	// map ~
					(c == '"')				? type(c - '"'  + '9' - '0' +  7) << shift	:	// map "
					(c == '[')				? type(c - '['  + '9' - '0' +  8) << shift	:	// map [
					(c == ']')				? type(c - ']'  + '9' - '0' +  9) << shift	:	// map ]
					(c == '(')				? type(c - '('  + '9' - '0' + 10) << shift	:	// map (
					(c == ')')				? type(c - ')'  + '9' - '0' + 11) << shift	:	// map )
					(c == '\\')				? type(c - '\\' + '9' - '0' + 12) << shift	:	// map \ - backslash
					(c == '$')				? type(c - '$'  + '9' - '0' + 13) << shift	:	// map $
					(c == '&')				? type(c - '&'  + '9' - '0' + 14) << shift	:	// map &
					(c == '|')				? type(c - '|'  + '9' - '0' + 15) << shift	:	// map |
					(c == '^')				? type(c - '^'  + '9' - '0' + 16) << shift	:	// map ^
					(c == '<')				? type(c - '<'  + '9' - '0' + 17) << shift	:	// map <
					(c == '>')				? type(c - '>'  + '9' - '0' + 18) << shift	:	// map >
					(c == ':')				? type(c - ':'  + '9' - '0' + 19) << shift	:	// map :
					(c == ';')				? type(c - ';'  + '9' - '0' + 20) << shift	:	// map ;
					(c == '=')				? type(c - '='  + '9' - '0' + 21) << shift	:	// map =
					(c == '%')				? type(c - '%'  + '9' - '0' + 22) << shift	:	// map %
					(c == '\0' or c == ' ')	? 0											:
					type(GXTypes::UMax);
					//_ON_ERROR( "unsupported char" );
			}


			forceinline static char _RevHash1 (const ubyte hash)
			{
				const int	az_size = ('Z' - 'A');

				return
					(hash == 0)				? ' '			:
					(hash <= az_size + 1)	? 'A' + hash-1	:
					(hash == az_size + 2)	? '.'			:
					(hash == az_size + 3)	? '-'			:
					(hash == az_size + 4)	? '+'			:
					(hash == az_size + 5)	? '_'			:
					'#';
			}


			forceinline static char _RevHash2 (const ubyte hash)
			{
				const int	zn_size = ('9' - '0');

				return
					(hash == 0)				? ' '			:
					(hash <= zn_size + 1)	? '0' + hash-1	:
					(hash == zn_size + 2)	? '*'			:
					(hash == zn_size + 3)	? '/'			:
					(hash == zn_size + 4)	? '!'			:
					(hash == zn_size + 5)	? '?'			:
					(hash == zn_size + 6)	? '~'			:
					(hash == zn_size + 7)	? '"'			:
					(hash == zn_size + 8)	? '['			:
					(hash == zn_size + 9)	? ']'			:
					(hash == zn_size + 10)	? '('			:
					(hash == zn_size + 11)	? ')'			:
					(hash == zn_size + 12)	? '\\'			:
					(hash == zn_size + 13)	? '$'			:
					(hash == zn_size + 14)	? '&'			:
					(hash == zn_size + 15)	? '|'			:
					(hash == zn_size + 16)	? '^'			:
					(hash == zn_size + 17)	? '<'			:
					(hash == zn_size + 18)	? '>'			:
					(hash == zn_size + 19)	? ':'			:
					(hash == zn_size + 20)	? ';'			:
					(hash == zn_size + 21)	? '='			:
					(hash == zn_size + 22)	? '%'			:
					'#';
			}


			inline static constexpr type _RecursiveHash (char const * const str, const size_t i, const size_t off, const size_t size)
			{
				return
					(i >= size or str[i] == '\0') ?
						0 :

					// error
					(off >= _MAX_OFFSET) ?
						_ON_ERROR( "overflow" ) :

					// try map to laters
					(_Hash1( str[i], off ) != type(GXTypes::UMax)) ?
						(_Hash1( str[i], off ) | _RecursiveHash( str, i+1, off + _SYMBOL_SIZE, size )) :

					// try map to numerics
					(off + _SYMBOL_SIZE < _MAX_OFFSET and _Hash2( str[i], off + _SYMBOL_SIZE ) != type(GXTypes::UMax)) ?
						((_NUMERIC << off) | _Hash2( str[i], off + _SYMBOL_SIZE ) | _RecursiveHash( str, i+1, off + _SYMBOL_SIZE*2, size )) :

					// error
					_ON_ERROR( "unsupported char" );
			}

#			undef _ON_ERROR

			
		// methods
		public:
			using StString_t		= GXTypes::StaticString< _STR_SIZE >;

			static const type	MASK = (1u << _OFFSET) - 1;


			forceinline static constexpr type CalcHash (char const * const str, const size_t size, const type id) noexcept
			{
				return size > _COUNT ? type(GXTypes::UMax) : (_RecursiveHash( str, 0, _OFFSET, size ) | (id & MASK));
			}


			static StString_t ReverseHash (const type id) noexcept
			{
				StString_t	result;
				const type	hash	= (id >> _OFFSET);

				for (usize off = 0;
					 off < _MAX_OFFSET - _OFFSET and result.Length() < result.MaxCapacity();
					 off += _SYMBOL_SIZE)
				{
					const ubyte	sym = (hash >> off) & _SYMBOL_MASK;
					
					if ( sym == _NUMERIC )
					{
						off += _SYMBOL_SIZE;

						result << _RevHash2( (hash >> off) & _SYMBOL_MASK );
						continue;
					}

					result << _RevHash1( sym );
				}

				return result;
			}
		};

	}	// _ctime_hidden_


#	define DECL_STRING_TO_ID( _name_, _id_ ) \
		struct _name_ { \
			using _type_ = ::GX_STL::CompileTime::_ctime_hidden_::_StringToID_Helper::type; \
			\
			enum type : _type_ { \
				_IDMask	= ::GX_STL::CompileTime::_ctime_hidden_::_StringToID_Helper::MASK, \
				_ID		= (_id_ & _IDMask), \
				Unknown	= _type_(GXTypes::UMax), \
			}; \
			\
			ND_ forceinline static type  FromString (::GX_STL::GXTypes::StringCRef str) noexcept { \
				return type(::GX_STL::CompileTime::_ctime_hidden_::_StringToID_Helper:: \
								CalcHash( str.cstr(), str.Count(), _type_(_name_::_ID) )); \
			} \
			\
			ND_ forceinline static ::GX_STL::CompileTime::_ctime_hidden_::_StringToID_Helper::StString_t \
						ToString (type value) noexcept { \
				return ::GX_STL::CompileTime::_ctime_hidden_::_StringToID_Helper::ReverseHash( _type_(value) ); \
			} \
		}; \
		\
		ND_ forceinline constexpr _name_::type  operator "" _##_name_ (char const * const str, const size_t size) noexcept \
		{ \
			return _name_::type(::GX_STL::CompileTime::_ctime_hidden_::_StringToID_Helper:: \
											CalcHash( str, size, _name_::_type_(_name_::_ID) )); \
		} \

	
	/*
	Example:

		DECL_STRING_TO_ID( MsgID, 0 );

		switch ( MsgID::type(8) )
		{
			case MsgID::Unknown	:	break;	// if operator _MsgID returns error this case make compilation error
			case "update"_MsgID :	break;
			case "draw"_MsgID	:	break;
		}
	*/

}	// CompileTime

	DECL_STRING_TO_ID( StringToID, 0 );

}	// GX_STL
