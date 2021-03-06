// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "Engine/Platforms/Public/OS/Input.h"

namespace Engine
{
namespace Platforms
{

	//
	// Input Manger
	//

	class InputManager final : public Module
	{
	// types
	private:
		using SupportedEvents_t		= Module::SupportedEvents_t;

		using InputThreads_t		= Set< ModulePtr >;


	// constants
	private:
		static const TypeIdList		_eventTypes;

		
	// variables
	private:
		InputThreads_t		_threads;
		

	// methods
	public:
		InputManager (UntypedID_t, GlobalSystemsRef gs, const CreateInfo::InputManager &ci);
		~InputManager ();
		
		static void Register ();
		static void Unregister ();

		
	// message handlers
	private:
		bool _AddToManager (const ModuleMsg::AddToManager &);
		bool _RemoveFromManager (const ModuleMsg::RemoveFromManager &);
		
	private:
		static ModulePtr _CreateInputThread (UntypedID_t, GlobalSystemsRef, const CreateInfo::InputThread &);
		static ModulePtr _CreateInputManager (UntypedID_t, GlobalSystemsRef, const CreateInfo::InputManager &);
	};


}	// Platforms
}	// Engine
