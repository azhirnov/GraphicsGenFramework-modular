// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "Core/Config/Engine.Config.h"

#ifdef GRAPHICS_API_SOFT

#include "Engine/Platforms/Public/GPU/Context.h"
#include "Engine/Platforms/Public/GPU/Thread.h"
#include "Engine/Platforms/Public/GPU/Pipeline.h"
#include "Engine/Platforms/Public/GPU/Buffer.h"
#include "Engine/Platforms/Public/GPU/CommandBuffer.h"
#include "Engine/Platforms/Public/GPU/Framebuffer.h"
#include "Engine/Platforms/Public/GPU/RenderPass.h"
#include "Engine/Platforms/Public/GPU/Sampler.h"
#include "Engine/Platforms/Public/GPU/Memory.h"
#include "Engine/Platforms/Soft/Impl/SWMessages.h"
#include "Engine/Platforms/Soft/SoftRendererObjectsConstructor.h"

namespace Engine
{
namespace Platforms
{

	//
	// Software Renderer Context
	//
	
	class SoftRendererContext final : public Module
	{
	// types
	private:
		using SupportedMessages_t	= MessageListFrom<
											ModuleMsg::AddToManager,
											ModuleMsg::RemoveFromManager,
											GpuMsg::GetGraphicsModules
										>;
		using SupportedEvents_t		= Module::SupportedEvents_t;

		using SWThreads_t			= Set< ModulePtr >;
		
		using SWThreadMsgList_t		= MessageListFrom<
											GpuMsg::ThreadBeginFrame,
											GpuMsg::ThreadEndFrame,
											GpuMsg::GetDeviceProperties,
											GpuMsg::GetSWDeviceInfo
										>;
		using SWThreadEventList_t	= MessageListFrom< GpuMsg::DeviceCreated, GpuMsg::DeviceBeforeDestroy >;


	// constants
	private:
		static const TypeIdList		_eventTypes;

		
	// variables
	private:
		CreateInfo::GpuContext	_createInfo;

		SWThreads_t				_threads;
		

	// methods
	public:
		SoftRendererContext (UntypedID_t, GlobalSystemsRef gs, const CreateInfo::GpuContext &ci);
		~SoftRendererContext ();

		
	// message handlers
	private:
		bool _AddToManager (const ModuleMsg::AddToManager &);
		bool _RemoveFromManager (const ModuleMsg::RemoveFromManager &);
		bool _GetGraphicsModules (const GpuMsg::GetGraphicsModules &);
	};
//-----------------------------------------------------------------------------


	
	const TypeIdList	SoftRendererContext::_eventTypes{ UninitializedT< SupportedEvents_t >() };

/*
=================================================
	constructor
=================================================
*/
	SoftRendererContext::SoftRendererContext (UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::GpuContext &ci) :
		Module( gs, ModuleConfig{ id, 1 }, &_eventTypes ),
		_createInfo( ci )
	{
		SetDebugName( "SoftRendererContext" );

		_SubscribeOnMsg( this, &SoftRendererContext::_OnModuleAttached_Impl );
		_SubscribeOnMsg( this, &SoftRendererContext::_OnModuleDetached_Impl );
		_SubscribeOnMsg( this, &SoftRendererContext::_AttachModule_Impl );
		_SubscribeOnMsg( this, &SoftRendererContext::_DetachModule_Impl );
		_SubscribeOnMsg( this, &SoftRendererContext::_FindModule_Empty );
		_SubscribeOnMsg( this, &SoftRendererContext::_ModulesDeepSearch_Empty );
		_SubscribeOnMsg( this, &SoftRendererContext::_Update_Empty );
		_SubscribeOnMsg( this, &SoftRendererContext::_Link_Impl );
		_SubscribeOnMsg( this, &SoftRendererContext::_Compose_Impl );
		_SubscribeOnMsg( this, &SoftRendererContext::_Delete_Impl );
		_SubscribeOnMsg( this, &SoftRendererContext::_AddToManager );
		_SubscribeOnMsg( this, &SoftRendererContext::_RemoveFromManager );
		_SubscribeOnMsg( this, &SoftRendererContext::_GetGraphicsModules );
		
		ASSERT( _ValidateMsgSubscriptions< SupportedMessages_t >() );
	}
	
/*
=================================================
	destructor
=================================================
*/
	SoftRendererContext::~SoftRendererContext ()
	{
		//LOG( "SoftRendererContext finalized", ELog::Debug );

		ASSERT( _threads.Empty() );
	}
	
/*
=================================================
	_AddToManager
=================================================
*/
	bool SoftRendererContext::_AddToManager (const ModuleMsg::AddToManager &msg)
	{
		CHECK_ERR( msg.module );
		CHECK_ERR( msg.module->SupportsAllMessages< SWThreadMsgList_t >() );
		CHECK_ERR( msg.module->GetSupportedEvents().HasAllTypes< SWThreadEventList_t >() );
		ASSERT( not _threads.IsExist( msg.module ) );

		_threads.Add( msg.module );
		return true;
	}
	
/*
=================================================
	_RemoveFromManager
=================================================
*/
	bool SoftRendererContext::_RemoveFromManager (const ModuleMsg::RemoveFromManager &msg)
	{
		CHECK_ERR( msg.module );

		ModulePtr	module = msg.module.Lock();

		if ( not module )
			return false;

		ASSERT( _threads.IsExist( module ) );

		_threads.Erase( module );
		return true;
	}
		
/*
=================================================
	_GetGraphicsModules
=================================================
*/	
	bool SoftRendererContext::_GetGraphicsModules (const GpuMsg::GetGraphicsModules &msg)
	{
		msg.result.Set({ SoftRendererObjectsConstructor::GetComputeModules(),
						 SoftRendererObjectsConstructor::GetGraphicsModules() });
		return true;
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	GetGraphicsModules
=================================================
*/	
	GraphicsModuleIDs SoftRendererObjectsConstructor::GetGraphicsModules ()
	{
		GraphicsModuleIDs	graphics;
		//graphics.pipeline		= SWGraphicsPipelineModuleID;
		//graphics.framebuffer	= SWFramebufferModuleID;
		//graphics.renderPass		= SWRenderPassModuleID;
		graphics.context		= SWContextModuleID;
		graphics.thread			= SWThreadModuleID;
		graphics.commandBuffer	= SWCommandBufferModuleID;
		graphics.commandBuilder	= SWCommandBuilderModuleID;
		graphics.buffer			= SWBufferModuleID;
		graphics.image			= SWImageModuleID;
		graphics.sampler		= SWSamplerModuleID;
		graphics.memory			= SWMemoryModuleID;
		graphics.resourceTable	= SWPipelineResourceTableModuleID;
		return graphics;
	}
	
/*
=================================================
	GetComputeModules
=================================================
*/	
	ComputeModuleIDs SoftRendererObjectsConstructor::GetComputeModules ()
	{
		ComputeModuleIDs	compute;
		compute.pipeline		= SWComputePipelineModuleID;
		compute.context			= SWContextModuleID;
		compute.thread			= SWThreadModuleID;
		compute.commandBuffer	= SWCommandBufferModuleID;
		compute.commandBuilder	= SWCommandBuilderModuleID;
		compute.buffer			= SWBufferModuleID;
		compute.image			= SWImageModuleID;
		compute.sampler			= SWSamplerModuleID;
		compute.memory			= SWMemoryModuleID;
		compute.resourceTable	= SWPipelineResourceTableModuleID;
		return compute;
	}

/*
=================================================
	Register
=================================================
*/
	void SoftRendererObjectsConstructor::Register ()
	{
		auto	mf = GetMainSystemInstance()->GlobalSystems()->modulesFactory;

		CHECK( mf->Register( SWContextModuleID, &CreateSoftRendererContext ) );
		CHECK( mf->Register( SWThreadModuleID, &CreateSoftRendererThread ) );
		
		CHECK( mf->Register( SWImageModuleID, &CreateSWImage ) );
		CHECK( mf->Register( SWMemoryModuleID, &CreateSWMemory ) );
		CHECK( mf->Register( SWBufferModuleID, &CreateSWBuffer ) );
		CHECK( mf->Register( SWSamplerModuleID, &CreateSWSampler ) );
		//CHECK( mf->Register( SWRenderPassModuleID, &CreateSWRenderPass ) );
		//CHECK( mf->Register( SWFramebufferModuleID, &CreateSWFramebuffer ) );
		CHECK( mf->Register( SWSyncManagerModuleID, &CreateSWSyncManager ) );
		CHECK( mf->Register( SWCommandQueueModuleID, &CreateSWCommandQueue ) );
		CHECK( mf->Register( SWCommandBufferModuleID, &CreateSWCommandBuffer ) );
		CHECK( mf->Register( SWCommandBuilderModuleID, &CreateSWCommandBuilder ) );
		CHECK( mf->Register( SWComputePipelineModuleID, &CreateSWComputePipeline ) );
		//CHECK( mf->Register( SWGraphicsPipelineModuleID, &CreateSWGraphicsPipeline ) );
		CHECK( mf->Register( SWPipelineResourceTableModuleID, &CreateSWPipelineResourceTable ) );
		
		if ( not mf->IsRegistered< CreateInfo::PipelineTemplate >( PipelineTemplateModuleID ) )
			CHECK( mf->Register( PipelineTemplateModuleID, &CreatePipelineTemplate ) );
	}
	
/*
=================================================
	Unregister
=================================================
*/
	void SoftRendererObjectsConstructor::Unregister ()
	{
		auto	mf = GetMainSystemInstance()->GlobalSystems()->modulesFactory;

		mf->UnregisterAll( SWContextModuleID );
		mf->UnregisterAll( SWThreadModuleID );
		
		mf->UnregisterAll( SWImageModuleID );
		mf->UnregisterAll( SWMemoryModuleID );
		mf->UnregisterAll( SWBufferModuleID );
		mf->UnregisterAll( SWSamplerModuleID );
		//mf->UnregisterAll( SWRenderPassModuleID );
		//mf->UnregisterAll( SWFramebufferModuleID );
		mf->UnregisterAll( SWSyncManagerModuleID );
		mf->UnregisterAll( SWCommandQueueModuleID );
		mf->UnregisterAll( SWCommandBufferModuleID );
		mf->UnregisterAll( SWCommandBuilderModuleID );
		//mf->UnregisterAll( SWGraphicsPipelineModuleID );
		mf->UnregisterAll( SWPipelineResourceTableModuleID );

		//mf->UnregisterAll< Platforms::PipelineTemplate >();	// TODO
	}

/*
=================================================
	CreateSoftRendererContext
=================================================
*/
	ModulePtr SoftRendererObjectsConstructor::CreateSoftRendererContext (ModuleMsg::UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::GpuContext &ci)
	{
		return New< SoftRendererContext >( id, gs, ci );
	}


}	// Platforms
}	// Engine

#endif	// GRAPHICS_API_SOFT
