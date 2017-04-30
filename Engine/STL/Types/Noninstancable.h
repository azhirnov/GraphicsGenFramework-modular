// Copyright �  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "Engine/STL/Common/Types.h"

namespace GX_STL
{
namespace GXTypes
{

	//
	// Non-instancable base class
	//

	class Noninstancable
	{
	private:
		Noninstancable () = delete;

		//~Noninstancable () = delete;

		Noninstancable (const Noninstancable &) = delete;

		Noninstancable (Noninstancable &&) = delete;

		void operator = (const Noninstancable &) = delete;

		void operator = (Noninstancable &&) = delete;
	};


}	// GXTypes
}	// GX_STL
