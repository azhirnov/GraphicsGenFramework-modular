// Copyright �  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "Engine/Platforms/OpenGL/Impl/GL4Device.h"

#if defined( GRAPHICS_API_OPENGL )

#include "Engine/Platforms/OpenGL/Impl/GL4SystemFramebuffer.h"
#include "Engine/Platforms/Shared/GPU/RenderPass.h"
#include "Engine/Platforms/Shared/GPU/CommandBuffer.h"

namespace Engine
{
namespace PlatformGL
{
	using namespace gl;

/*
=================================================
	constructor
=================================================
*/
	GL4Device::GL4Device (GlobalSystemsRef gs) :
		BaseObject( gs ),
		_colorPixelFormat( EPixelFormat::Unknown ),
		_depthStencilPixelFormat( EPixelFormat::Unknown ),
		_currentImageIndex( -1 ),	_swapchainLength( 1 ),
		_numExtensions( 0 ),		_initialized( false ),
		_frameStarted( false )
	{
		SetDebugName( "GL4Device" );

		CHECK( GlobalSystems()->Get< ModulesFactory >()->Register(
			GLSystemFramebufferModuleID,
			&GL4SystemFramebuffer::CreateModule
		) );
	}
	
/*
=================================================
	destructor
=================================================
*/
	GL4Device::~GL4Device ()
	{
		ASSERT( not _initialized );
	}

/*
=================================================
	Initialize
=================================================
*/
	bool GL4Device::Initialize (const uint2 &surfaceSize, EPixelFormat::type colorFormat, EPixelFormat::type depthStencilFormat, MultiSamples samples)
	{
		CHECK_ERR( GL4_Init() );

		GL4_WriteInfo();
		
		CHECK_ERR( GL4_GetVersion() >= 450 or IsExtensionSupported("GL_EXT_direct_state_access") );

		_surfaceSize			 = surfaceSize;
		_colorPixelFormat		 = colorFormat;
		_depthStencilPixelFormat = depthStencilFormat;
		_samples				 = samples;

		CHECK_ERR( _CreateCommandBuffer() );
		CHECK_ERR( _CreateRenderPass() );
		CHECK_ERR( _CreateFramebuffer() );

		ModuleUtils::Initialize({ _framebuffer, _renderPass, _commandBuilder });

		_initialized = true;
		return true;
	}
/*
=================================================
	Deinitialize
=================================================
*/
	void GL4Device::Deinitialize ()
	{
		_surfaceSize			 = Uninitialized;
		_colorPixelFormat		 = Uninitialized;
		_depthStencilPixelFormat = Uninitialized;
		_samples				 = Uninitialized;

		_commandBuilder		= null;
		_framebuffer		= null;
		_renderPass			= null;

		_currentImageIndex	= -1;
		_swapchainLength	= 1;

		_numExtensions		= 0;
		_initialized		= false;
		_frameStarted		= false;

		GL4_Delete();
	}
	
/*
=================================================
	InitDebugReport
=================================================
*/
	bool GL4Device::InitDebugReport ()
	{
		DEBUG_ONLY(
			GL_CALL( glDebugMessageCallback( &_StaticDebugCallback, this ) );

			// disable notifications
			GL_CALL( glDebugMessageControl( GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION,
											0, null, GL_FALSE ) );
		)
		return true;
	}
	
/*
=================================================
	_StaticDebugCallback
=================================================
*/
	void GL4Device::_StaticDebugCallback (GLenum source, GLenum type, GLuint id, GLenum severity,
										   GLsizei length, const GLchar* message, const void* userParam)
	{
		String	str;

		switch ( severity )
		{
			case GL_DEBUG_SEVERITY_HIGH 			: str << "[High]";			break;
			case GL_DEBUG_SEVERITY_MEDIUM 			: str << "[Medium]";		break;
			case GL_DEBUG_SEVERITY_LOW 				: str << "[Low]";			break;
			case GL_DEBUG_SEVERITY_NOTIFICATION		: str << "[Notification]";	break;
			default									: str << "[unknown]";		WARNING( "unknown severity" );
		}

		str << " src: ";

		switch ( source )
		{
			case GL_DEBUG_SOURCE_API				: str << "OpenGL";			break;
			case GL_DEBUG_SOURCE_WINDOW_SYSTEM		: str << "OS";				break;
			case GL_DEBUG_SOURCE_SHADER_COMPILER	: str << "GL_Compiler";		break;
			case GL_DEBUG_SOURCE_THIRD_PARTY		: str << "Thirf_Party";		break;
			case GL_DEBUG_SOURCE_APPLICATION 		: str << "Application";		break;
			case GL_DEBUG_SOURCE_OTHER 				: str << "Other";			break;
			default									: str << "Unknown";			WARNING( "unknown source" );
		}

		str << ", type: ";

		switch ( type )
		{
			case GL_DEBUG_TYPE_ERROR 				: str << "Error";				break;
			case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR	: str << "Deprecated";			break;
			case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR	: str << "Undefined_Behavior";	break;
			case GL_DEBUG_TYPE_PORTABILITY 			: str << "Portability";			break;
			case GL_DEBUG_TYPE_PERFORMANCE 			: str << "Perfomance";			break;
			case GL_DEBUG_TYPE_MARKER 				: str << "Marker";				break;
			case GL_DEBUG_TYPE_PUSH_GROUP 			: str << "Push_Group";			break;
			case GL_DEBUG_TYPE_POP_GROUP 			: str << "Pop_Group";			break;
			case GL_DEBUG_TYPE_OTHER 				: str << "Other";				break;
			default									: str << "Unknown";				WARNING( "unknown type" );
		}

		str << ", message:\n";

		if ( message != null and length > 0 )
			str += message;

		LOG( str.cstr(), ELog::Debug );
	}
	
/*
=================================================
	BeginFrame
=================================================
*/
	bool GL4Device::BeginFrame ()
	{
		DBG_CHECK_ERR( _initialized );
		CHECK_ERR( not _frameStarted );

		_frameStarted = true;
		return true;
	}
	
/*
=================================================
	EndFrame
=================================================
*/
	bool GL4Device::EndFrame ()
	{
		DBG_CHECK_ERR( _initialized );
		CHECK_ERR( _frameStarted );

		_currentImageIndex = (_currentImageIndex+1) % _swapchainLength;
		_frameStarted = false;
		return true;
	}
	
/*
=================================================
	OnResize
=================================================
*/
	bool GL4Device::OnResize (const uint2 &size)
	{
		CHECK_ERR( not _frameStarted );

		_surfaceSize = size;

		if ( _framebuffer )
			_framebuffer.ToPtr< GL4SystemFramebuffer >()->Resize( _surfaceSize );

		return true;
	}
	
/*
=================================================
	IsExtensionSupported
=================================================
*/
	bool GL4Device::IsExtensionSupported (StringCRef name)
	{
		const char *	ext		= null;
		GLint			count	= 0;
		
		GL_CALL( glGetIntegerv( GL_NUM_EXTENSIONS, OUT &count ) );

		for (int i = 0; i < count; ++i)
		{
			GL_CALL( ext = (const char *) glGetStringi( GL_EXTENSIONS, i ) );

			if ( name.EqualsIC( ext ) )
				return true;
		}
		return false;
	}
	
/*
=================================================
	SetObjectName
=================================================
*/
	bool GL4Device::SetObjectName (gl::GLuint id, StringCRef name, EGpuObject::type type) const
	{
		if ( name.Empty() or id == 0  )
			return false;

		GL_CALL( glObjectLabel( GL4Enum( type ), id, name.Length(), name.ptr() ) );
		return true;
	}
	
/*
=================================================
	SetObjectName
=================================================
*/
	bool GL4Device::SetObjectName (void* ptr, StringCRef name, EGpuObject::type type) const
	{
		CHECK_ERR( type == EGpuObject::Fence );

		if ( name.Empty() or ptr == null  )
			return false;

		GL_CALL( glObjectPtrLabel( ptr, name.Length(), name.ptr() ) );
		return true;
	}

/*
=================================================
	_CreateCommandBuffer
=================================================
*/
	bool GL4Device::_CreateCommandBuffer ()
	{
		CHECK_ERR( GlobalSystems()->Get< ModulesFactory >()->Create(
			Platforms::GLCommandBuilderModuleID,
			GlobalSystems(),
			CreateInfo::GpuCommandBuilder{},
			OUT _commandBuilder )
		);

		ModuleUtils::Initialize( {_commandBuilder}, this );
		return true;
	}
	
/*
=================================================
	_CreateRenderPass
=================================================
*/
	bool GL4Device::_CreateRenderPass ()
	{
		CHECK_ERR( not _renderPass );

		ModulePtr	module;
		CHECK_ERR( GlobalSystems()->Get< ModulesFactory >()->Create(
					Platforms::GLRenderPassModuleID,
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
	_CreateFramebuffer
=================================================
*/
	bool GL4Device::_CreateFramebuffer ()
	{
		CHECK_ERR( _renderPass );
		CHECK_ERR( not _framebuffer );

		auto fb = New< GL4SystemFramebuffer >( GlobalSystems() );

		fb->Send< ModuleMsg::AttachModule >({ _renderPass });

		CHECK_ERR( fb->CreateFramebuffer( _surfaceSize, _colorPixelFormat, _depthStencilPixelFormat, _samples ) );

		_framebuffer = fb;
		return true;
	}

}	// PlatformGL
}	// Engine

#endif	// GRAPHICS_API_OPENGL
