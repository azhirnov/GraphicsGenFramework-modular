// Copyright �  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "Engine/Base/Modules/IDs.h"
#include "Engine/Base/Threads/ParallelThread.h"

namespace Engine
{
namespace Base
{

	//
	// Parallel Thread
	//

	class ParallelThreadImpl final : public ParallelThread
	{
		friend class ThreadManager;

	// types
	private:
		using SupportedMessages_t	= Module::SupportedMessages_t::Erase< MessageListFrom<
											ModuleMsg::Update
										> >
										::Append< MessageListFrom<
											ModuleMsg::OnManagerChanged
										> >;
		using SupportedEvents_t		= Module::SupportedEvents_t;
		
		using OnStartThreadFunc_t	= CreateInfo::Thread::OnStartThreadFunc_t;


	// constants
	private:
		static const Runtime::VirtualTypeList	_msgTypes;
		static const Runtime::VirtualTypeList	_eventTypes;


	// variables
	private:
		OS::Thread				_thread;
		OnStartThreadFunc_t		_onStarted;
		TimeProfilerD			_timer;
		bool					_isLooping;


	// methods
	public:
		ParallelThreadImpl (const GlobalSystemsRef gs, const CreateInfo::Thread &);
		~ParallelThreadImpl ();


	// message handlers
	private:
		bool _Link (const Message< ModuleMsg::Link > &);
		bool _Compose (const Message< ModuleMsg::Compose > &);
		bool _Delete (const Message< ModuleMsg::Delete > &);

	private:
		void _OnEnter ();
		void _Loop ();
		void _OnExit ();
		void _Wait ();
		void _SyncUpdate ();
	};


}	// Base
}	// Engine