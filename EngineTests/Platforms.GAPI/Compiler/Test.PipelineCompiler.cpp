// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "PApp.h"
	
extern void Test_PipelineCompiler (StringCRef device, bool debug)
{
	CHECK( OS::FileSystem::FindAndSetCurrentDir( "EngineTests/Platforms.GAPI/Compute" ) );

	#ifdef GRAPHICS_API_VULKAN
	{
		PApp	app;
		app.Initialize( "VK 1.0"_GAPI, device, debug );

		for (; app.Update();) {}

		app.Quit();
	}
	GetMainSystemInstance()->Send( ModuleMsg::Delete{} );
	#endif

	#if 0 //def GRAPHICS_API_OPENGL
	{
		PApp	app;
		app.Initialize( "GL 4.5"_GAPI, device, debug );

		for (; app.Update();) {}

		app.Quit();
	}
	GetMainSystemInstance()->Send( ModuleMsg::Delete{} );
	#endif
	
	#ifdef COMPUTE_API_OPENCL
	{
		PApp	app;
		app.Initialize( "CL 1.2"_GAPI, device, debug );

		for (; app.Update();) {}

		app.Quit();
	}
	GetMainSystemInstance()->Send( ModuleMsg::Delete{} );
	#endif
	
	#ifdef GRAPHICS_API_SOFT
	{
		PApp	app;
		app.Initialize( "SW 1.0"_GAPI, device, debug );

		for (; app.Update();) {}

		app.Quit();
	}
	GetMainSystemInstance()->Send( ModuleMsg::Delete{} );
	#endif
}
