// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'
/*
	MainSystem - this class contains all global systems.
*/

#pragma once

#include "Engine/Base/Modules/ModulesFactory.h"
#include "Engine/Base/Common/IDs.h"
#include "Engine/Base/Tasks/TaskManager.h"
#include "Engine/Base/Threads/ThreadManager.h"

namespace Engine
{
namespace Base
{

	//
	// Main System
	//

	class MainSystem final : public Module
	{
	// types
	private:
		using SupportedMessages_t	= MessageListFrom<
											ModuleMsg::AttachModule,
											ModuleMsg::DetachModule,
											ModuleMsg::FindModule,
											ModuleMsg::ModulesDeepSearch,
											ModuleMsg::Link,
											ModuleMsg::Compose,
											ModuleMsg::Update,
											ModuleMsg::Delete
										>;
		using SupportedEvents_t		= MessageListFrom<
											ModuleMsg::Link,
											ModuleMsg::Compose,
											ModuleMsg::Update
										>;

		SHARED_POINTER( TaskManager );
		SHARED_POINTER( ThreadManager );

		
	// constants
	private:
		static const TypeIdList		_eventTypes;


	// variables
	private:
		ModulesFactory		_factory;
		TaskManagerPtr		_taskMngr;
		ThreadManagerPtr	_threadMngr;


	// methods
	public:
		explicit MainSystem (GlobalSystemsRef gs);
		~MainSystem ();
		

	// message handlers
	private:
		bool _Delete (const ModuleMsg::Delete &);

	private:
		void _Create () noexcept;
		void _Destroy () noexcept;

		static ModulePtr _CreateThreadManager (UntypedID_t, GlobalSystemsRef, const CreateInfo::ThreadManager &);
		static ModulePtr _CreateTaskManager (UntypedID_t, GlobalSystemsRef, const CreateInfo::TaskManager &);
	};


}	// Base
}	// Engine
