// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "Core/Config/Engine.Config.h"

#ifdef GRAPHICS_API_OPENGL

#include "Engine/Platforms/Public/GPU/Image.h"
#include "Engine/Platforms/Public/GPU/RenderPass.h"
#include "Engine/Platforms/Public/GPU/Framebuffer.h"
#include "Engine/Platforms/OpenGL/450/GL4BaseModule.h"
#include "Engine/Platforms/OpenGL/450/GL4ResourceCache.h"
#include "Engine/Platforms/OpenGL/OpenGLObjectsConstructor.h"
#include "Engine/Platforms/Public/Tools/ImageUtils.h"

namespace Engine
{
namespace PlatformGL
{
	using namespace gl;


	
	//
	// OpenGL Framebuffer
	//

	class GL4Framebuffer final : public GL4BaseModule
	{
	// types
	private:
		using SupportedMessages_t	= GL4BaseModule::SupportedMessages_t::Append< MessageListFrom<
											GpuMsg::GetFramebufferDescription,
											GpuMsg::GetGLFramebufferID,
											GpuMsg::FramebufferAttachImage
										> >;

		using SupportedEvents_t		= GL4BaseModule::SupportedEvents_t;
		
		using RenderPassMsgList_t	= MessageListFrom< GpuMsg::GetRenderPassDescription, GpuMsg::GetGLRenderPassID >;
		using ImageMsgList_t		= MessageListFrom< GpuMsg::GetImageDescription, GpuMsg::CreateGLImageView >;
		
		struct AttachmentInfo : CompileTime::PODStruct
		{
		// variables
			ModuleName_t			name;
			GLuint					imageId		= 0;
			ImageViewDescription	descr;
			MultiSamples			samples;

		// methods
			AttachmentInfo () {}
		};
	
		using Attachments_t		= FixedSizeArray< AttachmentInfo, GlobalConst::GAPI_MaxColorBuffers >;
		
		using ImageUtils			= PlatformTools::ImageUtils;


	// constants
	private:
		static const TypeIdList		_eventTypes;


	// variables
	private:
		GLuint						_framebufferId;

		FramebufferDescription		_descr;
		
		Attachments_t				_attachments;


	// methods
	public:
		GL4Framebuffer (UntypedID_t, GlobalSystemsRef gs, const CreateInfo::GpuFramebuffer &ci);
		~GL4Framebuffer ();


	// message handlers
	private:
		bool _Compose (const ModuleMsg::Compose &);
		bool _Delete (const ModuleMsg::Delete &);
		bool _AttachModule (const ModuleMsg::AttachModule &);
		bool _DetachModule (const ModuleMsg::DetachModule &);
		bool _GetGLFramebufferID (const GpuMsg::GetGLFramebufferID &);
		bool _GetFramebufferDescription (const GpuMsg::GetFramebufferDescription &);
		bool _FramebufferAttachImage (const GpuMsg::FramebufferAttachImage &);

	private:
		bool _IsCreated () const;
		bool _CreateFramebuffer ();
		void _DestroyFramebuffer ();
		
		bool _CreateRenderPassByAttachment (OUT RenderPassDescription &rpDescr);
		bool _ValidateAttachment (const RenderPassDescription &rpDescr) const;

		static bool _GetAttachmentTarget (const AttachmentInfo &info, const RenderPassDescription &rpDescr, OUT usize &index, OUT GLenum &target);

		static void _ValidateDescription (INOUT FramebufferDescription &descr);
	};
//-----------------------------------------------------------------------------


	
	const TypeIdList	GL4Framebuffer::_eventTypes{ UninitializedT< SupportedEvents_t >() };

/*
=================================================
	constructor
=================================================
*/
	GL4Framebuffer::GL4Framebuffer (UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::GpuFramebuffer &ci) :
		GL4BaseModule( gs, ModuleConfig{ id, UMax }, &_eventTypes ),
		_framebufferId( 0 ),
		_descr( ci.size, ci.layers )
	{
		SetDebugName( "GL4Framebuffer" );

		_SubscribeOnMsg( this, &GL4Framebuffer::_OnModuleAttached_Impl );
		_SubscribeOnMsg( this, &GL4Framebuffer::_OnModuleDetached_Impl );
		_SubscribeOnMsg( this, &GL4Framebuffer::_AttachModule );
		_SubscribeOnMsg( this, &GL4Framebuffer::_DetachModule );
		_SubscribeOnMsg( this, &GL4Framebuffer::_FindModule_Impl );
		_SubscribeOnMsg( this, &GL4Framebuffer::_ModulesDeepSearch_Impl );
		_SubscribeOnMsg( this, &GL4Framebuffer::_Link_Impl );
		_SubscribeOnMsg( this, &GL4Framebuffer::_Compose );
		_SubscribeOnMsg( this, &GL4Framebuffer::_Delete );
		_SubscribeOnMsg( this, &GL4Framebuffer::_OnManagerChanged );
		_SubscribeOnMsg( this, &GL4Framebuffer::_GetGLFramebufferID );
		_SubscribeOnMsg( this, &GL4Framebuffer::_GetFramebufferDescription );
		_SubscribeOnMsg( this, &GL4Framebuffer::_GetDeviceInfo );
		_SubscribeOnMsg( this, &GL4Framebuffer::_GetGLDeviceInfo );
		_SubscribeOnMsg( this, &GL4Framebuffer::_GetGLPrivateClasses );
		_SubscribeOnMsg( this, &GL4Framebuffer::_FramebufferAttachImage );

		ASSERT( _ValidateMsgSubscriptions< SupportedMessages_t >() );

		_AttachSelfToManager( _GetGPUThread( ci.gpuThread ), UntypedID_t(0), true );

		_ValidateDescription( INOUT _descr );
	}
	
/*
=================================================
	destructor
=================================================
*/
	GL4Framebuffer::~GL4Framebuffer ()
	{
		ASSERT( not _IsCreated() );
	}

/*
=================================================
	_Compose
=================================================
*/
	bool GL4Framebuffer::_Compose (const ModuleMsg::Compose &msg)
	{
		if ( _IsComposedState( GetState() ) )
			return true;	// already composed

		CHECK_ERR( GetState() == EState::Linked );

		_SendForEachAttachments( msg );

		CHECK_COMPOSING( _CreateFramebuffer() );
		
		// very paranoic check
		CHECK( _ValidateAllSubscriptions() );

		CHECK( _SetState( EState::ComposedMutable ) );
		
		_SendUncheckedEvent( ModuleMsg::AfterCompose{} );
		return true;
	}
	
/*
=================================================
	_Delete
=================================================
*/
	bool GL4Framebuffer::_Delete (const ModuleMsg::Delete &msg)
	{
		_DestroyFramebuffer();

		_descr = Uninitialized;
		_attachments.Clear();

		return Module::_Delete_Impl( msg );
	}
	
/*
=================================================
	_AttachModule
=================================================
*/
	bool GL4Framebuffer::_AttachModule (const ModuleMsg::AttachModule &msg)
	{
		CHECK_ERR( msg.newModule );

		// render pass must be unique
		bool	is_render_pass	= msg.newModule->SupportsAllMessages< RenderPassMsgList_t >();
		bool	is_image		= msg.newModule->SupportsAllMessages< ImageMsgList_t >();

		if ( _Attach( msg.name, msg.newModule ) and (is_image or is_render_pass) )
		{
			CHECK( _SetState( EState::Initial ) );
			_DestroyFramebuffer();
		}
		return true;
	}
	
/*
=================================================
	_AttachModule
=================================================
*/
	bool GL4Framebuffer::_FramebufferAttachImage (const GpuMsg::FramebufferAttachImage &msg)
	{
		ModulePtr	mod = GetModuleByName( msg.name );
		if ( mod ) {
			CHECK( _Detach( mod ) );
		}

		bool			found = false;
		AttachmentInfo	new_att;
		
		new_att.name			= msg.name;
		new_att.descr			= msg.viewDescr;
		new_att.descr.swizzle	= ImageSwizzle();

		CHECK( new_att.descr.swizzle == msg.viewDescr.swizzle );

		FOR( i, _attachments )
		{
			auto&	att = _attachments[i];

			// replace
			if ( att.name == msg.name )
			{
				att		= new_att;
				found	= true;
				break;
			}
		}

		// add new attachment
		if ( not found ) {
			_attachments.PushBack( new_att );
		}
		
		if ( _Attach( msg.name, msg.image ) )
		{
			CHECK( _SetState( EState::Initial ) );
			_DestroyFramebuffer();
		}
		return true;
	}
	
/*
=================================================
	_Delete
=================================================
*/
	bool GL4Framebuffer::_DetachModule (const ModuleMsg::DetachModule &msg)
	{
		CHECK_ERR( msg.oldModule );

		bool	is_render_pass	= msg.oldModule->SupportsAllMessages< RenderPassMsgList_t >();
		bool	is_image		= msg.oldModule->SupportsAllMessages< ImageMsgList_t >();

		if ( _Detach( msg.oldModule ) and (is_image or is_render_pass) )
		{
			CHECK( _SetState( EState::Initial ) );
			_DestroyFramebuffer();
		}
		return true;
	}
	
/*
=================================================
	_GetGLFramebufferID
=================================================
*/
	bool GL4Framebuffer::_GetGLFramebufferID (const GpuMsg::GetGLFramebufferID &msg)
	{
		ASSERT( _IsCreated() );

		msg.result.Set( _framebufferId );
		return true;
	}

/*
=================================================
	_GetFramebufferDescription
=================================================
*/
	bool GL4Framebuffer::_GetFramebufferDescription (const GpuMsg::GetFramebufferDescription &msg)
	{
		msg.result.Set( _descr );
		return true;
	}
	
/*
=================================================
	_IsCreated
=================================================
*/
	bool GL4Framebuffer::_IsCreated () const
	{
		return _framebufferId != 0;
	}
	
/*
=================================================
	_CreateFramebuffer
=================================================
*/
	bool GL4Framebuffer::_CreateFramebuffer ()
	{
		CHECK_ERR( not _IsCreated() );
		CHECK_ERR( not _attachments.Empty() );
		
		using ImageViewMessages_t		= MessageListFrom< GpuMsg::GetImageDescription, GpuMsg::GetGLImageID >;
		using ColorAttachmentInfos_t	= FixedSizeArray< AttachmentInfo, GlobalConst::GAPI_MaxColorBuffers >;
		
		RenderPassDescription	render_pass_descr;
		ModulePtr				render_pass;
		
		// get render pass
		{
			ModulePtr	mod = GetModuleByMsg< RenderPassMsgList_t >();

			if ( mod )
			{
				render_pass			= mod;
				render_pass_descr	= mod->Request(GpuMsg::GetRenderPassDescription{});
			}
		}

		// get attachments by name
		FOR( i, _attachments )
		{
			auto&		att = _attachments[i];
			ModulePtr	mod;
			CHECK_ERR( mod = GetModuleByName( att.name ) );
			CHECK_ERR( _IsComposedState( mod->GetState() ) );
			
			const auto&	img_res = GetResourceCache()->GetImageID( mod );

			att.descr.format	= att.descr.format == EPixelFormat::Unknown ? img_res.Get<1>().format : att.descr.format;
			att.descr.viewType	= att.descr.viewType == EImage::Unknown ? img_res.Get<1>().imageType : att.descr.viewType;
			att.samples			= img_res.Get<1>().samples;
			att.imageId			= img_res.Get<0>();

			const uint4	dim	= Max( ImageUtils::LevelDimension( att.descr.viewType, img_res.Get<1>().dimension, att.descr.baseLevel.Get() ), 1u );

			// validate
			CHECK_ERR( All( dim.xy() >= _descr.size ) );

			if ( _descr.layers > 1 )
			{
				CHECK_ERR( EImage::IsArray( att.descr.viewType ) );
				CHECK_ERR( dim.w == _descr.layers and att.descr.baseLayer.Get() == 0 );	// OpenGL doesn't support range for layers
			}
			else
				att.descr.baseLayer = Uninitialized;
		}

		
		// check attachments
		if ( render_pass == null )
		{
			CHECK_ERR( _CreateRenderPassByAttachment( OUT render_pass_descr ) );
		}
		else
			CHECK_ERR( _ValidateAttachment( render_pass_descr ) );
		
		
		// create framebuffer
		GL_CALL( glGenFramebuffers( 1, OUT &_framebufferId ) );
		CHECK_ERR( _framebufferId != 0 );
		GL_CALL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, _framebufferId ) );
		
		_descr.colorAttachments.Resize( render_pass_descr.ColorAttachments().Count() );
		_descr.samples = _attachments.Front().samples;	// TODO

		FOR( i, _attachments )
		{
			const auto&	att			= _attachments[i];
			bool		is_depth	= att.name == render_pass_descr.DepthStencilAttachment().name;
			usize		index		= UMax;
			GLenum		target		= 0;

			_GetAttachmentTarget( att, render_pass_descr, OUT index, OUT target );
			
			if ( att.descr.baseLayer.IsDefined() )
				GL_CALL( glFramebufferTextureLayer( GL_DRAW_FRAMEBUFFER, target, att.imageId, att.descr.baseLevel.Get(), att.descr.baseLayer.Get() ) )
			else
				GL_CALL( glFramebufferTexture( GL_DRAW_FRAMEBUFFER, target, att.imageId, att.descr.baseLevel.Get() ) );

			if ( is_depth )
				_descr.depthStencilAttachment = FramebufferDescription::AttachmentInfo{ att.name, att.descr.viewType };
			else
				_descr.colorAttachments[index] = FramebufferDescription::AttachmentInfo{ att.name, att.descr.viewType };
		}

		// check status
		GLenum status = 0;
		GL_CALL( status = glCheckFramebufferStatus( GL_DRAW_FRAMEBUFFER ) );

		GL_CALL( glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 ) );
		
		if ( status != GL_FRAMEBUFFER_COMPLETE )
		{
			String	msg( "OpenGL FrameBuffer status: " );

			switch ( status )
			{
				case GL_FRAMEBUFFER_UNDEFINED :						msg << "GL_FRAMEBUFFER_UNDEFINED";						break;
				case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT :			msg << "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";			break;
				case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT :	msg << "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";	break;
				case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER :		msg << "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";			break;
				case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER :		msg << "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";			break;
				case GL_FRAMEBUFFER_UNSUPPORTED :					msg << "GL_FRAMEBUFFER_UNSUPPORTED";					break;
				case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE :		msg << "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";			break;
				case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS :		msg << "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";		break;
				default :											msg << "code 0x" << String().FormatI( status, 16 );		break;
			}
			RETURN_ERR( msg );
		}

		GetDevice()->SetObjectName( _framebufferId, GetDebugName(), EGpuObject::Framebuffer );
		return true;
	}
	
/*
=================================================
	_GetAttachmentTarget
=================================================
*/
	bool GL4Framebuffer::_GetAttachmentTarget (const AttachmentInfo &info, const RenderPassDescription &rpDescr, OUT usize &index, OUT GLenum &target)
	{
		// depth stencil
		if ( info.name == rpDescr.DepthStencilAttachment().name )
		{
			index = UMax; // not needed for depth stencil

			if ( EPixelFormat::IsDepthStencil( info.descr.format ) ) {
				target = GL_DEPTH_STENCIL_ATTACHMENT;
				return true;
			}

			if ( EPixelFormat::HasDepth( info.descr.format ) ) {
				target = GL_DEPTH_ATTACHMENT;
				return true;
			}

			if ( EPixelFormat::HasStencil( info.descr.format ) ) {
				target = GL_STENCIL_ATTACHMENT;
				return true;
			}

			RETURN_ERR( "format is not supported for depth stencil attachment '" << info.name << "'" );
		}

		FOR( i, rpDescr.ColorAttachments() )
		{
			if ( rpDescr.ColorAttachments()[i].name == info.name )
			{
				CHECK_ERR( EPixelFormat::IsColor( info.descr.format ) );
				CHECK_ERR( i < GlobalConst::GAPI_MaxColorBuffers );

				index  = i;
				target = GL_COLOR_ATTACHMENT0 + uint(i);
				return true;
			}
		}

		RETURN_ERR( "Attachment '" << info.name << "' not found in render pass" );
	}
	
/*
=================================================
	_CreateRenderPassByAttachment
=================================================
*/
	bool GL4Framebuffer::_CreateRenderPassByAttachment (OUT RenderPassDescription &rpDescr)
	{
		auto	builder = RenderPassDescrBuilder::CreateForFramebuffer();

		FOR( i, _attachments )
		{
			const auto&	att = _attachments[i];

			ASSERT( att.imageId != 0 );
			builder.Add( att.name, att.descr.format, att.samples );
		}

		ModulePtr	render_pass;
		CHECK_ERR( GlobalSystems()->modulesFactory->Create(
							GLRenderPassModuleID,
							GlobalSystems(),
							CreateInfo::GpuRenderPass{ null, builder.Finish() },
							OUT render_pass
						));

		ModuleUtils::Initialize({ render_pass });

		CHECK_ERR( _Attach( "renderpass", render_pass ) );

		rpDescr = render_pass->Request( GpuMsg::GetRenderPassDescription{} );
		return true;
	}

/*
=================================================
	_ValidateAttachment
=================================================
*/
	bool GL4Framebuffer::_ValidateAttachment (const RenderPassDescription &rpDescr) const
	{
		CHECK_ERR( _attachments.Count() == rpDescr.ColorAttachments().Count() + uint(rpDescr.DepthStencilAttachment().IsEnabled()) );

		FOR( i, _attachments )
		{
			const auto&	att = _attachments[i];

			// check in depth stencil attachment
			if ( att.name == rpDescr.DepthStencilAttachment().name )
			{
				CHECK_ERR( att.descr.format == rpDescr.DepthStencilAttachment().format );
				CHECK_ERR( att.samples == rpDescr.DepthStencilAttachment().samples );
				continue;
			}

			// check in color attachments
			bool	found = false;

			FOR( j, rpDescr.ColorAttachments() )
			{
				const auto&	col = rpDescr.ColorAttachments()[j];

				if ( col.name == att.name )
				{
					CHECK_ERR( col.format == att.descr.format );
					CHECK_ERR( col.samples == att.samples );
					found = true;
					break;
				}
			}
			if ( found )
				continue;

			RETURN_ERR( "Attachment '" << att.name << "' not presented in render pass" );
		}
		return true;
	}

/*
=================================================
	_DestroyFramebuffer
=================================================
*/
	void GL4Framebuffer::_DestroyFramebuffer ()
	{
		if ( _framebufferId != 0 )
		{
			GL_CALL( glDeleteFramebuffers( 1, &_framebufferId ) );
		}

		_framebufferId	= 0;
	}

/*
=================================================
	_ValidateDescription
=================================================
*/
	void GL4Framebuffer::_ValidateDescription (INOUT FramebufferDescription &descr)
	{
		CHECK( Any( descr.size != uint2(0) ) );

		descr.layers	= Max( descr.layers, 1u );
	}

}	// PlatformGL
//-----------------------------------------------------------------------------

namespace Platforms
{
	ModulePtr OpenGLObjectsConstructor::CreateGL4Framebuffer (ModuleMsg::UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::GpuFramebuffer &ci)
	{
		return New< PlatformGL::GL4Framebuffer >( id, gs, ci );
	}
}	// Platforms
}	// Engine

#endif	// GRAPHICS_API_OPENGL
