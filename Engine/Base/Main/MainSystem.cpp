// Copyright � 2014-2017  Zhirnov Andrey. All rights reserved.

#include "Engine/Base/Main/MainSystem.h"

namespace Engine
{
namespace Base
{
	
	const Runtime::VirtualTypeList	MainSystem::_msgTypes{ UninitializedT< SupportedMessages_t >() };
	const Runtime::VirtualTypeList	MainSystem::_eventTypes{ UninitializedT< SupportedEvents_t >() };
	
/*
=================================================
	GetMainSystemInstace
=================================================
*/
	Ptr<Module>  GetMainSystemInstace ()
	{
		static EngineSubSystems	global_systems;
		static MainSystem		main_system{ SubSystemsRef(&global_systems) };
		return &main_system;
	}

/*
=================================================
	constructor
=================================================
*/
	MainSystem::MainSystem (const SubSystemsRef gs) :
		Module( gs, GetStaticID(), &_msgTypes, &_eventTypes ),
		_factory( GlobalSystems() ),
		_taskMngr( GlobalSystems(), CreateInfo::TaskManager() ),
		_threadMngr( GlobalSystems(), CreateInfo::ThreadManager() )
	{
		SetDebugName( "MainSystem" );

		GlobalSystems()->GetSetter< MainSystem >().Set( this );

		_SubscribeOnMsg( this, &MainSystem::_AttachModule_Impl );
		_SubscribeOnMsg( this, &MainSystem::_DetachModule_Impl );
		_SubscribeOnMsg( this, &MainSystem::_FindModule_Impl );
		_SubscribeOnMsg( this, &MainSystem::_Update_Impl );
		_SubscribeOnMsg( this, &MainSystem::_Link_Impl );
		_SubscribeOnMsg( this, &MainSystem::_Compose_Impl );
		_SubscribeOnMsg( this, &MainSystem::_Delete );
		
		CHECK( _ValidateMsgSubscriptions() );

		CHECK( _factory.Register( ThreadManager::GetStaticID(), &_threadMngr, &_CreateThreadManager ) );
		CHECK( _factory.Register( TaskManager::GetStaticID(),   &_taskMngr,   &_CreateTaskManager   ) );

		_Attach( &_taskMngr );
		_Attach( &_threadMngr );
		
		GlobalSystems()->Get< ParallelThread >()->
			AddModule( TaskModule::GetStaticID(), CreateInfo::TaskModule() );
	}

/*
=================================================
	destructor
=================================================
*/
	MainSystem::~MainSystem ()
	{
		_factory.Unregister<ThreadManager>();
		_factory.Unregister<TaskManager>();

		GlobalSystems()->GetSetter< MainSystem >().Set( null );
		
		LOG( "MainSystem finalized", ELog::Debug );
	}
	
/*
=================================================
	destructor
=================================================
*/
	MainSystem::_FinalChecks::~_FinalChecks ()
	{
		Logger::GetInstance()->Close();

		DEBUG_ONLY( RefCountedObject::s_ChenckNotReleasedObjects() );
	}

/*
=================================================
	_Delete
=================================================
*/
	void MainSystem::_Delete (const Message< ModuleMsg::Delete > &msg)
	{
		_threadMngr.Send( msg );

		_SendForEachAttachments( msg );

		Module::_Delete_Impl( msg );
	}

/*
=================================================
	_CreateThreadManager
=================================================
*/
	ModulePtr MainSystem::_CreateThreadManager (SubSystemsRef gs, const CreateInfo::ThreadManager &info)
	{
		return GXTypes::New<ThreadManager>( gs, info );
	}
	
/*
=================================================
	_CreateTaskManager
=================================================
*/
	ModulePtr MainSystem::_CreateTaskManager (SubSystemsRef gs, const CreateInfo::TaskManager &info)
	{
		return GXTypes::New<TaskManager>( gs, info );
	}

	
}	// Base
}	// Engine
