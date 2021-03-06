// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "Core/Config/Engine.Config.h"

#ifdef GRAPHICS_API_OPENGL

#include "Engine/Platforms/OpenGL/450/GL4BaseModule.h"
#include "Engine/Platforms/OpenGL/450/GL4SamplerCache.h"
#include "Engine/Platforms/OpenGL/OpenGLObjectsConstructor.h"
#include "Engine/Platforms/Public/Tools/SamplerUtils.h"

namespace Engine
{
namespace PlatformGL
{
	
	//
	// OpenGL Sampler
	//

	class GL4Sampler final : public GL4BaseModule
	{
	// types
	private:
		using SupportedMessages_t	= GL4BaseModule::SupportedMessages_t::Append< MessageListFrom<
											GpuMsg::GetSamplerDescription,
											GpuMsg::GetGLSamplerID
										> >;

		using SupportedEvents_t		= GL4BaseModule::SupportedEvents_t;


	// constants
	private:
		static const TypeIdList		_eventTypes;


	// variables
	private:
		SamplerDescription	_descr;

		gl::GLuint			_samplerId;


	// methods
	public:
		GL4Sampler (UntypedID_t, GlobalSystemsRef gs, const CreateInfo::GpuSampler &ci);
		~GL4Sampler ();

		SamplerDescription const&	GetDescription ()	const	{ return _descr; }


	// message handlers
	private:
		bool _Compose (const ModuleMsg::Compose &);
		bool _Delete (const ModuleMsg::Delete &);
		bool _GetGLSamplerID (const GpuMsg::GetGLSamplerID &);
		bool _GetSamplerDescription (const GpuMsg::GetSamplerDescription &);

	private:
		bool _IsCreated () const;
		bool _CreateSampler ();
		void _DestroySampler ();
	};
//-----------------------------------------------------------------------------



	const TypeIdList	GL4Sampler::_eventTypes{ UninitializedT< SupportedEvents_t >() };

/*
=================================================
	constructor
=================================================
*/
	GL4Sampler::GL4Sampler (UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::GpuSampler &ci) :
		GL4BaseModule( gs, ModuleConfig{ id, UMax }, &_eventTypes ),
		_descr( ci.descr ),
		_samplerId( 0 )
	{
		SetDebugName( "GL4Sampler" );

		_SubscribeOnMsg( this, &GL4Sampler::_OnModuleAttached_Impl );
		_SubscribeOnMsg( this, &GL4Sampler::_OnModuleDetached_Impl );
		_SubscribeOnMsg( this, &GL4Sampler::_AttachModule_Impl );
		_SubscribeOnMsg( this, &GL4Sampler::_DetachModule_Impl );
		_SubscribeOnMsg( this, &GL4Sampler::_FindModule_Impl );
		_SubscribeOnMsg( this, &GL4Sampler::_ModulesDeepSearch_Impl );
		_SubscribeOnMsg( this, &GL4Sampler::_Link_Impl );
		_SubscribeOnMsg( this, &GL4Sampler::_Compose );
		_SubscribeOnMsg( this, &GL4Sampler::_Delete );
		_SubscribeOnMsg( this, &GL4Sampler::_OnManagerChanged );
		_SubscribeOnMsg( this, &GL4Sampler::_GetGLSamplerID );
		_SubscribeOnMsg( this, &GL4Sampler::_GetSamplerDescription );
		_SubscribeOnMsg( this, &GL4Sampler::_GetDeviceInfo );
		_SubscribeOnMsg( this, &GL4Sampler::_GetGLDeviceInfo );
		_SubscribeOnMsg( this, &GL4Sampler::_GetGLPrivateClasses );

		ASSERT( _ValidateMsgSubscriptions< SupportedMessages_t >() );

		_AttachSelfToManager( _GetGPUThread( ci.gpuThread ), UntypedID_t(0), true );
	}
	
/*
=================================================
	destructor
=================================================
*/
	GL4Sampler::~GL4Sampler ()
	{
		ASSERT( not _IsCreated() );
	}
	
/*
=================================================
	_Compose
=================================================
*/
	bool GL4Sampler::_Compose (const ModuleMsg::Compose &msg)
	{
		if ( _IsComposedState( GetState() ) )
			return true;	// already linked

		CHECK_ERR( GetState() == EState::Linked );

		CHECK_COMPOSING( _CreateSampler() );

		_SendForEachAttachments( msg );
		
		// very paranoic check
		CHECK( _ValidateAllSubscriptions() );

		CHECK( _SetState( EState::ComposedImmutable ) );
		
		_SendUncheckedEvent( ModuleMsg::AfterCompose{} );
		return true;
	}
	
/*
=================================================
	_Delete
=================================================
*/
	bool GL4Sampler::_Delete (const ModuleMsg::Delete &msg)
	{
		_DestroySampler();

		return Module::_Delete_Impl( msg );
	}
	
/*
=================================================
	_GetGLSamplerID
=================================================
*/
	bool GL4Sampler::_GetGLSamplerID (const GpuMsg::GetGLSamplerID &msg)
	{
		ASSERT( _IsCreated() );

		msg.result.Set( _samplerId );
		return true;
	}

/*
=================================================
	_GetSamplerDescription
=================================================
*/
	bool GL4Sampler::_GetSamplerDescription (const GpuMsg::GetSamplerDescription &msg)
	{
		msg.result.Set( _descr );
		return true;
	}
	
/*
=================================================
	_IsCreated
=================================================
*/
	bool GL4Sampler::_IsCreated () const
	{
		return _samplerId != 0;
	}
	
/*
=================================================
	_CreateSampler
=================================================
*/
	bool GL4Sampler::_CreateSampler ()
	{
		using namespace gl;

		CHECK_ERR( not _IsCreated() );

		// create sampler
		GL_CALL( glGenSamplers( 1, OUT &_samplerId ) );
		CHECK_ERR( _samplerId != 0 );

		// wrap
		GL_CALL( glSamplerParameteri( _samplerId, GL_TEXTURE_WRAP_S, GL4Enum( _descr.AddressMode().x ) ) );
		GL_CALL( glSamplerParameteri( _samplerId, GL_TEXTURE_WRAP_T, GL4Enum( _descr.AddressMode().y ) ) );
		GL_CALL( glSamplerParameteri( _samplerId, GL_TEXTURE_WRAP_R, GL4Enum( _descr.AddressMode().z ) ) );
		
		// border color
		if ( _descr.BorderColor()[ ESamplerBorderColor::Int ] )
		{
			uint4	color;
			CHECK_ERR( GL4Enum( _descr.BorderColor(), OUT color ) );
			GL_CALL( glSamplerParameteriv( _samplerId, GL_TEXTURE_BORDER_COLOR, Cast<GLint const*>(color.ptr()) ) );
		}
		else
		{
			float4	color;
			CHECK_ERR( GL4Enum( _descr.BorderColor(), OUT color ) );
			GL_CALL( glSamplerParameterfv( _samplerId, GL_TEXTURE_BORDER_COLOR, color.ptr() ) );
		}

		// filtering
		GL4MinFilter	min_filter;
		GL4MagFilter	mag_filter;

		CHECK_ERR( GL4Enum( _descr.Filter(), OUT min_filter, OUT mag_filter ) );

		GL_CALL( glSamplerParameteri( _samplerId, GL_TEXTURE_MIN_FILTER, min_filter ) );
		GL_CALL( glSamplerParameteri( _samplerId, GL_TEXTURE_MAG_FILTER, mag_filter ) );

		// anisotropy
		const uint	aniso = EFilter::GetAnisotropic( _descr.Filter() );

		if ( aniso > 0 )
		{
			GL_CALL( glSamplerParameteri( _samplerId, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso ) );
		}

		// compare func
		if ( _descr.CompareOp() != ECompareFunc::None )
		{
			GL_CALL( glSamplerParameteri( _samplerId, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE ) );
			GL_CALL( glSamplerParameteri( _samplerId, GL_TEXTURE_COMPARE_FUNC, GL4Enum( _descr.CompareOp() ) ) );
		}
		else
		{
			GL_CALL( glSamplerParameteri( _samplerId, GL_TEXTURE_COMPARE_MODE, GL_NONE ) );
		}
		
		GetDevice()->SetObjectName( _samplerId, GetDebugName(), EGpuObject::Sampler );
		return true;
	}
	
/*
=================================================
	_DestroySampler
=================================================
*/
	void GL4Sampler::_DestroySampler ()
	{
		using namespace gl;
		
		if ( _samplerId != 0 ) {
			GL_CALL( glDeleteSamplers( 1, &_samplerId ) );
		}

		_samplerId	= 0;
		_descr		= Uninitialized;
	}
//-----------------------------------------------------------------------------
	

	
/*
=================================================
	SearchableSampler
=================================================
*/
	inline bool GL4SamplerCache::SearchableSampler::operator == (const SearchableSampler &right) const	{ return samp->GetDescription() == right.samp->GetDescription(); }
	inline bool GL4SamplerCache::SearchableSampler::operator >  (const SearchableSampler &right) const	{ return samp->GetDescription() >  right.samp->GetDescription(); }
		
//-----------------------------------------------------------------------------


/*
=================================================
	SamplerSearch
=================================================
*/	
	inline bool GL4SamplerCache::SamplerSearch::operator == (const SearchableSampler &right) const	{ return descr == right.samp->GetDescription(); }
	inline bool GL4SamplerCache::SamplerSearch::operator >  (const SearchableSampler &right) const	{ return descr >  right.samp->GetDescription(); }
	
//-----------------------------------------------------------------------------


/*
=================================================
	constructor
=================================================
*/
	GL4SamplerCache::GL4SamplerCache () :
		_maxAnisotropy( 0 ),
		_initialized( false )
	{
		_samplers.Reserve( 16 );
	}
	
/*
=================================================
	_Initialize
=================================================
*/
	void GL4SamplerCache::_Initialize ()
	{
		using namespace gl;

		_initialized = true;

		GL_CALL( glGetIntegerv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, Cast<GLint*>(&_maxAnisotropy) ) );
	}

/*
=================================================
	Create
=================================================
*/
	GL4SamplerCache::GL4SamplerPtr  GL4SamplerCache::Create (ModuleMsg::UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::GpuSampler &ci)
	{
		if ( not _initialized )
		{
			_Initialize();
		}
		
		SamplerDescription	descr = ci.descr;
		PlatformTools::SamplerUtils::ValidateDescription( INOUT descr, GetMaxAnisotropyLevel() );

		// find cached sampler
		Samplers_t::const_iterator	iter;

		if ( _samplers.CustomSearch().Find( SamplerSearch( descr ), OUT iter ) and
			 iter->samp->GetState() == Module::EState::ComposedImmutable )
		{
			return iter->samp;
		}

		// create new sampler
		CreateInfo::GpuSampler	create_info;
		create_info.gpuThread	= ci.gpuThread;
		create_info.descr		= descr;

		auto result = New< GL4Sampler >( id, gs, create_info );

		ModuleUtils::Initialize({ result });

		CHECK_ERR( result->GetState() == Module::EState::ComposedImmutable );

		_samplers.Add( SearchableSampler( result ) );
		return result;
	}

}	// PlatformGL
//-----------------------------------------------------------------------------

namespace Platforms
{
	ModulePtr OpenGLObjectsConstructor::CreateGL4Sampler (ModuleMsg::UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::GpuSampler &ci)
	{
		ModulePtr	mod;
		CHECK_ERR( mod = gs->parallelThread->GetModuleByMsg< ModuleMsg::MessageListFrom< GpuMsg::GetGLPrivateClasses >>() );

		GpuMsg::GetGLPrivateClasses	req_classes;
		mod->Send( req_classes );
		CHECK_ERR( req_classes.result and req_classes.result->samplerCache );

		return req_classes.result->samplerCache->Create( id, gs, ci );
	}

}	// Platforms
}	// Engine

#endif	// GRAPHICS_API_OPENGL
