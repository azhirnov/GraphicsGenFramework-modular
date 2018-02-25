// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "Engine/Platforms/Shared/OS/Display.h"
#include "Engine/Platforms/Shared/OS/Platform.h"

#ifdef PLATFORM_WINDOWS

namespace Engine
{
namespace PlatformWin
{
	using namespace Platforms;


	//
	// Windows Display Helper
	//

	struct WinDisplay final
	{
	// types
	private:
		using HMONITOR_t			= OS::HiddenOSTypeFrom<void*>;

		using Displays_t			= OSMsg::GetDisplays::Displays_t;
		using MonitorHandles_t		= FixedSizeArray< HMONITOR_t, Displays_t::MemoryContainer_t::SIZE >;


	// variables
	private:
		Displays_t			_displays;
		MonitorHandles_t	_monitors;


	// methods
	public:
		WinDisplay ();

		bool Update ();

		ArrayCRef<Display> GetDisplays () const		{ return _displays; }

	private:
		float2 _ScreenPhysicalSize () const;
		uint   _GetDisplayFrequency () const;
	};


}	// PlatformWin
}	// Engine

#endif	// PLATFORM_WINDOWS
