// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "Core/Config/Engine.Config.h"

#ifdef GRAPHICS_API_VULKAN

#include "Engine/Platforms/Public/GPU/Pipeline.h"
#include "Engine/Platforms/Public/GPU/RenderPass.h"
#include "Engine/Platforms/Public/GPU/VertexInputState.h"
#include "Engine/Platforms/Vulkan/110/Vk1BaseModule.h"
#include "Engine/Platforms/Vulkan/110/Vk1PipelineCache.h"
#include "Engine/Platforms/Vulkan/110/Vk1PipelineLayout.h"
#include "Engine/Platforms/Vulkan/VulkanObjectsConstructor.h"

namespace Engine
{
namespace PlatformVK
{
	
	//
	// Vulkan Graphics Pipeline
	//

	class Vk1GraphicsPipeline final : public Vk1BaseModule
	{
	// types
	private:
		using SupportedMessages_t	= Vk1BaseModule::SupportedMessages_t::Append< MessageListFrom<
											GpuMsg::GetGraphicsPipelineDescription,
											GpuMsg::GetVkGraphicsPipelineID,
											GpuMsg::GetPipelineLayoutDescription,
											GpuMsg::GetVkDescriptionLayouts,
											GpuMsg::GetVkPipelineLayoutID,
											GpuMsg::GetVkPipelineLayoutPushConstants
										> >;

		using SupportedEvents_t		= Vk1BaseModule::SupportedEvents_t;

		using ShadersMsgList_t		= MessageListFrom< GpuMsg::GetVkShaderModuleIDs >;
		using RenderPassMsgList_t	= MessageListFrom< GpuMsg::GetVkRenderPassID, GpuMsg::GetRenderPassDescription >;

		using Description_t			= GraphicsPipelineDescription;
		using LayoutDesc_t			= PipelineLayoutDescription;


	// constants
	private:
		static const TypeIdList		_eventTypes;


	// variables
	private:
		vk::VkPipeline			_pipelineId;
		Description_t			_descr;
		LayoutDesc_t			_layoutDesc;
		Vk1PipelineLayoutPtr	_layout;


	// methods
	public:
		Vk1GraphicsPipeline (UntypedID_t, GlobalSystemsRef gs, const CreateInfo::GraphicsPipeline &ci);
		~Vk1GraphicsPipeline ();


	// message handlers
	private:
		bool _Link (const ModuleMsg::Link &);
		bool _Compose (const ModuleMsg::Compose &);
		bool _Delete (const ModuleMsg::Delete &);
		bool _AttachModule (const ModuleMsg::AttachModule &);
		bool _DetachModule (const ModuleMsg::DetachModule &);

		bool _GetVkGraphicsPipelineID (const GpuMsg::GetVkGraphicsPipelineID &);
		bool _GetGraphicsPipelineDescription (const GpuMsg::GetGraphicsPipelineDescription &);
		bool _GetPipelineLayoutDescription (const GpuMsg::GetPipelineLayoutDescription &);
		bool _GetVkDescriptionLayouts (const GpuMsg::GetVkDescriptionLayouts &);
		bool _GetVkPipelineLayoutID (const GpuMsg::GetVkPipelineLayoutID &);
		bool _GetVkPipelineLayoutPushConstants (const GpuMsg::GetVkPipelineLayoutPushConstants &);

	private:
		bool _IsCreated () const;
		bool _CreatePipeline ();
		void _DestroyPipeline ();
		bool _ValidateRenderPass (const ModulePtr &renderPass) const;
	};
//-----------------------------------------------------------------------------



	const TypeIdList	Vk1GraphicsPipeline::_eventTypes{ UninitializedT< SupportedEvents_t >() };

/*
=================================================
	constructor
=================================================
*/
	Vk1GraphicsPipeline::Vk1GraphicsPipeline (UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::GraphicsPipeline &ci) :
		Vk1BaseModule( gs, ModuleConfig{ id, UMax }, &_eventTypes ),
		_pipelineId{ VK_NULL_HANDLE },	_descr{ ci.descr },
		_layoutDesc{ ci.layout }
	{
		SetDebugName( "Vk1GraphicsPipeline" );

		_SubscribeOnMsg( this, &Vk1GraphicsPipeline::_OnModuleAttached_Impl );
		_SubscribeOnMsg( this, &Vk1GraphicsPipeline::_OnModuleDetached_Impl );
		_SubscribeOnMsg( this, &Vk1GraphicsPipeline::_AttachModule );
		_SubscribeOnMsg( this, &Vk1GraphicsPipeline::_DetachModule );
		_SubscribeOnMsg( this, &Vk1GraphicsPipeline::_FindModule_Impl );
		_SubscribeOnMsg( this, &Vk1GraphicsPipeline::_ModulesDeepSearch_Impl );
		_SubscribeOnMsg( this, &Vk1GraphicsPipeline::_Link );
		_SubscribeOnMsg( this, &Vk1GraphicsPipeline::_Compose );
		_SubscribeOnMsg( this, &Vk1GraphicsPipeline::_Delete );
		_SubscribeOnMsg( this, &Vk1GraphicsPipeline::_OnManagerChanged );
		_SubscribeOnMsg( this, &Vk1GraphicsPipeline::_GetVkGraphicsPipelineID );
		_SubscribeOnMsg( this, &Vk1GraphicsPipeline::_GetGraphicsPipelineDescription );
		_SubscribeOnMsg( this, &Vk1GraphicsPipeline::_GetDeviceInfo );
		_SubscribeOnMsg( this, &Vk1GraphicsPipeline::_GetVkDeviceInfo );
		_SubscribeOnMsg( this, &Vk1GraphicsPipeline::_GetVkPrivateClasses );
		_SubscribeOnMsg( this, &Vk1GraphicsPipeline::_GetPipelineLayoutDescription );
		_SubscribeOnMsg( this, &Vk1GraphicsPipeline::_GetVkDescriptionLayouts );
		_SubscribeOnMsg( this, &Vk1GraphicsPipeline::_GetVkPipelineLayoutID );
		_SubscribeOnMsg( this, &Vk1GraphicsPipeline::_GetVkPipelineLayoutPushConstants );
		
		ASSERT( _ValidateMsgSubscriptions< SupportedMessages_t >() );

		_AttachSelfToManager( _GetGPUThread( ci.gpuThread ), UntypedID_t(0), true );
	}
	
/*
=================================================
	destructor
=================================================
*/
	Vk1GraphicsPipeline::~Vk1GraphicsPipeline ()
	{
		ASSERT( not _IsCreated() );
	}
	
/*
=================================================
	_Link
=================================================
*/
	bool Vk1GraphicsPipeline::_Link (const ModuleMsg::Link &msg)
	{
		if ( _IsComposedOrLinkedState( GetState() ) )
			return true;	// already linked
		
		CHECK_ERR( _GetManager() );

		if ( not GetModuleByMsg< RenderPassMsgList_t >() )
		{
			// validate render pass
			GpuMsg::GetDeviceInfo	req_dev;
			CHECK( _GetManager()->Send( req_dev ) );

			_descr.subpass	= 0;

			CHECK_ERR( _Attach( "", req_dev.result->renderPass ) );

			LOG( "used default render pass", ELog::Debug );
		}

		CHECK_LINKING( GetModuleByMsg< ShadersMsgList_t >() );

		return Module::_Link_Impl( msg );
	}

/*
=================================================
	_Compose
=================================================
*/
	bool Vk1GraphicsPipeline::_Compose (const ModuleMsg::Compose &msg)
	{
		if ( _IsComposedState( GetState() ) )
			return true;	// already composed

		CHECK_ERR( GetState() == EState::Linked );

		CHECK_COMPOSING( _CreatePipeline() );

		_SendForEachAttachments( msg );
		
		// very paranoic check
		CHECK( _ValidateAllSubscriptions() );
		
		CHECK( _SetState( EState::ComposedMutable ) );
		
		_SendUncheckedEvent( ModuleMsg::AfterCompose{} );
		return true;
	}
	
/*
=================================================
	_AttachModule
=================================================
*/
	bool Vk1GraphicsPipeline::_AttachModule (const ModuleMsg::AttachModule &msg)
	{
		CHECK_ERR( msg.newModule );

		// render pass and shader must be unique
		const bool	is_dependent =  msg.newModule->SupportsAllMessages< RenderPassMsgList_t >() or
									msg.newModule->SupportsAllMessages< ShadersMsgList_t >();

		CHECK( _Attach( msg.name, msg.newModule ) );

		if ( is_dependent )
		{
			CHECK( _SetState( EState::Initial ) );
			_DestroyPipeline();
		}
		return true;
	}
	
/*
=================================================
	_DetachModule
=================================================
*/
	bool Vk1GraphicsPipeline::_DetachModule (const ModuleMsg::DetachModule &msg)
	{
		CHECK_ERR( msg.oldModule );
		
		const bool	is_dependent =  msg.oldModule->SupportsAllMessages< RenderPassMsgList_t >() or
									msg.oldModule->SupportsAllMessages< ShadersMsgList_t >();

		if ( _Detach( msg.oldModule ) and is_dependent )
		{
			CHECK( _SetState( EState::Initial ) );
			_DestroyPipeline();
		}
		return true;
	}

/*
=================================================
	_Delete
=================================================
*/
	bool Vk1GraphicsPipeline::_Delete (const ModuleMsg::Delete &msg)
	{
		_DestroyPipeline();
		
		_descr = Uninitialized;

		return Module::_Delete_Impl( msg );
	}
	
/*
=================================================
	_GetVkGraphicsPipelineID
=================================================
*/
	bool Vk1GraphicsPipeline::_GetVkGraphicsPipelineID (const GpuMsg::GetVkGraphicsPipelineID &msg)
	{
		ASSERT( _IsCreated() );

		msg.result.Set( _pipelineId );
		return true;
	}
	
/*
=================================================
	_GetGraphicsPipelineDescription
=================================================
*/
	bool Vk1GraphicsPipeline::_GetGraphicsPipelineDescription (const GpuMsg::GetGraphicsPipelineDescription &msg)
	{
		msg.result.Set( _descr );
		return true;
	}
	
/*
=================================================
	_GetPipelineLayoutDescription
=================================================
*/
	bool Vk1GraphicsPipeline::_GetPipelineLayoutDescription (const GpuMsg::GetPipelineLayoutDescription &msg)
	{
		if ( _layout )
			msg.result.Set( _layout->GetDescription() );
		return true;
	}
	
/*
=================================================
	_GetVkDescriptionLayouts
=================================================
*/
	bool Vk1GraphicsPipeline::_GetVkDescriptionLayouts (const GpuMsg::GetVkDescriptionLayouts &msg)
	{
		if ( _layout )
			msg.result.Set( _layout->GetDescriptionLayouts() );
		return true;
	}
		
/*
=================================================
	_GetVkPipelineLayoutID
=================================================
*/
	bool Vk1GraphicsPipeline::_GetVkPipelineLayoutID (const GpuMsg::GetVkPipelineLayoutID &msg)
	{
		if ( _layout )
			msg.result.Set( _layout->GetLayoutID() );
		return true;
	}
	
/*
=================================================
	_GetVkPipelineLayoutPushConstants
=================================================
*/
	bool Vk1GraphicsPipeline::_GetVkPipelineLayoutPushConstants (const GpuMsg::GetVkPipelineLayoutPushConstants &msg)
	{
		if ( _layout )
			msg.result.Set( _layout->GetPushConstants() );
		return true;
	}

/*
=================================================
	_IsCreated
=================================================
*/
	bool Vk1GraphicsPipeline::_IsCreated () const
	{
		return _pipelineId != VK_NULL_HANDLE;
	}
	
/*
=================================================
	_CreatePipeline
=================================================
*/
	bool Vk1GraphicsPipeline::_CreatePipeline ()
	{
		using namespace vk;

		CHECK_ERR( not _IsCreated() );
		
		// get render pass
		ModulePtr	render_pass;
		CHECK_ERR( render_pass = GetModuleByMsg< RenderPassMsgList_t >() );

		CHECK_ERR( _ValidateRenderPass( render_pass ) );

		
		// get render pass id
		GpuMsg::GetVkRenderPassID			req_pass_id;
		GpuMsg::GetRenderPassDescription	req_pass_descr;

		render_pass->Send( req_pass_id );
		render_pass->Send( req_pass_descr );

		RenderPassDescription const&	rp_descr	= *req_pass_descr.result;
		VkRenderPass					rp			= req_pass_id.result.Get( VK_NULL_HANDLE );
		CHECK_ERR( rp != VK_NULL_HANDLE );
		
		
		// get shader
		ModulePtr	shaders;
		CHECK_ERR( shaders = GetModuleByMsg< ShadersMsgList_t >() );

		// get shader modules
		GpuMsg::GetVkShaderModuleIDs	req_shader_ids;
		shaders->Send( req_shader_ids );
		CHECK_ERR( req_shader_ids.result and not req_shader_ids.result->Empty() );
		
		// create layout
		GpuMsg::GetVkPrivateClasses		req_cache;
		Send( req_cache );

		CHECK_ERR(( _layout = req_cache.result->layoutCache->Create( _layoutDesc ) ));

		// create graphics pipeline
		CHECK_ERR( req_cache.result->pipelineCache->
						CreatePipeline( OUT _pipelineId,
										*req_shader_ids.result,
										_layout->GetLayoutID(),
										_descr.vertexInput,
										_descr.renderState,
										_descr.dynamicStates,
										_descr.patchControlPoints,
										rp_descr,
										rp,
										_descr.subpass )
		);
		
		return true;
	}
	
/*
=================================================
	_DestroyPipeline
=================================================
*/
	void Vk1GraphicsPipeline::_DestroyPipeline ()
	{
		using namespace vk;

		auto	dev = GetVkDevice();

		if ( dev != VK_NULL_HANDLE and _pipelineId != VK_NULL_HANDLE )
		{
			vkDestroyPipeline( dev, _pipelineId, null );
		}

		_pipelineId	= VK_NULL_HANDLE;
	}
	
/*
=================================================
	_ValidateRenderPass
=================================================
*/
	bool Vk1GraphicsPipeline::_ValidateRenderPass (const ModulePtr &renderPass) const
	{
		GpuMsg::GetRenderPassDescription		req_descr;
		renderPass->Send( req_descr );

		const auto&	descr = *req_descr.result;

		CHECK_ERR( _descr.subpass < descr.Subpasses().Count() );
		
		const auto&	subpass	= descr.Subpasses()[ _descr.subpass ];

		CHECK_ERR( _descr.fragOutput.Count() == subpass.colors.Count() );

		// can't make mistake when number of attachment is 1
		if ( _descr.fragOutput.Count() <= 1 )
			return true;

		FOR( i, subpass.colors )
		{
			auto	frag = _descr.fragOutput( subpass.colors[i].name );
			CHECK_ERR( frag and frag->index == i );
		}

		return true;
	}
//-----------------------------------------------------------------------------


	
	//
	// Vulkan Compute Pipeline
	//
	
	class Vk1ComputePipeline final : public Vk1BaseModule
	{
	// types
	private:
		using SupportedMessages_t	= Vk1BaseModule::SupportedMessages_t::Append< MessageListFrom<
											GpuMsg::GetComputePipelineDescription,
											GpuMsg::GetVkComputePipelineID,
											GpuMsg::GetPipelineLayoutDescription,
											GpuMsg::GetVkDescriptionLayouts,
											GpuMsg::GetVkPipelineLayoutID,
											GpuMsg::GetVkPipelineLayoutPushConstants
										> >;

		using SupportedEvents_t		= Vk1BaseModule::SupportedEvents_t;
		
		using ShadersMsgList_t		= MessageListFrom< GpuMsg::GetVkShaderModuleIDs >;

		using Description_t			= ComputePipelineDescription;
		using LayoutDesc_t			= PipelineLayoutDescription;


	// constants
	private:
		static const TypeIdList		_eventTypes;


	// variables
	private:
		vk::VkPipeline			_pipelineId;
		Description_t			_descr;
		LayoutDesc_t			_layoutDesc;
		Vk1PipelineLayoutPtr	_layout;


	// methods
	public:
		Vk1ComputePipeline (UntypedID_t, GlobalSystemsRef gs, const CreateInfo::ComputePipeline &ci);
		~Vk1ComputePipeline ();


	// message handlers
	private:
		bool _Link (const ModuleMsg::Link &);
		bool _Compose (const ModuleMsg::Compose &);
		bool _Delete (const ModuleMsg::Delete &);
		bool _AttachModule (const ModuleMsg::AttachModule &);
		bool _DetachModule (const ModuleMsg::DetachModule &);

		bool _GetVkComputePipelineID (const GpuMsg::GetVkComputePipelineID &);
		bool _GetComputePipelineDescription (const GpuMsg::GetComputePipelineDescription &);
		bool _GetPipelineLayoutDescription (const GpuMsg::GetPipelineLayoutDescription &);
		bool _GetVkDescriptionLayouts (const GpuMsg::GetVkDescriptionLayouts &);
		bool _GetVkPipelineLayoutID (const GpuMsg::GetVkPipelineLayoutID &);
		bool _GetVkPipelineLayoutPushConstants (const GpuMsg::GetVkPipelineLayoutPushConstants &);

	private:
		bool _IsCreated () const;
		bool _CreatePipeline ();
		void _DestroyPipeline ();
	};
//-----------------------------------------------------------------------------



	const TypeIdList	Vk1ComputePipeline::_eventTypes{ UninitializedT< SupportedEvents_t >() };

/*
=================================================
	constructor
=================================================
*/
	Vk1ComputePipeline::Vk1ComputePipeline (UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::ComputePipeline &ci) :
		Vk1BaseModule( gs, ModuleConfig{ id, UMax }, &_eventTypes ),
		_pipelineId{ VK_NULL_HANDLE },		_descr{ ci.descr },
		_layoutDesc{ ci.layout }
	{
		SetDebugName( "Vk1ComputePipeline" );

		_SubscribeOnMsg( this, &Vk1ComputePipeline::_OnModuleAttached_Impl );
		_SubscribeOnMsg( this, &Vk1ComputePipeline::_OnModuleDetached_Impl );
		_SubscribeOnMsg( this, &Vk1ComputePipeline::_AttachModule );
		_SubscribeOnMsg( this, &Vk1ComputePipeline::_DetachModule );
		_SubscribeOnMsg( this, &Vk1ComputePipeline::_FindModule_Impl );
		_SubscribeOnMsg( this, &Vk1ComputePipeline::_ModulesDeepSearch_Impl );
		_SubscribeOnMsg( this, &Vk1ComputePipeline::_Link );
		_SubscribeOnMsg( this, &Vk1ComputePipeline::_Compose );
		_SubscribeOnMsg( this, &Vk1ComputePipeline::_Delete );
		_SubscribeOnMsg( this, &Vk1ComputePipeline::_OnManagerChanged );
		_SubscribeOnMsg( this, &Vk1ComputePipeline::_GetVkComputePipelineID );
		_SubscribeOnMsg( this, &Vk1ComputePipeline::_GetComputePipelineDescription );
		_SubscribeOnMsg( this, &Vk1ComputePipeline::_GetDeviceInfo );
		_SubscribeOnMsg( this, &Vk1ComputePipeline::_GetVkDeviceInfo );
		_SubscribeOnMsg( this, &Vk1ComputePipeline::_GetVkPrivateClasses );
		_SubscribeOnMsg( this, &Vk1ComputePipeline::_GetPipelineLayoutDescription );
		_SubscribeOnMsg( this, &Vk1ComputePipeline::_GetVkDescriptionLayouts );
		_SubscribeOnMsg( this, &Vk1ComputePipeline::_GetVkPipelineLayoutID );
		_SubscribeOnMsg( this, &Vk1ComputePipeline::_GetVkPipelineLayoutPushConstants );
		
		ASSERT( _ValidateMsgSubscriptions< SupportedMessages_t >() );

		_AttachSelfToManager( _GetGPUThread( ci.gpuThread ), UntypedID_t(0), true );
	}
	
/*
=================================================
	destructor
=================================================
*/
	Vk1ComputePipeline::~Vk1ComputePipeline ()
	{
		ASSERT( not _IsCreated() );
	}
	
/*
=================================================
	_Link
=================================================
*/
	bool Vk1ComputePipeline::_Link (const ModuleMsg::Link &msg)
	{
		if ( _IsComposedOrLinkedState( GetState() ) )
			return true;	// already linked

		CHECK_LINKING( GetModuleByMsg< ShadersMsgList_t >() );

		return Module::_Link_Impl( msg );
	}
	
/*
=================================================
	_Compose
=================================================
*/
	bool Vk1ComputePipeline::_Compose (const ModuleMsg::Compose &msg)
	{
		if ( _IsComposedState( GetState() ) )
			return true;	// already composed

		CHECK_ERR( GetState() == EState::Linked );

		CHECK_COMPOSING( _CreatePipeline() );

		_SendForEachAttachments( msg );
		
		// very paranoic check
		CHECK( _ValidateAllSubscriptions() );
		
		CHECK( _SetState( EState::ComposedMutable ) );
		
		_SendUncheckedEvent( ModuleMsg::AfterCompose{} );
		return true;
	}
	
/*
=================================================
	_AttachModule
=================================================
*/
	bool Vk1ComputePipeline::_AttachModule (const ModuleMsg::AttachModule &msg)
	{
		CHECK_ERR( msg.newModule );

		// render pass and shader must be unique
		const bool	is_dependent = msg.newModule->SupportsAllMessages< ShadersMsgList_t >();

		CHECK( _Attach( msg.name, msg.newModule ) );

		if ( is_dependent )
		{
			CHECK( _SetState( EState::Initial ) );
			_DestroyPipeline();
		}
		return true;
	}
	
/*
=================================================
	_DetachModule
=================================================
*/
	bool Vk1ComputePipeline::_DetachModule (const ModuleMsg::DetachModule &msg)
	{
		CHECK_ERR( msg.oldModule );
		
		const bool	is_dependent = msg.oldModule->SupportsAllMessages< ShadersMsgList_t >();

		if ( _Detach( msg.oldModule ) and is_dependent )
		{
			CHECK( _SetState( EState::Initial ) );
			_DestroyPipeline();
		}
		return true;
	}

/*
=================================================
	_Delete
=================================================
*/
	bool Vk1ComputePipeline::_Delete (const ModuleMsg::Delete &msg)
	{
		_DestroyPipeline();
		
		_descr = Uninitialized;

		return Module::_Delete_Impl( msg );
	}
	
/*
=================================================
	_GetVkComputePipelineID
=================================================
*/
	bool Vk1ComputePipeline::_GetVkComputePipelineID (const GpuMsg::GetVkComputePipelineID &msg)
	{
		ASSERT( _IsCreated() );

		msg.result.Set( _pipelineId );
		return true;
	}
	
/*
=================================================
	_GetComputePipelineDescription
=================================================
*/
	bool Vk1ComputePipeline::_GetComputePipelineDescription (const GpuMsg::GetComputePipelineDescription &msg)
	{
		msg.result.Set( _descr );
		return true;
	}
	
/*
=================================================
	_GetPipelineLayoutDescription
=================================================
*/
	bool Vk1ComputePipeline::_GetPipelineLayoutDescription (const GpuMsg::GetPipelineLayoutDescription &msg)
	{
		if ( _layout )
			msg.result.Set( _layout->GetDescription() );
		return true;
	}
	
/*
=================================================
	_GetVkDescriptionLayouts
=================================================
*/
	bool Vk1ComputePipeline::_GetVkDescriptionLayouts (const GpuMsg::GetVkDescriptionLayouts &msg)
	{
		if ( _layout )
			msg.result.Set( _layout->GetDescriptionLayouts() );
		return true;
	}
		
/*
=================================================
	_GetVkPipelineLayoutID
=================================================
*/
	bool Vk1ComputePipeline::_GetVkPipelineLayoutID (const GpuMsg::GetVkPipelineLayoutID &msg)
	{
		if ( _layout )
			msg.result.Set( _layout->GetLayoutID() );
		return true;
	}
	
/*
=================================================
	_GetVkPipelineLayoutPushConstants
=================================================
*/
	bool Vk1ComputePipeline::_GetVkPipelineLayoutPushConstants (const GpuMsg::GetVkPipelineLayoutPushConstants &msg)
	{
		if ( _layout )
			msg.result.Set( _layout->GetPushConstants() );
		return true;
	}

/*
=================================================
	_IsCreated
=================================================
*/
	bool Vk1ComputePipeline::_IsCreated () const
	{
		return _pipelineId != VK_NULL_HANDLE;
	}
	
/*
=================================================
	_CreatePipeline
=================================================
*/
	bool Vk1ComputePipeline::_CreatePipeline ()
	{
		using namespace vk;

		CHECK_ERR( not _IsCreated() );

		// get shader
		ModulePtr	shaders;
		CHECK_ERR( shaders = GetModuleByMsg< ShadersMsgList_t >() );

		// get shader modules
		GpuMsg::GetVkShaderModuleIDs	req_shader_ids;
		shaders->Send( req_shader_ids );
		CHECK_ERR( req_shader_ids.result and not req_shader_ids.result->Empty() );

		usize	cs_index = UMax;

		FOR( i, *req_shader_ids.result ) {
			if ( (*req_shader_ids.result)[i].type == EShader::Compute ) {
				cs_index = i;
				break;
			}
		}
		CHECK_ERR( cs_index < req_shader_ids.result->Count() );	// compute shader not found

		// create layout
		GpuMsg::GetVkPrivateClasses		req_cache;
		Send( req_cache );

		CHECK_ERR(( _layout = req_cache.result->layoutCache->Create( _layoutDesc ) ));

		// create compute pipeline
		CHECK_ERR( req_cache.result->pipelineCache->
						CreatePipeline( OUT _pipelineId,
										(*req_shader_ids.result)[cs_index],
										_layout->GetLayoutID() )
		);
		
		return true;
	}
	
/*
=================================================
	_DestroyPipeline
=================================================
*/
	void Vk1ComputePipeline::_DestroyPipeline ()
	{
		using namespace vk;

		auto	dev = GetVkDevice();

		if ( dev != VK_NULL_HANDLE and _pipelineId != VK_NULL_HANDLE )
		{
			vkDestroyPipeline( dev, _pipelineId, null );
		}

		_pipelineId	= VK_NULL_HANDLE;
	}
//-----------------------------------------------------------------------------

}	// PlatformVK

namespace Platforms
{

	ModulePtr VulkanObjectsConstructor::CreateVk1GraphicsPipeline (ModuleMsg::UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::GraphicsPipeline &ci)
	{
		return New< PlatformVK::Vk1GraphicsPipeline >( id, gs, ci );
	}
	
	ModulePtr VulkanObjectsConstructor::CreateVk1ComputePipeline (ModuleMsg::UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::ComputePipeline &ci)
	{
		return New< PlatformVK::Vk1ComputePipeline >( id, gs, ci );
	}

	ModulePtr VulkanObjectsConstructor::CreateCachedVk1GraphicsPipeline (ModuleMsg::UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::GraphicsPipeline &ci)
	{
		ModulePtr	mod;
		CHECK_ERR( mod = gs->parallelThread->GetModuleByMsg< ModuleMsg::MessageListFrom< GpuMsg::GetVkPrivateClasses >>() );

		GpuMsg::GetVkPrivateClasses		req_classes;
		mod->Send( req_classes );
		CHECK_ERR( req_classes.result and req_classes.result->pipelineCache );

		return req_classes.result->pipelineCache->Create( id, gs, ci );
	}
		
	ModulePtr VulkanObjectsConstructor::CreateCachedVk1ComputePipeline (ModuleMsg::UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::ComputePipeline &ci)
	{
		ModulePtr	mod;
		CHECK_ERR( mod = gs->parallelThread->GetModuleByMsg< ModuleMsg::MessageListFrom< GpuMsg::GetVkPrivateClasses >>() );

		GpuMsg::GetVkPrivateClasses		req_classes;
		mod->Send( req_classes );
		CHECK_ERR( req_classes.result and req_classes.result->pipelineCache );

		return req_classes.result->pipelineCache->Create( id, gs, ci );
	}

}	// Platforms
}	// Engine

#endif	// GRAPHICS_API_VULKAN
