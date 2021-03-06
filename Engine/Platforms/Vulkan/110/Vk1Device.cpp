// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "Core/Config/Engine.Config.h"

#ifdef GRAPHICS_API_VULKAN

#include "Engine/Platforms/Vulkan/110/Vk1Device.h"
#include "Engine/Platforms/Vulkan/110/vulkan1_utils.h"
#include "Engine/Platforms/Public/GPU/Framebuffer.h"
#include "Engine/Platforms/Public/GPU/RenderPass.h"
#include "Engine/Platforms/Public/GPU/CommandBuffer.h"
#include "Engine/Platforms/Vulkan/VulkanObjectsConstructor.h"
#include "Engine/Platforms/Vulkan/110/Vk1SwapchainImage.h"


namespace Engine
{
namespace PlatformVK
{
	using namespace vk;

/*
=================================================
	constructor
=================================================
*/
	Vk1Device::Vk1Device (GlobalSystemsRef gs) :
		BaseObject( gs ),
		_logicalDevice( VK_NULL_HANDLE ),
		_physicalDevice( VK_NULL_HANDLE ),
		_instance( VK_NULL_HANDLE ),
		_surface( VK_NULL_HANDLE ),
		_swapchain( VK_NULL_HANDLE ),
		_vsync( false ),
		_depthStencilView( VK_NULL_HANDLE ),
		_colorPixelFormat( EPixelFormat::Unknown ),
		_depthStencilPixelFormat( EPixelFormat::Unknown ),
		_colorFormat( VK_FORMAT_UNDEFINED ),
		_colorSpace( VK_COLOR_SPACE_MAX_ENUM_KHR ),
		_depthStencilFormat( VK_FORMAT_UNDEFINED ),
		_queueFamilyIndex( UMax ),
		_queueFamily(),
		_currentImageIndex( UMax ),
		_debugCallback( VK_NULL_HANDLE ),
		_enableDebugMarkers( false ),
		_primiryFunctionsLoaded( false ),
		_isInstanceFunctionsLoaded( false ),
		_isDeviceFunctionsLoaded( false ),
		_debugReportCounter( 0 )
	{
		SetDebugName( "Vk1Device" );

		_imageBuffers.Reserve( 8 );
	}
	
/*
=================================================
	destructor
=================================================
*/
	Vk1Device::~Vk1Device ()
	{
		CHECK( not IsInstanceCreated() );
		CHECK( not HasPhyiscalDevice() );
		CHECK( not IsDeviceCreated() );
		CHECK( not IsSurfaceCreated() );
		CHECK( not IsSwapchainCreated() );
		CHECK( not IsDebugCallbackCreated() );

		if ( _debugReportCounter > 0 )
		{
			WARNING( "Threre are a few warnings, check debug output!" );
		}
	}
	
/*
=================================================
	_LoadFunctions
=================================================
*/
	bool Vk1Device::_LoadFunctions () const
	{
		bool	load = (_isDeviceFunctionsLoaded != IsDeviceCreated() or
						_isInstanceFunctionsLoaded != IsInstanceCreated() or
						not _primiryFunctionsLoaded);

		if ( load ) {
			CHECK_ERR( Vk1_Init( _instance, _logicalDevice ) );
		}

		_primiryFunctionsLoaded		= true;
		_isInstanceFunctionsLoaded	= IsInstanceCreated();
		_isDeviceFunctionsLoaded	= IsDeviceCreated();

		return true;
	}
	
/*
=================================================
	_LoadInstanceLayers
=================================================
*/
	bool Vk1Device::_LoadInstanceLayers () const
	{
		if ( not _instanceLayers.Empty() )
			return true;

		_LoadFunctions();
		
		uint32_t	count = 0;
		VK_CALL( vkEnumerateInstanceLayerProperties( OUT &count, null ) );

		if ( count == 0 ) {
			_instanceLayers << VkLayerProperties{};
			return true;
		}

		_instanceLayers.Resize( count );
		VK_CALL( vkEnumerateInstanceLayerProperties( OUT &count, OUT _instanceLayers.ptr() ) );

		return true;
	}
	
/*
=================================================
	_LoadInstanceExtensions
=================================================
*/
	bool Vk1Device::_LoadInstanceExtensions () const
	{
		if ( not _instanceExtensions.Empty() )
			return true;

		_LoadFunctions();
		
		uint32_t	count = 0;
		VK_CALL( vkEnumerateInstanceExtensionProperties( null, OUT &count, null ) );

		if ( count == 0 ) {
			_instanceExtensions << "";
			return true;
		}

		Array< VkExtensionProperties >		inst_ext;
		inst_ext.Resize( count );

		VK_CALL( vkEnumerateInstanceExtensionProperties( null, OUT &count, OUT inst_ext.ptr() ) );

		for (auto& ext : inst_ext) {
			_instanceExtensions.Add( StringCRef(ext.extensionName) );
		}
		return true;
	}

/*
=================================================
	CreateInstance
=================================================
*/
	bool Vk1Device::CreateInstance (StringCRef applicationName, uint32_t applicationVersion,
									 uint32_t vulkanVersion, ExtensionNames_t ext, ValidationLayers_t layers)
	{
		CHECK_ERR( not IsInstanceCreated() );

		_LoadFunctions();

		VkApplicationInfo		app_info				= {};
		VkInstanceCreateInfo	instance_create_info	= {};
		Array< const char* >	instance_layers			= layers;
		Array< const char* >	instance_extensions		= { VK_KHR_SURFACE_EXTENSION_NAME };

		instance_extensions << ext;

		CHECK_ERR( _CheckLayers( INOUT instance_layers ) );
		CHECK_ERR( _CheckExtensions( INOUT instance_extensions ) );

		app_info.sType				= VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.apiVersion			= vulkanVersion;

		app_info.pApplicationName	= applicationName.cstr();
		app_info.applicationVersion	= applicationVersion;

		app_info.pEngineName		= Engine::ENGINE_NAME;
		app_info.engineVersion		= Engine::ENGINE_VERSION;

		
		instance_create_info.sType						= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instance_create_info.pApplicationInfo			= &app_info;

		instance_create_info.enabledExtensionCount		= uint32_t(instance_extensions.Count());
		instance_create_info.ppEnabledExtensionNames	= instance_extensions.RawPtr();
		
		//instance_create_info.enabledLayerCount			= uint32_t(instance_layers.Count());
		//instance_create_info.ppEnabledLayerNames		= instance_layers.RawPtr();


		VK_CHECK( vkCreateInstance( &instance_create_info, null, OUT &_instance ) );


		// update instance extensions
		{
			_instanceExtensions.Clear();

			for (auto& ie : instance_extensions) {
				_instanceExtensions << ie;
			}

			if ( _instanceExtensions.Empty() )
				_instanceExtensions << "";
		}

		return true;
	}
	
/*
=================================================
	DestroyInstance
=================================================
*/
	bool Vk1Device::DestroyInstance ()
	{
		if ( _instance == VK_NULL_HANDLE )
			return true;

		CHECK_ERR( not IsDeviceCreated() );
		CHECK_ERR( not IsSurfaceCreated() );
		CHECK_ERR( not IsSwapchainCreated() );
		CHECK_ERR( not IsDebugCallbackCreated() );

		vkDestroyInstance( _instance, null );

		_physicalDevice	= VK_NULL_HANDLE;
		_instance		= VK_NULL_HANDLE;

		_isDeviceFunctionsLoaded	= false;
		_isInstanceFunctionsLoaded	= false;
		_primiryFunctionsLoaded		= false;

		_instanceLayers.Clear();
		_instanceExtensions.Clear();
		
		Vk1_Delete();
		return true;
	}
	
/*
=================================================
	HasLayer
=================================================
*/
	bool Vk1Device::HasLayer (StringCRef name) const
	{
		_LoadInstanceLayers();

		// TODO: optimize search
		FOR( i, _instanceLayers )
		{
			if ( name.EqualsIC( _instanceLayers[i].layerName ) )
				return true;
		}
		return false;
	}
	
/*
=================================================
	HasExtension
=================================================
*/
	bool Vk1Device::HasExtension (StringCRef name) const
	{
		_LoadInstanceExtensions();

		return _instanceExtensions.IsExist( name );
	}

/*
=================================================
	CreateDebugCallback
=================================================
*/
	bool Vk1Device::CreateDebugCallback (VkDebugReportFlagBitsEXT flags)
	{
		CHECK_ERR( IsInstanceCreated() );
		CHECK_ERR( not IsDebugCallbackCreated() );

		_LoadFunctions();

		VkDebugReportCallbackCreateInfoEXT	dbg_callback_info = {};

		dbg_callback_info.sType			= VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		dbg_callback_info.flags			= flags;
		dbg_callback_info.pfnCallback	= &_DebugReportCallback;
		dbg_callback_info.pUserData		= this;

		VK_CHECK( vkCreateDebugReportCallbackEXT( _instance, &dbg_callback_info, null, OUT &_debugCallback ) );
		return true;
	}
	
/*
=================================================
	DestroyDebugCallback
=================================================
*/
	bool Vk1Device::DestroyDebugCallback ()
	{
		if ( _debugCallback == VK_NULL_HANDLE )
			return true;

		CHECK_ERR( IsInstanceCreated() );

		vkDestroyDebugReportCallbackEXT( _instance, _debugCallback, null );

		_debugCallback = VK_NULL_HANDLE;
		return true;
	}

/*
=================================================
	GetPhysicalDeviceInfo
=================================================
*/
	bool Vk1Device::GetPhysicalDeviceInfo (OUT AppendableAdaptor<DeviceInfo> deviceInfo) const
	{
		CHECK_ERR( IsInstanceCreated() );

		_LoadFunctions();
		
		uint32_t						count	= 0;
		Array< VkPhysicalDevice >		devices;
		Array< VkDisplayPropertiesKHR >	display_props;		display_props.Reserve( 8 );

		VK_CALL( vkEnumeratePhysicalDevices( _instance, OUT &count, null ) );
		CHECK_ERR( count > 0 );

		devices.Resize( count );
		VK_CALL( vkEnumeratePhysicalDevices( _instance, OUT &count, OUT devices.ptr() ) );

		FOR( i, devices )
		{
			VkPhysicalDeviceProperties			prop	 = {};
			VkPhysicalDeviceFeatures			feat	 = {};
			VkPhysicalDeviceMemoryProperties	mem_prop = {};
			DeviceInfo							info;

			vkGetPhysicalDeviceProperties( devices[i], OUT &prop );
			vkGetPhysicalDeviceFeatures( devices[i], OUT &feat );
			vkGetPhysicalDeviceMemoryProperties( devices[i], OUT &mem_prop );
			// vkGetPhysicalDeviceQueueFamilyProperties
			// vkGetPhysicalDeviceSparseImageFormatProperties
			// vkGetPhysicalDeviceDisplayPlanePropertiesKHR
			// vkGetPhysicalDeviceFeatures2KHR
			// vkGetPhysicalDeviceProperties2KHR
			// vkGetPhysicalDeviceMemoryProperties2KHR
			// vkGetPhysicalDeviceSparseImageFormatProperties2KHR

			/*uint32_t	prop_count = 0;
			VK_CALL( vkGetPhysicalDeviceDisplayPropertiesKHR( devices[i], OUT &prop_count, null ) );

			if ( prop_count > 0 )
			{
				display_props.Resize( prop_count );
				VK_CALL( vkGetPhysicalDeviceDisplayPropertiesKHR( devices[i], OUT &prop_count, OUT display_props.ptr() ) );

				FOR( j, display_props )
				{
				}
			}*/


			info.id				= devices[i];
			info.device			= prop.deviceName;
			info.version		= prop.apiVersion;
			info.integratedGPU	= prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
			info.isGPU			= (prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU or
								   prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);
			info.isCPU			= prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU;
			info.maxInvocations	= prop.limits.maxComputeWorkGroupInvocations;

			info.supportsTesselation	= feat.tessellationShader;
			info.supportsGeometryShader	= feat.geometryShader;

			info.globalMemory		= _CalcTotalMemory( mem_prop );
			info.computeSharedMem	= BytesU(prop.limits.maxComputeSharedMemorySize);

			deviceInfo.PushBack( RVREF(info) );
		}
		return true;
	}
	
/*
=================================================
	CreatePhysicalDevice
=================================================
*/
	bool Vk1Device::CreatePhysicalDevice (VkPhysicalDevice id)
	{
		CHECK_ERR( IsInstanceCreated() );
		
		_LoadFunctions();

		_physicalDevice = id;

		vkGetPhysicalDeviceProperties( _physicalDevice, OUT &_deviceProperties );
		vkGetPhysicalDeviceFeatures( _physicalDevice, OUT &_deviceFeatures );
		vkGetPhysicalDeviceMemoryProperties( _physicalDevice, OUT &_deviceMemoryProperties );

		_UpdateProperties();
		return true;
	}
	
/*
=================================================
	_DeviceTypeToString
=================================================
*/
	StringCRef Vk1Device::_DeviceTypeToString (VkPhysicalDeviceType value)
	{
		switch ( value )
		{
			case VK_PHYSICAL_DEVICE_TYPE_OTHER :			return "Other";
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU :	return "Intergrated GPU";
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU :		return "Discrete GPU";
			case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU :		return "Virtual GPU";
			case VK_PHYSICAL_DEVICE_TYPE_CPU :				return "CPU";
		}
		RETURN_ERR( "unknown physical device type!" );
	}
	
/*
=================================================
	_CalcTotalMemory
=================================================
*/
	BytesU Vk1Device::_CalcTotalMemory (VkPhysicalDeviceMemoryProperties memProps)
	{
		BytesU	total;

		for (uint32_t j = 0; j < memProps.memoryTypeCount; ++j)
		{
			if ( EnumEq( memProps.memoryTypes[j].propertyFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ) )
			{
				const uint32_t idx = memProps.memoryTypes[j].heapIndex;

				if ( EnumEq( memProps.memoryHeaps[idx].flags, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT ) )
				{
					total += BytesU( memProps.memoryHeaps[idx].size );
					memProps.memoryHeaps[idx].size = 0;
				}
			}
		}
		return total;
	}

/*
=================================================
	_UpdateProperties
=================================================
*/
	void Vk1Device::_UpdateProperties ()
	{
		_properties.maxComputeWorkGroupInvocations	= _deviceProperties.limits.maxComputeWorkGroupInvocations;
		_properties.maxComputeWorkGroupSize			= ReferenceCast<uint3>(_deviceProperties.limits.maxComputeWorkGroupSize);
		_properties.maxComputeWorkGroupCount		= ReferenceCast<uint3>(_deviceProperties.limits.maxComputeWorkGroupCount);

		_properties.explicitMemoryObjects			= true;
	}
	
/*
=================================================
	WritePhysicalDeviceInfo
=================================================
*/
	void Vk1Device::WritePhysicalDeviceInfo () const
	{
		CHECK_ERR( HasPhyiscalDevice(), void() );

		struct ApiVersionBits {
			uint	patch : 12;
			uint	minor : 10;
			uint	major : 10;
		};

		const ApiVersionBits	api_ver = ReferenceCast<ApiVersionBits>(_deviceProperties.apiVersion);

		String	str;

		str << "Vulkan info\n---------------------";
		str << "\nversion:                 " << api_ver.major <<'.'<< api_ver.minor <<'.'<< api_ver.patch;
		str << "\ndevice name:             " << _deviceProperties.deviceName;
		str << "\ndevice type:             " << _DeviceTypeToString( _deviceProperties.deviceType );
		str << "\nglobal memory:           " << ToString( _CalcTotalMemory( _deviceMemoryProperties ) );

		str << "\npush constants:          " << ToString( BytesU(_deviceProperties.limits.maxPushConstantsSize) );
		str << "\nuniform buf size:        " << ToString( BytesU(_deviceProperties.limits.maxUniformBufferRange) );
		str << "\nstorage buf size:        " << ToString( BytesU(_deviceProperties.limits.maxStorageBufferRange) );
		str << "\nmax mem allocations:     " << _deviceProperties.limits.maxMemoryAllocationCount;
		str << "\nmax sampler allocations: " << _deviceProperties.limits.maxSamplerAllocationCount;
		str << "\nper stage resources:     " << _deviceProperties.limits.maxPerStageResources;
		
		str << "\nmax color attachments:   " << _deviceProperties.limits.maxColorAttachments;
		str << "\nmax viewports:           " << _deviceProperties.limits.maxViewports;
		str << "\nmax anisotropy:          " << _deviceProperties.limits.maxSamplerAnisotropy;
		str << "\nmax tess gen level:      " << _deviceProperties.limits.maxTessellationGenerationLevel;
		str << "\nmax patch vertices:      " << _deviceProperties.limits.maxTessellationPatchSize;
		str << "\nmax attribs:             " << _deviceProperties.limits.maxVertexInputAttributes;
		str << "\nmax vb bindings:         " << _deviceProperties.limits.maxVertexInputBindings;

		// compute
		str << "\ncompute shared mem:      " << ToString( BytesU(_deviceProperties.limits.maxComputeSharedMemorySize) );
		str << "\ncompute invocations:     " << _deviceProperties.limits.maxComputeWorkGroupInvocations;
		str << "\ncompute local size:      " << ToString( ReferenceCast<uint3>(_deviceProperties.limits.maxComputeWorkGroupSize) );
		str << "\ncompute work groups:     " << ToString( ReferenceCast<uint3>(_deviceProperties.limits.maxComputeWorkGroupCount) );

		str << "\n---------------------";

		LOG( str, ELog::Info | ELog::SpoilerFlag );
	}
	
/*
=================================================
	CreateDevice
=================================================
*/
	bool Vk1Device::CreateDevice (const VkPhysicalDeviceFeatures &enabledFeatures,
								   EQueueFamily::bits queueFamilies,
								   ExtensionNames_t enabledExtensions)
	{
		const bool useSwapchain = queueFamilies[ EQueueFamily::Present ];

		CHECK_ERR( HasPhyiscalDevice() );
		CHECK_ERR( useSwapchain == IsSurfaceCreated() );
		CHECK_ERR( not IsDeviceCreated() );

		Array< VkDeviceQueueCreateInfo >	queue_infos;
		Array< const char * >				device_extensions	= enabledExtensions;
		VkDeviceCreateInfo					device_info			= {};

		queue_infos.PushBack({});
		CHECK_ERR( _GetQueueCreateInfo( OUT queue_infos.Back(), queueFamilies ) );
		
		_queueFamily		= queueFamilies;
		_queueFamilyIndex	= queue_infos.Back().queueFamilyIndex;

		if ( _queueFamily[ EQueueFamily::Present ] )
			device_extensions << VK_KHR_SWAPCHAIN_EXTENSION_NAME;
		
		CHECK_ERR( _CheckDeviceExtensions( INOUT device_extensions ) );

		_enableDebugMarkers = HasExtension( VK_EXT_DEBUG_MARKER_EXTENSION_NAME );
		
		device_info.sType					= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_info.queueCreateInfoCount	= uint32_t(queue_infos.Count());
		device_info.pQueueCreateInfos		= queue_infos.ptr();
		device_info.pEnabledFeatures		= &enabledFeatures;
		
		if ( not device_extensions.Empty() )
		{
			device_info.enabledExtensionCount	= uint32_t(device_extensions.Count());
			device_info.ppEnabledExtensionNames	= device_extensions.RawPtr();
		}

		VK_CHECK( vkCreateDevice( _physicalDevice, &device_info, null, OUT &_logicalDevice ) );

		if ( &_deviceFeatures != &enabledFeatures )
			_deviceFeatures = enabledFeatures;

		// reload function pointers for current device
		_LoadFunctions();


		// update device extensions
		{
			_deviceExtensions.Clear();

			for (auto& de : device_extensions) {
				_deviceExtensions << de;
			}

			if ( _deviceExtensions.Empty() )
				_deviceExtensions << "";
		}


		// write extensions to log
		{
			String	log = "Vulkan layers:";
			
			FOR( i, _instanceLayers ) {
				log << _instanceLayers[i].layerName << " (" << _instanceLayers[i].description << ")\n";
			}
			log << "\nVulkan instance extensions:";

			FOR( i, _instanceExtensions ) {
				log << (i ? ", " : "") << ((i&3) ? "" : "\n") << _instanceExtensions[i];
			}
			log << "\nVulkan device extensions:";

			FOR( i, _deviceExtensions ) {
				log << (i ? ", " : "") << ((i&3) ? "" : "\n") << _deviceExtensions[i];
			}
			log << "\n------------------------";

			LOG( log, ELog::Info | ELog::SpoilerFlag );
		}

		return true;
	}
	
/*
=================================================
	DestroyDevice
=================================================
*/
	bool Vk1Device::DestroyDevice ()
	{
		if ( _logicalDevice == VK_NULL_HANDLE )
			return true;

		CHECK_ERR( IsInstanceCreated() );
		CHECK_ERR( not IsSwapchainCreated() );
		
		vkDestroyDevice( _logicalDevice, null );

		_logicalDevice		= VK_NULL_HANDLE;
		_queueFamilyIndex	= UMax;
		_queueFamily		= EQueueFamily::bits();

		// unload function pointers that spcified for destroyed device
		_LoadFunctions();

		_deviceExtensions.Clear();
		return true;
	}
	
/*
=================================================
	DeviceWaitIdle
=================================================
*/
	void Vk1Device::DeviceWaitIdle ()
	{
		ASSERT( IsDeviceCreated() );

		// TODO: check device lost error
		VK_CALL( vkDeviceWaitIdle( _logicalDevice ) );
	}

/*
=================================================
	CreateSwapchain
=================================================
*/
	bool Vk1Device::CreateSwapchain (const uint2 &size, bool vsync, uint32_t imageArrayLayers, EPixelFormat::type depthStencilFormat, MultiSamples samples,
									 EImageUsage::bits colorImageUsage, EImageUsage::bits depthStencilImageUsage)
	{
		CHECK_ERR( HasPhyiscalDevice() );
		CHECK_ERR( IsDeviceCreated() );
		CHECK_ERR( IsSurfaceCreated() );
		//CHECK_ERR( not IsSwapchainCreated() );

		VkSurfaceCapabilitiesKHR	surf_caps;
		VK_CHECK( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( _physicalDevice, _surface, OUT &surf_caps ) );

		VkSwapchainKHR				old_swapchain	= _swapchain;
		VkSwapchainCreateInfoKHR	swapchain_info	= {};
		
		swapchain_info.sType					= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchain_info.pNext					= null;
		swapchain_info.surface					= _surface;
		swapchain_info.imageFormat				= _colorFormat;
		swapchain_info.imageColorSpace			= _colorSpace;
		swapchain_info.imageExtent				= { size.x, size.y };
		swapchain_info.imageArrayLayers			= imageArrayLayers;
		swapchain_info.queueFamilyIndexCount	= 0;
		swapchain_info.pQueueFamilyIndices		= null;
		swapchain_info.oldSwapchain				= old_swapchain;
		swapchain_info.clipped					= VK_TRUE;
		
		_GetSurfaceImageCount( OUT swapchain_info.minImageCount, surf_caps );
		_GetSurfaceTransform( OUT swapchain_info.preTransform, surf_caps );
		_GetSwapChainExtent( INOUT swapchain_info.imageExtent, surf_caps );
		_GetPresentMode( OUT swapchain_info.presentMode, vsync );
		_GetSharingMode( OUT swapchain_info.imageSharingMode );
		CHECK_ERR( _GetImageUsage( OUT swapchain_info.imageUsage, swapchain_info.presentMode, colorImageUsage, surf_caps ) );
		CHECK_ERR( _GetCompositeAlpha( OUT swapchain_info.compositeAlpha, surf_caps ) );

		VK_CHECK( vkCreateSwapchainKHR( _logicalDevice, &swapchain_info, null, OUT &_swapchain ) );

		_surfaceSize.x	= swapchain_info.imageExtent.width;
		_surfaceSize.y	= swapchain_info.imageExtent.height;
		_vsync			= vsync;

		// destroy obsolete resources
		_DeleteSwapchain( old_swapchain );
		_DeleteFramebuffers();

		// create dependent resources
		CHECK_ERR( _CreateColorAttachment( samples, Vk1EnumRevert( swapchain_info.imageUsage ) ) );
		CHECK_ERR( _CreateDepthStencilAttachment( depthStencilFormat, depthStencilImageUsage ) );
		CHECK_ERR( _CreateRenderPass() );
		CHECK_ERR( _CreateFramebuffers() );

		return true;
	}
	
/*
=================================================
	_GetCompositeAlpha
=================================================
*/
	bool Vk1Device::_GetCompositeAlpha (OUT VkCompositeAlphaFlagBitsKHR &compositeAlpha, const VkSurfaceCapabilitiesKHR &surfaceCaps) const
	{
		const VkCompositeAlphaFlagBitsKHR		composite_alpha_flags[] = {
			VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
			VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
		};
		
		compositeAlpha = VK_COMPOSITE_ALPHA_FLAG_BITS_MAX_ENUM_KHR;

		for (auto& flag : composite_alpha_flags)
		{
			if ( EnumEq( surfaceCaps.supportedCompositeAlpha, flag ) )
			{
				compositeAlpha = flag;
				return true;
			}
		}

		RETURN_ERR( "no suitable composite alpha flags found!" );
	}

/*
=================================================
	RecreateSwapchain
=================================================
*/
	bool Vk1Device::RecreateSwapchain (const uint2 &size)
	{
		CHECK_ERR( IsSwapchainCreated() );
		
		// TODO: check device lost error
		VK_CALL( vkDeviceWaitIdle( _logicalDevice ) );

		CHECK_ERR( CreateSwapchain( size, _vsync ) );	// TODO imageArrayLayers
		
		// TODO: check device lost error
		VK_CALL( vkDeviceWaitIdle( _logicalDevice ) );
		return true;
	}

	bool Vk1Device::RecreateSwapchain ()
	{
		return RecreateSwapchain( _surfaceSize );
	}

/*
=================================================
	DestroySwapchain
=================================================
*/
	bool Vk1Device::DestroySwapchain ()
	{
		if ( _swapchain == VK_NULL_HANDLE )
			return true;

		CHECK_ERR( IsDeviceCreated() );

		_DeleteSwapchain( _swapchain );
		_framebuffers.Clear();
		
		_renderPass = null;

		_DeleteDepthStencilAttachment();

		_surfaceSize = uint2();
		return true;
	}
	
/*
=================================================
	SetSurface
=================================================
*/
	bool Vk1Device::SetSurface (VkSurfaceKHR surface, EPixelFormat::type colorFmt)
	{
		CHECK_ERR( surface != VK_NULL_HANDLE );
		CHECK_ERR( HasPhyiscalDevice() );
		CHECK_ERR( not IsSurfaceCreated() );

		const VkFormat			required_format			= Vk1Enum( colorFmt );
		const VkColorSpaceKHR	required_color_space	= VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;	// EPixelFormat::IsNonLinear( _settings.format );

		_surface = surface;

		CHECK_ERR( _ChooseColorFormat( OUT _colorFormat, OUT _colorSpace, required_format, required_color_space ) );

		_colorPixelFormat = Vk1Enum( _colorFormat );
		return true;
	}
	
/*
=================================================
	DestroySurface
=================================================
*/
	bool Vk1Device::DestroySurface ()
	{
		if ( _surface == VK_NULL_HANDLE )
			return true;

		CHECK_ERR( IsInstanceCreated() );
		CHECK_ERR( not IsSwapchainCreated() );

		vkDestroySurfaceKHR( _instance, _surface, null );

		_surface			= VK_NULL_HANDLE;
		_colorFormat		= VK_FORMAT_UNDEFINED;
		_colorSpace			= VK_COLOR_SPACE_MAX_ENUM_KHR;
		_colorPixelFormat	= EPixelFormat::Unknown;
		return true;
	}
	
/*
=================================================
	_ChooseQueueIndex
=================================================
*/
	bool Vk1Device::_ChooseQueueIndex (INOUT EQueueFamily::bits &family, OUT uint32_t &index) const
	{
		Array< VkQueueFamilyProperties >	queue_family_props;
		CHECK_ERR( _GetQueueFamilyProperties( OUT queue_family_props ) );
		
		FOR( i, queue_family_props )
		{
			EQueueFamily::bits	flags;

			VkBool32	supports_present = false;
			
			if ( _surface )
			{
				VK_CALL( vkGetPhysicalDeviceSurfaceSupportKHR( _physicalDevice, uint32_t(i), _surface, OUT &supports_present ) );
			}

			if ( supports_present )
			{
				flags |= EQueueFamily::Present;
			}

			if ( queue_family_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT )
			{
				flags |= EQueueFamily::Graphics | EQueueFamily::Transfer;
			}
	
			if ( queue_family_props[i].queueFlags & VK_QUEUE_COMPUTE_BIT )
			{
				flags |= EQueueFamily::Compute | EQueueFamily::Transfer;
			}
			
			if ( queue_family_props[i].queueFlags & VK_QUEUE_TRANSFER_BIT )
			{
				flags |= EQueueFamily::Transfer;
			}
			
			if ( queue_family_props[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT )
			{
				flags |= EQueueFamily::SparseBinding;
			}
			
			if ( queue_family_props[i].queueFlags & VK_QUEUE_PROTECTED_BIT )
			{
				flags |= EQueueFamily::Protected;
			}
			
			if ( (flags & family) == family )
			{
				index = uint32_t(i);
				return true;
			}
		}

		// TODO: find nearest queue family

		RETURN_ERR( "no suitable queue family found!" );
	}
	
/*
=================================================
	_GetQueueCreateInfo
=================================================
*/
	bool Vk1Device::_GetQueueCreateInfo (OUT VkDeviceQueueCreateInfo &queueCreateInfo, EQueueFamily::bits queueFamily) const
	{
		uint32_t	queue_index = 0;
		CHECK_ERR( _ChooseQueueIndex( INOUT queueFamily, OUT queue_index ) );

		ZeroMem( queueCreateInfo );
		queueCreateInfo.sType				= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex	= queue_index;
		queueCreateInfo.queueCount			= 2;
		queueCreateInfo.pQueuePriorities	= &DEFAULT_QUEUE_PRIORITY;

		return true;
	}

/*
=================================================
	CreateQueue
=================================================
*
	bool Vk1Device::CreateQueue ()
	{
		CHECK_ERR( IsDeviceCreated() );
		CHECK_ERR( not IsQueueCreated() );

		vkGetDeviceQueue( _logicalDevice, _queueIndex, 0, OUT &_queue );
		return true;
	}
	
/*
=================================================
	DestroyQueue
=================================================
*
	void Vk1Device::DestroyQueue ()
	{
		_queue			= VK_NULL_HANDLE;
		_queueIndex		= UMax;
		_queueFamily	= EQueueFamily::bits();
	}

/*
=================================================
	BeginFrame
=================================================
*/
	bool Vk1Device::BeginFrame (VkSemaphore imageAvailable)
	{
		CHECK_ERR( IsSwapchainCreated() );
		CHECK_ERR( not IsFrameStarted() );

		_currentImageIndex		= UMax;

		uint32_t	image_index = UMax;
		VkResult	result		= vkAcquireNextImageKHR( _logicalDevice, _swapchain, UMax, imageAvailable,
														 VK_NULL_HANDLE, OUT &image_index );

		if ( result == VK_SUCCESS )
		{}
		else
		if ( result == VK_ERROR_OUT_OF_DATE_KHR or
			 result == VK_SUBOPTIMAL_KHR )
		{
			CHECK_ERR( RecreateSwapchain() );
			return false;
		}
		else
		{
			if ( not Vk1_CheckErrors( result, "vkAcquireNextImageKHR", GX_FUNCTION_NAME, __FILE__, __LINE__ ) )
				return false;
		}

		_currentImageIndex = image_index;
		return true;
	}
	
/*
=================================================
	EndFrame
=================================================
*/
	bool Vk1Device::EndFrame (vk::VkQueue queue, VkSemaphore renderFinished)
	{
		CHECK_ERR( IsSwapchainCreated() );
		CHECK_ERR( IsFrameStarted() );
		
		VkSwapchainKHR		swap_chains[]		= { _swapchain };
		VkSemaphore			wait_semaphores[]	= { renderFinished };

		VkPresentInfoKHR	present_info = {};
		present_info.sType				= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.swapchainCount		= uint32_t(CountOf( swap_chains ));
		present_info.pSwapchains		= swap_chains;
		present_info.pImageIndices		= &_currentImageIndex;

		if ( renderFinished != VK_NULL_HANDLE )
		{
			present_info.waitSemaphoreCount		= uint32_t(CountOf( wait_semaphores ));
			present_info.pWaitSemaphores		= wait_semaphores;
		}

		VK_CHECK( vkQueuePresentKHR( queue, &present_info ) );
		
		_currentImageIndex	= UMax;
		return true;
	}

/*
=================================================
	IsFrameStarted
=================================================
*/
	bool Vk1Device::IsFrameStarted () const
	{
		return _currentImageIndex < _framebuffers.Count();
	}

/*
=================================================
	_GetSwapChainExtent
=================================================
*/
	void Vk1Device::_GetSwapChainExtent (INOUT VkExtent2D &extent, const VkSurfaceCapabilitiesKHR &surfaceCaps) const
	{
		if ( surfaceCaps.currentExtent.width  == UMax and
			 surfaceCaps.currentExtent.height == UMax )
		{
			// keep window size
		}
		else
		{
			extent.width  = surfaceCaps.currentExtent.width;
			extent.height = surfaceCaps.currentExtent.height;
		}
	}
	
/*
=================================================
	_GetPresentMode
=================================================
*/
	void Vk1Device::_GetPresentMode (OUT VkPresentModeKHR &presentMode, bool vsync) const
	{
		uint32_t					count		= 0;
		Array< VkPresentModeKHR >	present_modes;
		
		presentMode = VK_PRESENT_MODE_FIFO_KHR;

		VK_CALL( vkGetPhysicalDeviceSurfacePresentModesKHR( _physicalDevice, _surface, OUT &count, null ) );
		CHECK_ERR( count > 0, void() );

		present_modes.Resize( count );
		VK_CALL( vkGetPhysicalDeviceSurfacePresentModesKHR( _physicalDevice, _surface, OUT &count, OUT present_modes.ptr() ) );

		if ( not vsync )
		{
			FOR( i, present_modes )
			{
				if ( present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR )
				{
					presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
					break;
				}

				/*if ( presentMode != VK_PRESENT_MODE_MAILBOX_KHR and
					 present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR )
				{
					presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
				}*/
			}
		}
	}

/*
=================================================
	_GetSurfaceImageCount
=================================================
*/
	void Vk1Device::_GetSurfaceImageCount (OUT uint32_t &minImageCount, const VkSurfaceCapabilitiesKHR &surfaceCaps) const
	{
		minImageCount = surfaceCaps.minImageCount + 1;

		if ( surfaceCaps.maxImageCount > 0 and minImageCount > surfaceCaps.maxImageCount )
		{
			minImageCount = surfaceCaps.maxImageCount;
		}
	}
	
/*
=================================================
	_GetSurfaceTransform
=================================================
*/
	void Vk1Device::_GetSurfaceTransform (OUT VkSurfaceTransformFlagBitsKHR &transform,
											const VkSurfaceCapabilitiesKHR &surfaceCaps) const
	{
		if ( surfaceCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR )
		{
			transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		}
		else
		{
			transform = surfaceCaps.currentTransform;
		}
	}
	
/*
=================================================
	_GetImageUsage
=================================================
*/
	bool Vk1Device::_GetImageUsage (OUT VkImageUsageFlags &imageUsage, VkPresentModeKHR presentMode,
									EImageUsage::bits requiredUsage, const VkSurfaceCapabilitiesKHR &surfaceCaps) const
	{
		if ( presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR	or
			 presentMode == VK_PRESENT_MODE_MAILBOX_KHR		or
			 presentMode == VK_PRESENT_MODE_FIFO_KHR		or
			 presentMode == VK_PRESENT_MODE_FIFO_RELAXED_KHR )
		{
			imageUsage = surfaceCaps.supportedUsageFlags & Vk1Enum( requiredUsage );
		}
		else
		if ( presentMode == VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR	or
			 presentMode == VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR )
		{
			VkPhysicalDeviceSurfaceInfo2KHR	surf_info = {};
			surf_info.sType		= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
			surf_info.surface	= _surface;

			VkSurfaceCapabilities2KHR	surf_caps2;
			VK_CALL( vkGetPhysicalDeviceSurfaceCapabilities2KHR( _physicalDevice, &surf_info, OUT &surf_caps2 ) );

			for (VkBaseInStructure const *iter = reinterpret_cast<VkBaseInStructure const *>(&surf_caps2);
				 iter != null;)
			{
				if ( iter->sType == VK_STRUCTURE_TYPE_SHARED_PRESENT_SURFACE_CAPABILITIES_KHR )
				{
					imageUsage = reinterpret_cast<VkSharedPresentSurfaceCapabilitiesKHR const*>(iter)->sharedPresentSupportedUsageFlags & Vk1Enum( requiredUsage );
					break;
				}
			}
		}
		else
		{
			RETURN_ERR( "unsupported presentMode, can't choose imageUsage!" );
		}

		ASSERT( EnumEq( imageUsage, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ) );
		imageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		

		// validation:
		VkFormatProperties	format_props;
		vkGetPhysicalDeviceFormatProperties( _physicalDevice, _colorFormat, OUT &format_props );

		CHECK_ERR( EnumEq( format_props.optimalTilingFeatures, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT ) );
		ASSERT( EnumEq( format_props.optimalTilingFeatures, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT ) );
		
		if ( EnumEq( imageUsage, VK_IMAGE_USAGE_TRANSFER_SRC_BIT ) and
			 (not EnumEq( format_props.optimalTilingFeatures, VK_FORMAT_FEATURE_TRANSFER_SRC_BIT ) or
			  not EnumEq( format_props.optimalTilingFeatures, VK_FORMAT_FEATURE_BLIT_DST_BIT )) )
		{
			imageUsage &= ~VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}
		
		if ( EnumEq( imageUsage, VK_IMAGE_USAGE_TRANSFER_DST_BIT ) and
			 not EnumEq( format_props.optimalTilingFeatures, VK_FORMAT_FEATURE_TRANSFER_DST_BIT ) )
		{
			imageUsage &= ~VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}
		
		if ( EnumEq( imageUsage, VK_IMAGE_USAGE_STORAGE_BIT ) and
			 not EnumEq( format_props.optimalTilingFeatures, VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT ) )
		{
			imageUsage &= ~VK_IMAGE_USAGE_STORAGE_BIT;
		}

		if ( EnumEq( imageUsage, VK_IMAGE_USAGE_SAMPLED_BIT ) and
			 not EnumEq( format_props.optimalTilingFeatures, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT ) )
		{
			imageUsage &= ~VK_IMAGE_USAGE_SAMPLED_BIT;
		}

		if ( EnumEq( imageUsage, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ) and
			 not EnumEq( format_props.optimalTilingFeatures, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT ) )
		{
			imageUsage &= ~VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		}

		return true;
	}
	
/*
=================================================
	_GetSharingMode
=================================================
*/
	void Vk1Device::_GetSharingMode (OUT VkSharingMode &sharingMode) const
	{
		sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

/*
=================================================
	_DeleteSwapchain
=================================================
*/
	void Vk1Device::_DeleteSwapchain (VkSwapchainKHR &swapchain)
	{
		FOR( i, _imageBuffers )
		{
			if ( _imageBuffers[i].module ) {
				_imageBuffers[i].module->Send( ModuleMsg::Delete{} );
			}
		}

		if ( swapchain != VK_NULL_HANDLE ) 
		{
			vkDestroySwapchainKHR( _logicalDevice, swapchain, null );
		}

		swapchain = VK_NULL_HANDLE;
		_imageBuffers.Clear();
	}
	
/*
=================================================
	_CreateColorAttachment
=================================================
*/
	bool Vk1Device::_CreateColorAttachment (MultiSamples samples, EImageUsage::bits usage)
	{
		using VkImages_t = FixedSizeArray< VkImage, MAX_SWAPCHAIN_SIZE >;

		uint32_t		count = 0;
		VkImages_t		images;

		VK_CHECK( vkGetSwapchainImagesKHR( _logicalDevice, _swapchain, OUT &count, null ) );
		CHECK_ERR( count > 0 );
		images.Resize( count );

		VK_CHECK( vkGetSwapchainImagesKHR( _logicalDevice, _swapchain, OUT &count, OUT images.ptr() ) );
		CHECK_ERR( count > 0 );
		_imageBuffers.Resize( count );

		ImageDescription		descr;
		descr.dimension	= uint4( _surfaceSize, 0, 0 );
		descr.format	= _colorPixelFormat;
		descr.imageType	= EImage::Tex2D;
		descr.samples	= samples;
		descr.usage		= usage;

		FOR( i, _imageBuffers )
		{
			auto&		buf = _imageBuffers[i];

			buf.module = New< Vk1SwapchainImage >( GlobalSystems(), CreateInfo::GpuImage{ descr } );
			ModuleUtils::Initialize({ buf.module });

			buf.view = buf.module->Request( GpuMsg::SetVkSwapchainImage{ images[i] });
		}
		return true;
	}
	
/*
=================================================
	_CreateDepthStencilAttachment
=================================================
*/
	bool Vk1Device::_CreateDepthStencilAttachment (EPixelFormat::type depthStencilFormat, EImageUsage::bits usage)
	{
		if ( depthStencilFormat == EPixelFormat::Unknown )
		{
			_DeleteDepthStencilAttachment();
			return true;
		}

		CHECK_ERR( GlobalSystems()->modulesFactory->Create(
									VkImageModuleID,
									GlobalSystems(),
									CreateInfo::GpuImage{
										ImageDescription{
											EImage::Tex2D,
											uint4( _surfaceSize, 0, 0 ),
											depthStencilFormat,
											EImageUsage::DepthStencilAttachment | usage
										},
										EGpuMemory::LocalInGPU,
										EMemoryAccess::GpuReadWrite
									},
									OUT _depthStencilImage ) );

		ModuleUtils::Initialize({ _depthStencilImage });

		GpuMsg::GetVkImageID	req_id;
		_depthStencilImage->Send( req_id );

		_depthStencilView = req_id.result->defaultView;
		return true;
	}
	
/*
=================================================
	_DeleteDepthStencilAttachment
=================================================
*/
	void Vk1Device::_DeleteDepthStencilAttachment ()
	{
		if ( _depthStencilImage ) {
			_depthStencilImage->Send( ModuleMsg::Delete{} );
		}

		_depthStencilImage			= null;
		_depthStencilFormat			= VK_FORMAT_UNDEFINED;
		_depthStencilPixelFormat	= EPixelFormat::Unknown;
		_depthStencilView			= VK_NULL_HANDLE;
	}
	
/*
=================================================
	GetMemoryTypeIndex
=================================================
*/
	bool Vk1Device::GetMemoryTypeIndex (const uint32_t memoryTypeBits, const VkMemoryPropertyFlags flags, OUT uint32_t &index) const
	{
		index = UMax;

		for (uint32_t i = 0; i < _deviceMemoryProperties.memoryTypeCount; ++i)
		{
			const auto&		mem_type = _deviceMemoryProperties.memoryTypes[i];

			if ( ((memoryTypeBits >> i) & 1) == 1		and
				 EnumEq( mem_type.propertyFlags, flags ) )
			{
				index = i;
				return true;
			}
		}
		return false;
	}
	
/*
=================================================
	CompareMemoryTypes
=================================================
*/
	bool Vk1Device::CompareMemoryTypes (const uint32_t memoryTypeBits, const VkMemoryPropertyFlags flags, const uint32_t index) const
	{
		CHECK_ERR( index < _deviceMemoryProperties.memoryTypeCount );

		const auto&		mem_type = _deviceMemoryProperties.memoryTypes[ index ];

		if ( ((memoryTypeBits >> index) & 1) == 1	and
			 EnumEq( mem_type.propertyFlags, flags ) )
		{
			return true;
		}
		return false;
	}

/*
=================================================
	SetObjectName
=================================================
*/
	bool Vk1Device::SetObjectName (uint64_t id, StringCRef name, EGpuObject::type type) const
	{
		if ( name.Empty() or id == VK_NULL_HANDLE or not _enableDebugMarkers )
			return false;

		VkDebugMarkerObjectNameInfoEXT	info = {};
		info.sType			= VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
		info.objectType		= Vk1Enum( type );
		info.object			= id;
		info.pObjectName	= name.cstr();

		VK_CALL( vkDebugMarkerSetObjectNameEXT( GetLogicalDevice(), &info ) );
		return true;
	}

/*
=================================================
	_CreateRenderPass
=================================================
*/
	bool Vk1Device::_CreateRenderPass ()
	{
		if ( _renderPass )
			return true;

		ASSERT( _depthStencilView != VK_NULL_HANDLE ?
				_depthStencilPixelFormat != EPixelFormat::Unknown :
				_depthStencilPixelFormat == EPixelFormat::Unknown );

		ModulePtr	module;
		CHECK_ERR( GlobalSystems()->modulesFactory->Create(
					VkRenderPassModuleID,
					GlobalSystems(),
					CreateInfo::GpuRenderPass{
						null,
						RenderPassDescrBuilder::CreateForSurface( _colorPixelFormat, _depthStencilPixelFormat )
					},
					OUT module ) );

		_renderPass = module;
		return true;
	}

/*
=================================================
	_CreateFramebuffers
=================================================
*/
	bool Vk1Device::_CreateFramebuffers ()
	{
		CHECK_ERR( not _imageBuffers.Empty() );
		CHECK_ERR( _framebuffers.Empty() );
		CHECK_ERR( _renderPass );

		_framebuffers.Resize( _imageBuffers.Count() );

		FOR( i, _framebuffers )
		{
			ModulePtr	fb;
			CHECK_ERR( GlobalSystems()->modulesFactory->Create(
										VkFramebufferModuleID,
										GlobalSystems(),
										CreateInfo::GpuFramebuffer{ _surfaceSize },
										OUT fb ) );

			fb->Send( ModuleMsg::AttachModule{ _renderPass });
			fb->Send( ModuleMsg::AttachModule{ "Color0", _imageBuffers[i].module });

			if ( _depthStencilImage ) {
				fb->Send( ModuleMsg::AttachModule{ "Depth", _depthStencilImage });
			}

			_framebuffers[i] = fb;
		}

		ModuleUtils::Initialize( _framebuffers );
		return true;
	}
	
/*
=================================================
	_DeleteFramebuffers
=================================================
*/
	void Vk1Device::_DeleteFramebuffers ()
	{
		for (auto& fbo : _framebuffers) {
			fbo->Send( ModuleMsg::Delete{} );
		}

		_framebuffers.Clear();
	}

/*
=================================================
	_ChooseColorFormat
=================================================
*/
	bool Vk1Device::_ChooseColorFormat (OUT VkFormat &colorFormat, OUT VkColorSpaceKHR &colorSpace,
										const VkFormat requiredFormat, const VkColorSpaceKHR requiredColorSpace) const
	{
		uint32_t					count		= 0;
		Array< VkSurfaceFormatKHR >	surf_formats;
		const VkFormat				def_format	= VK_FORMAT_B8G8R8A8_UNORM;
		const VkColorSpaceKHR		def_space	= VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

		VK_CHECK( vkGetPhysicalDeviceSurfaceFormatsKHR( _physicalDevice, _surface, OUT &count, null ) );
		CHECK_ERR( count > 0 );

		surf_formats.Resize( count );
		VK_CHECK( vkGetPhysicalDeviceSurfaceFormatsKHR( _physicalDevice, _surface, OUT &count, OUT surf_formats.ptr() ) );
		
		if ( count == 1 and
			 surf_formats[0].format == VK_FORMAT_UNDEFINED )
		{
			colorFormat = requiredFormat;
			colorSpace  = surf_formats[0].colorSpace;
		}
		else
		{
			usize	both_match_idx		= UMax;
			usize	format_match_idx	= UMax;
			usize	space_match_idx		= UMax;
			usize	def_format_idx		= 0;
			usize	def_space_idx		= 0;

			FOR( i, surf_formats )
			{
				if ( surf_formats[i].format		== requiredFormat and
					 surf_formats[i].colorSpace	== requiredColorSpace )
				{
					both_match_idx = i;
				}
				else
				// separate check
				if ( surf_formats[i].format		== requiredFormat )
					format_match_idx = i;
				else
				if ( surf_formats[i].colorSpace	== requiredColorSpace )
					space_match_idx = i;

				// check with default
				if ( surf_formats[i].format		== def_format )
					def_format_idx = i;

				if ( surf_formats[i].colorSpace	== def_space )
					def_space_idx = i;
			}

			usize	idx = 0;

			if ( both_match_idx != UMax )
				idx = both_match_idx;
			else
			if ( format_match_idx != UMax )
				idx = format_match_idx;
			else
			if ( def_format_idx != UMax )
				idx = def_format_idx;

			// TODO: space_match_idx and def_space_idx are unused yet

			colorFormat	= surf_formats[ idx ].format;
			colorSpace	= surf_formats[ idx ].colorSpace;
		}

		return true;
	}
	
/*
=================================================
	_GetQueueFamilyProperties
=================================================
*/
	bool Vk1Device::_GetQueueFamilyProperties (OUT Array<VkQueueFamilyProperties> &properties) const
	{
		uint32_t	count = 0;

		vkGetPhysicalDeviceQueueFamilyProperties( _physicalDevice, OUT &count, null );
		CHECK_ERR( count > 0 );

		properties.Resize( count );
		vkGetPhysicalDeviceQueueFamilyProperties( _physicalDevice, OUT &count, OUT properties.ptr() );
		return true;
	}

/*
=================================================
	_LoadDeviceExtensions
=================================================
*/
	bool Vk1Device::_LoadDeviceExtensions () const
	{
		CHECK_ERR( HasPhyiscalDevice() );

		if ( not _deviceExtensions.Empty() )
			return true;

		uint32_t	count = 0;
		VK_CALL( vkEnumerateDeviceExtensionProperties( _physicalDevice, null, OUT &count, null ) );

		if ( count == 0 ) {
			_deviceExtensions << "";
			return true;
		}

		Array< VkExtensionProperties >	dev_ext;
		dev_ext.Resize( count );

		VK_CHECK( vkEnumerateDeviceExtensionProperties( _physicalDevice, null, OUT &count, OUT dev_ext.ptr() ) );

		for (auto& ext : dev_ext) {
			_deviceExtensions.Add( StringCRef(ext.extensionName) );
		}

		return true;
	}
	
/*
=================================================
	HasDeviceExtension
=================================================
*/
	bool Vk1Device::HasDeviceExtension (StringCRef name) const
	{
		_LoadDeviceExtensions();

		return _deviceExtensions.IsExist( name );
	}

/*
=================================================
	_CheckDeviceExtensions
=================================================
*/
	bool Vk1Device::_CheckDeviceExtensions (INOUT Array<const char*> &extensions) const
	{
		CHECK_ERR( HasPhyiscalDevice() );

		FOR( i, extensions )
		{
			StringCRef	name = extensions[i];
			
			if ( not HasDeviceExtension( name ) )
			{
				LOG( "Vulkan device extension \""_str << name << "\" not supported and will be removed", ELog::Info );

				extensions.Erase( i );
				--i;
			}
		}
		return true;
	}
	
/*
=================================================
	_CheckLayers
=================================================
*/
	bool Vk1Device::_CheckLayers (INOUT Array<const char*> &layers) const
	{
		FOR( i, layers )
		{
			StringCRef	name = layers[i];
			
			if ( not HasLayer( name ) )
			{
				LOG( "Vulkan layer \""_str << name << "\" not supported and will be removed", ELog::Info );

				layers.Erase( i );
				--i;
			}
		}
		return true;
	}
	
/*
=================================================
	_CheckExtensions
=================================================
*/
	bool Vk1Device::_CheckExtensions (INOUT Array<const char*> &extensions) const
	{
		FOR( i, extensions )
		{
			StringCRef	name = extensions[i];
			
			if ( not HasExtension( name ) )
			{
				LOG( "Vulkan extension \""_str << name << "\" not supported and will be removed", ELog::Info );

				extensions.Erase( i );
				--i;
			}
		}
		return true;
	}

/*
=================================================
	_DebugReportCallback
=================================================
*/
	VkBool32 VKAPI_CALL Vk1Device::_DebugReportCallback (VkDebugReportFlagsEXT flags,
														  VkDebugReportObjectTypeEXT objectType,
														  uint64_t object,
														  size_t /*location*/,
														  int32_t messageCode,
														  const char* pLayerPrefix,
														  const char* pMessage,
														  void* pUserData)
	{
		String	log;

		log << "Vulkan " << _DebugReportFlagsToString( VkDebugReportFlagBitsEXT(flags) )
			<< " in object: " << _DebugReportObjectTypeToString( objectType )
			<< '(' << String().FormatAlignedI( object, 8, '0', 16 ) << ')'
			<< ", layer: " << pLayerPrefix << '(' << messageCode << ')'
			<< ", message:\n" << pMessage;

		LOG( log, _DebugReportFlagsToLogType( VkDebugReportFlagBitsEXT(flags) ) );

		Cast< Vk1Device *>(pUserData)->_debugReportCounter++;

		return VK_FALSE;
	}
	
/*
=================================================
	_DebugReportFlagsToString
=================================================
*/
	StringCRef Vk1Device::_DebugReportFlagsToString (VkDebugReportFlagBitsEXT flags)
	{
		if ( EnumEq( flags, VK_DEBUG_REPORT_ERROR_BIT_EXT ) )
			return "error";

		if ( EnumEq( flags, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT ) )
			return "performance";

		if ( EnumEq( flags, VK_DEBUG_REPORT_WARNING_BIT_EXT ) )
			return "warning";

		if ( EnumEq( flags, VK_DEBUG_REPORT_INFORMATION_BIT_EXT ) )
			return "info";

		if ( EnumEq( flags, VK_DEBUG_REPORT_DEBUG_BIT_EXT ) )
			return "debug";

		return "";
	}
	
/*
=================================================
	_DebugReportFlagsToLogType
=================================================
*/
	ELog::type Vk1Device::_DebugReportFlagsToLogType (VkDebugReportFlagBitsEXT flags)
	{
		if ( EnumEq( flags, VK_DEBUG_REPORT_ERROR_BIT_EXT ) )
			return ELog::Warning;

		if ( EnumEq( flags, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT ) )
			return ELog::Debug;

		if ( EnumEq( flags, VK_DEBUG_REPORT_WARNING_BIT_EXT ) )
			return ELog::Debug;

		if ( EnumEq( flags, VK_DEBUG_REPORT_INFORMATION_BIT_EXT ) )
			return ELog::Debug;

		if ( EnumEq( flags, VK_DEBUG_REPORT_DEBUG_BIT_EXT ) )
			return ELog::Debug;

		return ELog::Debug;
	}

/*
=================================================
	_DebugReportObjectTypeToString
=================================================
*/
	StringCRef Vk1Device::_DebugReportObjectTypeToString (VkDebugReportObjectTypeEXT objType)
	{
		switch ( objType )
		{
			//case VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT : return "unknown";
			case VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT			: return "Instance";
			case VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT	: return "PhysicalDevice";
			case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT				: return "Device";
			case VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT				: return "Queue";
			case VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT			: return "Semaphore";
			case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT		: return "CommandBuffer";
			case VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT				: return "Fence";
			case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT		: return "DeviceMemory";
			case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT				: return "Buffer";
			case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT				: return "Image";
			case VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT				: return "Event";
			case VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT			: return "QueryPool";
			case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT		: return "BufferView";
			case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT			: return "ImageView";
			case VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT		: return "ShaderModule";
			case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT		: return "PipelineCache";
			case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT	: return "PipelineLayout";
			case VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT		: return "RenderPass";
			case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT			: return "Pipeline";
			case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT	: return "DescriptorSetLayout";
			case VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT			: return "Sampler";
			case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT	: return "DescriptorPool";
			case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT		: return "DescriptorSet";
			case VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT		: return "Framebuffer";
			case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT		: return "CommandPool";
			case VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT		: return "Surface";
			case VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT		: return "Swapchain";
			case VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT		: return "DebugReport";
			case VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_KHR_EXT		: return "Display";
			case VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_MODE_KHR_EXT	: return "DisplayMode";
			case VK_DEBUG_REPORT_OBJECT_TYPE_OBJECT_TABLE_NVX_EXT	: return "ObjectTableNvx";
			case VK_DEBUG_REPORT_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NVX_EXT	: return "IndirectCommandsLayoutNvx";
			default :
				CHECK( objType >= VK_DEBUG_REPORT_OBJECT_TYPE_BEGIN_RANGE_EXT and
					   objType <= VK_DEBUG_REPORT_OBJECT_TYPE_END_RANGE_EXT );
				return "unknown";
		}
	}
	
	
}	// PlatformVK
}	// Engine

#endif	// GRAPHICS_API_VULKAN
