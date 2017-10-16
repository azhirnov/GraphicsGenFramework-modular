// Copyright �  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "Engine/Platforms/Shared/GPU/Thread.h"

#if defined( GRAPHICS_API_OPENGL )

#include "Engine/Platforms/Windows/WinMessages.h"
#include "Engine/Platforms/OpenGL/Impl/GL4Device.h"
#include "Engine/Platforms/OpenGL/Windows/GLWinContext.h"

namespace Engine
{
namespace Platforms
{

	//
	// OpenGL Thread
	//
	
	class OpenGLThread final : public Module
	{
	// types
	private:
		using SupportedMessages_t	= Module::SupportedMessages_t::Erase< MessageListFrom<
											ModuleMsg::Compose
										> >
										::Append< MessageListFrom<
											ModuleMsg::OnManagerChanged,
											ModuleMsg::WindowCreated,
											ModuleMsg::WindowBeforeDestroy,
											ModuleMsg::WindowDescriptorChanged
										> >;
		using SupportedEvents_t		= Module::SupportedEvents_t::Append< MessageListFrom<
											GpuMsg::DeviceCreated,
											GpuMsg::DeviceBeforeDestroy
										> >;

		using VideoSettings_t		= CreateInfo::GpuContext;
		using GLContext				= PlatformGL::GLRenderingContext;
		using GLDevice				= PlatformGL::GL4Device;


	// constants
	private:
		static const Runtime::VirtualTypeList	_msgTypes;
		static const Runtime::VirtualTypeList	_eventTypes;

		
	// variables
	private:
		VideoSettings_t		_settings;

		ModulePtr			_window;
		
		GLContext			_context;
		GLDevice			_device;


	// methods
	public:
		OpenGLThread (const GlobalSystemsRef gs, const CreateInfo::GpuThread &ci);
		~OpenGLThread ();
		
		Ptr< GLDevice >			GetDevice ()			{ return &_device; }


	// message handlers
	private:
		bool _Delete (const Message< ModuleMsg::Delete > &);
		bool _Link (const Message< ModuleMsg::Link > &);
		bool _Update (const Message< ModuleMsg::Update > &);
		
		bool _OnModuleAttached (const Message< ModuleMsg::OnModuleAttached > &);
		bool _OnModuleDetached (const Message< ModuleMsg::OnModuleDetached > &);
		bool _AttachModule (const Message< ModuleMsg::AttachModule > &);
		bool _DetachModule (const Message< ModuleMsg::DetachModule > &);

		bool _WindowCreated (const Message< ModuleMsg::WindowCreated > &);
		bool _WindowBeforeDestroy (const Message< ModuleMsg::WindowBeforeDestroy > &);
		bool _WindowDescriptorChanged (const Message< ModuleMsg::WindowDescriptorChanged > &);

	private:
		bool _CreateDevice (const ModuleMsg::WindowCreated &);
		void _DetachWindow ();

		static bool _IsWindow (const ModulePtr &mod);
	};


}	// Platforms
}	// Engine

#endif	// GRAPHICS_API_OPENGL
