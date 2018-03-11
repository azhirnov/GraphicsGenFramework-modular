// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "Engine/Config/Engine.Config.h"

#ifdef GRAPHICS_API_VULKAN

#include "Engine/Platforms/Vulkan/Impl/Vk1Device.h"
#include "Engine/Platforms/Vulkan/Impl/Vk1Messages.h"

namespace Engine
{
namespace PlatformVK
{

	//
	// Vulkan Base Object
	//

	class Vk1BaseObject : public StaticRefCountedObject
	{
	// variables
	private:
		Ptr< Vk1Device >	_vkDevice;


	// emthods
	public:
		explicit Vk1BaseObject (Ptr<Vk1Device> dev) : _vkDevice(dev) {}

		Ptr< Vk1Device >	GetDevice ()	const	{ return _vkDevice; }
		vk::VkDevice		GetVkDevice ()	const	{ return GetDevice()->GetLogicalDevice(); }
	};


}	// PlatformVK
}	// Engine

#endif	// GRAPHICS_API_VULKAN
