// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "Engine/Config/Engine.Config.h"

#ifdef GRAPHICS_API_OPENGL

#include "Engine/Platforms/Public/GPU/Image.h"
#include "Engine/Platforms/Public/GPU/Memory.h"
#include "Engine/Platforms/Public/Tools/ImageViewHashMap.h"
#include "Engine/Platforms/OpenGL/450/GL4BaseModule.h"
#include "Engine/Platforms/OpenGL/OpenGLObjectsConstructor.h"

#define GX_OGL_TEXSTORAGE

namespace Engine
{
namespace PlatformGL
{
	using namespace gl;


	//
	// OpenGL Texture
	//

	class GL4Image final : public GL4BaseModule
	{
	// types
	private:
		using ForwardToMem_t		= MessageListFrom< 
											ModuleMsg::GetStreamDescriptor,
											ModuleMsg::ReadFromStream,
											ModuleMsg::WriteToStream,
											GpuMsg::MapMemoryToCpu,
											GpuMsg::MapImageToCpu,
											GpuMsg::FlushMemoryRange,
											GpuMsg::UnmapMemory,
											GpuMsg::ReadFromGpuMemory,
											GpuMsg::WriteToGpuMemory,
											GpuMsg::ReadFromImageMemory,
											GpuMsg::WriteToImageMemory
										>;

		using SupportedMessages_t	= GL4BaseModule::SupportedMessages_t::Append< MessageListFrom<
											GpuMsg::GetImageDescriptor,
											GpuMsg::SetImageDescriptor,
											GpuMsg::GetGLImageID,
											GpuMsg::CreateGLImageView,
											GpuMsg::GpuMemoryRegionChanged,
											GpuMsg::GetImageMemoryLayout
										> >::Append< ForwardToMem_t >;

		using SupportedEvents_t		= GL4BaseModule::SupportedEvents_t;
		
		using MemoryEvents_t		= MessageListFrom< GpuMsg::OnMemoryBindingChanged >;

		using Utils					= Platforms::ImageUtils;
		
		using ImageViewMap_t	= PlatformTools::ImageViewHashMap< GLuint >;
		using ImageView_t		= ImageViewMap_t::Key_t;

		
	// constants
	private:
		static const TypeIdList		_msgTypes;
		static const TypeIdList		_eventTypes;


	// variables
	private:
		ImageDescriptor			_descr;
		ModulePtr				_memObj;
		ImageViewMap_t			_viewMap;
		GLuint					_imageId;
		GLuint					_imageView;		// default image view, has all mipmaps and all layers
		
		EGpuMemory::bits		_memFlags;		// -|-- this flags is requirements for memory obj, don't use it anywhere
		EMemoryAccess::bits		_memAccess;		// -|
		bool					_useMemMngr;	// -|

		bool					_isBindedToMemory;
		

	// methods
	public:
		GL4Image (UntypedID_t, GlobalSystemsRef gs, const CreateInfo::GpuImage &ci);
		~GL4Image ();


	// message handlers
	private:
		bool _Link (const Message< ModuleMsg::Link > &);
		bool _Compose (const Message< ModuleMsg::Compose > &);
		bool _Delete (const Message< ModuleMsg::Delete > &);
		bool _AttachModule (const Message< ModuleMsg::AttachModule > &);
		bool _DetachModule (const Message< ModuleMsg::DetachModule > &);
		bool _GetGLImageID (const Message< GpuMsg::GetGLImageID > &);
		bool _CreateGLImageView (const Message< GpuMsg::CreateGLImageView > &);
		bool _GetImageDescriptor (const Message< GpuMsg::GetImageDescriptor > &);
		bool _SetImageDescriptor (const Message< GpuMsg::SetImageDescriptor > &);
		bool _GpuMemoryRegionChanged (const Message< GpuMsg::GpuMemoryRegionChanged > &);
		bool _GetImageMemoryLayout (const Message< GpuMsg::GetImageMemoryLayout > &);
		
	// event handlers
		bool _OnMemoryBindingChanged (const Message< GpuMsg::OnMemoryBindingChanged > &);

	private:
		bool _IsImageCreated () const;

		bool _CreateImage ();
		bool _CreateDefaultView ();
		
		void _DestroyAll ();
		void _DestroyViews ();

		bool _CanHaveImageView () const;
	};
//-----------------------------------------------------------------------------


	
	const TypeIdList	GL4Image::_msgTypes{ UninitializedT< SupportedMessages_t >() };
	const TypeIdList	GL4Image::_eventTypes{ UninitializedT< SupportedEvents_t >() };

/*
=================================================
	constructor
=================================================
*/
	GL4Image::GL4Image (UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::GpuImage &ci) :
		GL4BaseModule( gs, ModuleConfig{ id, UMax }, &_msgTypes, &_eventTypes ),
		_descr( ci.descr ),				_imageId( 0 ),
		_imageView( 0 ),				_memFlags( ci.memFlags ),
		_memAccess( ci.access ),		_useMemMngr( ci.allocMem ),
		_isBindedToMemory( false )
	{
		SetDebugName( "GL4Image" );
		
		_SubscribeOnMsg( this, &GL4Image::_OnModuleAttached_Impl );
		_SubscribeOnMsg( this, &GL4Image::_OnModuleDetached_Impl );
		_SubscribeOnMsg( this, &GL4Image::_AttachModule );
		_SubscribeOnMsg( this, &GL4Image::_DetachModule );
		_SubscribeOnMsg( this, &GL4Image::_FindModule_Impl );
		_SubscribeOnMsg( this, &GL4Image::_ModulesDeepSearch_Impl );
		_SubscribeOnMsg( this, &GL4Image::_Link );
		_SubscribeOnMsg( this, &GL4Image::_Compose );
		_SubscribeOnMsg( this, &GL4Image::_Delete );
		_SubscribeOnMsg( this, &GL4Image::_OnManagerChanged );
		_SubscribeOnMsg( this, &GL4Image::_GetGLImageID );
		_SubscribeOnMsg( this, &GL4Image::_CreateGLImageView );
		_SubscribeOnMsg( this, &GL4Image::_GetImageDescriptor );
		_SubscribeOnMsg( this, &GL4Image::_SetImageDescriptor );
		_SubscribeOnMsg( this, &GL4Image::_GetDeviceInfo );
		_SubscribeOnMsg( this, &GL4Image::_GetGLDeviceInfo );
		_SubscribeOnMsg( this, &GL4Image::_GetGLPrivateClasses );
		_SubscribeOnMsg( this, &GL4Image::_GpuMemoryRegionChanged );
		_SubscribeOnMsg( this, &GL4Image::_GetImageMemoryLayout );

		_AttachSelfToManager( _GetGPUThread( ci.gpuThread ), UntypedID_t(0), true );

		Utils::ValidateDescriptor( INOUT _descr );
	}
	
/*
=================================================
	destructor
=================================================
*/
	GL4Image::~GL4Image ()
	{
		ASSERT( not _IsImageCreated() );
	}
	
/*
=================================================
	_Link
=================================================
*/
	bool GL4Image::_Link (const Message< ModuleMsg::Link > &msg)
	{
		if ( _IsComposedOrLinkedState( GetState() ) )
			return true;	// already linked

		CHECK_ERR( GetState() == EState::Initial or GetState() == EState::LinkingFailed );
		
		_memObj = GetModuleByEvent< MemoryEvents_t >();

		if ( not _memObj and _useMemMngr )
		{
			ModulePtr	mem_module;
			CHECK_ERR( GlobalSystems()->modulesFactory->Create(
								GLMemoryModuleID,
								GlobalSystems(),
								CreateInfo::GpuMemory{ null, _memFlags, _memAccess },
								OUT mem_module ) );

			CHECK_ERR( _Attach( "mem", mem_module, true ) );
			_memObj = mem_module;
		}
		CHECK_ATTACHMENT( _memObj );

		_memObj->Subscribe( this, &GL4Image::_OnMemoryBindingChanged );
		
		CHECK_LINKING( _CopySubscriptions< ForwardToMem_t >( _memObj ) );
		
		return Module::_Link_Impl( msg );
	}
	
/*
=================================================
	_Compose
=================================================
*/
	bool GL4Image::_Compose (const Message< ModuleMsg::Compose > &msg)
	{
		if ( _IsComposedState( GetState() ) )
			return true;	// already composed

		CHECK_ERR( GetState() == EState::Linked );

		CHECK_COMPOSING( _CreateImage() );
		
		_SendForEachAttachments( msg );
		
		// very paranoic check
		CHECK( _ValidateAllSubscriptions() );

		// composed state will be changed when memory binded to image
		return true;
	}
	
/*
=================================================
	_Delete
=================================================
*/
	bool GL4Image::_Delete (const Message< ModuleMsg::Delete > &msg)
	{
		_DestroyAll();

		return Module::_Delete_Impl( msg );
	}
	
/*
=================================================
	_AttachModule
=================================================
*/
	bool GL4Image::_AttachModule (const Message< ModuleMsg::AttachModule > &msg)
	{
		const bool	is_mem	= msg->newModule->GetSupportedEvents().HasAllTypes< MemoryEvents_t >();

		CHECK( _Attach( msg->name, msg->newModule, is_mem ) );

		if ( is_mem )
		{
			CHECK( _SetState( EState::Initial ) );
			_DestroyViews();
		}
		return true;
	}
	
/*
=================================================
	_DetachModule
=================================================
*/
	bool GL4Image::_DetachModule (const Message< ModuleMsg::DetachModule > &msg)
	{
		CHECK( _Detach( msg->oldModule ) );

		if ( msg->oldModule->GetSupportedEvents().HasAllTypes< MemoryEvents_t >() )
		{
			CHECK( _SetState( EState::Initial ) );
			_DestroyViews();
		}
		return true;
	}
		
/*
=================================================
	_IsImageCreated
=================================================
*/
	bool GL4Image::_IsImageCreated () const
	{
		return _imageId != 0;
	}

/*
=================================================
	_CreateImage
=================================================
*/
	bool GL4Image::_CreateImage ()
	{
		CHECK_ERR( _imageId == 0 );

		const GLenum	target = GL4Enum( _descr.imageType );
		
		GL_CALL( glGenTextures( 1, &_imageId ) );
		CHECK_ERR( _imageId != 0 );
		
		GL_CALL( glActiveTexture( GL_TEXTURE0 ) );
		GL_CALL( glBindTexture( target, _imageId ) );
		GL_CALL( glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 ) );

		switch ( _descr.imageType )
		{
			case EImage::Tex1D :
			{
				GL4InternalPixelFormat	ifmt;
				GL4PixelFormat			fmt;
				GL4PixelType			type;
				CHECK_ERR( GL4Enum( _descr.format, OUT ifmt, OUT fmt, OUT type ) );

				#ifdef GX_OGL_TEXSTORAGE
				GL_CALL( glTexStorage1D( target, _descr.maxLevel.Get(), ifmt, _descr.dimension.x ) );
				#else
				for (uint level = 0; level < _descr.maxLevel.Get(); ++level)
				{
					const uint2	size = Max( _descr.dimension.xy() >> level, 1u );
					GL_CALL( glTexImage1D( target, level, ifmt, size.x, 0, fmt, type, null ) );
				}
				#endif
				break;
			}
			case EImage::Tex2D :
			{
				GL4InternalPixelFormat	ifmt;
				GL4PixelFormat			fmt;
				GL4PixelType			type;
				CHECK_ERR( GL4Enum( _descr.format, OUT ifmt, OUT fmt, OUT type ) );

				#ifdef GX_OGL_TEXSTORAGE
				GL_CALL( glTexStorage2D( target, _descr.maxLevel.Get(), ifmt, _descr.dimension.x, _descr.dimension.y ) );
				#else
				for (uint level = 0; level < _descr.maxLevel.Get(); ++level)
				{
					const uint2	size = Max( _descr.dimension.xy() >> level, 1u );
					GL_CALL( glTexImage2D( target, level, ifmt, size.x, size.y, 0, fmt, type, null ) );
				}
				#endif
				break;
			}
			case EImage::Tex2DMS :
			{
				GL4InternalPixelFormat	ifmt;
				CHECK_ERR( GL4Enum( _descr.format, OUT ifmt ) );
				
				#ifdef GX_OGL_TEXSTORAGE
				GL_CALL( glTexStorage2DMultisample( target, _descr.samples.Get(), ifmt, _descr.dimension.x, _descr.dimension.y, true ) );
				#else
				GL_CALL( glTexImage2DMultisample( target, _descr.samples.Get(), ifmt, _descr.dimension.x, _descr.dimension.y, true ) );
				#endif
				break;
			}
			case EImage::Tex2DArray :
			{
				GL4InternalPixelFormat	ifmt;
				GL4PixelFormat			fmt;
				GL4PixelType			type;
				CHECK_ERR( GL4Enum( _descr.format, OUT ifmt, OUT fmt, OUT type ) );
				
				#ifdef GX_OGL_TEXSTORAGE
				GL_CALL( glTexStorage3D( target, _descr.maxLevel.Get(), ifmt, _descr.dimension.x, _descr.dimension.y, _descr.dimension.w ) );
				#else
				for (uint level = 0; level < _descr.maxLevel.Get(); ++level)
				{
					const uint2	size = Max( _descr.dimension.xy() >> level, 1u );
					GL_CALL( glTexImage3D( target, level, ifmt, size.x, size.y, _descr.dimension.w, 0, fmt, type, null ) );
				}
				#endif
				break;
			}
			case EImage::Tex2DMSArray :
			{
				GL4InternalPixelFormat	ifmt;
				CHECK_ERR( GL4Enum( _descr.format, OUT ifmt ) );
				
				#ifdef GX_OGL_TEXSTORAGE
				GL_CALL( glTexStorage3DMultisample( target, _descr.samples.Get(), ifmt, _descr.dimension.x, _descr.dimension.y, _descr.dimension.w, true ) );
				#else
				GL_CALL( glTexImage3DMultisample( target, _descr.samples.Get(), ifmt, _descr.dimension.x, _descr.dimension.y,
													_descr.dimension.w, true ) );
				#endif
				break;
			}
			case EImage::Tex3D :
			{
				GL4InternalPixelFormat	ifmt;
				GL4PixelFormat			fmt;
				GL4PixelType			type;
				CHECK_ERR( GL4Enum( _descr.format, OUT ifmt, OUT fmt, OUT type ) );
				
				#ifdef GX_OGL_TEXSTORAGE
				GL_CALL( glTexStorage3D( target, _descr.maxLevel.Get(), ifmt, _descr.dimension.x, _descr.dimension.y, _descr.dimension.z ) );
				#else
				for (uint level = 0; level < _descr.maxLevel.Get(); ++level)
				{
					const uint3	size = Max( _descr.dimension.xyz() >> level, 1u );
					GL_CALL( glTexImage3D( target, level, ifmt, size.x, size.y, size.z, 0, fmt, type, null ) );
				}
				#endif
				break;
			}
			case EImage::TexCube :
			{
				GL4InternalPixelFormat	ifmt;
				GL4PixelFormat			fmt;
				GL4PixelType			type;
				CHECK_ERR( GL4Enum( _descr.format, OUT ifmt, OUT fmt, OUT type ) );
				
				#ifdef GX_OGL_TEXSTORAGE
				GL_CALL( glTexStorage2D( target, _descr.maxLevel.Get(), ifmt, _descr.dimension.x, _descr.dimension.y ) );
				#else
				for (uint level = 0; level < _descr.maxLevel.Get(); ++level)
				{
					const uint2	size = Max( _descr.dimension.xy() >> level, 1u );

					for (uint i = 0; i < _descr.dimension.z; ++i) {
						GL_CALL( glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, level, ifmt,
												size.x, size.y, 0, fmt, type, null ) );
					}
				}
				#endif
				break;
			}
			case EImage::TexCubeArray :
			{
				GL4InternalPixelFormat	ifmt;
				GL4PixelFormat			fmt;
				GL4PixelType			type;
				CHECK_ERR( GL4Enum( _descr.format, OUT ifmt, OUT fmt, OUT type ) );
				
				#ifdef GX_OGL_TEXSTORAGE
				GL_CALL( glTexStorage3D( target, _descr.maxLevel.Get(), ifmt, _descr.dimension.x, _descr.dimension.y, _descr.dimension.z * _descr.dimension.w ) );
				#else
				for (uint level = 0; level < _descr.maxLevel.Get(); ++level)
				{
					const uint2	size = Max( _descr.dimension.xy() >> level, 1u );
					GL_CALL( glTexImage3D( target, level, ifmt, size.x, size.y, _descr.dimension.z * _descr.dimension.w,
											0, fmt, type, null ) );
				}
				#endif
				break;
			}
			default :
			{
				_DestroyAll();
				RETURN_ERR( "invalid texture type" );
			}
		}
		
		GL_CALL( glTexParameteri( target, GL_TEXTURE_BASE_LEVEL, GLint(0) ) );
		GL_CALL( glTexParameteri( target, GL_TEXTURE_MAX_LEVEL,  Max(GLint(_descr.maxLevel.Get())-1, 0) ) );
		GL_CALL( glBindTexture( target, 0 ) );
		
		GetDevice()->SetObjectName( _imageId, GetDebugName(), EGpuObject::Image );
		return true;
	}
	
/*
=================================================
	_CreateDefaultView
=================================================
*/
	bool GL4Image::_CreateDefaultView ()
	{
		CHECK_ERR( _IsImageCreated() );
		CHECK_ERR( _imageView == 0 );
		
		if ( _CanHaveImageView() )
		{
			Message< GpuMsg::CreateGLImageView >	create{ _descr };

			CHECK_ERR( _CreateGLImageView( create ) );

			_imageView = *create->result;
		}
		return true;
	}

/*
=================================================
	_DestroyAll
=================================================
*/
	void GL4Image::_DestroyAll ()
	{
		_DestroyViews();

		if ( _imageId != 0 ) {
			GL_CALL( glDeleteTextures( 1, &_imageId ) );
		}

		_imageId	= 0;
		_descr		= Uninitialized;
	}
	
/*
=================================================
	_DestroyViews
=================================================
*/
	void GL4Image::_DestroyViews ()
	{
		FOR( i, _viewMap ) {
			GL_CALL( glDeleteTextures( 1, &_viewMap[i].second ) );
		}

		if ( _memObj )
		{
			this->UnsubscribeAll( _memObj );
			_memObj->UnsubscribeAll( this );
			_memObj = null;
		}

		_viewMap.Clear();
		_imageView			= 0;
		_isBindedToMemory	= false;
	}

/*
=================================================
	_GetGLImageID
=================================================
*/
	bool GL4Image::_GetGLImageID (const Message< GpuMsg::GetGLImageID > &msg)
	{
		msg->result.Set( _imageId );
		return true;
	}
	
/*
=================================================
	_CreateGLImageView
=================================================
*/
	bool GL4Image::_CreateGLImageView (const Message< GpuMsg::CreateGLImageView > &msg)
	{
		CHECK_ERR( _IsImageCreated() );
		CHECK_ERR( _isBindedToMemory );
		CHECK_ERR( _CanHaveImageView() );

		ImageView_t		descr = msg->viewDescr;
		
		ImageViewMap_t::Validate( INOUT descr, _descr );
		
		// search in cache
		GLuint		img_view = _viewMap.Find( descr );
		
		if ( img_view != 0 )
		{
			msg->result.Set( img_view );
			return true;
		}

		// create new image view
		GL_CALL( glGenTextures( 1, &img_view ) );
		CHECK_ERR( img_view != 0 );
		
		GLenum	target = GL4Enum( descr.viewType );

		GL4InternalPixelFormat	ifmt;
		CHECK_ERR( GL4Enum( _descr.format, OUT ifmt ) );

		GL_CALL( glTextureView( img_view, target, _imageId, ifmt,
								descr.baseLevel.Get(), descr.levelCount,
								descr.baseLayer.Get(), descr.layerCount ) );

		// setup image
		const GLenum	components[]	= { GL_ZERO, GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA, GL_ZERO, GL_ONE };
		int4			swizzle_mask	= Min( uint4(uint(CountOf(components)-1)), descr.swizzle.ToVec() ).To<int4>();

		FOR( i, swizzle_mask ) { swizzle_mask[i] = components[ swizzle_mask[i] ]; }

		GL_CALL( glBindTexture( target, img_view ) );
		GL_CALL( glTexParameteriv( target, GL_TEXTURE_SWIZZLE_RGBA, swizzle_mask.ptr() ) );
		
		GL_CALL( glTexParameteri( target, GL_TEXTURE_BASE_LEVEL, GLint(0) ) );
		GL_CALL( glTexParameteri( target, GL_TEXTURE_MAX_LEVEL,  Max(GLint(descr.levelCount)-1, 0) ) );
		GL_CALL( glBindTexture( target, 0 ) );

		_viewMap.Add( descr, img_view );

		msg->result.Set( img_view );
		return true;
	}

/*
=================================================
	_GetImageDescriptor
=================================================
*/
	bool GL4Image::_GetImageDescriptor (const Message< GpuMsg::GetImageDescriptor > &msg)
	{
		msg->result.Set( _descr );
		return true;
	}
	
/*
=================================================
	_SetImageDescriptor
=================================================
*/
	bool GL4Image::_SetImageDescriptor (const Message< GpuMsg::SetImageDescriptor > &msg)
	{
		CHECK_ERR( GetState() == EState::Initial );

		_descr = msg->descr;

		Utils::ValidateDescriptor( INOUT _descr );
		return true;
	}

/*
=================================================
	_OnMemoryBindingChanged
=================================================
*/
	bool GL4Image::_OnMemoryBindingChanged (const Message< GpuMsg::OnMemoryBindingChanged > &msg)
	{
		CHECK_ERR( _IsComposedOrLinkedState( GetState() ) );

		using EBindingTarget = GpuMsg::OnMemoryBindingChanged::EBindingTarget;

		if (  msg->targetObject == this )
		{
			_isBindedToMemory = ( msg->newState == EBindingTarget::Image );

			if ( _isBindedToMemory )
			{
				CHECK( _CreateDefaultView() );
				CHECK( _SetState( EState::ComposedMutable ) );
		
				_SendUncheckedEvent< ModuleMsg::AfterCompose >({});
			}
			else
			{
				CHECK( _SetState( EState::Linked ) );
			}
		}
		return true;
	}

/*
=================================================
	_GpuMemoryRegionChanged
=================================================
*/
	bool GL4Image::_GpuMemoryRegionChanged (const Message< GpuMsg::GpuMemoryRegionChanged > &msg)
	{
		// request image memory barrier
		TODO( "" );
		return false;
	}
	
/*
=================================================
	_GetImageMemoryLayout
=================================================
*/
	bool GL4Image::_GetImageMemoryLayout (const Message< GpuMsg::GetImageMemoryLayout > &msg)
	{
		CHECK_ERR( _IsImageCreated() );
		CHECK_ERR( msg->mipLevel < _descr.maxLevel );

		const uint4		lvl_size	= Max( ImageUtils::LevelDimension( _descr.imageType, _descr.dimension, msg->mipLevel.Get() ), 1u );
		const BytesUL	bpp			= BytesUL(EPixelFormat::BitPerPixel( _descr.format ));
		const BytesUL	row_align	= BytesUL(uint(bpp) % 4 == 0 ? 4 : 1);

		GpuMsg::GetImageMemoryLayout::MemLayout	result;
		result.offset		= BytesUL(0);	// not supported
		result.rowPitch		= GXImageUtils::AlignedRowSize( lvl_size.x, bpp, row_align );
		result.slicePitch	= GXImageUtils::AlignedRowSize( lvl_size.y * result.rowPitch, BytesUL(1), row_align );
		result.size			= result.slicePitch * lvl_size.z * lvl_size.w;
		result.dimension	= lvl_size.xyz();
		
		ASSERT( result.slicePitch * result.dimension.z == result.size );

		msg->result.Set( result );
		return true;
	}
	
/*
=================================================
	_CanHaveImageView
=================================================
*/
	bool GL4Image::_CanHaveImageView () const
	{
		return _descr.usage != (_descr.usage & (EImageUsage::TransferSrc | EImageUsage::TransferDst));
	}

}	// PlatformGL
//-----------------------------------------------------------------------------

namespace Platforms
{
	ModulePtr OpenGLObjectsConstructor::CreateGL4Image (ModuleMsg::UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::GpuImage &ci)
	{
		return New< PlatformGL::GL4Image >( id, gs, ci );
	}
}	// Platforms
}	// Engine

#endif	// GRAPHICS_API_OPENGL
