// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "Engine/Graphics/Impl/GraphicsBaseModule.h"
#include "Engine/Platforms/Public/Tools/GPUThreadHelper.h"

namespace Engine
{
namespace Graphics
{

/*
=================================================
	constructor
=================================================
*/
	GraphicsBaseModule::GraphicsBaseModule (const GlobalSystemsRef gs,
											const ModuleConfig &config,
											const TypeIdList *msgTypes,
											const TypeIdList *eventTypes) :
		Module( gs, config, msgTypes, eventTypes )
	{}
	
/*
=================================================
	_OnManagerChanged
=================================================
*/
	bool GraphicsBaseModule::_OnManagerChanged (const Message< ModuleMsg::OnManagerChanged > &msg)
	{
		if ( msg->newManager )
			msg->newManager->Subscribe( this, &GraphicsBaseModule::_DeviceBeforeDestroy );
		
		if ( msg->oldManager )
			msg->oldManager->UnsubscribeAll( this );

		return true;
	}
	
/*
=================================================
	_DeviceBeforeDestroy
=================================================
*/
	bool GraphicsBaseModule::_DeviceBeforeDestroy (const Message< GpuMsg::DeviceBeforeDestroy > &)
	{
		_SendMsg< ModuleMsg::Delete >({});
		return true;
	}
	
/*
=================================================
	_GetGpuThread
=================================================
*/
	ModulePtr GraphicsBaseModule::_GetGpuThread (const ModulePtr &gpuThread) const
	{
		return gpuThread ? gpuThread : PlatformTools::GPUThreadHelper::FindGraphicsThread( GlobalSystems() );
	}

}	// Graphics
}	// Engine
