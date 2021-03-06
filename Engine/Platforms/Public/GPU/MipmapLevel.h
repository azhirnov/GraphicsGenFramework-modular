// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "Engine/Platforms/Public/Common.h"

namespace Engine
{
namespace Platforms
{

	//
	// Mipmap Level
	//
	
	struct MipmapLevel final : CompileTime::PODStruct
	{
	// types
	private:
		using Self	= MipmapLevel;


	// variables
	private:
		uint		_value;


	// methods
	public:
		constexpr MipmapLevel (GX_DEFCTOR) : _value(0) {}

		explicit constexpr MipmapLevel (uint value) : _value(value) {}

		ND_ constexpr uint Get () const		{ return _value; }

		_GX_DIM_CMP_OPERATORS_SELF( _value )
	};

	
	ND_ inline constexpr MipmapLevel operator "" _mipmap (unsigned long long value)		{ return MipmapLevel( uint(value) ); }


	//
	// Mipmap Levels Range
	//
	/*
	struct MipmapLevelsRange
	{
	// types
	private:
		using Self	= MipmapLevelsRange;


	// variables
	private:
		uint2		_value;


	// methods
	public:
		MipmapLevelsRange (GX_DEFCTOR) : _value(0)
		{}

		explicit
		MipmapLevelsRange (uint2 value) : _value(value)
		{}

		uint2 Get () const		{ return _value; }
	};*/


}	// Platforms
}	// Engine

namespace GX_STL
{
namespace GXTypes
{
	template <>
	struct Hash< Engine::Platforms::MipmapLevel >
	{
		ND_ HashResult  operator () (const Engine::Platforms::MipmapLevel &value) const
		{
			return HashOf( value.Get() );
		}
	};

}	// GXTypes
}	// GX_STL
