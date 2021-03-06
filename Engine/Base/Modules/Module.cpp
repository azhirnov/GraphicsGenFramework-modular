// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "Engine/Base/Modules/Module.h"
#include "Engine/Base/Public/TaskModule.h"
#include "Engine/Base/Public/ParallelThread.h"
#include "Engine/Base/Modules/ModuleAsyncTasks.h"

namespace Engine
{
namespace Base
{

/*
=================================================
	constructor
=================================================
*/
	Module::Module (const GlobalSystemsRef gs,
					const ModuleConfig &config,
					const TypeIdList *eventTypes) :
		BaseObject( gs ),
		_state( EState::Initial ),
		_ownThread( ThreadID::GetCurrent() ),
		_moduleCfg( config ),
		_supportedEvents( *eventTypes )
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	Module::~Module ()
	{
		ASSERT( GetState() == EState::Deleting );

		CHECK( _attachments.Empty() );
		CHECK( _parents.Empty() );
		CHECK( _manager.IsNull() );
	}
	
/*
=================================================
	_Release
=================================================
*/
	void Module::_Release (INOUT RefCounter_t &rc)
	{
		if ( GetState() != EState::Deleting )
		{
			// send deleting message (and event)
			rc.Inc();
			CHECK( Send( ModuleMsg::Delete{} ) );
			(void)rc.DecAndTest();
		}
		
		if ( GetState() == EState::Deleting )
		{
			BaseObject::_Release( INOUT rc );
		}
		else
		{
			// something goes wrong...
			// maybe somebody save strong reference when processing 'Delete' message
			// so we need to keep this object alive and increase week reference counter
			rc.IncWeak();
		}
	}

/*
=================================================
	_Attach
=================================================
*/
	bool Module::_Attach (const ModuleName_t &name, const ModulePtr &unit, bool mustBeUniqueID)
	{
		CHECK_ERR( unit );
		CHECK_ERR( _IsMutableState( GetState() ) );

		// global and per-thread modules must be unique
		const bool	must_be_unique	= ((unit->GetModuleID() & GModID::_IDMask) == GModID::_ID) or
									  ((unit->GetModuleID() & TModID::_IDMask) == TModID::_ID) or
										mustBeUniqueID;
		
		for (auto& attachment : _attachments)
		{
			if ( attachment.second == unit and not _moduleCfg.multiAttachment )
			{
				LOG( "module is already attached!", ELog::Warning );
				return true;
			}

			if ( not name.Empty() and attachment.first == name )
			{
				RETURN_ERR( "module with name: \"" << name << "\" is already attached!" );
			}

			if ( attachment.second->GetModuleID() == unit->GetModuleID() and must_be_unique )
				RETURN_ERR( "module with ID '" << ToString( GModID::type(unit->GetModuleID()) ) << "' is alredy attached" );
		}

		_attachments.PushBack({ name, unit });

		ModuleMsg::OnModuleAttached		on_attached{ this, name, unit };

		_SendForEachAttachments( on_attached );
		_SendUncheckedEvent( on_attached );

		return true;
	}
	
/*
=================================================
	_Detach
=================================================
*/
	bool Module::_Detach (const ModulePtr &unit)
	{
		if ( _moduleCfg.multiAttachment )
			return _DetachMulti( unit );
		else
			return _DetachSingle( unit );
	}
	
/*
=================================================
	_DetachSingle
=================================================
*/
	bool Module::_DetachSingle (const ModulePtr &unit)
	{
		CHECK_ERR( unit );
		
		FOR( i, _attachments )
		{
			if ( _attachments[i].second == unit )
			{
				ModuleMsg::OnModuleDetached		on_detached{ this, _attachments[i].first, unit, true };

				_SendForEachAttachments( on_detached );
				_SendUncheckedEvent( on_detached );

				_attachments.Erase( i );
				return true;
			}
		}
		RETURN_ERR( "module is not attached" );
	}
	
/*
=================================================
	_DetachMulti
=================================================
*/
	bool Module::_DetachMulti (const ModulePtr &unit)
	{
		CHECK_ERR( unit );

		usize	first	= UMax;
		usize	count	= 0;

		const auto	Prepare = LAMBDA( this, &unit, &first, &count ) ()
		{{
			first = UMax;
			count = 0;
			FOR( i, _attachments ) {
				if ( _attachments[i].second == unit ) {
					first = count ? first : i;
					++count;
				}
			}
		}};

		const auto	Detach = LAMBDA( this ) (usize i, bool isLast)
		{{
			ModuleMsg::OnModuleDetached		on_detached{ this, _attachments[i].first, _attachments[i].second, isLast };

			_SendForEachAttachments( on_detached );
			_SendUncheckedEvent( on_detached );

			_attachments.Erase( i );
		}};

		Prepare();

		if ( count == 0 ) {
			RETURN_ERR( "module is not attached" );
		}

		while ( count > 0 )
		{
			Detach( first, count == 1 );
			Prepare();
		}
		return true;
	}

/*
=================================================
	_DetachAllAttachments
=================================================
*/
	void Module::_DetachAllAttachments ()
	{
		for (; not _attachments.Empty();)
		{
			_Detach( _attachments.Back().second );
		}
	}
	
/*
=================================================
	_DetachSelfFromParent
=================================================
*/
	void Module::_DetachSelfFromParent (const ModulePtr &parent)
	{
		if ( parent )
		{
			CHECK( parent->Send( ModuleMsg::DetachModule{ this }) );
		}
	}
	
/*
=================================================
	_DetachSelfFromAllParents
=================================================
*/
	void Module::_DetachSelfFromAllParents ()
	{
		for (; not _parents.Empty(); )
		{
			CHECK( _parents.Back()->Send( ModuleMsg::DetachModule{ this }) );
		}
	}
	
/*
=================================================
	_OnAttachedToParent
=================================================
*/
	bool Module::_OnAttachedToParent (const ModulePtr &parent)
	{
		CHECK_ERR( _parents.Count() < _moduleCfg.maxParents );

		_parents.Add( parent );
		return true;
	}
	
/*
=================================================
	_OnDetachedFromParent
=================================================
*/
	bool Module::_OnDetachedFromParent (const ModulePtr &parent)
	{
		CHECK( _parents.IsExist( parent ) );

		_parents.Erase( parent );
		return true;
	}

/*
=================================================
	_DetachSelfFromManager
=================================================
*/
	void Module::_DetachSelfFromManager ()
	{
		if ( not _manager )
			return;

		if ( _manager->GetThreadID() == this->GetThreadID() )
		{
			_manager->Send( ModuleMsg::RemoveFromManager{ this } );
		}
		else
		{
			CHECK( GlobalSystems()->taskModule->SendAsync( ModuleMsg::PushAsyncMessage{
						_manager->GetThreadID(),
						LAMBDA( mngr = _manager, self = ModuleWPtr(this) ) (GlobalSystemsRef)
						{
							mngr->Send( ModuleMsg::RemoveFromManager{ self });
						}}
			));
		}

		_SetManager( null );
	}
	
/*
=================================================
	_AttachSelfToManager
----
	helper function for per-thread modules that need to
	attach to manager in other thread, also works
	for other modules (with optimized version).
=================================================
*/
	bool Module::_AttachSelfToManager (const ModulePtr &mngr, UntypedID_t id, bool wait)
	{
		SharedPointerType<AttachModuleToManagerAsyncTask>	task;

		// attach to known manager
		if ( mngr )
		{
			//ASSERT( id == UntypedID_t(0) or id == mngr->GetModuleID() );

			if ( mngr->GetThreadID() == this->GetThreadID() )
			{
				// single-thread optimization
				CHECK( mngr->Send( ModuleMsg::AddToManager{ this }) );

				_SetManager( mngr );
				return true;
			}

			task = New< AttachModuleToManagerAsyncTask >( this, mngr );
		}
		else
		// find manager by moduleID and attach
		{
			ASSERT( id != UntypedID_t(0) and id != UntypedID_t(-1) );

			ModulePtr	where =	TModID::_ID == (GetModuleID() & TModID::_IDMask) ?  ModulePtr( GlobalSystems()->mainSystem.ptr() ) :
								OModID::_ID == (GetModuleID() & OModID::_IDMask) ?  ModulePtr( GlobalSystems()->parallelThread.ptr() ) :
								null;
			CHECK_ERR( where );

			task = New< AttachModuleToManagerAsyncTask >( this, where, id );
		}

		task->Execute();

		if ( wait )
		{
			CHECK_ERR( task->Wait() );
		}
		return true;
	}

/*
=================================================
	_SetManager
=================================================
*/
	void Module::_SetManager (const ModulePtr &mngr)
	{
		// don't forget to remove self from previous manager
		ASSERT( _manager == null or mngr == null );

		ModuleMsg::OnManagerChanged		msg{ _manager, mngr };

		_manager = mngr;

		if ( _manager )
		{
			const UntypedID_t	mask = GModID::_IDMask;

			CHECK( (_manager->GetModuleID() & mask) < (this->GetModuleID() & mask) );
		}

		CHECK( Send( msg ) );
	}

/*
=================================================
	_SetState
=================================================
*/
	bool Module::_SetState (EState newState)
	{
		if ( newState == _state )
			return true;

		// keep deleting state
		if ( _state == EState::Deleting )
			RETURN_ERR( "can't change 'deleteing' state" );

		// immutable state can be changed only to deleting state
		if ( _state == EState::ComposedImmutable )
		{
			if ( newState == EState::Deleting )
			{
				_state = newState;
				return true;
			}
			RETURN_ERR( "can't change state of immutable object!" );
		}

		// change to initial, deleting or error state
		if ( newState == EState::Initial or
			 newState == EState::Deleting or
			 _IsErrorState( newState ) )
		{
			_state = newState;
			return true;
		}

		// change error state to non-error state
		if ( _IsErrorState( _state ) )
		{
			if ( newState == EState::Initial or
				 newState == EState::Deleting )
			{
				_state = newState;
				return true;
			}

			RETURN_ERR( "can't change error state to non-error state, excepts 'initial' or 'deleting'" );
		}

		// Initial -> Linked -> ComposedMutable -> ComposedImmutable -> Deleting
		if ( newState > _state )
		{
			_state = newState;
			return true;
		}

		RETURN_ERR( "unknown or unsupported state combination used" );
	}
	
/*
=================================================
	_ValidateAllSubscriptions
=================================================
*/
	bool Module::_ValidateAllSubscriptions () const
	{
		return true; // TODO
		//return _msgHandler.Validate( GetSupportedMessages(), GetSupportedEvents() );
	}
	
/*
=================================================
	_DefCompose
=================================================
*/
	bool Module::_DefCompose (bool immutable)
	{
		CHECK_ERR( GetState() == EState::Linked );

		_SendForEachAttachments( ModuleMsg::Compose{} );
		//_SendEvent( ModuleMsg::Compose{} );
		
		// very paranoic check
		CHECK( _ValidateAllSubscriptions() );

		CHECK( _SetState( immutable ? EState::ComposedImmutable : EState::ComposedMutable ) );
		
		_SendUncheckedEvent( ModuleMsg::AfterCompose{} );
		return true;
	}
	
/*
=================================================
	_ClearMessageHandlers
=================================================
*/
	void Module::_ClearMessageHandlers ()
	{
		_msgHandler.Clear();
	}
	
/*
=================================================
	_FindAttachment
=================================================
*/
	bool Module::_FindAttachment (ArrayCRef<TypeId> messages, ArrayCRef<TypeId> events, OUT ModulePtr &result) const
	{
		for (auto& attachment : _attachments)
		{
			if ( attachment.second->SupportsAllMessages( messages ) and
				 attachment.second->GetSupportedEvents().HasAllTypes( events ) )
			{
				result = attachment.second;
				return true;
			}
		}
		return false;
	}
	
/*
=================================================
	_FindParent
=================================================
*/
	bool Module::_FindParent (ArrayCRef<TypeId> messages, ArrayCRef<TypeId> events, OUT ModulePtr &result) const
	{
		for (auto& parent : _parents)
		{
			if ( parent->SupportsAllMessages( messages ) and
				 parent->GetSupportedEvents().HasAllTypes( events ) )
			{
				result = parent;
				return true;
			}
		}
		return false;
	}

/*
=================================================
	_IsMutableState
=================================================
*/
	bool Module::_IsMutableState (EState state)
	{
		return	state == EState::Initial		or
				state == EState::Linked			or
				state == EState::ComposedMutable;
	}
	
/*
=================================================
	_IsComposedState
=================================================
*/
	bool Module::_IsComposedState (EState state)
	{
		return	state == EState::ComposedMutable	or
				state == EState::ComposedImmutable;
	}
	
/*
=================================================
	_IsComposedOrLinkedState
=================================================
*/
	bool Module::_IsComposedOrLinkedState (EState state)
	{
		return	state == EState::Linked				or
				state == EState::ComposedMutable	or
				state == EState::ComposedImmutable;
	}

/*
=================================================
	_IsErrorState
=================================================
*/
	bool Module::_IsErrorState (EState state)
	{
		return	state == EState::LinkingFailed			or
				state == EState::ComposingFailed		or
				state == EState::IncompleteAttachment;
	}
	
/*
=================================================
	_IsInitialState
=================================================
*/
	bool Module::_IsInitialState (EState state)
	{
		return	state == EState::Initial	or
				state == EState::LinkingFailed;	// previous linking was failed, but you can try again
	}

/*
=================================================
	GetModuleByID
=================================================
*/
	ModulePtr Module::GetModuleByID (UntypedID_t id)
	{
		CHECK_ERR( _ownThread == ThreadID::GetCurrent() );

		for (auto& attachment : _attachments)
		{
			if ( attachment.second->GetModuleID() == id )
				return attachment.second;
		}
		return null;
	}
	
/*
=================================================
	GetModuleByName
=================================================
*/
	ModulePtr Module::GetModuleByName (StringCRef name)
	{
		CHECK_ERR( _ownThread == ThreadID::GetCurrent() );
		
		for (auto& attachment : _attachments)
		{
			if ( attachment.first == name )
				return attachment.second;
		}
		return null;
	}
	
/*
=================================================
	SupportsAllMessages
=================================================
*/
	bool  Module::SupportsAllMessages (ArrayCRef<TypeId> messages) const
	{
		CHECK_ERR( _ownThread == ThreadID::GetCurrent() );

		for (auto& id : messages)
		{
			if ( not _msgHandler._handlers.CustomSearch().IsExist(MessageHandler::HandlerSearch{ id }) )
				return false;
		}
		return true;
	}
	
/*
=================================================
	SupportsAnyMessage
=================================================
*/
	bool  Module::SupportsAnyMessage (ArrayCRef<TypeId> messages) const
	{
		CHECK_ERR( _ownThread == ThreadID::GetCurrent() );

		for (auto& id : messages)
		{
			if ( _msgHandler._handlers.CustomSearch().IsExist(MessageHandler::HandlerSearch{ id }) )
				return true;
		}
		return false;
	}

/*
=================================================
	_FindModule_Impl
=================================================
*/
	bool Module::_FindModule_Impl (const ModuleMsg::FindModule &msg)
	{
		msg.result.Set( GetModuleByName( msg.name ) );
		return true;
	}
	
/*
=================================================
	_ModulesDeepSearch_Impl
=================================================
*/
	bool Module::_ModulesDeepSearch_Impl (const ModuleMsg::ModulesDeepSearch &)
	{
		TODO( "" );
		return false;
	}

/*
=================================================
	_AttachModule_Impl
=================================================
*/
	bool Module::_AttachModule_Impl (const ModuleMsg::AttachModule &msg)
	{
		CHECK( _Attach( msg.name, msg.newModule, false ) );
		return true;
	}
	
/*
=================================================
	_DetachModule_Impl
=================================================
*/
	bool Module::_DetachModule_Impl (const ModuleMsg::DetachModule &msg)
	{
		CHECK( _Detach( msg.oldModule ) );
		return true;
	}
	
/*
=================================================
	_ReplaceModule_Impl
=================================================
*/
	bool Module::_ReplaceModule_Impl (const ModuleMsg::ReplaceModule &msg)
	{
		CHECK_ERR( msg.newModule );

		ModulePtr	old_mod = msg.oldModule;

		if ( not msg.name.Empty() )
		{
			old_mod = GetModuleByName( msg.name );
			CHECK_ERR( old_mod.IsNull() or msg.oldModule.IsNull() or old_mod == msg.oldModule );
			old_mod = old_mod ? old_mod : msg.oldModule;	// module by name has more priority
		}
		CHECK_ERR( old_mod );

		Send( ModuleMsg::DetachModule{ old_mod } );
		Send( ModuleMsg::AttachModule{ msg.name, msg.newModule } );
		return true;
	}

/*
=================================================
	_Link_Impl
=================================================
*/
	bool Module::_Link_Impl (const ModuleMsg::Link &msg)
	{
		if ( _IsComposedOrLinkedState( GetState() ) )
			return true;	// already linked

		CHECK_ERR( _IsInitialState( GetState() ) );

		_SendForEachAttachments( msg );

		CHECK( _SetState( EState::Linked ) );
		
		_SendUncheckedEvent( ModuleMsg::AfterLink{} );
		return true;
	}
	
/*
=================================================
	_Compose_Impl
=================================================
*/
	bool Module::_Compose_Impl (const ModuleMsg::Compose &)
	{
		if ( _IsComposedState( GetState() ) )
			return true;	// already composed

		CHECK( _DefCompose( false ) );
		return true;
	}
	
/*
=================================================
	_Delete_Impl
=================================================
*/
	bool Module::_Delete_Impl (const ModuleMsg::Delete &)
	{
		_DetachAllAttachments();
		_DetachSelfFromAllParents();

		CHECK( _SetState( EState::Deleting ) );

		_DetachSelfFromManager();

		_msgHandler.Clear();

		return true;
	}

/*
=================================================
	_Update_Impl
=================================================
*/
	bool Module::_Update_Impl (const ModuleMsg::Update &msg)
	{
		CHECK_ERR( _IsComposedState( GetState() ) );

		_SendForEachAttachments( msg );
		return true;
	}
	
/*
=================================================
	_OnModuleAttached_Impl
=================================================
*/
	bool Module::_OnModuleAttached_Impl (const ModuleMsg::OnModuleAttached &msg)
	{
		if ( msg.attachedModule == this )
		{
			_OnAttachedToParent( msg.parent );
		}
		return true;
	}
	
/*
=================================================
	_OnModuleDetached_Impl
=================================================
*/
	bool Module::_OnModuleDetached_Impl (const ModuleMsg::OnModuleDetached &msg)
	{
		if ( msg.detachedModule == this )
		{
			if ( msg.isLast )
				_OnDetachedFromParent( msg.parent );
		}
		else
		{
			// TODO: msg.detachedModule->UnsubscribeAll( this ); ?
			UnsubscribeAll( msg.detachedModule );
		}
		return true;
	}
	
/*
=================================================
	_FindModule_Empty
=================================================
*/
	bool Module::_FindModule_Empty (const ModuleMsg::FindModule &)
	{
		return false;
	}
	
/*
=================================================
	_ModulesDeepSearch_Empty
=================================================
*/
	bool Module::_ModulesDeepSearch_Empty (const ModuleMsg::ModulesDeepSearch &)
	{
		return false;
	}

/*
=================================================
	_AttachModule_Empty
=================================================
*/
	bool Module::_AttachModule_Empty (const ModuleMsg::AttachModule &)
	{
		return false;
	}
	
/*
=================================================
	_DetachModule_Empty
=================================================
*/
	bool Module::_DetachModule_Empty (const ModuleMsg::DetachModule &)
	{
		return false;
	}
	
/*
=================================================
	_DetachModule_Empty
=================================================
*/
	bool _ReplaceModule_Empty (const ModuleMsg::ReplaceModule &)
	{
		return false;
	}

/*
=================================================
	_OnManagerChanged_Empty
=================================================
*/
	bool Module::_OnManagerChanged_Empty (const ModuleMsg::OnManagerChanged &)
	{
		return true;
	}

/*
=================================================
	_Update_Empty
=================================================
*/
	bool Module::_Update_Empty (const ModuleMsg::Update &)
	{
		CHECK_ERR( _IsComposedState( GetState() ) );
		return true;
	}
	
/*
=================================================
	_Compose_Empty
=================================================
*/
	bool Module::_Compose_Empty (const ModuleMsg::Compose &)
	{
		if ( _IsComposedState( GetState() ) )
			return true;	// already composed

		CHECK_ERR( GetState() == EState::Linked );

		CHECK( _SetState( EState::ComposedMutable ) );
		
		_SendUncheckedEvent( ModuleMsg::AfterCompose{} );
		return true;
	}

/*
=================================================
	_Link_Empty
=================================================
*/
	bool Module::_Link_Empty (const ModuleMsg::Link &)
	{
		if ( _IsComposedOrLinkedState( GetState() ) )
			return true;	// already linked

		CHECK( _SetState( EState::Linked ) );
		
		_SendUncheckedEvent( ModuleMsg::AfterLink{} );
		return true;
	}
	
/*
=================================================
	_Delete_Empty
=================================================
*/
	bool Module::_Delete_Empty (const ModuleMsg::Delete &)
	{
		CHECK( _SetState( EState::Deleting ) );

		_msgHandler.Clear();
		return true;
	}

}	// Base
}	// Engine
