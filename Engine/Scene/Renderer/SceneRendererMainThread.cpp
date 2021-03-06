// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "Engine/Scene/Public/Scene.h"
#include "Engine/Scene/Impl/SceneObjectConstructor.h"
#include "Engine/Scene/Impl/BaseSceneModule.h"
#include "Engine/Platforms/Public/Tools/GPUThreadHelper.h"

namespace Engine
{
namespace Scene
{

	//
	// Scene Renderer Main Thread
	//

	class SceneRendererMainThread final : public Module
	{
	// types
	protected:
		using SupportedMessages_t	= MessageListFrom<
											SceneMsg::GetScenePrivateClasses
										>;
		
		using SupportedEvents_t		= Module::SupportedEvents_t::Append< MessageListFrom<
											GpuMsg::DeviceCreated,
											GpuMsg::DeviceBeforeDestroy
										> >;

		using CmdBufferMngrMsg_t	= MessageListFrom<
											GraphicsMsg::CmdBeginFrame,
											GraphicsMsg::CmdEndFrame,
											GraphicsMsg::CmdBegin,
											GraphicsMsg::CmdEnd >;


	// constants
	private:
		static const TypeIdList		_eventTypes;


	// variables
	private:


	// methods
	public:
		SceneRendererMainThread (UntypedID_t, GlobalSystemsRef, const CreateInfo::SceneRenderer &);
		~SceneRendererMainThread ();


	// message handlers
	protected:
		bool _Link (const ModuleMsg::Link &);
		bool _Delete (const ModuleMsg::Delete &);
		bool _AddToManager (const ModuleMsg::AddToManager &);
		bool _RemoveFromManager (const ModuleMsg::RemoveFromManager &);
		bool _AttachModule (const ModuleMsg::AttachModule &);
		bool _DetachModule (const ModuleMsg::DetachModule &);
		bool _GetScenePrivateClasses (const SceneMsg::GetScenePrivateClasses &);

		// event handlers
		bool _DeviceCreated (const GpuMsg::DeviceCreated &);
		bool _DeviceBeforeDestroy (const GpuMsg::DeviceBeforeDestroy &);
	};
//-----------------------------------------------------------------------------


	
	const TypeIdList	SceneRendererMainThread::_eventTypes{ UninitializedT< SupportedEvents_t >() };

/*
=================================================
	constructor
=================================================
*/
	SceneRendererMainThread::SceneRendererMainThread (UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::SceneRenderer &ci) :
		Module( gs, ModuleConfig{ id, 1 }, &_eventTypes )
	{
		SetDebugName( "SceneRendererMainThread" );
		
		_SubscribeOnMsg( this, &SceneRendererMainThread::_OnModuleAttached_Impl );
		_SubscribeOnMsg( this, &SceneRendererMainThread::_OnModuleDetached_Impl );
		_SubscribeOnMsg( this, &SceneRendererMainThread::_AttachModule );
		_SubscribeOnMsg( this, &SceneRendererMainThread::_DetachModule );
		_SubscribeOnMsg( this, &SceneRendererMainThread::_FindModule_Empty );
		_SubscribeOnMsg( this, &SceneRendererMainThread::_ModulesDeepSearch_Empty );
		//_SubscribeOnMsg( this, &SceneRendererMainThread::_Update_Empty );
		_SubscribeOnMsg( this, &SceneRendererMainThread::_Link );
		_SubscribeOnMsg( this, &SceneRendererMainThread::_Delete );
		_SubscribeOnMsg( this, &SceneRendererMainThread::_AddToManager );
		_SubscribeOnMsg( this, &SceneRendererMainThread::_RemoveFromManager );
		_SubscribeOnMsg( this, &SceneRendererMainThread::_OnManagerChanged_Empty );
		_SubscribeOnMsg( this, &SceneRendererMainThread::_GetScenePrivateClasses );
		
		ASSERT( _ValidateMsgSubscriptions< SupportedMessages_t >() );

		_AttachSelfToManager( null, SceneManagerModuleID, false );
	}
	
/*
=================================================
	destructor
=================================================
*/
	SceneRendererMainThread::~SceneRendererMainThread ()
	{
	}
	
/*
=================================================
	_Link
=================================================
*/
	bool SceneRendererMainThread::_Link (const ModuleMsg::Link &msg)
	{
		if ( _IsComposedOrLinkedState( GetState() ) )
			return true;	// already linked

		CHECK_ERR( _IsInitialState( GetState() ) );
		
		CHECK_ERR( Module::_Link_Impl( msg ) );


		// find gpu thread
		ModulePtr	gthread;
		if ( not (gthread = PlatformTools::GPUThreadHelper::FindVRThread( GlobalSystems() )) ) {
			CHECK_ATTACHMENT( gthread = PlatformTools::GPUThreadHelper::FindGraphicsThread( GlobalSystems() ));
		}

		gthread->Subscribe( this, &SceneRendererMainThread::_DeviceCreated );
		gthread->Subscribe( this, &SceneRendererMainThread::_DeviceBeforeDestroy );


		// find or create command buffer manager
		ModulePtr	builder = GetModuleByMsg< CmdBufferMngrMsg_t >();
		if ( not builder )
		{
			CHECK_ERR( GlobalSystems()->modulesFactory->Create(
										Graphics::CommandBufferManagerModuleID,
										GlobalSystems(),
										CreateInfo::CommandBufferManager{ gthread, 2 },
										OUT builder ) );

			CHECK_ERR( _Attach( "builder", builder ) );
			builder->Send( msg );
		}


		// if gpu device already created
		if ( _IsComposedState( gthread->GetState() ) )
		{
			Send( GpuMsg::DeviceCreated{} );
		}
		return true;
	}
	
/*
=================================================
	_Delete
=================================================
*/
	bool SceneRendererMainThread::_Delete (const ModuleMsg::Delete &msg)
	{
		//TODO( "" );
		return Module::_Delete_Impl( msg );
	}
	
/*
=================================================
	_DeviceCreated
=================================================
*/
	bool SceneRendererMainThread::_DeviceCreated (const GpuMsg::DeviceCreated &)
	{
		CHECK_ERR( GetState() == EState::Linked );
		
		return _DefCompose( true );
	}
	
/*
=================================================
	_DeviceBeforeDestroy
=================================================
*/
	bool SceneRendererMainThread::_DeviceBeforeDestroy (const GpuMsg::DeviceBeforeDestroy &)
	{
		// TODO: destroy resources and wait for new device creation

		Send( ModuleMsg::Delete{} );
		return true;
	}

/*
=================================================
	_AttachModule
=================================================
*/
	bool SceneRendererMainThread::_AttachModule (const ModuleMsg::AttachModule &msg)
	{
		const bool	is_builder	= msg.newModule->GetSupportedEvents().HasAllTypes< CmdBufferMngrMsg_t >();

		CHECK( _Attach( msg.name, msg.newModule ) );

		if ( is_builder )
		{
			CHECK( _SetState( EState::Initial ) );
		}
		return true;
	}
	
/*
=================================================
	_DetachModule
=================================================
*/
	bool SceneRendererMainThread::_DetachModule (const ModuleMsg::DetachModule &msg)
	{
		CHECK( _Detach( msg.oldModule ) );

		if ( msg.oldModule->GetSupportedEvents().HasAllTypes< CmdBufferMngrMsg_t >() )
		{
			CHECK( _SetState( EState::Initial ) );
		}
		return true;
	}

/*
=================================================
	_AddToManager
=================================================
*/
	bool SceneRendererMainThread::_AddToManager (const ModuleMsg::AddToManager &msg)
	{
		// TODO
		return true;
	}
	
/*
=================================================
	_RemoveFromManager
=================================================
*/
	bool SceneRendererMainThread::_RemoveFromManager (const ModuleMsg::RemoveFromManager &msg)
	{
		// TODO
		return true;
	}
	
/*
=================================================
	_GetScenePrivateClasses
=================================================
*/
	bool SceneRendererMainThread::_GetScenePrivateClasses (const SceneMsg::GetScenePrivateClasses &msg)
	{
		msg.result.Set({ });	// TODO
		return true;
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	CreateSceneRenderer
=================================================
*/
	ModulePtr  SceneObjectConstructor::CreateSceneRenderer (ModuleMsg::UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::SceneRenderer &ci)
	{
		return New< SceneRendererMainThread >( id, gs, ci );
	}

}	// Scene
}	// Engine
