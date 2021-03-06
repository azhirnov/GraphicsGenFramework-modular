// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "Core/Config/Engine.Config.h"

#ifdef GRAPHICS_API_SOFT

#include "Engine/Platforms/Soft/Impl/SWEnums.h"
#include "Engine/Platforms/Public/GPU/Pipeline.h"
#include "Engine/Platforms/Public/GPU/CommandBuffer.h"
#include "Engine/Platforms/Public/GPU/Image.h"
#include "Engine/Platforms/Public/GPU/Sampler.h"

namespace Engine
{
namespace PlatformSW
{
	//
	// Sync Primitives
	//
	using SWFencePtr		= SharedPointerType< class SWFence >;
	using SWEventPtr		= SharedPointerType< class SWEvent >;
	using SWSemaphorePtr	= SharedPointerType< class SWSemaphore >;

}	// PlatformSW


namespace GpuMsg
{

	//
	// Get GPU Device Info
	//
	struct GetSWDeviceInfo : _MsgBase_
	{
	};


	//
	// Get Image Memory Layout
	//
	struct GetSWImageMemoryLayout : _MsgBase_
	{
	// types
		using EPixelFormat		= Platforms::EPixelFormat;
		using EImageLayout		= Platforms::EImageLayout;
		using EPipelineAccess	= Platforms::EPipelineAccess;
		using EPipelineStage	= Platforms::EPipelineStage;
		using EMemoryAccess		= Platforms::EMemoryAccess;

		struct ImgLayer : CompileTime::FastCopyable
		{
		// variables
			uint2					dimension;
			void *					memory	= null;	// initialized when binded to memory
			BytesU					size;
			EPixelFormat::type		format	= Uninitialized;
			EImageLayout::type		layout	= EImageLayout::Undefined;

		// methods
			ImgLayer (GX_DEFCTOR) {}

			BinArrayRef		Data ()			{ return BinArrayRef::FromVoid( memory, size ); }
			BinArrayCRef	Data () const	{ return BinArrayCRef::FromVoid( memory, size ); }
		};

		struct ImgLayers2D : CompileTime::FastCopyable
		{
		// types
			using ImgLayers_t = FixedSizeArray< ImgLayer, 14 >;
			
		// variables
			ImgLayers_t			mipmaps;
			uint2				dimension;		// must be equal to mipmaps[0].dimension
			BytesU				size;			// summary size of all mipmaps
			
		// methods
			ImgLayers2D (GX_DEFCTOR) {}
		};

		struct ImgLayers3D
		{
		// types
			using ImgLayers2D_t	= Array< ImgLayers2D >;
			
		// variables
			ImgLayers2D_t		layers;
			uint3				dimension;		// must be equal to uint3( layers[0].dimension, ... )
			BytesU				size;			// summary size of all layers
			BytesU				align	= 4_b;	// base align
			EMemoryAccess::bits	memAccess;		// TODO: remove, use 'access'
			
		// methods
			ImgLayers3D (GX_DEFCTOR) {}
		};

		// image1D:  { layers[1], dim(w,1,1) }, { mipmaps[], dim(w,1) }
		// image2D:  { layers[1], dim(w,h,1) }, { mipmaps[], dim(w,h) }
		// image2DA: { layers[a], dim(w,h,1) }, { mipmaps[], dim(w,h) }
		// image3D:  { layers[d], dim(w,h,d) }, { mipmaps[], dim(w,h) }

	// variables
		EPipelineAccess::bits	accessMask;
		EPipelineStage::type	stage	= EPipelineStage::Unknown;
		Out< ImgLayers3D >		result;

	// methods
		GetSWImageMemoryLayout (EPipelineAccess::bits access, EPipelineStage::type stage) :
			accessMask{access}, stage{stage} {}
	};


	//
	// Get Image View Memory Layout
	//
	struct GetSWImageViewMemoryLayout : _MsgBase_
	{
	// types
		using ImgLayers3D		= GetSWImageMemoryLayout::ImgLayers3D;
		using EPipelineAccess	= Platforms::EPipelineAccess;
		using EPipelineStage	= Platforms::EPipelineStage;

	// variables
		Platforms::ImageViewDescription	viewDescr;
		EPipelineAccess::bits			accessMask;
		EPipelineStage::type			stage	= EPipelineStage::Unknown;
		Out< ImgLayers3D >				result;

	// methods
		GetSWImageViewMemoryLayout (EPipelineAccess::bits access, EPipelineStage::type stage) :
			viewDescr{}, accessMask{access}, stage{stage} {}

		GetSWImageViewMemoryLayout (const Platforms::ImageViewDescription &descr, EPipelineAccess::bits access, EPipelineStage::type stage) :
			viewDescr{descr}, accessMask{access}, stage{stage} {}
	};


	//
	// Get Image Memory Requirements
	//
	struct GetSWImageMemoryRequirements : _MsgBase_
	{
	// types
		struct MemReq {
			BytesU		size;
			BytesU		align;
		};

	// variables
		Out< MemReq >	result;
	};


	//
	// Get Texture Memory Layout
	//
	struct GetSWTextureMemoryLayout : GetSWImageViewMemoryLayout
	{
	// variables
		Out< Platforms::SamplerDescription >		sampler;

	// methods
		explicit GetSWTextureMemoryLayout (EPipelineAccess::bits access, EPipelineStage::type stage) :
			GetSWImageViewMemoryLayout{access, stage} {}

		explicit GetSWTextureMemoryLayout (const Platforms::ImageViewDescription &descr, EPipelineAccess::bits access, EPipelineStage::type stage) :
			GetSWImageViewMemoryLayout{descr, access, stage} {}
	};


	//
	// Image Barrier
	//
	struct SWImageBarrier : _MsgBase_
	{
	// types
		using Barrier_t			= CmdPipelineBarrier::ImageMemoryBarrier;
		using Barriers_t		= CmdPipelineBarrier::ImageMemoryBarriers_t;
		using EPipelineStage	= Platforms::EPipelineStage;

	// variables
		Barriers_t				barriers;
		EPipelineStage::bits	srcStageMask;
		EPipelineStage::bits	dstStageMask;

	// methods
		SWImageBarrier () {}

		explicit SWImageBarrier (const Barrier_t &imgBarrier, EPipelineStage::bits srcStage, EPipelineStage::bits dstStage) :
			srcStageMask{srcStage}, dstStageMask{dstStage} { barriers.PushBack(imgBarrier); }
	};


	//
	// Get Buffer Memory Layout
	//
	struct GetSWBufferMemoryLayout : _MsgBase_
	{
	// types
		using EMemoryAccess		= Platforms::EMemoryAccess;
		using EPipelineAccess	= Platforms::EPipelineAccess;
		using EPipelineStage	= Platforms::EPipelineStage;

		struct Data {
			BinArrayRef			memory;
			BytesU				align;
			EMemoryAccess::bits	memAccess;	// TODO: remove, use 'access'
		};

	// variables
		BytesU					offset;
		BytesU					size;
		EPipelineAccess::bits	access;
		EPipelineStage::type	stage	= EPipelineStage::Unknown;
		Out< Data >				result;

	// methods
		GetSWBufferMemoryLayout (EPipelineAccess::bits access, EPipelineStage::type stage) : 
			offset{0}, size{usize(UMax)}, access{access}, stage{stage} {}
		
		GetSWBufferMemoryLayout (BytesU offset, BytesU size, EPipelineAccess::bits access, EPipelineStage::type stage) : 
			offset{offset}, size{size}, access{access}, stage{stage} {}
	};


	//
	// Get Buffer Memory Requirements
	//
	struct GetSWBufferMemoryRequirements : _MsgBase_
	{
	// types
		struct MemReq {
			BytesU		size;
			BytesU		align;
		};

	// variables
		Out< MemReq >	result;
	};


	//
	// Buffer Barrier
	//
	/*struct SWBufferBarrier : _MsgBase_
	{
	// types
		using Barrier_t			= CmdPipelineBarrier::BufferMemoryBarrier;
		using Barriers_t		= CmdPipelineBarrier::BufferMemoryBarriers_t;
		using EPipelineStage	= Platforms::EPipelineStage;

	// variables
		Barriers_t				barriers;
		EPipelineStage::bits	srcStageMask;
		EPipelineStage::bits	dstStageMask;

	// methods
		SWBufferBarrier () {}

		explicit SWBufferBarrier (const Barrier_t &bufBarrier, EPipelineStage::bits srcStage, EPipelineStage::bits dstStage) :
			srcStageMask{srcStage}, dstStageMask{dstStage} { barriers.PushBack(bufBarrier); }
	};*/


	//
	// Get Memory Data
	//
	struct GetSWMemoryData : _MsgBase_
	{
		Out< BinArrayRef >	result;
	};


	//
	// Memory Barrier
	//
	/*struct SWMemoryBarrier : _MsgBase_
	{
	// types
		using Barrier_t			= CmdPipelineBarrier::MemoryBarrier;
		using Barriers_t		= CmdPipelineBarrier::MemoryBarriers_t;
		using EPipelineStage	= Platforms::EPipelineStage;

	// variables
		Barriers_t				barriers;
		EPipelineStage::bits	srcStageMask;
		EPipelineStage::bits	dstStageMask;

	// methods
		SWMemoryBarrier () {}

		explicit SWMemoryBarrier (const Barrier_t &memBarrier, EPipelineStage::bits srcStage, EPipelineStage::bits dstStage) :
			srcStageMask{srcStage}, dstStageMask{dstStage} { barriers.PushBack(memBarrier); }
	};*/
	

	//
	// Get GPU Shader Module IDs
	//
	struct GetSWShaderModuleIDs : _MsgBase_
	{
	// types
		using SWInvoke_t	= Platforms::PipelineTemplateDescription::ShaderSource::SWInvoke_t;

		struct ShaderModule : CompileTime::PODStruct
		{
			SWInvoke_t					func	= null;
			Platforms::EShader::type	type	= Platforms::EShader::Unknown;
		};
		using Shaders_t		= FixedSizeArray< ShaderModule, Platforms::EShader::_Count >;

	// variables
		Out< Shaders_t >	result;
	};


	//
	// Get Pipeline Stage
	//
	struct GetSWPipelineStage : _MsgBase_
	{
	// types
		using SWInvoke_t	= Platforms::PipelineTemplateDescription::ShaderSource::SWInvoke_t;
		using EShader		= Platforms::EShader;

		struct Stage {
			SWInvoke_t		func;
		};

	// variables
		EShader::type	stage;
		Out< Stage >	result;

	// methods
		explicit GetSWPipelineStage (EShader::type stage) : stage{stage} {}
	};


	//
	// Resource Table Forward Message to Resource
	//
	template <typename Msg>
	struct ResourceTableForwardMsg : _MsgBase_
	{
	// variables
		mutable Msg		message;
		uint			index	= UMax;		// resource unique index

	// methods
		explicit ResourceTableForwardMsg (uint idx) : index{idx} {}
		
		ResourceTableForwardMsg (uint idx, const Msg &msg) : message{msg}, index{idx} {}

		template <typename ...ArgTypes>
		explicit ResourceTableForwardMsg (uint idx, ArgTypes&& ...args) : message{ FW<ArgTypes>(args)... }, index{idx} {}
	};


	//
	// Sync Client With Device
	//
	struct SyncSWClientWithDevice : _MsgBase_
	{
	// variables
		Platforms::GpuFenceId		fenceId	= Uninitialized;

	// methods
		explicit SyncSWClientWithDevice (Platforms::GpuFenceId id) : fenceId{id} {}
	};

	
	//
	// Software Renderer Commands
	//
	struct SetSWCommandBufferQueue : _MsgBase_
	{
	// types
		using Data_t	= Union< CmdBindComputePipeline,
								 CmdDispatch,
								 CmdDispatchIndirect,
								 CmdExecute,
								 CmdBindComputeResourceTable,
								 CmdCopyBuffer,
								 CmdCopyImage,
								 CmdCopyBufferToImage,
								 CmdCopyImageToBuffer,
								 CmdUpdateBuffer,
								 CmdFillBuffer,
								 CmdClearColorImage,
								 CmdPipelineBarrier,
								 CmdPushConstants,
								 CmdPushNamedConstants,
								 CmdDebugMarker,
								 CmdPushDebugGroup,
								 CmdPopDebugGroup >;

		using Func_t	= Delegate< void (VariantCRef data, StringCRef file, uint line) >;

		struct Command
		{
		// variables
			Data_t		data;

			DEBUG_ONLY(
				String	file;
				uint	line;
			)

		// methods
			Command () {}

			template <typename Data>
			Command (Data &&data, StringCRef file = StringCRef(), uint line = 0) :
				data(RVREF(data)) DEBUG_ONLY(, file(file), line(line) )
			{}
			
			template <typename Data>
			Command (const Data &data, StringCRef file = StringCRef(), uint line = 0) :
				data(data) DEBUG_ONLY(, file(file), line(line) )
			{}
		};

	// variables
		ReadOnce< Array<Command> >	commands;
		BinaryArray					bufferData;
		BinaryArray					pushConstData;

	// methods
		SetSWCommandBufferQueue (Array<Command> &&commands, BinaryArray &&bufferData, BinaryArray &&pushConstData) :
			commands{ RVREF(commands) }, bufferData{ RVREF(bufferData) }, pushConstData{ RVREF(pushConstData) } {}
	};
	

	//
	// Execute Command Buffer
	//
	struct ExecuteSWCommandBuffer : _MsgBase_
	{
	// variables
		Editable< bool  >	completed;
		Editable< usize >	lastCmdIndex;	// to continue executing from this command

	// methods
		ExecuteSWCommandBuffer () : completed{true}, lastCmdIndex{0} {}
	};


	//
	// Present Frame
	//
	struct SWPresent : _MsgBase_
	{
	// variables
		Function< void () >		callback;
		
	// methods
		explicit SWPresent (Function<void()> &&cb) : callback{ RVREF(cb) } {}
	};


	//
	// Get Fence
	//
	struct GetSWFence : _MsgBase_
	{
	// variables
		Platforms::GpuFenceId			id;
		Out< PlatformSW::SWFencePtr >	result;

	// methods
		explicit GetSWFence (Platforms::GpuFenceId id) : id{id} {}
	};


	//
	// Get Event
	//
	struct GetSWEvent : _MsgBase_
	{
	// variables
		Platforms::GpuEventId			id;
		Out< PlatformSW::SWEventPtr >	result;

	// methods
		explicit GetSWEvent (Platforms::GpuEventId id) : id{id} {}
	};


	//
	// Get Semaphore
	//
	struct GetSWSemaphore : _MsgBase_
	{
	// variables
		Platforms::GpuSemaphoreId			id;
		Out< PlatformSW::SWSemaphorePtr >	result;

	// methods
		explicit GetSWSemaphore (Platforms::GpuSemaphoreId id) : id{id} {}
	};


}	// GpuMsg
}	// Engine

#endif	// GRAPHICS_API_SOFT
