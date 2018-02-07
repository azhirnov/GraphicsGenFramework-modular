// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "Engine/STL/OS/Posix/OSPosix.h"

#ifdef PLATFORM_BASE_POSIX

namespace GX_STL
{
namespace OS
{

	//
	// Random Device
	//

	struct PlatformRandomDevice final : public Noncopyable
	{
	// variables
	private:
		int		_fd;


	// methods
	public:
		PlatformRandomDevice (StringCRef provider);
		~PlatformRandomDevice ();

		template <typename T>
		bool Generate (T &value) const
		{
			return _Generate( (char *) &value, sizeof(value) );
		}

	private:
		bool _Create (StringCRef provider);
		bool _CreateDefault ();

		bool _Generate (char *ptr, usize size) const;
	};

	
}	// OS
}	// GX_STL

#endif	// PLATFORM_BASE_POSIX
