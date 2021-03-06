// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "Engine/Base/Common/IDs.h"
#include "Engine/Base/Modules/Module.h"
#include "Engine/Base/Public/AsyncMessage.h"
#include "Engine/Base/Tasks/TaskManager.h"
#include "Engine/Base/Public/ParallelThread.h"

#include "Core/STL/Containers/CircularQueue.h"
#include "Core/STL/ThreadSafe/MtQueue.h"

namespace Engine
{
namespace Base
{

	//
	// Async Task Module
	//

	class TaskModuleImpl final : public Module
	{
	// types
	private:
		SHARED_POINTER( TaskModuleImpl );

		using SupportedMessages_t	= MessageListFrom<
											ModuleMsg::PushAsyncMessage,
											ModuleMsg::OnManagerChanged
										>;
		using SupportedEvents_t		= Module::SupportedEvents_t;
		using MsgQueue_t			= MtQueue< CircularQueue< AsyncMessage > >;
		

	// constants
	private:
		static const TypeIdList		_eventTypes;


	// variables
	private:
		MsgQueue_t		_msgQueue;


	// methods
	public:
		TaskModuleImpl (UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::TaskModule &);
		~TaskModuleImpl ();


	// message handlers
	private:
		bool _Update (const ModuleMsg::Update &);
		bool _Delete (const ModuleMsg::Delete &);
		bool _PushAsyncMessage (const ModuleMsg::PushAsyncMessage &) noexcept;


	private:
		usize _Push (AsyncMessage &&op);
		usize _Flush ();
		usize _ProcessMessages ();

		static constexpr usize _MaxPendingQueueSize ()	{ return 1024; }
	};
//-----------------------------------------------------------------------------



	const TypeIdList	TaskModuleImpl::_eventTypes{ UninitializedT< SupportedEvents_t >() };

/*
=================================================
	constructor
=================================================
*/
	TaskModuleImpl::TaskModuleImpl (UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::TaskModule &info) :
		Module( gs, ModuleConfig{ id, 1 }, &_eventTypes )
	{
		GlobalSystems()->taskModule._Set( this );

		SetDebugName( GlobalSystems()->parallelThread->GetDebugName() + "_Tasks"_str );

		_SubscribeOnMsg( this, &TaskModuleImpl::_OnModuleAttached_Impl );
		_SubscribeOnMsg( this, &TaskModuleImpl::_OnModuleDetached_Impl );
		_SubscribeOnMsg( this, &TaskModuleImpl::_AttachModule_Empty );
		_SubscribeOnMsg( this, &TaskModuleImpl::_DetachModule_Empty );
		_SubscribeOnMsg( this, &TaskModuleImpl::_OnManagerChanged_Empty );
		_SubscribeOnMsg( this, &TaskModuleImpl::_FindModule_Empty );
		_SubscribeOnMsg( this, &TaskModuleImpl::_ModulesDeepSearch_Empty );
		_SubscribeOnMsg( this, &TaskModuleImpl::_Update );
		_SubscribeOnMsg( this, &TaskModuleImpl::_Delete );
		_SubscribeOnMsg( this, &TaskModuleImpl::_PushAsyncMessage );
		
		ASSERT( _ValidateMsgSubscriptions< SupportedMessages_t >() );

		_msgQueue.ReserveCurrent( 256 );
		_msgQueue.ReservePending( 128 );

		// attach to manager
		_SetManager( info.manager );

		if ( _GetManager()->GetThreadID() == GetThreadID() )
		{
			_GetManager()->Send( ModuleMsg::AddTaskSchedulerToManager{ this, DelegateBuilder(this, &TaskModuleImpl::_Push) });
		}
		else
		{
			CHECK( _PushAsyncMessage( ModuleMsg::PushAsyncMessage{
						_GetManager()->GetThreadID(),
						LAMBDA( mngr = _GetManager(), task = TaskModuleImplPtr(this) ) (GlobalSystemsRef)
						{
							mngr->Send( ModuleMsg::AddTaskSchedulerToManager{ task, DelegateBuilder(task, &TaskModuleImpl::_Push) });
						}}
					));
		}

		CHECK( _SetState( EState::ComposedImmutable ) );

		//_SendUncheckedEvent( ModuleMsg::AfterCompose{} );	// TODO
	}
	
/*
=================================================
	destructor
=================================================
*/
	TaskModuleImpl::~TaskModuleImpl ()
	{
		//LOG( "TaskModule finalized", ELog::Debug );

		ASSERT( _msgQueue.GetCurrentQueueCount() == 0 );
		ASSERT( _msgQueue.GetPendingQueueCount() == 0 );

		if ( GetThreadID() == ThreadID::GetCurrent() ) {
			GlobalSystems()->taskModule._Set( null );
		}
	}
	
/*
=================================================
	_Update
=================================================
*/
	bool TaskModuleImpl::_Update (const ModuleMsg::Update &)
	{
		//ASSERT( _IsComposedState( GetState() ), void() );
		//ASSERT( msg.Sender() and _GetParents().CustomSearch().IsExist( msg.Sender() ) );

		_Flush();
		_ProcessMessages();
		return true;
	}
	
/*
=================================================
	_Delete
=================================================
*/
	bool TaskModuleImpl::_Delete (const ModuleMsg::Delete &)
	{
		//ASSERT( msg.Sender() and _GetParents().CustomSearch().IsExist( msg.Sender() ) );
		
		_Push(AsyncMessage{ LAMBDA( this ) (GlobalSystemsRef)
							{
								_DetachSelfFromManager();

								CHECK( Module::_Delete_Impl( ModuleMsg::Delete{} ) );
		
								ASSERT( _msgQueue.GetCurrentQueueCount() == 0 );
								ASSERT( _msgQueue.GetPendingQueueCount() == 0 );

								_msgQueue.ClearAll();
							}
			});
		return true;
	}

/*
=================================================
	_PushAsyncMessage
=================================================
*/
	bool TaskModuleImpl::_PushAsyncMessage (const ModuleMsg::PushAsyncMessage &msg) noexcept
	{
		CHECK_ERR( GetState() != EState::Deleting );
		CHECK_ERR( _GetManager() );

		return _GetManager()->SendAsync( msg );
	}

/*
=================================================
	_Push
----
	returns the size of pending queue
=================================================
*/
	usize TaskModuleImpl::_Push (AsyncMessage &&op)
	{
		const usize	res = _msgQueue.Push( RVREF( op ) );

		if ( res > _MaxPendingQueueSize() )
		{
			LOG( "pending queue is too big, queue will be force flushed!", ELog::Debug );
			_Flush();
			return 0;
		}
		return res;
	}
	
/*
=================================================
	_Flush
----
	returns the new size of current queue
=================================================
*/
	usize TaskModuleImpl::_Flush ()
	{
		return _msgQueue.Flush();
	}
	
/*
=================================================
	_ProcessMessages
----
	returns the number of processed messages
=================================================
*/
	usize TaskModuleImpl::_ProcessMessages ()
	{
		return _msgQueue.ProcessAll( LAMBDA(this) (const AsyncMessage &op) {{ op.Process( this->GlobalSystems() ); }} );
	}
//-----------------------------------------------------------------------------
	


/*
=================================================
	_CreateTaskModule
=================================================
*/
	ModulePtr TaskManager::_CreateTaskModule (UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::TaskModule &ci)
	{
		CHECK_ERR( ci.manager );
		return New< TaskModuleImpl >( id, gs, ci );
	}
	
}	// Base
}	// Engine
