// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "Engine/Platforms/Public/OS/OSEnums.h"
#include "Engine/Platforms/Public/OS/IDs.h"

namespace Engine
{
namespace CreateInfo
{

	//
	// Window Create Info
	//
	struct Window
	{
	// types
		struct EWindowFlags
		{
			enum type : uint
			{
				Fullscreen = 0,
				Resizable,
				Centered,
				_Count,
			};

			GX_ENUM_BITFIELD( EWindowFlags );
		};
		
		enum class EVisibility
		{
			VisibleFocused,		// visible and focused
			VisibleUnfocused,	// may be overlapped
			Invisible,			// minimized
		};

		using EWindowOrientation	= Platforms::EDisplayOrientation;


	// variables
		String						caption;
		EWindowFlags::bits			flags;
		GXMath::uint2				surfaceSize;
		GXMath::int2				position;
		EVisibility					initialVisibility;
		EWindowOrientation::type	orientation;

	// methods
		explicit
		Window (StringCRef					caption				= StringCRef(),
				EWindowFlags::bits			flags				= EWindowFlags::Resizable | EWindowFlags::Centered,
				GXMath::uint2				surfaceSize			= GXMath::uint2( 800, 600 ),
				GXMath::int2				position			= GXMath::int2(),
				EVisibility					initialVisibility	= EVisibility::VisibleFocused,
				EWindowOrientation::type	orientation			= EWindowOrientation::Default) :
			caption(caption),
			flags(flags),
			surfaceSize(surfaceSize),
			position(position),
			initialVisibility(initialVisibility),
			orientation(orientation)
		{}
	};

}	// CreateInfo


namespace Platforms
{

	//
	// Window Description
	//
	struct WindowDesc
	{
	// types
		using EWindowFlags			= CreateInfo::Window::EWindowFlags;
		using EWindowOrientation	= EDisplayOrientation;
		using EVisibility			= CreateInfo::Window::EVisibility;

	// variables
		String						caption;
		EWindowFlags::bits			flags;
		uint2						size;
		uint2						surfaceSize;
		int2						position;
		EVisibility					visibility		= EVisibility::VisibleFocused;
		EWindowOrientation::type	orientation		= EWindowOrientation::Default;
	};
	
}	// Platforms


namespace OSMsg
{

	//
	// Window events
	//
	struct WindowCreated : _MsgBase_
	{};

	struct WindowBeforeDestroy : _MsgBase_
	{};

	struct WindowAfterDestroy : _MsgBase_
	{};

	
	//
	// Set / Get / OnChanged Window Description
	//
	struct WindowSetDescription : _MsgBase_
	{
	// variables
		Platforms::WindowDesc			descr;

	// methods
		explicit WindowSetDescription (const Platforms::WindowDesc &descr) : descr{descr} {}
	};


	struct WindowGetDescription : _MsgBase_
	{
		Out< Platforms::WindowDesc >	result;
	};


	struct WindowDescriptionChanged : _MsgBase_
	{
	// variables
		Platforms::WindowDesc			descr;

	// methods
		explicit WindowDescriptionChanged (const Platforms::WindowDesc &descr) : descr{descr} {}
	};
	

	//
	// On Window Visibility / Focus Changed
	//
	struct WindowVisibilityChanged : _MsgBase_
	{
	// variables
		CreateInfo::Window::EVisibility		state;

	// methods
		explicit WindowVisibilityChanged (CreateInfo::Window::EVisibility state) : state{state} {}
	};


	//
	// Before Create Window
	//
	struct WindowBeforeCreate : _MsgBase_
	{
	// variables
		const     CreateInfo::Window		info;
		Editable< CreateInfo::Window >		editable;

	// methods
		WindowBeforeCreate () {}
		WindowBeforeCreate (const WindowBeforeCreate &msg) : info(msg.info), editable(msg.editable) {}
		explicit WindowBeforeCreate (const CreateInfo::Window &info) : info(info), editable(info) {}
	};


}	// OSMsg
}	// Engine
