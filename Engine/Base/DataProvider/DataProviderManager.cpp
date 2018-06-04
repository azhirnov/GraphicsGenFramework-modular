// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "Engine/Base/DataProvider/DataProviderObjectsConstructor.h"
#include "Engine/Base/DataProvider/DataMessages.h"
#include "Engine/Base/Main/MainSystem.h"

namespace Engine
{
namespace Base
{

	//
	// Data Provider Manager
	//

	class DataProviderManager : public Module
	{
	// types
	private:
		using SupportedMessages_t	= Module::SupportedMessages_t::Erase< MessageListFrom<
											ModuleMsg::Update
										> >::Append< MessageListFrom<
											ModuleMsg::AddToManager,
											ModuleMsg::RemoveFromManager,
											DSMsg::GetDataProviderForURI
										> >;

		using SupportedEvents_t		= Module::SupportedEvents_t;
		
		using DataProviders_t		= Set< ModulePtr >;

		using DataProviderMsgList_t	= MessageListFrom< DSMsg::IsUriExists >;


	// constants
	private:
		static const TypeIdList		_msgTypes;
		static const TypeIdList		_eventTypes;


	// variables
	private:
		DataProviders_t		_providers;


	// methods
	public:
		DataProviderManager (UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::DataProviderManager &info);
		~DataProviderManager ();
		

	// message handlers
	private:
		bool _Delete (const Message< ModuleMsg::Delete > &);
		bool _AddToManager (const Message< ModuleMsg::AddToManager > &);
		bool _RemoveFromManager (const Message< ModuleMsg::RemoveFromManager > &);
		bool _GetDataProviderForURI (const Message< DSMsg::GetDataProviderForURI > &);
	};
//-----------------------------------------------------------------------------


	const TypeIdList	DataProviderManager::_msgTypes{ UninitializedT< SupportedMessages_t >() };
	const TypeIdList	DataProviderManager::_eventTypes{ UninitializedT< SupportedEvents_t >() };

/*
=================================================
	constructor
=================================================
*/
	DataProviderManager::DataProviderManager (UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::DataProviderManager &ci) :
		Module( gs, ModuleConfig{ id, 1 }, &_msgTypes, &_eventTypes )
	{
		SetDebugName( "DataProviderManager" );

		_SubscribeOnMsg( this, &DataProviderManager::_OnModuleAttached_Impl );
		_SubscribeOnMsg( this, &DataProviderManager::_OnModuleDetached_Impl );
		_SubscribeOnMsg( this, &DataProviderManager::_AttachModule_Empty );
		_SubscribeOnMsg( this, &DataProviderManager::_DetachModule_Empty );
		_SubscribeOnMsg( this, &DataProviderManager::_FindModule_Empty );
		_SubscribeOnMsg( this, &DataProviderManager::_ModulesDeepSearch_Empty );
		_SubscribeOnMsg( this, &DataProviderManager::_Link_Impl );
		_SubscribeOnMsg( this, &DataProviderManager::_Compose_Impl );
		_SubscribeOnMsg( this, &DataProviderManager::_Delete );
		_SubscribeOnMsg( this, &DataProviderManager::_AddToManager );
		_SubscribeOnMsg( this, &DataProviderManager::_RemoveFromManager );
		_SubscribeOnMsg( this, &DataProviderManager::_GetDataProviderForURI );
		
		CHECK( _ValidateMsgSubscriptions() );
	}
	
/*
=================================================
	destructor
=================================================
*/
	DataProviderManager::~DataProviderManager ()
	{
		LOG( "DataProviderManager finalized", ELog::Debug );

		ASSERT( _providers.Empty() );
	}
	
/*
=================================================
	_Delete
=================================================
*/
	bool DataProviderManager::_Delete (const Message< ModuleMsg::Delete > &msg)
	{
		return Module::_Delete_Impl( msg );
	}

/*
=================================================
	_AddToManager
=================================================
*/
	bool DataProviderManager::_AddToManager (const Message< ModuleMsg::AddToManager > &msg)
	{
		CHECK_ERR( msg->module );
		CHECK_ERR( msg->module->GetSupportedMessages().HasAllTypes< DataProviderMsgList_t >() );
		//CHECK_ERR( msg->module->GetSupportedEvents().HasAllTypes< DataProviderEventList_t >() );
		ASSERT( not _providers.IsExist( msg->module ) );

		_providers.Add( msg->module );
		return true;
	}
	
/*
=================================================
	_RemoveFromManager
=================================================
*/
	bool DataProviderManager::_RemoveFromManager (const Message< ModuleMsg::RemoveFromManager > &msg)
	{
		CHECK_ERR( msg->module );

		ModulePtr	module = msg->module.Lock();

		if ( not module )
			return false;

		ASSERT( _providers.IsExist( module ) );

		_providers.Erase( module );
		return true;
	}
	
/*
=================================================
	_GetDataProviderForURI
=================================================
*/
	bool DataProviderManager::_GetDataProviderForURI (const Message< DSMsg::GetDataProviderForURI > &msg)
	{
		for (auto& provider : _providers)
		{
			Message< DSMsg::IsUriExists >	is_exists{ msg->uri };
			provider->Send( is_exists );

			if ( is_exists->result.Get(false) )
			{
				msg->result.Set( provider );
				break;
			}
		}
		return true;
	}
//-----------------------------------------------------------------------------
	
/*
=================================================
	CreateDataProviderManager
=================================================
*/
	ModulePtr DataProviderObjectsConstructor::CreateDataProviderManager (ModuleMsg::UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::DataProviderManager &ci)
	{
		return New< DataProviderManager >( id, gs, ci );
	}

}	// Base
}	// Engine
