// Copyright � 2014-2017  Zhirnov Andrey. All rights reserved.

#pragma once

#include "Engine/Platforms/Common/Common.h"

namespace Engine
{
namespace Platforms
{

	//
	// Multi Samples
	//
	
	struct MultiSamples
	{
	// types
	private:
		using Self	= MultiSamples;


	// variables
	private:
		PowOf2Value<uint>	_value;


	// methods
	public:
		MultiSamples (GX_DEFCTOR) : _value(0)
		{}

		explicit
		MultiSamples (PowOf2Value<uint> samples) : _value(samples)
		{}

		explicit
		MultiSamples (uint samples) : _value( PowOf2Value<uint>::From(samples) )
		{}

		uint Get ()			const		{ return _value.GetValue(); }
		uint GetPowerOf2 ()	const		{ return _value.GetPower(); }

		_GX_DIM_CMP_OPERATORS_SELF( _value )
	};


}	// Platforms
}	// Engine