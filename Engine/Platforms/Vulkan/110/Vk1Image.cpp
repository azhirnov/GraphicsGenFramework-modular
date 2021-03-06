// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "Core/Config/Engine.Config.h"

#ifdef GRAPHICS_API_VULKAN

#include "Engine/Platforms/Public/GPU/Image.h"
#include "Engine/Platforms/Public/GPU/Memory.h"
#include "Engine/Platforms/Vulkan/110/Vk1BaseModule.h"
#include "Engine/Platforms/Vulkan/VulkanObjectsConstructor.h"
#include "Engine/Platforms/Public/Tools/ImageViewHashMap.h"
#include "Engine/Platforms/Public/Tools/ImageUtils.h"

namespace Engine
{
namespace PlatformVK
{
	using namespace vk;


	//
	// Vulkan Image
	//

	class Vk1Image final : public Vk1BaseModule
	{
	// types
	private:
		using ForwardToMem_t		= MessageListFrom< 
											DSMsg::GetDataSourceDescription,
											DSMsg::ReadMemRange,
											DSMsg::WriteMemRange,
											GpuMsg::GetGpuMemoryDescription,
											GpuMsg::MapMemoryToCpu,
											GpuMsg::MapImageToCpu,
											GpuMsg::FlushMemoryRange,
											GpuMsg::UnmapMemory,
											GpuMsg::ReadFromGpuMemory,
											GpuMsg::WriteToGpuMemory,
											GpuMsg::ReadFromImageMemory,
											GpuMsg::WriteToImageMemory
										>;

		using SupportedMessages_t	= Vk1BaseModule::SupportedMessages_t::Append< MessageListFrom<
											GpuMsg::GetImageDescription,
											GpuMsg::SetImageDescription,
											GpuMsg::GetVkImageID,
											GpuMsg::CreateVkImageView,
											GpuMsg::GetImageMemoryLayout
										> >;

		using SupportedEvents_t		= Vk1BaseModule::SupportedEvents_t::Append< MessageListFrom<
											GpuMsg::SetImageDescription
										> >;
		
		using MemoryEvents_t		= MessageListFrom< GpuMsg::OnMemoryBindingChanged >;

		using Utils					= PlatformTools::ImageUtils;

		using ImageViewMap_t		= PlatformTools::ImageViewHashMap< VkImageView >;
		using ImageView_t			= ImageViewMap_t::Key_t;


	// constants
	private:
		static const TypeIdList		_eventTypes;


	// variables
	private:
		ImageDescription		_descr;
		ModulePtr				_memObj;
		ModulePtr				_memManager;	// optional
		ImageViewMap_t			_viewMap;
		VkImage					_imageId;
		VkImageView				_imageView;		// default image view, has all mipmaps and all layers
		EImageLayout::type		_layout;
		
		EGpuMemory::bits		_memFlags;		// -|-- this flags is requirements for memory obj, don't use it anywhere
		EMemoryAccess::bits		_memAccess;		// -|
		bool					_useMemMngr;	// -|

		bool					_isBindedToMemory;


	// methods
	public:
		Vk1Image (UntypedID_t, GlobalSystemsRef gs, const CreateInfo::GpuImage &ci);
		~Vk1Image ();


	// message handlers
	private:
		bool _Link (const ModuleMsg::Link &);
		bool _Compose (const ModuleMsg::Compose &);
		bool _Delete (const ModuleMsg::Delete &);
		bool _AttachModule (const ModuleMsg::AttachModule &);
		bool _DetachModule (const ModuleMsg::DetachModule &);
		bool _GetVkImageID (const GpuMsg::GetVkImageID &);
		bool _CreateVkImageView (const GpuMsg::CreateVkImageView &);
		bool _GetImageDescription (const GpuMsg::GetImageDescription &);
		bool _SetImageDescription (const GpuMsg::SetImageDescription &);
		bool _GetImageMemoryLayout (const GpuMsg::GetImageMemoryLayout &);
		
	// event handlers
		bool _OnMemoryBindingChanged (const GpuMsg::OnMemoryBindingChanged &);


	private:
		bool _IsImageCreated () const	{ return _imageId != VK_NULL_HANDLE; }

		bool _CreateImage ();
		bool _CreateDefaultView ();

		void _DestroyAll ();
		void _DestroyViews ();

		bool _CanHaveImageView () const;

		static VkImageType  _GetImageType (EImage::type type);

		static void _ValidateMemFlags (INOUT EGpuMemory::bits &flags);
		static void _ValidateDescription (INOUT ImageDescription &descr);
	};
//-----------------------------------------------------------------------------


	
	const TypeIdList	Vk1Image::_eventTypes{ UninitializedT< SupportedEvents_t >() };

/*
=================================================
	constructor
=================================================
*/
	Vk1Image::Vk1Image (UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::GpuImage &ci) :
		Vk1BaseModule( gs, ModuleConfig{ id, UMax }, &_eventTypes ),
		_descr( ci.descr ),					_memManager( ci.memManager ),
		_imageId( VK_NULL_HANDLE ),			_imageView( VK_NULL_HANDLE ),
		_layout( EImageLayout::Unknown ),	_memFlags( ci.memFlags ),
		_memAccess( ci.access ),			_useMemMngr( ci.allocMem or ci.memManager.IsNotNull() ),
		_isBindedToMemory( false )
	{
		SetDebugName( "Vk1Image" );

		_SubscribeOnMsg( this, &Vk1Image::_OnModuleAttached_Impl );
		_SubscribeOnMsg( this, &Vk1Image::_OnModuleDetached_Impl );
		_SubscribeOnMsg( this, &Vk1Image::_AttachModule );
		_SubscribeOnMsg( this, &Vk1Image::_DetachModule );
		_SubscribeOnMsg( this, &Vk1Image::_FindModule_Impl );
		_SubscribeOnMsg( this, &Vk1Image::_ModulesDeepSearch_Impl );
		_SubscribeOnMsg( this, &Vk1Image::_Link );
		_SubscribeOnMsg( this, &Vk1Image::_Compose );
		_SubscribeOnMsg( this, &Vk1Image::_Delete );
		_SubscribeOnMsg( this, &Vk1Image::_OnManagerChanged );
		_SubscribeOnMsg( this, &Vk1Image::_GetVkImageID );
		_SubscribeOnMsg( this, &Vk1Image::_CreateVkImageView );
		_SubscribeOnMsg( this, &Vk1Image::_GetImageDescription );
		_SubscribeOnMsg( this, &Vk1Image::_SetImageDescription );
		_SubscribeOnMsg( this, &Vk1Image::_GetDeviceInfo );
		_SubscribeOnMsg( this, &Vk1Image::_GetVkDeviceInfo );
		_SubscribeOnMsg( this, &Vk1Image::_GetVkPrivateClasses );
		_SubscribeOnMsg( this, &Vk1Image::_GetImageMemoryLayout );
		
		ASSERT( _ValidateMsgSubscriptions< SupportedMessages_t >() );

		_AttachSelfToManager( _GetGPUThread( ci.gpuThread ), UntypedID_t(0), true );
		
		_ValidateMemFlags( INOUT _memFlags );
		_ValidateDescription( INOUT _descr );
	}
	
/*
=================================================
	destructor
=================================================
*/
	Vk1Image::~Vk1Image ()
	{
		ASSERT( not _IsImageCreated() );
	}
	
/*
=================================================
	_Link
=================================================
*/
	bool Vk1Image::_Link (const ModuleMsg::Link &msg)
	{
		if ( _IsComposedOrLinkedState( GetState() ) )
			return true;	// already linked

		CHECK_ERR( _IsInitialState( GetState() ) );
		
		_memObj = GetModuleByEvent< MemoryEvents_t >();

		if ( not _memObj and _useMemMngr )
		{
			ModulePtr	mem_module;
			CHECK_ERR( GlobalSystems()->modulesFactory->Create(
								VkManagedMemoryModuleID,
								GlobalSystems(),
								CreateInfo::GpuMemory{ _memManager, _memFlags, _memAccess },
								OUT mem_module ) );

			CHECK_ERR( _Attach( "mem", mem_module ) );
			_memObj = mem_module;
		}
		CHECK_ATTACHMENT( _memObj );

		_memObj->Subscribe( this, &Vk1Image::_OnMemoryBindingChanged );
		_memObj->Subscribe( this, &Vk1Image::_Delete );

		CHECK_LINKING( _CopySubscriptions< ForwardToMem_t >( _memObj ) );

		CHECK_LINKING( _CreateImage() );
		
		return Module::_Link_Impl( msg );
	}
	
/*
=================================================
	_Compose
=================================================
*/
	bool Vk1Image::_Compose (const ModuleMsg::Compose &msg)
	{
		if ( _IsComposedState( GetState() ) /*or _IsImageCreated()*/ )
			return true;	// already composed

		CHECK_ERR( GetState() == EState::Linked );

		_SendForEachAttachments( msg );
		
		CHECK( _ValidateAllSubscriptions() );

		// composed state will be changed when memory binded to image
		return true;
	}
	
/*
=================================================
	_Delete
=================================================
*/
	bool Vk1Image::_Delete (const ModuleMsg::Delete &msg)
	{
		_DestroyAll();

		return Module::_Delete_Impl( msg );
	}
	
/*
=================================================
	_AttachModule
=================================================
*/
	bool Vk1Image::_AttachModule (const ModuleMsg::AttachModule &msg)
	{
		if ( msg.newModule->GetSupportedEvents().HasAllTypes< MemoryEvents_t >() )
		{
			CHECK_ERR( GetState() == EState::Initial	and
					   not _IsComposedState( msg.newModule->GetState() ) );
		}

		CHECK( _Attach( msg.name, msg.newModule ) );
		return true;
	}
	
/*
=================================================
	_DetachModule
=================================================
*/
	bool Vk1Image::_DetachModule (const ModuleMsg::DetachModule &msg)
	{
		CHECK( _Detach( msg.oldModule ) );

		if ( msg.oldModule == _memObj )
		{
			Send( ModuleMsg::Delete{} );
		}
		return true;
	}

/*
=================================================
	_CreateImage
=================================================
*/
	bool Vk1Image::_CreateImage ()
	{
		CHECK_ERR( not _IsImageCreated() );
		CHECK_ERR( _viewMap.Empty() );

		GpuMsg::GetGpuMemoryDescription	req_mem_descr;
		_memObj->Send( req_mem_descr );

		EMemoryAccess::bits	mem_access	= req_mem_descr.result->access;
		const bool			opt_tiling	= !(mem_access & EMemoryAccess::CpuReadWrite);

		_layout = opt_tiling ? EImageLayout::Undefined : EImageLayout::Preinitialized;

		// create image
		VkImageCreateInfo	info = {};
		info.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.pNext			= null;
		info.flags			= 0;
		info.imageType		= _GetImageType( _descr.imageType );
		info.format			= Vk1Enum( _descr.format );
		info.extent.width	= _descr.dimension.x;
		info.extent.height	= _descr.dimension.y;
		info.extent.depth	= _descr.dimension.z;
		info.mipLevels		= _descr.maxLevel.Get();
		info.arrayLayers	= _descr.dimension.w;
		info.samples		= Vk1Enum( _descr.samples );
		info.tiling			=  opt_tiling ? VK_IMAGE_TILING_OPTIMAL :	// use copy from buffer to write data to texture
											VK_IMAGE_TILING_LINEAR;
		info.usage			= Vk1Enum( _descr.usage );
		info.initialLayout	= Vk1Enum( _layout );	// Undefined or Preinitialized
		info.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;

		if ( _descr.imageType == EImage::TexCube or _descr.imageType == EImage::TexCubeArray )
			info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

		if ( EImage::IsArray( _descr.imageType ) )
			info.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;

		// TODO: VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT
		// TODO: VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT
		// TODO: VK_IMAGE_CREATE_ALIAS_BIT

		VK_CHECK( vkCreateImage( GetVkDevice(), &info, null, OUT &_imageId ) );
		
		GetDevice()->SetObjectName( ReferenceCast<uint64_t>(_imageId), GetDebugName(), EGpuObject::Image );
		return true;
	}
	
/*
=================================================
	_CreateDefaultView
=================================================
*/
	bool Vk1Image::_CreateDefaultView ()
	{
		CHECK_ERR( _IsImageCreated() );
		CHECK_ERR( _imageView == VK_NULL_HANDLE );
		
		if ( _CanHaveImageView() )
		{
			GpuMsg::CreateVkImageView	create;
			create.viewDescr.layerCount	= _descr.dimension.w;
			create.viewDescr.levelCount	= _descr.maxLevel.Get();

			CHECK_ERR( _CreateVkImageView( create ) );

			_imageView = *create.result;
		}
		return true;
	}

/*
=================================================
	_DestroyAll
=================================================
*/
	void Vk1Image::_DestroyAll ()
	{
		auto	dev = GetVkDevice();

		if ( dev != VK_NULL_HANDLE and _imageId != VK_NULL_HANDLE )
		{
			vkDestroyImage( dev, _imageId, null );
		}

		_DestroyViews();

		_imageId	= VK_NULL_HANDLE;
		_layout		= Uninitialized;
		_descr		= Uninitialized;
	}
	
/*
=================================================
	_DestroyViews
=================================================
*/
	void Vk1Image::_DestroyViews ()
	{
		auto	dev = GetVkDevice();

		if ( dev != VK_NULL_HANDLE and not _viewMap.Empty() )
		{
			FOR( i, _viewMap ) {
				vkDestroyImageView( dev, _viewMap[i].second, null );
			}
		}
		_viewMap.Clear();
		
		if ( _memObj )
		{
			this->UnsubscribeAll( _memObj );
			_memObj->UnsubscribeAll( this );
		}

		_memObj				= null;
		_memManager			= null;
		_imageView			= VK_NULL_HANDLE;
		_isBindedToMemory	= false;
	}

/*
=================================================
	_GetVkImageID
=================================================
*/
	bool Vk1Image::_GetVkImageID (const GpuMsg::GetVkImageID &msg)
	{
		ASSERT( _IsImageCreated() );

		msg.result.Set({ _imageId, _imageView });
		return true;
	}
	
/*
=================================================
	_CreateVkImageView
=================================================
*/
	bool Vk1Image::_CreateVkImageView (const GpuMsg::CreateVkImageView &msg)
	{
		CHECK_ERR( _IsImageCreated() );
		CHECK_ERR( _isBindedToMemory );
		CHECK_ERR( _CanHaveImageView() );

		ImageView_t		descr = msg.viewDescr;

		ImageViewMap_t::Validate( INOUT descr, _descr );

		// search in cache
		VkImageView		img_view = _viewMap.Find( descr );
		
		if ( img_view != VK_NULL_HANDLE )
		{
			msg.result.Set( img_view );
			return true;
		}

		const VkComponentSwizzle	components[] = {
			VK_COMPONENT_SWIZZLE_ZERO,	// unknown
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A,
			VK_COMPONENT_SWIZZLE_ZERO,
			VK_COMPONENT_SWIZZLE_ONE
		};
		const uint4		swizzle = Min( uint4(uint(CountOf(components)-1)), descr.swizzle.ToVec() );

		// create new image view
		VkImageViewCreateInfo	view_info	= {};

		view_info.sType			= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_info.pNext			= null;
		view_info.viewType		= Vk1Enum( descr.viewType );
		view_info.flags			= 0;
		view_info.image			= _imageId;
		view_info.format		= Vk1Enum( descr.format );
		view_info.components	= { components[swizzle.x], components[swizzle.y], components[swizzle.z], components[swizzle.w] };

		view_info.subresourceRange.aspectMask		= EPixelFormat::IsColor( descr.format ) ? VK_IMAGE_ASPECT_COLOR_BIT :
													  ((EPixelFormat::HasDepth( descr.format ) ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) |
													   (EPixelFormat::HasStencil( descr.format ) ? VK_IMAGE_ASPECT_STENCIL_BIT : 0));
		view_info.subresourceRange.baseMipLevel		= descr.baseLevel.Get();
		view_info.subresourceRange.levelCount		= descr.levelCount;
		view_info.subresourceRange.baseArrayLayer	= descr.baseLayer.Get();
		view_info.subresourceRange.layerCount		= descr.layerCount;
		
		VK_CHECK( vkCreateImageView( GetVkDevice(), &view_info, null, OUT &img_view ) );
		_viewMap.Add( descr, img_view );

		GetDevice()->SetObjectName( ReferenceCast<uint64_t>(img_view), GetDebugName(), EGpuObject::ImageView );

		msg.result.Set( img_view );
		return true;
	}

/*
=================================================
	_GetImageDescription
=================================================
*/
	bool Vk1Image::_GetImageDescription (const GpuMsg::GetImageDescription &msg)
	{
		msg.result.Set( _descr );
		return true;
	}
	
/*
=================================================
	_SetImageDescription
=================================================
*/
	bool Vk1Image::_SetImageDescription (const GpuMsg::SetImageDescription &msg)
	{
		CHECK_ERR( GetState() == EState::Initial );

		_descr = msg.descr;

		Utils::ValidateDescription( INOUT _descr );
		return true;
	}

/*
=================================================
	_OnMemoryBindingChanged
=================================================
*/
	bool Vk1Image::_OnMemoryBindingChanged (const GpuMsg::OnMemoryBindingChanged &msg)
	{
		CHECK_ERR( _IsComposedOrLinkedState( GetState() ) );

		if (  msg.targetObject == this )
		{
			if ( not _isBindedToMemory )
			{
				_isBindedToMemory = true;
				
				_memObj->Unsubscribe( this, &Vk1Image::_OnMemoryBindingChanged );

				CHECK( _CreateDefaultView() );
				CHECK( _SetState( EState::ComposedMutable ) );
				
				_SendUncheckedEvent( ModuleMsg::AfterCompose{} );
			}
		}
		return true;
	}
	
/*
=================================================
	_GetImageMemoryLayout
=================================================
*/
	bool Vk1Image::_GetImageMemoryLayout (const GpuMsg::GetImageMemoryLayout &msg)
	{
		CHECK_ERR( _IsImageCreated() );
		CHECK_ERR( msg.mipLevel < _descr.maxLevel );
		CHECK_ERR( msg.layer.Get() < _descr.dimension.w );

		// get subresource layout
		VkImageSubresource	sub_resource	= {};
		VkSubresourceLayout sub_res_layout	= {};

		const bool			is_color		= EPixelFormat::IsColor( _descr.format );
		const bool			is_depth		= EPixelFormat::IsDepth( _descr.format );
		const uint4			lvl_dim			= Max( Utils::LevelDimension( _descr.imageType, _descr.dimension, msg.mipLevel.Get() ), 1u );

		CHECK_ERR( is_color or is_depth );
		sub_resource.aspectMask	= is_color ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;
		sub_resource.mipLevel	= Clamp( msg.mipLevel.Get(), 0u, _descr.maxLevel.Get()-1 );
		sub_resource.arrayLayer	= Clamp( msg.layer.Get(), 0u, lvl_dim.w-1 );

		vkGetImageSubresourceLayout( GetVkDevice(), _imageId, &sub_resource, OUT &sub_res_layout );

		GpuMsg::GetImageMemoryLayout::MemLayout	result;
		result.offset		= BytesU(sub_res_layout.offset);
		result.size			= BytesU(sub_res_layout.size);
		result.rowPitch		= BytesU(sub_res_layout.rowPitch);
		result.dimension	= lvl_dim.xyz();
		
		if ( _descr.imageType == EImage::Tex3D ) {
			ASSERT( sub_res_layout.arrayPitch == sub_res_layout.depthPitch );
			result.slicePitch = BytesU(sub_res_layout.depthPitch);
		} else {
			ASSERT( sub_res_layout.depthPitch == sub_res_layout.arrayPitch );
			result.slicePitch = BytesU(sub_res_layout.arrayPitch);
		}
		
		if ( result.rowPitch == 0 )
			result.rowPitch = BytesU(result.dimension.x * EPixelFormat::BitPerPixel( _descr.format ));

		if ( result.slicePitch == 0 )
			result.slicePitch = result.dimension.y * result.rowPitch;

		ASSERT( result.slicePitch * result.dimension.z <= result.size );

		msg.result.Set( result );
		return true;
	}

/*
=================================================
	_GetImageType
=================================================
*/
	VkImageType  Vk1Image::_GetImageType (EImage::type type)
	{
		switch ( type )
		{
			case EImage::Tex1D :
				return VK_IMAGE_TYPE_1D;

			case EImage::Tex2D :
			case EImage::Tex2DMS :
			case EImage::TexCube :
				return VK_IMAGE_TYPE_2D;

			case EImage::Tex1DArray :
			case EImage::Tex2DArray :
			case EImage::Tex2DMSArray :
			case EImage::TexCubeArray :
			case EImage::Tex3D :
				return VK_IMAGE_TYPE_3D;
		}
		RETURN_ERR( "not supported", VK_IMAGE_TYPE_MAX_ENUM );
	}
	
/*
=================================================
	_CanHaveImageView
=================================================
*/
	bool Vk1Image::_CanHaveImageView () const
	{
		return _descr.usage != (_descr.usage & (EImageUsage::TransferSrc | EImageUsage::TransferDst));
	}
	
/*
=================================================
	_ValidateMemFlags
=================================================
*/
	void Vk1Image::_ValidateMemFlags (INOUT EGpuMemory::bits &flags)
	{
		if ( flags[EGpuMemory::Dedicated] and flags[EGpuMemory::SupportAliasing] )
		{
			WARNING( "not supported" );

			flags[EGpuMemory::Dedicated] = false;
		}
	}
	
/*
=================================================
	_ValidateDescription
=================================================
*/
	void Vk1Image::_ValidateDescription (INOUT ImageDescription &descr)
	{
		// description may be invalid for sharing or for delayed initialization
		if ( Utils::IsValidDescription( descr ) )
			Utils::ValidateDescription( INOUT descr );
	}

}	// PlatformVK
//-----------------------------------------------------------------------------

namespace Platforms
{
	ModulePtr VulkanObjectsConstructor::CreateVk1Image (ModuleMsg::UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::GpuImage &ci)
	{
		return New< PlatformVK::Vk1Image >( id, gs, ci );
	}
}	// Platforms
}	// Engine

#endif	// GRAPHICS_API_VULKAN
