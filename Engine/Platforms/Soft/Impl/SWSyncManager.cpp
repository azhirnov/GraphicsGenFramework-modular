// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "Core/Config/Engine.Config.h"

#ifdef GRAPHICS_API_SOFT

#include "Engine/Platforms/Public/GPU/Sync.h"
#include "Engine/Platforms/Soft/Impl/SWBaseModule.h"
#include "Engine/Platforms/Soft/Impl/SWSyncObjects.h"
#include "Engine/Platforms/Soft/SoftRendererObjectsConstructor.h"

namespace Engine
{
namespace PlatformSW
{

	//
	// Software Renderer Sync Manager
	//

	class SWSyncManager final : public SWBaseModule
	{
	// types
	private:
		using ForwardToQueueMsg_t	= MessageListFrom<
											GpuMsg::ClientWaitDeviceIdle,
											GpuMsg::ClientWaitFence
										>;

		using SupportedMessages_t	= SWBaseModule::SupportedMessages_t::Append< MessageListFrom<
											GpuMsg::CreateFence,
											GpuMsg::DestroyFence,
											GpuMsg::GetSWFence,
											GpuMsg::CreateEvent,
											GpuMsg::DestroyEvent,
											GpuMsg::SetEvent,
											GpuMsg::ResetEvent,
											GpuMsg::GetSWEvent,
											GpuMsg::CreateSemaphore,
											GpuMsg::DestroySemaphore,
											GpuMsg::GetSWSemaphore
										> >::Append< ForwardToQueueMsg_t >;

		using QueueMsgList_t		= MessageListFrom<
											GpuMsg::SubmitCommands,
											GpuMsg::SWPresent
										>::Append< ForwardToQueueMsg_t >;

		using SupportedEvents_t		= SWBaseModule::SupportedEvents_t;
		
		using SyncUnion_t			= Union< SWFencePtr, SWEventPtr, SWSemaphorePtr >;
		using SyncArray_t			= Map< ulong, SyncUnion_t >;


	// constants
	private:
		static const TypeIdList		_eventTypes;


	// variables
	private:
		SyncArray_t		_syncs;
		ulong			_counter;


	// methods
	public:
		SWSyncManager (UntypedID_t, GlobalSystemsRef gs, const CreateInfo::GpuSyncManager &ci);
		~SWSyncManager ();


	// message handlers
	private:
		bool _Link (const ModuleMsg::Link &);
		bool _Delete (const ModuleMsg::Delete &);
		bool _OnManagerChanged2 (const ModuleMsg::OnManagerChanged &msg);

		bool _CreateFence (const GpuMsg::CreateFence &);
		bool _DestroyFence (const GpuMsg::DestroyFence &);
		bool _GetSWFence (const GpuMsg::GetSWFence &);
		bool _CreateEvent (const GpuMsg::CreateEvent &);
		bool _DestroyEvent (const GpuMsg::DestroyEvent &);
		bool _SetEvent (const GpuMsg::SetEvent &);
		bool _ResetEvent (const GpuMsg::ResetEvent &);
		bool _GetSWEvent (const GpuMsg::GetSWEvent &);
		bool _CreateSemaphore (const GpuMsg::CreateSemaphore &);
		bool _DestroySemaphore (const GpuMsg::DestroySemaphore &);
		bool _GetSWSemaphore (const GpuMsg::GetSWSemaphore &);
	};
//-----------------------------------------------------------------------------

	
	const TypeIdList	SWSyncManager::_eventTypes{ UninitializedT< SupportedEvents_t >() };
	
/*
=================================================
	constructor
=================================================
*/
	SWSyncManager::SWSyncManager (UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::GpuSyncManager &ci) :
		SWBaseModule( gs, ModuleConfig{ id, 1 }, &_eventTypes ),
		_counter{ 0 }
	{
		SetDebugName( "SWSyncManager" );

		_SubscribeOnMsg( this, &SWSyncManager::_OnModuleAttached_Impl );
		_SubscribeOnMsg( this, &SWSyncManager::_OnModuleDetached_Impl );
		_SubscribeOnMsg( this, &SWSyncManager::_AttachModule_Empty );
		_SubscribeOnMsg( this, &SWSyncManager::_DetachModule_Empty );
		_SubscribeOnMsg( this, &SWSyncManager::_FindModule_Impl );
		_SubscribeOnMsg( this, &SWSyncManager::_ModulesDeepSearch_Impl );
		_SubscribeOnMsg( this, &SWSyncManager::_Link );
		_SubscribeOnMsg( this, &SWSyncManager::_Compose_Impl );
		_SubscribeOnMsg( this, &SWSyncManager::_Delete );
		_SubscribeOnMsg( this, &SWSyncManager::_OnManagerChanged2 );
		_SubscribeOnMsg( this, &SWSyncManager::_GetDeviceInfo );
		_SubscribeOnMsg( this, &SWSyncManager::_GetSWDeviceInfo );
		_SubscribeOnMsg( this, &SWSyncManager::_GetSWPrivateClasses );
		_SubscribeOnMsg( this, &SWSyncManager::_CreateFence );
		_SubscribeOnMsg( this, &SWSyncManager::_DestroyFence );
		_SubscribeOnMsg( this, &SWSyncManager::_GetSWFence );
		_SubscribeOnMsg( this, &SWSyncManager::_CreateEvent );
		_SubscribeOnMsg( this, &SWSyncManager::_DestroyEvent );
		_SubscribeOnMsg( this, &SWSyncManager::_SetEvent );
		_SubscribeOnMsg( this, &SWSyncManager::_ResetEvent );
		_SubscribeOnMsg( this, &SWSyncManager::_GetSWEvent );
		_SubscribeOnMsg( this, &SWSyncManager::_CreateSemaphore );
		_SubscribeOnMsg( this, &SWSyncManager::_DestroySemaphore );
		_SubscribeOnMsg( this, &SWSyncManager::_GetSWSemaphore );

		_AttachSelfToManager( _GetGPUThread( ci.gpuThread ), UntypedID_t(0), true );
	}
	
/*
=================================================
	destructor
=================================================
*/
	SWSyncManager::~SWSyncManager ()
	{
	}

/*
=================================================
	_Delete
=================================================
*/
	bool SWSyncManager::_Delete (const ModuleMsg::Delete &msg)
	{
		_syncs.Clear();

		return Module::_Delete_Impl( msg );
	}
	
/*
=================================================
	_Link
=================================================
*/
	bool SWSyncManager::_Link (const ModuleMsg::Link &msg)
	{
		if ( _IsComposedOrLinkedState( GetState() ) )
			return true;	// already linked

		CHECK_ERR( _IsInitialState( GetState() ) );
		CHECK_ERR( _GetManager() );

		// find command queue
		{
			ModulePtr	cmd_queue;
			CHECK_LINKING( cmd_queue = _GetManager()->GetModuleByMsg< QueueMsgList_t >() );
			CHECK_LINKING( _CopySubscriptions< ForwardToQueueMsg_t >( cmd_queue ) );
		}

		return Module::_Link_Impl( msg );
	}

/*
=================================================
	_OnManagerChanged2
=================================================
*/
	bool SWSyncManager::_OnManagerChanged2 (const ModuleMsg::OnManagerChanged &msg)
	{
		_OnManagerChanged( msg );
		
		if ( msg.newManager )
		{
			msg.newManager->UnsubscribeAll( this );
		}
		return true;
	}

/*
=================================================
	_CreateFence
=================================================
*/
	bool SWSyncManager::_CreateFence (const GpuMsg::CreateFence &msg)
	{
		for (;;)
		{
			++_counter;

			if ( not _syncs.IsExist( _counter ) )
				break;
		}
		_syncs.Add( _counter, SyncUnion_t{SWFencePtr{ new SWFence( msg.name ) }} );

		msg.result.Set( GpuFenceId(_counter) );
		return true;
	}
	
/*
=================================================
	_DestroyFence
=================================================
*/
	bool SWSyncManager::_DestroyFence (const GpuMsg::DestroyFence &msg)
	{
		SyncArray_t::iterator	iter;

		if ( _syncs.Find( ulong(msg.id), OUT iter ) )
		{
			CHECK_ERR( iter->second.Is< SWFencePtr >() );

			_syncs.EraseByIter( iter );
		}
		return true;
	}
	
/*
=================================================
	_GetSWFence
=================================================
*/
	bool SWSyncManager::_GetSWFence (const GpuMsg::GetSWFence &msg)
	{
		SyncArray_t::iterator	iter;

		if ( _syncs.Find( ulong(msg.id), OUT iter ) )
		{
			msg.result.Set( iter->second.Get< SWFencePtr >( null ) );
		}
		return true;
	}

/*
=================================================
	_CreateEvent
=================================================
*/
	bool SWSyncManager::_CreateEvent (const GpuMsg::CreateEvent &msg)
	{
		for (;;)
		{
			++_counter;

			if ( not _syncs.IsExist( _counter ) )
				break;
		}
		_syncs.Add( _counter, SyncUnion_t{SWEventPtr{ new SWEvent( msg.name ) }} );

		msg.result.Set( GpuEventId(_counter) );
		return true;
	}
	
/*
=================================================
	_DestroyEvent
=================================================
*/
	bool SWSyncManager::_DestroyEvent (const GpuMsg::DestroyEvent &msg)
	{
		SyncArray_t::iterator	iter;

		if ( _syncs.Find( ulong(msg.id), OUT iter ) )
		{
			CHECK_ERR( iter->second.Is< SWEventPtr >() );
			
			_syncs.EraseByIter( iter );
		}
		return true;
	}
	
/*
=================================================
	_SetEvent
=================================================
*/
	bool SWSyncManager::_SetEvent (const GpuMsg::SetEvent &)
	{
		TODO( "" );
		return false;
	}
	
/*
=================================================
	_ResetEvent
=================================================
*/
	bool SWSyncManager::_ResetEvent (const GpuMsg::ResetEvent &)
	{
		TODO( "" );
		return false;
	}
	
/*
=================================================
	_GetSWEvent
=================================================
*/
	bool SWSyncManager::_GetSWEvent (const GpuMsg::GetSWEvent &msg)
	{
		SyncArray_t::iterator	iter;

		if ( _syncs.Find( ulong(msg.id), OUT iter ) )
		{
			msg.result.Set( iter->second.Get< SWEventPtr >( null ) );
		}
		return true;
	}

/*
=================================================
	_CreateSemaphore
=================================================
*/
	bool SWSyncManager::_CreateSemaphore (const GpuMsg::CreateSemaphore &msg)
	{
		for (;;)
		{
			++_counter;

			if ( not _syncs.IsExist( _counter ) )
				break;
		}
		_syncs.Add( _counter, SyncUnion_t{SWSemaphorePtr{ new SWSemaphore( msg.name ) }} );

		msg.result.Set( GpuSemaphoreId(_counter) );
		return true;
	}
	
/*
=================================================
	_DestroySemaphore
=================================================
*/
	bool SWSyncManager::_DestroySemaphore (const GpuMsg::DestroySemaphore &msg)
	{
		SyncArray_t::iterator	iter;

		if ( _syncs.Find( ulong(msg.id), OUT iter ) )
		{
			CHECK_ERR( iter->second.Is< SWSemaphorePtr >() );

			_syncs.EraseByIter( iter );
		}
		return true;
	}
	
/*
=================================================
	_DestroySemaphore
=================================================
*/
	bool SWSyncManager::_GetSWSemaphore (const GpuMsg::GetSWSemaphore &msg)
	{
		SyncArray_t::iterator	iter;

		if ( _syncs.Find( ulong(msg.id), OUT iter ) )
		{
			msg.result.Set( iter->second.Get< SWSemaphorePtr >( null ) );
		}
		return true;
	}

}	// PlatformSW
//-----------------------------------------------------------------------------

namespace Platforms
{
	ModulePtr SoftRendererObjectsConstructor::CreateSWSyncManager (ModuleMsg::UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::GpuSyncManager &ci)
	{
		return New< PlatformSW::SWSyncManager >( id, gs, ci );
	}

}	// Platforms
}	// Engine

#endif	// GRAPHICS_API_SOFT
