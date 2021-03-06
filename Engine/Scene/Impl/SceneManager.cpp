// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "Engine/Scene/Public/Scene.h"
#include "Engine/Scene/Impl/SceneObjectConstructor.h"
#include "Engine/Scene/Impl/BaseSceneModule.h"
#include "Engine/Platforms/VR/VRObjectsConstructor.h"
#include "Engine/Platforms/Public/Tools/WindowHelper.h"
#include "Engine/Platforms/Public/Tools/GPUThreadHelper.h"

namespace Engine
{
namespace Scene
{
	using namespace Engine::Platforms;


	//
	// Scene Manager Module
	//

	class SceneManager final : public Module
	{
	// types
	protected:
		using SupportedMessages_t	= MessageListFrom<
											ModuleMsg::AddToManager,
											ModuleMsg::RemoveFromManager
										>;
		
		using SupportedEvents_t		= Module::SupportedEvents_t::Append< MessageListFrom<
											OSMsg::WindowCreated,
											OSMsg::WindowBeforeDestroy,
											OSMsg::WindowAfterDestroy,
											GpuMsg::DeviceCreated,
											GpuMsg::DeviceBeforeDestroy
										> >;
	
		using SceneThreads_t		= Set< ModulePtr >;
		using VRSettings_t			= CreateInfo::SceneManager::VRSettings;
		
		using GpuContextMsgList_t	= MessageListFrom< ModuleMsg::AddToManager, ModuleMsg::RemoveFromManager, GpuMsg::GetGraphicsModules >;

		using KeyInputEventList_t	= MessageListFrom< ModuleMsg::InputKey >;
		using MouseInputEventList_t	= MessageListFrom< ModuleMsg::InputMotion >;


	// constants
	private:
		static const TypeIdList		_eventTypes;


	// variables
	private:
		SceneThreads_t			_threads;

		const GraphicsSettings	_settings;
		const VRSettings_t		_vrSettings;


	// methods
	public:
		SceneManager (UntypedID_t, GlobalSystemsRef, const CreateInfo::SceneManager &);
		~SceneManager ();


	// message handlers
	protected:
		bool _Link (const ModuleMsg::Link &);
		bool _Compose (const ModuleMsg::Compose &);
		bool _Delete (const ModuleMsg::Delete &);
		bool _AddToManager (const ModuleMsg::AddToManager &);
		bool _RemoveFromManager (const ModuleMsg::RemoveFromManager &);
	};
//-----------------------------------------------------------------------------


	
	const TypeIdList	SceneManager::_eventTypes{ UninitializedT< SupportedEvents_t >() };

/*
=================================================
	constructor
=================================================
*/
	SceneManager::SceneManager (UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::SceneManager &ci) :
		Module( gs, ModuleConfig{ id, 1 }, &_eventTypes ),
		_settings{ ci.settings },	_vrSettings{ ci.vrSettings }
	{
		SetDebugName( "SceneManager" );
		
		_SubscribeOnMsg( this, &SceneManager::_OnModuleAttached_Impl );
		_SubscribeOnMsg( this, &SceneManager::_OnModuleDetached_Impl );
		_SubscribeOnMsg( this, &SceneManager::_AttachModule_Impl );
		_SubscribeOnMsg( this, &SceneManager::_DetachModule_Impl );
		_SubscribeOnMsg( this, &SceneManager::_FindModule_Empty );
		_SubscribeOnMsg( this, &SceneManager::_ModulesDeepSearch_Empty );
		_SubscribeOnMsg( this, &SceneManager::_Link );
		_SubscribeOnMsg( this, &SceneManager::_Compose );
		_SubscribeOnMsg( this, &SceneManager::_Delete );
		_SubscribeOnMsg( this, &SceneManager::_AddToManager );
		_SubscribeOnMsg( this, &SceneManager::_RemoveFromManager );
		
		ASSERT( _ValidateMsgSubscriptions< SupportedMessages_t >() );
	}
	
/*
=================================================
	destructor
=================================================
*/
	SceneManager::~SceneManager ()
	{
		//LOG( "SceneManager finalized", ELog::Debug );

		ASSERT( _threads.Empty() );
	}
	
/*
=================================================
	_Link
=================================================
*/
	bool SceneManager::_Link (const ModuleMsg::Link &msg)
	{
		if ( _IsComposedOrLinkedState( GetState() ) )
			return true;	// already linked

		CHECK_ERR( _IsInitialState( GetState() ) );

		ModulePtr	platform	= PlatformTools::WindowHelper::FindPlatform( GlobalSystems() );
		ModulePtr	input_mngr	= GlobalSystems()->mainSystem->GetModuleByID( InputManagerModuleID );	// TODO
		ModulePtr	data_mngr	= GlobalSystems()->mainSystem->GetModuleByID( DataProviderManagerModuleID );	// TODO
		ModulePtr	gpu_context	= GlobalSystems()->mainSystem->GetModuleByMsg< GpuContextMsgList_t >();

		if ( not platform )
		{
			CHECK_LINKING( GlobalSystems()->modulesFactory->Create( 0, GlobalSystems(), CreateInfo::Platform{}, OUT platform ) );
			GlobalSystems()->mainSystem->Send( ModuleMsg::AttachModule{ platform });
			platform->Send( msg );
		}

		if ( not input_mngr )
		{
			CHECK_LINKING( GlobalSystems()->modulesFactory->Create( InputManagerModuleID, GlobalSystems(), CreateInfo::InputManager{}, OUT input_mngr ) );
			GlobalSystems()->mainSystem->Send( ModuleMsg::AttachModule{ input_mngr });
			input_mngr->Send( msg );
		}

		if ( not data_mngr )
		{
			CHECK_LINKING( GlobalSystems()->modulesFactory->Create( DataProviderManagerModuleID, GlobalSystems(), CreateInfo::DataProviderManager{}, OUT data_mngr ) );
			GlobalSystems()->mainSystem->Send( ModuleMsg::AttachModule{ data_mngr });
			data_mngr->Send( msg );
		}

		if ( not gpu_context )
		{
			CHECK_LINKING( GlobalSystems()->modulesFactory->Create( 0, GlobalSystems(), CreateInfo::GpuContext{ _settings }, OUT gpu_context ) );
			GlobalSystems()->mainSystem->Send( ModuleMsg::AttachModule{ gpu_context });
			gpu_context->Send( msg );
		}
		
		ModulePtr	window		= PlatformTools::WindowHelper::FindWindow( GlobalSystems() );
		ModulePtr	vr_thread	= PlatformTools::GPUThreadHelper::FindVRThread( GlobalSystems() );
		ModulePtr	gpu_thread	= PlatformTools::GPUThreadHelper::FindGraphicsThread( GlobalSystems() );
		ModulePtr	input_thread= GlobalSystems()->parallelThread->GetModuleByID( InputThreadModuleID );	// TODO

		if ( not window )
		{
			OSMsg::GetOSModules		req_ids;
			platform->Send( req_ids );

			CHECK_LINKING( GlobalSystems()->modulesFactory->Create( req_ids.result->window, GlobalSystems(), CreateInfo::Window{}, OUT window ) );
			GlobalSystems()->parallelThread->Send( ModuleMsg::AttachModule{ window });
			window->Send( msg );
		}
		
		DEBUG_ONLY( window->AddModule( Profilers::FPSCounterModuleID, CreateInfo::FPSCounter{} ) );

		if ( not input_thread )
		{
			CHECK_LINKING( GlobalSystems()->modulesFactory->Create( InputThreadModuleID, GlobalSystems(), CreateInfo::InputThread{}, OUT input_thread ) );
			GlobalSystems()->parallelThread->Send( ModuleMsg::AttachModule{ input_thread });
			input_thread->Send( msg );
		}
		
		if ( _vrSettings.enabled and not vr_thread )
		{
			// TODO: create default VR thread
			#ifdef GX_EMULATOR_VR
			CHECK_ERR( GlobalSystems()->modulesFactory->Create( EmulatorVRThreadModuleID,
																GlobalSystems(),
																CreateInfo::VRThread{ gpu_thread, _vrSettings.eyeTextureDimension, _vrSettings.layered },
																OUT vr_thread ) );
			GlobalSystems()->parallelThread->Send( ModuleMsg::AttachModule{ vr_thread });
			vr_thread->Send( msg );
			gpu_thread = vr_thread;
			#endif
		}

		if ( not gpu_thread )
		{
			auto	graphics_ids = gpu_context->Request( GpuMsg::GetGraphicsModules{} ).graphics;

			CHECK_LINKING( GlobalSystems()->modulesFactory->Create( graphics_ids.thread, GlobalSystems(), CreateInfo::GpuThread{ _settings, gpu_context }, OUT gpu_thread ) );
			GlobalSystems()->parallelThread->Send( ModuleMsg::AttachModule{ gpu_thread });
			gpu_thread->Send( msg );
		}

		ModulePtr	key_input	= window->GetModuleByEvent< KeyInputEventList_t >();
		ModulePtr	mouse_input	= window->GetModuleByEvent< MouseInputEventList_t >();
		
		if ( not key_input )
		{
			OSMsg::GetOSModules		req_ids;
			platform->Send( req_ids );

			CHECK_LINKING( GlobalSystems()->modulesFactory->Create( req_ids.result->keyInput, GlobalSystems(), CreateInfo::RawInputHandler{}, OUT key_input ) );
			window->Send( ModuleMsg::AttachModule{ key_input });
			key_input->Send( msg );
		}

		if ( not mouse_input )
		{
			OSMsg::GetOSModules		req_ids;
			platform->Send( req_ids );

			CHECK_LINKING( GlobalSystems()->modulesFactory->Create( req_ids.result->mouseInput, GlobalSystems(), CreateInfo::RawInputHandler{}, OUT mouse_input ) );
			window->Send( ModuleMsg::AttachModule{ mouse_input });
			mouse_input->Send( msg );
		}

		ModulePtr	scene		= GlobalSystems()->parallelThread->GetModuleByID( SceneModuleID );			// TODO
		ModulePtr	renderer	= GlobalSystems()->parallelThread->GetModuleByID( SceneRendererModuleID );	// TODO
		ModulePtr	physics		= GlobalSystems()->parallelThread->GetModuleByID( ScenePhysicsModuleID );	// TODO
		ModulePtr	logic		= GlobalSystems()->parallelThread->GetModuleByID( SceneLogicModuleID );		// TODO

		if ( not scene )
		{
			CHECK_LINKING( GlobalSystems()->modulesFactory->Create( SceneModuleID, GlobalSystems(), CreateInfo::SceneMain{}, OUT scene ) );
			GlobalSystems()->parallelThread->Send( ModuleMsg::AttachModule{ scene });
			scene->Send( msg );
		}

		if ( not renderer )
		{
			CHECK_LINKING( GlobalSystems()->modulesFactory->Create( SceneRendererModuleID, GlobalSystems(), CreateInfo::SceneRenderer{}, OUT renderer ) );
			GlobalSystems()->parallelThread->Send( ModuleMsg::AttachModule{ renderer });
			renderer->Send( msg );
		}
		
		if ( not physics )
		{
			CHECK_LINKING( GlobalSystems()->modulesFactory->Create( ScenePhysicsModuleID, GlobalSystems(), CreateInfo::ScenePhysics{}, OUT physics ) );
			GlobalSystems()->parallelThread->Send( ModuleMsg::AttachModule{ physics });
			physics->Send( msg );
		}

		if ( not logic )
		{
			CHECK_LINKING( GlobalSystems()->modulesFactory->Create( SceneLogicModuleID, GlobalSystems(), CreateInfo::SceneLogic{}, OUT logic ) );
			GlobalSystems()->parallelThread->Send( ModuleMsg::AttachModule{ logic });
			logic->Send( msg );
		}

		CHECK(( window->ReceiveEvents< MessageListFrom< OSMsg::WindowCreated, OSMsg::WindowBeforeDestroy, OSMsg::WindowAfterDestroy > >( this ) ));
		CHECK(( gpu_thread->ReceiveEvents< MessageListFrom< GpuMsg::DeviceCreated, GpuMsg::DeviceBeforeDestroy > >( this ) ));
		
		return Module::_Link_Impl( msg );
	}
	
/*
=================================================
	_Compose
=================================================
*/
	bool SceneManager::_Compose (const ModuleMsg::Compose &msg)
	{
		if ( _IsComposedState( GetState() ) )
			return true;	// already composed

		CHECK_ERR( GetState() == EState::Linked );

		// TODO

		return _DefCompose( true );
	}
	
/*
=================================================
	_Delete
=================================================
*/
	bool SceneManager::_Delete (const ModuleMsg::Delete &msg)
	{
		//TODO( "" );
		return Module::_Delete_Impl( msg );
	}

/*
=================================================
	_AddToManager
=================================================
*/
	bool SceneManager::_AddToManager (const ModuleMsg::AddToManager &msg)
	{
		CHECK_ERR( msg.module );
		CHECK_ERR( msg.module->GetThreadID() == GetThreadID() );	// must be in single thread
		ASSERT( not _threads.IsExist( msg.module ) );

		_threads.Add( msg.module );
		return true;
	}
	
/*
=================================================
	_RemoveFromManager
=================================================
*/
	bool SceneManager::_RemoveFromManager (const ModuleMsg::RemoveFromManager &msg)
	{
		CHECK_ERR( msg.module );

		ModulePtr	module = msg.module.Lock();

		if ( not module )
			return false;

		ASSERT( _threads.IsExist( module ) );

		_threads.Erase( module );
		return true;
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	CreateSceneManager
=================================================
*/
	ModulePtr  SceneObjectConstructor::CreateSceneManager (ModuleMsg::UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::SceneManager &ci)
	{
		return New< SceneManager >( id, gs, ci );
	}

}	// Scene
}	// Engine
