// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "Engine/Graphics/Impl/GraphicsBaseModule.h"
#include "Engine/Graphics/Public/Commands.h"
#include "Engine/Graphics/Impl/GraphicsObjectsConstructor.h"

namespace Engine
{
namespace Graphics
{
	using namespace Engine::Platforms;


	//
	// Command Buffers Manager
	//

	class CommandBufferManager final : public GraphicsBaseModule
	{
	// types
	private:
		using CmdBufferMsgList_t	= MessageListFrom<
											GpuMsg::CmdBegin,
											GpuMsg::CmdEnd,
											GpuMsg::CmdSetViewport,
											GpuMsg::CmdSetScissor,
											GpuMsg::CmdSetDepthBounds,
											GpuMsg::CmdSetBlendColor,
											GpuMsg::CmdSetDepthBias,
											GpuMsg::CmdSetLineWidth,
											GpuMsg::CmdSetStencilCompareMask,
											GpuMsg::CmdSetStencilWriteMask,
											GpuMsg::CmdSetStencilReference,
											GpuMsg::CmdBeginRenderPass,
											GpuMsg::CmdEndRenderPass,
											GpuMsg::CmdNextSubpass,
											GpuMsg::CmdBindGraphicsPipeline,
											GpuMsg::CmdBindComputePipeline,
											GpuMsg::CmdBindVertexBuffers,
											GpuMsg::CmdBindIndexBuffer,
											GpuMsg::CmdDraw,
											GpuMsg::CmdDrawIndexed,
											GpuMsg::CmdDrawIndirect,
											GpuMsg::CmdDrawIndexedIndirect,
											GpuMsg::CmdDispatch,
											GpuMsg::CmdDispatchIndirect,
											GpuMsg::CmdExecute,
											GpuMsg::CmdBindGraphicsResourceTable,
											GpuMsg::CmdBindComputeResourceTable,
											GpuMsg::CmdCopyBuffer,
											GpuMsg::CmdCopyImage,
											GpuMsg::CmdCopyBufferToImage,
											GpuMsg::CmdCopyImageToBuffer,
											GpuMsg::CmdBlitImage,
											GpuMsg::CmdUpdateBuffer,
											GpuMsg::CmdFillBuffer,
											GpuMsg::CmdClearAttachments,
											GpuMsg::CmdClearColorImage,
											GpuMsg::CmdClearDepthStencilImage,
											GpuMsg::CmdPipelineBarrier,
											GpuMsg::SetCommandBufferDependency,
											GpuMsg::GetCommandBufferState
										>;
		
		using SupportedMessages_t	= GraphicsBaseModule::SupportedMessages_t::Append< MessageListFrom<
											GraphicsMsg::CmdBegin,
											GraphicsMsg::CmdEnd,
											GraphicsMsg::CmdBeginFrame,
											GraphicsMsg::CmdEndFrame,
											GraphicsMsg::CmdBeginVRFrame,
											GraphicsMsg::CmdEndVRFrame,
											GraphicsMsg::CmdAppend,
											GraphicsMsg::CmdGetCurrentState,
											GraphicsMsg::CmdAddFrameDependency,
											GraphicsMsg::SubscribeOnFrameCompleted
										> >
										::Append< CmdBufferMsgList_t >;

		using SupportedEvents_t		= GraphicsBaseModule::SupportedEvents_t::Append< MessageListFrom<
											GraphicsMsg::OnCmdBeginFrame,
											GraphicsMsg::OnCmdEndFrame
										> >;

		using VRThreadMsgList_t		= MessageListFrom<
											GpuMsg::ThreadBeginVRFrame,
											GpuMsg::ThreadEndVRFrame
										>;

		using CmdBuffers_t			= FixedSizeArray< ModulePtr, 32 >;

		using Semaphores_t			= FixedSizeArray< GpuSemaphoreId, 16 >;
		using WaitSemaphores_t		= FixedSizeArray<Pair< GpuSemaphoreId, EPipelineStage::bits >, 16 >;
		using Fences_t				= FixedSizeArray< GpuFenceId, 8 >;

		using Callback_t			= GraphicsMsg::SubscribeOnFrameCompleted::Callback_t;
		using Callbacks_t			= Array< Callback_t >;

		struct PerFrame : CompileTime::FastCopyable
		{
			GpuFenceId			fence		= Uninitialized;	// wait before reuse command buffers
			CmdBuffers_t		commands;						// used command buffers
			CmdBuffers_t		freeBuffers;					// available command buffers
			CmdBuffers_t		externalBuffers;				// command buffers that added by 'CmdAppend' command
			Fences_t			waitFences;						// wait before begin frame (in client side)
			WaitSemaphores_t	waitSemaphores;					// wait before begin executing (in gpu side)
			Semaphores_t		signalSemaphores;				// signal after executing (in gpu side)
			Callbacks_t			callbacks;
		};

		using PerFrameArray_t		= Array< PerFrame >;

		using EScope				= GraphicsMsg::CmdGetCurrentState::EScope;


	// constants
	private:
		static const TypeIdList		_msgTypes;
		static const TypeIdList		_eventTypes;


	// variables
	private:
		ModulePtr			_syncManager;
		ModulePtr			_builder;
		UntypedID_t			_cmdBufferId;

		Array<ModulePtr>	_tempBuffers;
		PerFrameArray_t		_perFrame;
		const uint			_bufferChainLength;
		uint				_bufferIndex;

		EScope				_scope;
		ModulePtr			_framebuffer;
		uint				_frameIndex;
		
		bool				_isVRCompatible;
		bool				_isVRFrame;


	// methods
	public:
		CommandBufferManager (GlobalSystemsRef gs, const CreateInfo::CommandBufferManager &ci);
		~CommandBufferManager ();


	// message handlers
	private:
		bool _Link (const Message< ModuleMsg::Link > &);
		bool _Compose (const Message< ModuleMsg::Compose > &);
		bool _Delete (const Message< ModuleMsg::Delete > &);
		bool _AttachModule (const Message< ModuleMsg::AttachModule > &);
		bool _DetachModule (const Message< ModuleMsg::DetachModule > &);

		bool _CmdBegin (const Message< GraphicsMsg::CmdBegin > &);
		bool _CmdEnd (const Message< GraphicsMsg::CmdEnd > &);
		bool _CmdBeginFrame (const Message< GraphicsMsg::CmdBeginFrame > &);
		bool _CmdEndFrame (const Message< GraphicsMsg::CmdEndFrame > &);
		bool _CmdBeginVRFrame (const Message< GraphicsMsg::CmdBeginVRFrame > &);
		bool _CmdEndVRFrame (const Message< GraphicsMsg::CmdEndVRFrame > &);
		bool _CmdAppend (const Message< GraphicsMsg::CmdAppend > &);
		bool _CmdGetCurrentState (const Message< GraphicsMsg::CmdGetCurrentState > &);
		bool _CmdAddFrameDependency (const Message< GraphicsMsg::CmdAddFrameDependency > &);
		bool _SubscribeOnFrameCompleted (const Message< GraphicsMsg::SubscribeOnFrameCompleted > &);

		// only to change scope
		bool _Scope_CmdBegin (const Message< GpuMsg::CmdBegin > &);
		bool _Scope_CmdEnd (const Message< GpuMsg::CmdEnd > &);
		bool _Scope_CmdBeginRenderPass (const Message< GpuMsg::CmdBeginRenderPass > &);
		bool _Scope_CmdEndRenderPass (const Message< GpuMsg::CmdEndRenderPass > &);

		// TODO: delete command buffers when swapchain recreated

	private:
		bool _BeginFrame ();
	};
//-----------------------------------------------------------------------------


	const TypeIdList	CommandBufferManager::_msgTypes{ UninitializedT< SupportedMessages_t >() };
	const TypeIdList	CommandBufferManager::_eventTypes{ UninitializedT< SupportedEvents_t >() };

/*
=================================================
	constructor
=================================================
*/
	CommandBufferManager::CommandBufferManager (GlobalSystemsRef gs, const CreateInfo::CommandBufferManager &ci) :
		GraphicsBaseModule( gs, ModuleConfig{ CommandBufferManagerModuleID, UMax }, &_msgTypes, &_eventTypes ),
		_bufferChainLength{ Max( 2u, ci.bufferChainLength ) },		_bufferIndex{ 0 },
		_scope{ EScope::None },										_frameIndex{ 0 },
		_isVRCompatible{ false },									_isVRFrame{ false }
	{
		SetDebugName( "CommandBufferManager" );

		_SubscribeOnMsg( this, &CommandBufferManager::_OnModuleAttached_Impl );
		_SubscribeOnMsg( this, &CommandBufferManager::_OnModuleDetached_Impl );
		_SubscribeOnMsg( this, &CommandBufferManager::_AttachModule );
		_SubscribeOnMsg( this, &CommandBufferManager::_DetachModule );
		_SubscribeOnMsg( this, &CommandBufferManager::_FindModule_Impl );
		_SubscribeOnMsg( this, &CommandBufferManager::_ModulesDeepSearch_Impl );
		_SubscribeOnMsg( this, &CommandBufferManager::_Link );
		_SubscribeOnMsg( this, &CommandBufferManager::_Compose );
		_SubscribeOnMsg( this, &CommandBufferManager::_Delete );
		_SubscribeOnMsg( this, &CommandBufferManager::_OnManagerChanged );
		_SubscribeOnMsg( this, &CommandBufferManager::_CmdBegin );
		_SubscribeOnMsg( this, &CommandBufferManager::_CmdEnd );
		_SubscribeOnMsg( this, &CommandBufferManager::_CmdBeginFrame );
		_SubscribeOnMsg( this, &CommandBufferManager::_CmdEndFrame );
		_SubscribeOnMsg( this, &CommandBufferManager::_CmdBeginVRFrame );
		_SubscribeOnMsg( this, &CommandBufferManager::_CmdEndVRFrame );
		_SubscribeOnMsg( this, &CommandBufferManager::_CmdAppend );
		_SubscribeOnMsg( this, &CommandBufferManager::_CmdGetCurrentState );
		_SubscribeOnMsg( this, &CommandBufferManager::_CmdAddFrameDependency );
		_SubscribeOnMsg( this, &CommandBufferManager::_SubscribeOnFrameCompleted );
		_SubscribeOnMsg( this, &CommandBufferManager::_Scope_CmdBegin );
		_SubscribeOnMsg( this, &CommandBufferManager::_Scope_CmdEnd );
		_SubscribeOnMsg( this, &CommandBufferManager::_Scope_CmdBeginRenderPass );
		_SubscribeOnMsg( this, &CommandBufferManager::_Scope_CmdEndRenderPass );

		_AttachSelfToManager( _GetGpuThread( ci.gpuThread ), UntypedID_t(0), true );

		_perFrame.Resize( _bufferChainLength );
	}
		
/*
=================================================
	desctructor
=================================================
*/
	CommandBufferManager::~CommandBufferManager ()
	{
	}
	
/*
=================================================
	_Link
=================================================
*/
	bool CommandBufferManager::_Link (const Message< ModuleMsg::Link > &msg)
	{
		if ( _IsComposedOrLinkedState( GetState() ) )
			return true;	// already linked

		CHECK_ERR( GetState() == EState::Initial or GetState() == EState::LinkingFailed );
		CHECK_ERR( _GetManager() /*and _IsComposedState( _GetManager()->GetState() )*/ );


		Message< GpuMsg::GetGraphicsModules >	req_ids;
		CHECK( _GetManager()->Send( req_ids ) );

		_cmdBufferId	= req_ids->graphics->commandBuffer;
		_isVRCompatible	= _GetManager()->GetSupportedMessages().HasAllTypes< VRThreadMsgList_t >();


		// create builder
		_builder = GetModuleByMsg< CmdBufferMsgList_t >();

		if ( not _builder )
		{
			CHECK_ERR( GlobalSystems()->modulesFactory->Create(
										req_ids->graphics->commandBuilder,
										GlobalSystems(),
										CreateInfo::GpuCommandBuilder{ _GetManager() },
										OUT _builder ) );

			CHECK_ERR( _Attach( "builder", _builder, true ) );
			_builder->Send( msg );
		}

		CHECK_ERR( _CopySubscriptions< CmdBufferMsgList_t >( _builder ) );

		return Module::_Link_Impl( msg );
	}

/*
=================================================
	_Compose
=================================================
*/
	bool CommandBufferManager::_Compose (const Message< ModuleMsg::Compose > &msg)
	{
		if ( _IsComposedState( GetState() ) )
			return true;	// already composed

		CHECK_ERR( GetState() == EState::Linked );
		
		Message< GpuMsg::GetDeviceInfo >	req_dev;
		_GetManager()->Send( req_dev );
		CHECK_COMPOSING( _syncManager = req_dev->result->syncManager );

		FOR( i, _perFrame )
		{
			Message< GpuMsg::CreateFence >	req_fence;
			CHECK( _syncManager->Send( req_fence ) );

			_perFrame[i].fence = *req_fence->result;
		}
		
		_SendForEachAttachments( msg );
		
		// very paranoic check
		CHECK( _ValidateAllSubscriptions() );

		CHECK( _SetState( EState::ComposedMutable ) );
		return true;
	}
	
/*
=================================================
	_Delete
=================================================
*/
	bool CommandBufferManager::_Delete (const Message< ModuleMsg::Delete > &msg)
	{
		_SendForEachAttachments( msg );

		FOR( i, _perFrame )
		{
			auto&	cmds	= _perFrame[i].commands;
			auto	fence	= _perFrame[i].fence;

			if ( fence != GpuFenceId::Unknown )
			{
				CHECK( _syncManager->Send< GpuMsg::ClientWaitFence >({ fence }) );
				CHECK( _syncManager->Send< GpuMsg::DestroyFence >({ fence }) );
			}

			FOR( j, cmds ) {
				cmds[j]->Send( msg );
			}
		}

		_perFrame.Clear();

		_syncManager = null;

		return Module::_Delete_Impl( msg );
	}
	
/*
=================================================
	_AttachModule
=================================================
*/
	bool CommandBufferManager::_AttachModule (const Message< ModuleMsg::AttachModule > &msg)
	{
		const bool	is_builder	= msg->newModule->GetSupportedEvents().HasAllTypes< CmdBufferMsgList_t >();

		CHECK( _Attach( msg->name, msg->newModule, is_builder ) );

		if ( is_builder )
		{
			CHECK( _SetState( EState::Initial ) );
			_builder = null;
		}
		return true;
	}
	
/*
=================================================
	_DetachModule
=================================================
*/
	bool CommandBufferManager::_DetachModule (const Message< ModuleMsg::DetachModule > &msg)
	{
		CHECK( _Detach( msg->oldModule ) );

		if ( msg->oldModule->GetSupportedEvents().HasAllTypes< CmdBufferMsgList_t >() )
		{
			CHECK( _SetState( EState::Initial ) );
			_builder = null;
		}
		return true;
	}

/*
=================================================
	_BeginFrame
=================================================
*/
	bool CommandBufferManager::_BeginFrame ()
	{
		_bufferIndex = (_bufferIndex + 1) % _bufferChainLength;

		// wait until executing will be completed
		{
			auto&	per_frame	= _perFrame[ _bufferIndex ];
			per_frame.waitFences.PushFront( per_frame.fence );

			CHECK( _syncManager->Send< GpuMsg::ClientWaitFence >({ per_frame.waitFences }) );

			Message< GpuMsg::SetCommandBufferState >	completed_state{ GpuMsg::SetCommandBufferState::EState::Completed };

			ModuleUtils::Send( per_frame.commands, completed_state );
			ModuleUtils::Send( per_frame.externalBuffers, completed_state );
			ModuleUtils::Send( per_frame.externalBuffers, Message<ModuleMsg::Delete>{} );
		
			FOR( i, per_frame.callbacks ) {
				per_frame.callbacks[i]( _bufferIndex );
			}

			per_frame.freeBuffers.Append( per_frame.commands );

			per_frame.waitFences.Clear();
			per_frame.commands.Clear();
			per_frame.callbacks.Clear();
			per_frame.externalBuffers.Clear();
		}
		return true;
	}
	
/*
=================================================
	_CmdBeginFrame
=================================================
*/
	bool CommandBufferManager::_CmdBeginFrame (const Message< GraphicsMsg::CmdBeginFrame > &msg)
	{
		CHECK_ERR( _IsComposedState( GetState() ) );
		CHECK_ERR( _scope == EScope::None );
		
		CHECK_ERR( _BeginFrame() );

		// begin frame
		{
			Message< GpuMsg::ThreadBeginFrame >		begin;
			CHECK( _GetManager()->Send( begin ) );

			msg->result.Set({ begin->result->framebuffer, begin->result->frameIndex, _bufferIndex });

			_framebuffer = begin->result->framebuffer;
			_frameIndex	 = begin->result->frameIndex;
			_scope		 = EScope::Frame;

			_SendEvent< GraphicsMsg::OnCmdBeginFrame >({ _bufferIndex });
		}
		return true;
	}
	
/*
=================================================
	_CmdEndFrame
=================================================
*/
	bool CommandBufferManager::_CmdEndFrame (const Message< GraphicsMsg::CmdEndFrame > &msg)
	{
		CHECK_ERR( _scope == EScope::Frame and not _isVRFrame );
		CHECK_ERR( msg->framebuffer == null or msg->framebuffer == _framebuffer );
		
		auto&	per_frame = _perFrame[ _bufferIndex ];

		_tempBuffers.Append( per_frame.commands );
		_tempBuffers.Append( per_frame.externalBuffers );

		Message< GpuMsg::ThreadEndFrame >	end;
		end->fence			= per_frame.fence;
		end->commands		= _tempBuffers;
		end->framebuffer	= _framebuffer;
		end->waitSemaphores.Append( per_frame.waitSemaphores );
		end->signalSemaphores.Append( per_frame.signalSemaphores );

		CHECK( _GetManager()->Send( end ) );

		per_frame.waitSemaphores.Clear();
		_tempBuffers.Clear();

		_framebuffer = null;
		_frameIndex	 = UMax;
		_scope		 = EScope::None;
		
		_SendEvent< GraphicsMsg::OnCmdEndFrame >({ _bufferIndex });
		return true;
	}
	
/*
=================================================
	_CmdBeginVRFrame
=================================================
*/
	bool CommandBufferManager::_CmdBeginVRFrame (const Message< GraphicsMsg::CmdBeginVRFrame > &msg)
	{
		CHECK_ERR( _IsComposedState( GetState() ) );
		CHECK_ERR( _isVRCompatible );
		CHECK_ERR( _scope == EScope::None );
		
		CHECK_ERR( _BeginFrame() );

		// begin frame
		{
			Message< GpuMsg::ThreadBeginVRFrame >		begin;
			CHECK( _GetManager()->Send( begin ) );

			msg->result.Set({ *begin->result, _bufferIndex });

			//_framebuffer = begin->result->framebuffer;
			_frameIndex	 = begin->result->frameIindex;
			_scope		 = EScope::Frame;
			_isVRFrame	 = true;

			_SendEvent< GraphicsMsg::OnCmdBeginFrame >({ _bufferIndex });
		}
		return true;
	}
	
/*
=================================================
	_CmdEndVRFrame
=================================================
*/
	bool CommandBufferManager::_CmdEndVRFrame (const Message< GraphicsMsg::CmdEndVRFrame > &msg)
	{
		CHECK_ERR( _scope == EScope::Frame and _isVRFrame );
		
		auto&	per_frame = _perFrame[ _bufferIndex ];

		_tempBuffers.Append( per_frame.commands );
		_tempBuffers.Append( per_frame.externalBuffers );

		Message< GpuMsg::ThreadEndVRFrame >		end;
		end->fence		= per_frame.fence;
		end->commands	= _tempBuffers;
		end->waitSemaphores.Append( per_frame.waitSemaphores );
		end->signalSemaphores.Append( per_frame.signalSemaphores );
		
		CHECK( _GetManager()->Send( end ) );

		per_frame.waitSemaphores.Clear();
		_tempBuffers.Clear();

		_framebuffer = null;
		_frameIndex	 = UMax;
		_scope		 = EScope::None;
		_isVRFrame	 = false;

		_SendEvent< GraphicsMsg::OnCmdEndFrame >({ _bufferIndex });
		return true;
	}

/*
=================================================
	_CmdAppend
=================================================
*/
	bool CommandBufferManager::_CmdAppend (const Message< GraphicsMsg::CmdAppend > &msg)
	{
		CHECK_ERR( _scope == EScope::Frame );

		_perFrame[ _bufferIndex ].externalBuffers.Append( msg->commands );
		return true;
	}

/*
=================================================
	_CmdGetCurrentState
=================================================
*/
	bool CommandBufferManager::_CmdGetCurrentState (const Message< GraphicsMsg::CmdGetCurrentState > &msg)
	{
		msg->result.Set({ _framebuffer, _frameIndex, _bufferIndex, _scope });
		return true;
	}
	
/*
=================================================
	_CmdAddFrameDependency
=================================================
*/
	bool CommandBufferManager::_CmdAddFrameDependency (const Message< GraphicsMsg::CmdAddFrameDependency > &msg)
	{
		CHECK_ERR( _IsComposedState( GetState() ) );

		const uint	next_idx	= (_bufferIndex+1) % _bufferChainLength;
		auto &		per_frame	= _perFrame[ next_idx ];

		per_frame.waitFences.Append( msg->waitFences );
		per_frame.waitSemaphores.Append( msg->waitSemaphores );
		per_frame.signalSemaphores.Append( msg->signalSemaphores );
		return true;
	}
	
/*
=================================================
	_SubscribeOnFrameCompleted
=================================================
*/
	bool CommandBufferManager::_SubscribeOnFrameCompleted (const Message< GraphicsMsg::SubscribeOnFrameCompleted > &msg)
	{
		auto &	per_frame	= _perFrame[ _bufferIndex ];

		per_frame.callbacks.PushBack( msg->callback );

		msg->index.Set( _bufferIndex );
		return true;
	}
	
/*
=================================================
	_Scope_CmdBegin
=================================================
*/
	bool CommandBufferManager::_Scope_CmdBegin (const Message< GpuMsg::CmdBegin > &)
	{
		_scope = EScope::Command;
		return false;
	}
	
/*
=================================================
	_Scope_CmdEnd
=================================================
*/
	bool CommandBufferManager::_Scope_CmdEnd (const Message< GpuMsg::CmdEnd > &)
	{
		_scope = EScope::Frame;
		return false;
	}
	
/*
=================================================
	_Scope_CmdBeginRenderPass
=================================================
*/
	bool CommandBufferManager::_Scope_CmdBeginRenderPass (const Message< GpuMsg::CmdBeginRenderPass > &)
	{
		_scope = EScope::RenderPass;
		return false;
	}
	
/*
=================================================
	_Scope_CmdEndRenderPass
=================================================
*/
	bool CommandBufferManager::_Scope_CmdEndRenderPass (const Message< GpuMsg::CmdEndRenderPass > &)
	{
		_scope = EScope::Command;
		return false;
	}

/*
=================================================
	_CmdBegin
=================================================
*/
	bool CommandBufferManager::_CmdBegin (const Message< GraphicsMsg::CmdBegin > &)
	{
		CHECK_ERR( _scope == EScope::Frame );

		auto&	per_frame = _perFrame[ _bufferIndex ];

		// find or create command buffer
		if ( per_frame.freeBuffers.Empty() )
		{
			ModulePtr	cmd_buf;
			CHECK_ERR( GlobalSystems()->modulesFactory->Create(
							_cmdBufferId,
							GlobalSystems(),
							CreateInfo::GpuCommandBuffer{
								_GetManager(),
								CommandBufferDescriptor{ ECmdBufferCreate::ImplicitResetable }
							},
							OUT cmd_buf ) );

			_builder->Send< ModuleMsg::AttachModule >({ "", cmd_buf });

			per_frame.commands.PushBack( cmd_buf );
		}
		else
		{
			per_frame.commands.PushBack( per_frame.freeBuffers.Back() );
			per_frame.freeBuffers.PopBack();
		}


		Message< GpuMsg::CmdBegin >		begin;
		begin->targetCmdBuffer	= per_frame.commands.Back();

		CHECK( _builder->Send( begin ) );

		_scope = EScope::Command;
		return true;
	}
	
/*
=================================================
	_CmdEnd
=================================================
*/
	bool CommandBufferManager::_CmdEnd (const Message< GraphicsMsg::CmdEnd > &)
	{
		CHECK_ERR( _scope == EScope::Command );

		CHECK( _builder->Send< GpuMsg::CmdEnd >({}) );

		_scope = EScope::Frame;
		return true;
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	CreateCommandBufferManager
=================================================
*/
	ModulePtr  GraphicsObjectsConstructor::CreateCommandBufferManager (GlobalSystemsRef gs, const CreateInfo::CommandBufferManager &ci)
	{
		return New< CommandBufferManager >( gs, ci );
	}

}	// Graphics
}	// Engine