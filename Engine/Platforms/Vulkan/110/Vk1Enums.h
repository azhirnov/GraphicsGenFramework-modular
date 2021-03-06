// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "Core/Config/Engine.Config.h"

#ifdef GRAPHICS_API_VULKAN

#include "Engine/Platforms/Vulkan/110/vulkan1.h"
#include "Engine/Platforms/Public/GPU/BufferEnums.h"
#include "Engine/Platforms/Public/GPU/CommandEnums.h"
#include "Engine/Platforms/Public/GPU/MemoryEnums.h"
#include "Engine/Platforms/Public/GPU/MultiSamples.h"
#include "Engine/Platforms/Public/GPU/PixelFormatEnums.h"
#include "Engine/Platforms/Public/GPU/RenderPassEnums.h"
#include "Engine/Platforms/Public/GPU/RenderStateEnums.h"
#include "Engine/Platforms/Public/GPU/SamplerEnums.h"
#include "Engine/Platforms/Public/GPU/ShaderEnums.h"
#include "Engine/Platforms/Public/GPU/ImageEnums.h"
#include "Engine/Platforms/Public/GPU/VertexEnums.h"
#include "Engine/Platforms/Public/GPU/ObjectEnums.h"
#include "Engine/Platforms/Public/GPU/QueryEnums.h"

namespace Engine
{
namespace PlatformVK
{
	using namespace Engine::Platforms;
	
/*
=================================================
	MemoryPropertyFlags
=================================================
*/
	ND_ inline vk::VkMemoryPropertyFlags  Vk1Enum (EGpuMemory::bits value)
	{
		using namespace vk;

		VkMemoryPropertyFlags	result = 0;

		if ( value[ EGpuMemory::LocalInGPU ] )
			result |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		if ( value[ EGpuMemory::CoherentWithCPU ] )
			result |= (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		if ( value[ EGpuMemory::CachedInCPU ] )
			result |= (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

		return result;
	}

/*
=================================================
	LogicOp
=================================================
*/
	ND_ inline vk::VkLogicOp  Vk1Enum (ELogicOp::type value)
	{
		using namespace vk;

		switch ( value )
		{
			case ELogicOp::Clear		: return VK_LOGIC_OP_CLEAR;
			case ELogicOp::Set			: return VK_LOGIC_OP_SET;
			case ELogicOp::Copy			: return VK_LOGIC_OP_COPY;
			case ELogicOp::CopyInverted	: return VK_LOGIC_OP_COPY_INVERTED;
			case ELogicOp::Noop			: return VK_LOGIC_OP_NO_OP;
			case ELogicOp::Invert		: return VK_LOGIC_OP_INVERT;
			case ELogicOp::And			: return VK_LOGIC_OP_AND;
			case ELogicOp::NotAnd		: return VK_LOGIC_OP_NAND;
			case ELogicOp::Or			: return VK_LOGIC_OP_OR;
			case ELogicOp::NotOr		: return VK_LOGIC_OP_NOR;
			case ELogicOp::Xor			: return VK_LOGIC_OP_XOR;
			case ELogicOp::Equiv		: return VK_LOGIC_OP_EQUIVALENT;
			case ELogicOp::AndReverse	: return VK_LOGIC_OP_AND_REVERSE;
			case ELogicOp::AndInverted	: return VK_LOGIC_OP_AND_INVERTED;
			case ELogicOp::OrReverse	: return VK_LOGIC_OP_OR_REVERSE;
			case ELogicOp::OrInverted	: return VK_LOGIC_OP_OR_INVERTED;
		}
		RETURN_ERR( "invalid logical op", VK_LOGIC_OP_MAX_ENUM );
	}
	
/*
=================================================
	BlendFunc
=================================================
*/
	ND_ inline vk::VkBlendFactor  Vk1Enum (EBlendFunc::type value)
	{
		using namespace vk;

		switch ( value )
		{
			case EBlendFunc::Zero				: return VK_BLEND_FACTOR_ZERO;
			case EBlendFunc::One				: return VK_BLEND_FACTOR_ONE;
			case EBlendFunc::SrcColor			: return VK_BLEND_FACTOR_SRC_COLOR;
			case EBlendFunc::OneMinusSrcColor	: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
			case EBlendFunc::DstColor			: return VK_BLEND_FACTOR_DST_COLOR;
			case EBlendFunc::OneMinusDstColor	: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
			case EBlendFunc::SrcAlpha			: return VK_BLEND_FACTOR_SRC_ALPHA;
			case EBlendFunc::OneMinusSrcAlpha	: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			case EBlendFunc::DstAlpha			: return VK_BLEND_FACTOR_DST_ALPHA;
			case EBlendFunc::OneMinusDstAlpha	: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
			case EBlendFunc::ConstColor			: return VK_BLEND_FACTOR_CONSTANT_COLOR;
			case EBlendFunc::OneMinusConstColor	: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
			case EBlendFunc::ConstAlpha			: return VK_BLEND_FACTOR_CONSTANT_ALPHA;
			case EBlendFunc::OneMinusConstAlpha	: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
			case EBlendFunc::SrcAlphaSaturate	: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
			// TODO
			//	VK_BLEND_FACTOR_SRC1_COLOR
			//	VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR
			//	VK_BLEND_FACTOR_SRC1_ALPHA
			//	VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA
		}
		RETURN_ERR( "invalid blend func", VK_BLEND_FACTOR_MAX_ENUM );
	}
	
/*
=================================================
	BlendEq
=================================================
*/
	ND_ inline vk::VkBlendOp  Vk1Enum (EBlendEq::type value)
	{
		using namespace vk;

		switch ( value )
		{
			case EBlendEq::Add		: return VK_BLEND_OP_ADD;
			case EBlendEq::Sub		: return VK_BLEND_OP_SUBTRACT;
			case EBlendEq::RevSub	: return VK_BLEND_OP_REVERSE_SUBTRACT;
			case EBlendEq::Min		: return VK_BLEND_OP_MIN;
			case EBlendEq::Max		: return VK_BLEND_OP_MAX;
		}
		RETURN_ERR( "invalid blend equation", VK_BLEND_OP_MAX_ENUM );
	}
	
/*
=================================================
	CompareFunc
=================================================
*/
	ND_ inline vk::VkCompareOp  Vk1Enum (ECompareFunc::type value)
	{
		using namespace vk;

		switch ( value )
		{
			case ECompareFunc::Never	: return VK_COMPARE_OP_NEVER;
			case ECompareFunc::Less		: return VK_COMPARE_OP_LESS;
			case ECompareFunc::Equal	: return VK_COMPARE_OP_EQUAL;
			case ECompareFunc::LEqual	: return VK_COMPARE_OP_LESS_OR_EQUAL;
			case ECompareFunc::Greater	: return VK_COMPARE_OP_GREATER;
			case ECompareFunc::NotEqual	: return VK_COMPARE_OP_NOT_EQUAL;
			case ECompareFunc::GEqual	: return VK_COMPARE_OP_GREATER_OR_EQUAL;
			case ECompareFunc::Always	: return VK_COMPARE_OP_ALWAYS;
		}
		RETURN_ERR( "invalid compare func", VK_COMPARE_OP_MAX_ENUM );
	}
	
/*
=================================================
	StencilOp
=================================================
*/
	ND_ inline vk::VkStencilOp  Vk1Enum (EStencilOp::type value)
	{
		using namespace vk;

		switch ( value )
		{
			case EStencilOp::Keep		: return VK_STENCIL_OP_KEEP;
			case EStencilOp::Zero		: return VK_STENCIL_OP_ZERO;
			case EStencilOp::Replace	: return VK_STENCIL_OP_REPLACE;
			case EStencilOp::Incr		: return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
			case EStencilOp::IncrWrap	: return VK_STENCIL_OP_INCREMENT_AND_WRAP;
			case EStencilOp::Decr		: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
			case EStencilOp::DecrWrap	: return VK_STENCIL_OP_DECREMENT_AND_WRAP;
			case EStencilOp::Invert		: return VK_STENCIL_OP_INVERT;
		}
		RETURN_ERR( "invalid stencil op", VK_STENCIL_OP_MAX_ENUM );
	}
	
/*
=================================================
	PolygonMode
=================================================
*/
	ND_ inline vk::VkPolygonMode  Vk1Enum (EPolygonMode::type value)
	{
		using namespace vk;

		switch ( value )
		{
			case EPolygonMode::Point	: return VK_POLYGON_MODE_POINT;
			case EPolygonMode::Line		: return VK_POLYGON_MODE_LINE;
			case EPolygonMode::Fill		: return VK_POLYGON_MODE_FILL;
		}
		RETURN_ERR( "invalid polygon mode", VK_POLYGON_MODE_MAX_ENUM );
	}
	
/*
=================================================
	CullMode
=================================================
*/
	inline bool Vk1Enum (EPolygonFace::type value, OUT vk::VkCullModeFlagBits &cullMode)
	{
		using namespace vk;

		cullMode = VK_CULL_MODE_NONE;

		if ( EnumEq( value, EPolygonFace::Front ) )
			cullMode = VkCullModeFlagBits( cullMode | VK_CULL_MODE_FRONT_BIT );
		
		if ( EnumEq( value, EPolygonFace::Back ) )
			cullMode = VkCullModeFlagBits( cullMode | VK_CULL_MODE_BACK_BIT );

		return true;
	}
	
/*
=================================================
	StencilFace
=================================================
*/
	inline bool Vk1Enum (EPolygonFace::type value, OUT vk::VkStencilFaceFlagBits &stencilFaceMask)
	{
		using namespace vk;

		stencilFaceMask = VkStencilFaceFlagBits(0);

		if ( EnumEq( value, EPolygonFace::Front ) )
			stencilFaceMask = VkStencilFaceFlagBits( stencilFaceMask | VK_STENCIL_FACE_FRONT_BIT );
		
		if ( EnumEq( value, EPolygonFace::Back ) )
			stencilFaceMask = VkStencilFaceFlagBits( stencilFaceMask | VK_STENCIL_FACE_BACK_BIT );

		CHECK_ERR( stencilFaceMask != 0 );
		return true;
	}

/*
=================================================
	Primitive
=================================================
*/
	ND_ inline vk::VkPrimitiveTopology  Vk1Enum (EPrimitive::type value)
	{
		using namespace vk;

		switch ( value )
		{
			case EPrimitive::Point			:	return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
			case EPrimitive::LineList		:	return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
			case EPrimitive::LineStrip		:	return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
			case EPrimitive::TriangleList	:	return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			case EPrimitive::TriangleStrip	:	return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			case EPrimitive::Patch			:	return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
			// TODO
			//	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN
			//	VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY
			//	VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY
			//	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY
			//	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY
			//	VK_PRIMITIVE_TOPOLOGY_PATCH_LIST
		}
		RETURN_ERR( "invalid primitive type", VK_PRIMITIVE_TOPOLOGY_MAX_ENUM );
	}
	
/*
=================================================
	BorderColor
=================================================
*/
	ND_ inline vk::VkBorderColor  Vk1Enum (ESamplerBorderColor::bits value)
	{
		using namespace vk;

		// int
		if ( value[ ESamplerBorderColor::Int ] )
		{
			if ( value[ ESamplerBorderColor::Black ] )
				return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			
			if ( value[ ESamplerBorderColor::White ] )
				return VK_BORDER_COLOR_INT_OPAQUE_WHITE;

			//if ( value[ ESamplerBorderColor::Transparent ] )
				return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
		}
		// float
		else
		{
			if ( value[ ESamplerBorderColor::Black ] )
				return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
			
			if ( value[ ESamplerBorderColor::White ] )
				return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

			//if ( value[ ESamplerBorderColor::Transparent ] )
				return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		}
	}
	
/*
=================================================
	AddressMode
=================================================
*/
	ND_ inline vk::VkSamplerAddressMode  Vk1Enum (EAddressMode::type value)
	{
		using namespace vk;

		switch ( value )
		{
			case EAddressMode::ClampUnnorm		:
			case EAddressMode::ClampToEdge		:	return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			case EAddressMode::ClampToBorder	:	return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			case EAddressMode::Repeat			:	return VK_SAMPLER_ADDRESS_MODE_REPEAT;
			case EAddressMode::MirroredRepeat	:	return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			case EAddressMode::MirroredClamp	:	return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
		}

		RETURN_ERR( "invalid address mode", VK_SAMPLER_ADDRESS_MODE_MAX_ENUM );
	}
	
/*
=================================================
	PipelineDynamicState
=================================================
*/
	ND_ inline vk::VkDynamicState  Vk1Enum (EPipelineDynamicState::type value)
	{
		using namespace vk;

		switch ( value )
		{
			case EPipelineDynamicState::Viewport			: return VK_DYNAMIC_STATE_VIEWPORT;
			case EPipelineDynamicState::Scissor				: return VK_DYNAMIC_STATE_SCISSOR;
			case EPipelineDynamicState::LineWidth			: return VK_DYNAMIC_STATE_LINE_WIDTH;
			case EPipelineDynamicState::DepthBias			: return VK_DYNAMIC_STATE_DEPTH_BIAS;
			case EPipelineDynamicState::BlendConstants		: return VK_DYNAMIC_STATE_BLEND_CONSTANTS;
			case EPipelineDynamicState::DepthBounds			: return VK_DYNAMIC_STATE_DEPTH_BOUNDS;
			case EPipelineDynamicState::StencilCompareMask	: return VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK;
			case EPipelineDynamicState::StencilWriteMask	: return VK_DYNAMIC_STATE_STENCIL_WRITE_MASK;
			case EPipelineDynamicState::StencilReference	: return VK_DYNAMIC_STATE_STENCIL_REFERENCE;
		}

		RETURN_ERR( "invalid dynamic state type", VK_DYNAMIC_STATE_MAX_ENUM );
	}
	
/*
=================================================
	BufferUsage
=================================================
*/
	ND_ inline vk::VkBufferUsageFlags  Vk1Enum (EBufferUsage::bits values)
	{
		using namespace vk;

		VkBufferUsageFlags	result = 0;

		FOR( i, values )
		{
			const auto t = EBufferUsage::type(i);

			if ( not values[t] )
				continue;

			switch ( t )
			{
				case EBufferUsage::TransferSrc	:	result |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;			break;
				case EBufferUsage::TransferDst	:	result |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;			break;
				case EBufferUsage::UniformTexel	:	result |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;	break;
				case EBufferUsage::StorageTexel	:	result |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;	break;
				case EBufferUsage::Uniform		:	result |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;		break;
				case EBufferUsage::Storage		:	result |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;		break;
				case EBufferUsage::Index		:	result |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;			break;
				case EBufferUsage::Vertex		:	result |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;		break;
				case EBufferUsage::Indirect		:	result |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;		break;
				default							:	RETURN_ERR( "invalid buffer usage", 0 );
			}
		}

		return result;
	}
	
/*
=================================================
	BufferSparse
=================================================
*/
	inline bool  Vk1Enum (OUT vk::VkBufferCreateFlagBits &flags, ESparseMemory::bits values)
	{
		using namespace vk;

		flags = VkBufferCreateFlagBits(0);
		VkBufferCreateFlags result = 0;

		FOR( i, values )
		{
			const auto t = ESparseMemory::type(i);

			if ( not values[t] )
				continue;

			switch ( t )
			{
				case ESparseMemory::None		:	break;
				case ESparseMemory::Binding		:	result |= VK_BUFFER_CREATE_SPARSE_BINDING_BIT;	break;
				case ESparseMemory::Residency	:	result |= VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT;	break;
				case ESparseMemory::Aliased		:	result |= VK_BUFFER_CREATE_SPARSE_ALIASED_BIT;	break;
				default							:	RETURN_ERR( "invalid buffer sparse type" );
			}
		}

		flags = VkBufferCreateFlagBits( result );
		return true;
	}
	
/*
=================================================
	ImageSparse
=================================================
*/
	inline bool  Vk1Enum (OUT vk::VkImageCreateFlagBits &flags, ESparseMemory::bits values)
	{
		using namespace vk;
		
		flags = VkImageCreateFlagBits(0);
		VkImageCreateFlags result = 0;

		FOR( i, values )
		{
			const auto t = ESparseMemory::type(i);

			if ( not values[t] )
				continue;

			switch ( t )
			{
				case ESparseMemory::None		:	break;
				case ESparseMemory::Binding		:	result |= VK_IMAGE_CREATE_SPARSE_BINDING_BIT;	break;
				case ESparseMemory::Residency	:	result |= VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT;	break;
				case ESparseMemory::Aliased		:	result |= VK_IMAGE_CREATE_SPARSE_ALIASED_BIT;	break;
				default							:	RETURN_ERR( "invalid buffer sparse type" );
			}
		}
		
		flags = VkImageCreateFlagBits( result );
		return true;
	}
	
/*
=================================================
	IndexType
=================================================
*/
	ND_ inline vk::VkIndexType  Vk1Enum (EIndex::type value)
	{
		using namespace vk;

		switch ( value )
		{
			case EIndex::UShort	:	return VK_INDEX_TYPE_UINT16;
			case EIndex::UInt	:	return VK_INDEX_TYPE_UINT32;
		}

		RETURN_ERR( "invalid index type", VK_INDEX_TYPE_MAX_ENUM );
	}
	
/*
=================================================
	ShaderStage
=================================================
*/
	ND_ inline vk::VkShaderStageFlagBits  Vk1Enum (EShader::type value)
	{
		using namespace vk;

		switch ( value )
		{
			case EShader::Vertex			: return VK_SHADER_STAGE_VERTEX_BIT;
			case EShader::TessControl		: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			case EShader::TessEvaluation	: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			case EShader::Geometry			: return VK_SHADER_STAGE_GEOMETRY_BIT;
			case EShader::Fragment			: return VK_SHADER_STAGE_FRAGMENT_BIT;
			case EShader::Compute			: return VK_SHADER_STAGE_COMPUTE_BIT;
		}

		RETURN_ERR( "invalid shader type", VkShaderStageFlagBits(0) );
	}
	
/*
=================================================
	VertexInputRate
=================================================
*/
	ND_ inline vk::VkVertexInputRate  Vk1Enum (EVertexInputRate::type value)
	{
		using namespace vk;

		switch ( value )
		{
			case EVertexInputRate::Vertex	: return VK_VERTEX_INPUT_RATE_VERTEX;
			case EVertexInputRate::Instance	: return VK_VERTEX_INPUT_RATE_INSTANCE;
		}

		RETURN_ERR( "invalid vertex input rate", VK_VERTEX_INPUT_RATE_MAX_ENUM );
	}
	
/*
=================================================
	MultiSamples
=================================================
*/
	ND_ inline vk::VkSampleCountFlagBits  Vk1Enum (MultiSamples value)
	{
		using namespace vk;

		return Clamp( VkSampleCountFlagBits(value.Get()), VK_SAMPLE_COUNT_1_BIT, VK_SAMPLE_COUNT_64_BIT );
	}
	
/*
=================================================
	AttachmentLoadOp
=================================================
*/
	ND_ inline vk::VkAttachmentLoadOp  Vk1Enum (EAttachmentLoadOp::type value)
	{
		using namespace vk;

		switch ( value )
		{
			case EAttachmentLoadOp::Invalidate :	return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			case EAttachmentLoadOp::Load :			return VK_ATTACHMENT_LOAD_OP_LOAD;
			case EAttachmentLoadOp::Clear :			return VK_ATTACHMENT_LOAD_OP_CLEAR;
		}

		RETURN_ERR( "invalid load op type", VK_ATTACHMENT_LOAD_OP_MAX_ENUM );
	}
	
/*
=================================================
	AttachmentStoreOp
=================================================
*/
	ND_ inline vk::VkAttachmentStoreOp  Vk1Enum (EAttachmentStoreOp::type value)
	{
		using namespace vk;

		switch ( value )
		{
			case EAttachmentStoreOp::Invalidate :	return VK_ATTACHMENT_STORE_OP_DONT_CARE;
			case EAttachmentStoreOp::Store :		return VK_ATTACHMENT_STORE_OP_STORE;
		}

		RETURN_ERR( "invalid store op type", VK_ATTACHMENT_STORE_OP_MAX_ENUM );
	}
	
/*
=================================================
	ImageLayout
=================================================
*/
	ND_ inline vk::VkImageLayout  Vk1Enum (EImageLayout::type value)
	{
		using namespace vk;

		switch ( value )
		{
			case EImageLayout::Undefined :								return VK_IMAGE_LAYOUT_UNDEFINED;
			case EImageLayout::General :								return VK_IMAGE_LAYOUT_GENERAL;
			case EImageLayout::ColorAttachmentOptimal :					return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			case EImageLayout::DepthStencilAttachmentOptimal :			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			case EImageLayout::DepthStencilReadOnlyOptimal :			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			case EImageLayout::ShaderReadOnlyOptimal :					return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			case EImageLayout::TransferSrcOptimal :						return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			case EImageLayout::TransferDstOptimal :						return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			case EImageLayout::Preinitialized :							return VK_IMAGE_LAYOUT_PREINITIALIZED;
			case EImageLayout::PresentSrc :								return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			case EImageLayout::DepthReadOnlyStencilAttachmentOptimal :	return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
			case EImageLayout::DepthAttachmentStencilReadOptimal :		return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
			case EImageLayout::SharedPresent :							return VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR;
		}

		RETURN_ERR( "invalid image layout type", VK_IMAGE_LAYOUT_MAX_ENUM );
	}
	
/*
=================================================
	SubpassDependency
=================================================
*/
	ND_ inline vk::VkDependencyFlags  Vk1Enum (ESubpassDependency::bits values)
	{
		using namespace vk;

		VkDependencyFlags	flags = 0;

		FOR( i, values )
		{
			const auto t = ESubpassDependency::type(i);

			if ( not values[t] )
				continue;

			switch ( t )
			{
				case ESubpassDependency::ByRegion		: flags |= VK_DEPENDENCY_BY_REGION_BIT;		break;
				case ESubpassDependency::DeviceGroup	: flags |= VK_DEPENDENCY_DEVICE_GROUP_BIT;	break;
				case ESubpassDependency::ViewLocal		: flags |= VK_DEPENDENCY_VIEW_LOCAL_BIT;	break;
				default									: RETURN_ERR( "invalid dependency type", VK_DEPENDENCY_FLAG_BITS_MAX_ENUM );
			}
		}
		return flags;
	}
	
/*
=================================================
	PipelineStage
=================================================
*/
	ND_ inline vk::VkPipelineStageFlagBits  Vk1Enum (EPipelineStage::type value)
	{
		using namespace vk;

		switch ( value )
		{
			case EPipelineStage::TopOfPipe				: return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			case EPipelineStage::DrawIndirect			: return VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
			case EPipelineStage::VertexInput			: return VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
			case EPipelineStage::VertexShader			: return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
			case EPipelineStage::TessControlShader		: return VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
			case EPipelineStage::TessEvaluationShader	: return VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
			case EPipelineStage::GeometryShader			: return VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
			case EPipelineStage::FragmentShader			: return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			case EPipelineStage::EarlyFragmentTests		: return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			case EPipelineStage::LateFragmentTests		: return VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			case EPipelineStage::ColorAttachmentOutput	: return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			case EPipelineStage::ComputeShader			: return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			case EPipelineStage::Transfer				: return VK_PIPELINE_STAGE_TRANSFER_BIT;
			case EPipelineStage::BottomOfPipe			: return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			case EPipelineStage::Host					: return VK_PIPELINE_STAGE_HOST_BIT;
		}
		RETURN_ERR( "invalid pipeline state", VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM );
	}
	
/*
=================================================
	PipelineStage
=================================================
*/
	ND_ inline vk::VkPipelineStageFlags  Vk1Enum (EPipelineStage::bits values)
	{
		using namespace vk;

		VkPipelineStageFlags	flags = 0;

		if ( values == EPipelineStage::AllCommands )
			return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		if ( values == EPipelineStage::AllGraphics )
			return VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;

		FOR( i, values )
		{
			const auto t = EPipelineStage::type(i);

			if ( not values[t] )
				continue;

			flags |= Vk1Enum( t );
		}
		return flags;
	}
		
/*
=================================================
	ShaderStage
=================================================
*/
	ND_ inline vk::VkShaderStageFlagBits  Vk1Enum (EShader::bits values)
	{
		using namespace vk;

		VkShaderStageFlags	flags = 0;

		FOR( i, values )
		{
			const auto	t = EShader::type(i);

			if ( not values[t] )
				continue;

			switch ( t )
			{
				case EShader::Vertex :			flags |= VK_SHADER_STAGE_VERTEX_BIT;					break;
				case EShader::TessControl :		flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;		break;
				case EShader::TessEvaluation :	flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;	break;
				case EShader::Geometry :		flags |= VK_SHADER_STAGE_GEOMETRY_BIT;					break;
				case EShader::Fragment :		flags |= VK_SHADER_STAGE_FRAGMENT_BIT;					break;
				case EShader::Compute :			flags |= VK_SHADER_STAGE_COMPUTE_BIT;					break;
				default :						RETURN_ERR( "unknown shader type!", VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM );
			}
		}
		return VkShaderStageFlagBits(flags);
	}

/*
=================================================
	PipelineAccess
=================================================
*/
	ND_ inline vk::VkAccessFlags  Vk1Enum (EPipelineAccess::bits values)
	{
		using namespace vk;

		VkAccessFlags	flags = 0;

		FOR( i, values )
		{
			const auto t = EPipelineAccess::type(i);

			if ( not values[t] )
				continue;

			switch ( t )
			{
				case EPipelineAccess::IndirectCommandRead			: flags |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;				break;
				case EPipelineAccess::IndexRead						: flags |= VK_ACCESS_INDEX_READ_BIT;						break;
				case EPipelineAccess::VertexAttributeRead			: flags |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;				break;
				case EPipelineAccess::UniformRead					: flags |= VK_ACCESS_UNIFORM_READ_BIT;						break;
				case EPipelineAccess::InputAttachmentRead			: flags |= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;				break;
				case EPipelineAccess::ShaderRead					: flags |= VK_ACCESS_SHADER_READ_BIT;						break;
				case EPipelineAccess::ShaderWrite					: flags |= VK_ACCESS_SHADER_WRITE_BIT;						break;
				case EPipelineAccess::ColorAttachmentRead			: flags |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;				break;
				case EPipelineAccess::ColorAttachmentWrite			: flags |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;			break;
				case EPipelineAccess::DepthStencilAttachmentRead	: flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;		break;
				case EPipelineAccess::DepthStencilAttachmentWrite	: flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;	break;
				case EPipelineAccess::TransferRead					: flags |= VK_ACCESS_TRANSFER_READ_BIT;						break;
				case EPipelineAccess::TransferWrite					: flags |= VK_ACCESS_TRANSFER_WRITE_BIT;					break;
				case EPipelineAccess::HostRead						: flags |= VK_ACCESS_HOST_READ_BIT;							break;
				case EPipelineAccess::HostWrite						: flags |= VK_ACCESS_HOST_WRITE_BIT;						break;
				case EPipelineAccess::MemoryRead					: flags |= VK_ACCESS_MEMORY_READ_BIT;						break;
				case EPipelineAccess::MemoryWrite					: flags |= VK_ACCESS_MEMORY_WRITE_BIT;						break;
				default												: RETURN_ERR( "invalid access type", VK_ACCESS_FLAG_BITS_MAX_ENUM );
			}
		}
		return flags;
	}

/*
=================================================
	ImageUsage
=================================================
*/
	ND_ inline vk::VkImageUsageFlags  Vk1Enum (EImageUsage::bits values)
	{
		using namespace vk;

		VkImageUsageFlags	flags = 0;

		FOR( i, values )
		{
			const auto t = EImageUsage::type(i);
			
			if ( not values[t] )
				continue;

			switch ( t )
			{
				case EImageUsage::TransferSrc				: flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;				break;
				case EImageUsage::TransferDst				: flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;				break;
				case EImageUsage::Sampled					: flags |= VK_IMAGE_USAGE_SAMPLED_BIT;					break;
				case EImageUsage::Storage					: flags |= VK_IMAGE_USAGE_STORAGE_BIT;					break;
				case EImageUsage::ColorAttachment			: flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;			break;
				case EImageUsage::DepthStencilAttachment	: flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;	break;
				case EImageUsage::TransientAttachment		: flags |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;		break;
				case EImageUsage::InputAttachment			: flags |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;			break;
				default										: RETURN_ERR( "invalid image usage type", VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM );
			}
		}
		return flags;
	}

/*
=================================================
	ImageUsage
=================================================
*/
	ND_ inline EImageUsage::bits  Vk1EnumRevert (vk::VkImageUsageFlags flags)
	{
		EImageUsage::bits	result;
		if ( EnumEq( flags, vk::VK_IMAGE_USAGE_TRANSFER_SRC_BIT ) )					result |= EImageUsage::TransferSrc;
		if ( EnumEq( flags, vk::VK_IMAGE_USAGE_TRANSFER_DST_BIT ) )					result |= EImageUsage::TransferDst;
		if ( EnumEq( flags, vk::VK_IMAGE_USAGE_SAMPLED_BIT ) )						result |= EImageUsage::Sampled;
		if ( EnumEq( flags, vk::VK_IMAGE_USAGE_STORAGE_BIT ) )						result |= EImageUsage::Storage;
		if ( EnumEq( flags, vk::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ) )				result |= EImageUsage::ColorAttachment;
		if ( EnumEq( flags, vk::VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ) )		result |= EImageUsage::DepthStencilAttachment;
		if ( EnumEq( flags, vk::VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT ) )			result |= EImageUsage::TransientAttachment;
		if ( EnumEq( flags, vk::VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT ) )				result |= EImageUsage::InputAttachment;
		return result;
	}

/*
=================================================
	ImageViewType
=================================================
*/
	ND_ inline vk::VkImageViewType  Vk1Enum (EImage::type value)
	{
		using namespace vk;
		
		switch ( value )
		{
			case EImage::Tex1D		: return VK_IMAGE_VIEW_TYPE_1D;
			case EImage::Tex1DArray	: return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
			case EImage::Tex2D		: return VK_IMAGE_VIEW_TYPE_2D;
			case EImage::Tex2DArray	: return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			//case EImage::Tex2DMS
			//case EImage::Tex2DMSArray
			case EImage::TexCube		: return VK_IMAGE_VIEW_TYPE_CUBE;
			case EImage::TexCubeArray	: return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
			case EImage::Tex3D		: return VK_IMAGE_VIEW_TYPE_3D;
			//case EImage::Buffer
		}

		RETURN_ERR( "unsupported image view type", VK_IMAGE_VIEW_TYPE_MAX_ENUM );
	}
		
/*
=================================================
	ImageAspectFlags
=================================================
*/
	ND_ inline vk::VkImageAspectFlags  Vk1Enum (EImageAspect::bits values)
	{
		using namespace vk;

		VkImageAspectFlags	flags = 0;

		FOR( i, values )
		{
			const auto t = EImageAspect::type(i);
			
			if ( not values[t] )
				continue;
			
			switch ( t )
			{
				case EImageAspect::Color	: flags |= VK_IMAGE_ASPECT_COLOR_BIT;		break;
				case EImageAspect::Depth	: flags |= VK_IMAGE_ASPECT_DEPTH_BIT;		break;
				case EImageAspect::Stencil	: flags |= VK_IMAGE_ASPECT_STENCIL_BIT;		break;
				case EImageAspect::Metadata	: flags |= VK_IMAGE_ASPECT_METADATA_BIT;	break;
				default						: RETURN_ERR( "invalid image aspect type", VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM );
			}
		}
		return flags;
	}
	
/*
=================================================
	Query
=================================================
*/
	ND_ inline vk::VkQueryType  Vk1Enum (EQuery::type value)
	{
		using namespace vk;

		switch ( value )
		{
			case EQuery::Occlusion :			return VK_QUERY_TYPE_OCCLUSION;
			case EQuery::PipelineStatistic :	return VK_QUERY_TYPE_PIPELINE_STATISTICS;
			case EQuery::Timestamp :			return VK_QUERY_TYPE_TIMESTAMP;
		}
		RETURN_ERR( "unknown query type!", VK_QUERY_TYPE_MAX_ENUM );
	}
	
/*
=================================================
	PipelineStatistic
=================================================
*/
	ND_ inline vk::VkQueryPipelineStatisticFlags  Vk1Enum (EPipelineStatistic::bits values)
	{
		using namespace vk;

		VkQueryPipelineStatisticFlags	flags = 0;

		FOR( i, values )
		{
			const auto t = EPipelineStatistic::type(i);
			
			if ( not values[t] )
				continue;
			
			switch ( t )
			{
				case EPipelineStatistic::InputAssemblyVertices :					flags |= VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT;						break;
				case EPipelineStatistic::TessellationControllShaderPatches :		flags |= VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT;			break;
				case EPipelineStatistic::InputAssembluPrimitives :					flags |= VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT;						break;
				case EPipelineStatistic::GeometryShaderPrimitives :					flags |= VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT;					break;
				case EPipelineStatistic::ClippingPrimitives :						flags |= VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT;							break;
				case EPipelineStatistic::VertexShaderInvocations :					flags |= VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT;						break;
				case EPipelineStatistic::GeometryShaderInvocations :				flags |= VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT;					break;
				case EPipelineStatistic::ClippingInvocations :						flags |= VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT;							break;
				case EPipelineStatistic::FragmentShaderInvocations :				flags |= VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT;					break;
				case EPipelineStatistic::TessellationEvaluationShaderInvocations :	flags |= VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT;	break;
				case EPipelineStatistic::ComputeShaderInvocations :					flags |= VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;					break;
				default :															RETURN_ERR( "invalid pipeline statistic type", VK_QUERY_PIPELINE_STATISTIC_FLAG_BITS_MAX_ENUM );
			}
		}
		return flags;
	}

/*
=================================================
	GpuObject
=================================================
*/
	ND_ inline vk::VkDebugReportObjectTypeEXT  Vk1Enum (EGpuObject::type value)
	{
		using namespace vk;

		switch ( value )
		{
			case EGpuObject::Instance :				return VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT;
			case EGpuObject::PhysicalDevice :		return VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT;
			case EGpuObject::Device :				return VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT;
			case EGpuObject::Queue :				return VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT;
			case EGpuObject::Semaphore :			return VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT;
			case EGpuObject::CommandBuffer :		return VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT;
			case EGpuObject::Fence :				return VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT;
			case EGpuObject::DeviceMemory :			return VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT;
			case EGpuObject::Buffer :				return VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT;
			case EGpuObject::Image :				return VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT;
			case EGpuObject::Event :				return VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT;
			case EGpuObject::QueryPool :			return VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT;
			case EGpuObject::BufferView :			return VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT;
			case EGpuObject::ImageView :			return VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT;
			case EGpuObject::ShaderModule :			return VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT;
			case EGpuObject::PipelineCache :		return VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT;
			case EGpuObject::PipelineLayout :		return VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT;
			case EGpuObject::RenderPass :			return VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT;
			case EGpuObject::Pipeline :				return VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT;
			case EGpuObject::Sampler :				return VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT;
			case EGpuObject::DescriptorSetLayout :	return VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT;
			case EGpuObject::DescriptorPool :		return VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT;
			case EGpuObject::DescriptorSet :		return VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT;
			case EGpuObject::Framebuffer :			return VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT;
			case EGpuObject::CommandPool :			return VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT;
			case EGpuObject::Surface :				return VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT;
			case EGpuObject::Swapchain :			return VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT;
			case EGpuObject::Display :				return VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_KHR_EXT;
			case EGpuObject::Query :				break;	// not supported
			case EGpuObject::VertexArray :			break;	// not supported
		}
		RETURN_ERR( "invalid object type", VK_DEBUG_REPORT_OBJECT_TYPE_MAX_ENUM_EXT );
	}

/*
=================================================
	Format
=================================================
*/
	ND_ inline vk::VkFormat  Vk1Enum (EVertexAttribute::type value)
	{
		using namespace vk;

		switch ( value )
		{
			case EVertexAttribute::Byte			: return VK_FORMAT_R8_SINT;
			case EVertexAttribute::Byte2		: return VK_FORMAT_R8G8_SINT;
			case EVertexAttribute::Byte3		: return VK_FORMAT_R8G8B8_SINT;
			case EVertexAttribute::Byte4		: return VK_FORMAT_R8G8B8A8_SINT;
			case EVertexAttribute::Byte_Norm	: return VK_FORMAT_R8_SNORM;
			case EVertexAttribute::Byte2_Norm	: return VK_FORMAT_R8G8_SNORM;
			case EVertexAttribute::Byte3_Norm	: return VK_FORMAT_R8G8B8_SNORM;
			case EVertexAttribute::Byte4_Norm	: return VK_FORMAT_R8G8B8A8_SNORM;
			case EVertexAttribute::UByte		: return VK_FORMAT_R8_UINT;
			case EVertexAttribute::UByte2		: return VK_FORMAT_R8G8_UINT;
			case EVertexAttribute::UByte3		: return VK_FORMAT_R8G8B8_UINT;
			case EVertexAttribute::UByte4		: return VK_FORMAT_R8G8B8A8_UINT;
			case EVertexAttribute::UByte_Norm	: return VK_FORMAT_R8_UNORM;
			case EVertexAttribute::UByte2_Norm	: return VK_FORMAT_R8G8_UNORM;
			case EVertexAttribute::UByte3_Norm	: return VK_FORMAT_R8G8B8_UNORM;
			case EVertexAttribute::UByte4_Norm	: return VK_FORMAT_R8G8B8A8_UNORM;
			case EVertexAttribute::Short		: return VK_FORMAT_R16_SINT;
			case EVertexAttribute::Short2		: return VK_FORMAT_R16G16_SINT;
			case EVertexAttribute::Short3		: return VK_FORMAT_R16G16B16_SINT;
			case EVertexAttribute::Short4		: return VK_FORMAT_R16G16B16A16_SINT;
			case EVertexAttribute::Short_Norm	: return VK_FORMAT_R16_SNORM;
			case EVertexAttribute::Short2_Norm	: return VK_FORMAT_R16G16_SNORM;
			case EVertexAttribute::Short3_Norm	: return VK_FORMAT_R16G16B16_SNORM;
			case EVertexAttribute::Short4_Norm	: return VK_FORMAT_R16G16B16A16_SNORM;
			case EVertexAttribute::UShort		: return VK_FORMAT_R16_UINT;
			case EVertexAttribute::UShort2		: return VK_FORMAT_R16G16_UINT;
			case EVertexAttribute::UShort3		: return VK_FORMAT_R16G16B16_UINT;
			case EVertexAttribute::UShort4		: return VK_FORMAT_R16G16B16A16_UINT;
			case EVertexAttribute::UShort_Norm	: return VK_FORMAT_R16_UNORM;
			case EVertexAttribute::UShort2_Norm	: return VK_FORMAT_R16G16_UNORM;
			case EVertexAttribute::UShort3_Norm	: return VK_FORMAT_R16G16B16_UNORM;
			case EVertexAttribute::UShort4_Norm	: return VK_FORMAT_R16G16B16A16_UNORM;
			case EVertexAttribute::Int			: return VK_FORMAT_R32_SINT;
			case EVertexAttribute::Int2			: return VK_FORMAT_R32G32_SINT;
			case EVertexAttribute::Int3			: return VK_FORMAT_R32G32B32_SINT;
			case EVertexAttribute::Int4			: return VK_FORMAT_R32G32B32A32_SINT;
			case EVertexAttribute::UInt			: return VK_FORMAT_R32_UINT;
			case EVertexAttribute::UInt2		: return VK_FORMAT_R32G32_UINT;
			case EVertexAttribute::UInt3		: return VK_FORMAT_R32G32B32_UINT;
			case EVertexAttribute::UInt4		: return VK_FORMAT_R32G32B32A32_UINT;
			case EVertexAttribute::Long			: return VK_FORMAT_R64_SINT;
			case EVertexAttribute::Long2		: return VK_FORMAT_R64G64_SINT;
			case EVertexAttribute::Long3		: return VK_FORMAT_R64G64B64_SINT;
			case EVertexAttribute::Long4		: return VK_FORMAT_R64G64B64A64_SINT;
			case EVertexAttribute::ULong		: return VK_FORMAT_R64_UINT;
			case EVertexAttribute::ULong2		: return VK_FORMAT_R64G64_UINT;
			case EVertexAttribute::ULong3		: return VK_FORMAT_R64G64B64_UINT;
			case EVertexAttribute::ULong4		: return VK_FORMAT_R64G64B64A64_UINT;
			case EVertexAttribute::Float		: return VK_FORMAT_R32_SFLOAT;
			case EVertexAttribute::Float2		: return VK_FORMAT_R32G32_SFLOAT;
			case EVertexAttribute::Float3		: return VK_FORMAT_R32G32B32_SFLOAT;
			case EVertexAttribute::Float4		: return VK_FORMAT_R32G32B32A32_SFLOAT;
			case EVertexAttribute::Double		: return VK_FORMAT_R64_SFLOAT;
			case EVertexAttribute::Double2		: return VK_FORMAT_R64G64_SFLOAT;
			case EVertexAttribute::Double3		: return VK_FORMAT_R64G64B64_SFLOAT;
			case EVertexAttribute::Double4		: return VK_FORMAT_R64G64B64A64_SFLOAT;
		}
		RETURN_ERR( "invalid attrib format", VK_FORMAT_MAX_ENUM );
	}

/*
=================================================
	Format
=================================================
*/
#	define VK_PIXEL_FORMATS( _builder_ ) \
		_builder_( Unknown,				VK_FORMAT_UNDEFINED ) \
		_builder_( RGBA16_SNorm,		VK_FORMAT_R16G16B16A16_SNORM ) \
		_builder_( RGBA8_SNorm,			VK_FORMAT_R8G8B8A8_SNORM ) \
		_builder_( RGB16_SNorm,			VK_FORMAT_R16G16B16_SNORM ) \
		_builder_( RGB8_SNorm,			VK_FORMAT_R8G8B8_SNORM ) \
		_builder_( RG16_SNorm,			VK_FORMAT_R16G16_SNORM ) \
		_builder_( RG8_SNorm,			VK_FORMAT_R8G8_SNORM ) \
		_builder_( R16_SNorm,			VK_FORMAT_R16_SNORM ) \
		_builder_( R8_SNorm,			VK_FORMAT_R8_SNORM ) \
		_builder_( RGBA16_UNorm,		VK_FORMAT_R16G16B16A16_UNORM ) \
		_builder_( RGBA8_UNorm,			VK_FORMAT_R8G8B8A8_UNORM ) \
		_builder_( RGB16_UNorm,			VK_FORMAT_R16G16B16_UNORM ) \
		_builder_( RGB8_UNorm,			VK_FORMAT_R8G8B8_UNORM ) \
		_builder_( RG16_UNorm,			VK_FORMAT_R16G16_UNORM ) \
		_builder_( RG8_UNorm,			VK_FORMAT_R8G8_UNORM ) \
		_builder_( R16_UNorm,			VK_FORMAT_R16_UNORM ) \
		_builder_( R8_UNorm,			VK_FORMAT_R8_UNORM ) \
		_builder_( RGB10_A2_UNorm,		VK_FORMAT_A2B10G10R10_UNORM_PACK32 ) \
		_builder_( RGBA4_UNorm,			VK_FORMAT_R4G4B4A4_UNORM_PACK16 ) \
		_builder_( RGB5_A1_UNorm,		VK_FORMAT_R5G5B5A1_UNORM_PACK16 ) \
		_builder_( RGB_5_6_5_UNorm,		VK_FORMAT_R5G6B5_UNORM_PACK16 ) \
		_builder_( BGR8_UNorm,			VK_FORMAT_B8G8R8_UNORM ) \
		_builder_( BGRA8_UNorm,			VK_FORMAT_B8G8R8A8_UNORM ) \
		_builder_( sRGB8,				VK_FORMAT_R8G8B8_SRGB ) \
		_builder_( sRGB8_A8,			VK_FORMAT_R8G8B8A8_SRGB ) \
		_builder_( R8I,					VK_FORMAT_R8_SINT ) \
		_builder_( RG8I,				VK_FORMAT_R8G8_SINT ) \
		_builder_( RGB8I,				VK_FORMAT_R8G8B8_SINT ) \
		_builder_( RGBA8I,				VK_FORMAT_R8G8B8A8_SINT ) \
		_builder_( R16I,				VK_FORMAT_R16_SINT ) \
		_builder_( RG16I,				VK_FORMAT_R16G16_SINT ) \
		_builder_( RGB16I,				VK_FORMAT_R16G16B16_SINT ) \
		_builder_( RGBA16I,				VK_FORMAT_R16G16B16A16_SINT ) \
		_builder_( R32I,				VK_FORMAT_R32_SINT ) \
		_builder_( RG32I,				VK_FORMAT_R32G32_SINT ) \
		_builder_( RGB32I,				VK_FORMAT_R32G32B32_SINT ) \
		_builder_( RGBA32I,				VK_FORMAT_R32G32B32A32_UINT ) \
		_builder_( R8U,					VK_FORMAT_R8_UINT ) \
		_builder_( RG8U,				VK_FORMAT_R8G8_UINT ) \
		_builder_( RGB8U,				VK_FORMAT_R8G8B8_UINT ) \
		_builder_( RGBA8U,				VK_FORMAT_R8G8B8A8_UINT ) \
		_builder_( R16U,				VK_FORMAT_R16_UINT ) \
		_builder_( RG16U,				VK_FORMAT_R16G16_UINT ) \
		_builder_( RGB16U,				VK_FORMAT_R16G16B16_UINT ) \
		_builder_( RGBA16U,				VK_FORMAT_R16G16B16A16_UINT ) \
		_builder_( R32U,				VK_FORMAT_R32_UINT ) \
		_builder_( RG32U,				VK_FORMAT_R32G32_UINT ) \
		_builder_( RGB32U,				VK_FORMAT_R32G32B32_UINT ) \
		_builder_( RGBA32U,				VK_FORMAT_R32G32B32A32_SINT ) \
		_builder_( RGB10_A2U,			VK_FORMAT_A2B10G10R10_UINT_PACK32 ) \
		_builder_( R16F,				VK_FORMAT_R16_SFLOAT ) \
		_builder_( RG16F,				VK_FORMAT_R16G16_SFLOAT ) \
		_builder_( RGB16F,				VK_FORMAT_R16G16B16_SFLOAT ) \
		_builder_( RGBA16F,				VK_FORMAT_R16G16B16A16_SFLOAT ) \
		_builder_( R32F,				VK_FORMAT_R32_SFLOAT ) \
		_builder_( RG32F,				VK_FORMAT_R32G32_SFLOAT ) \
		_builder_( RGB32F,				VK_FORMAT_R32G32B32_SFLOAT ) \
		_builder_( RGBA32F,				VK_FORMAT_R32G32B32A32_SFLOAT ) \
		_builder_( RGB_11_11_10F,		VK_FORMAT_B10G11R11_UFLOAT_PACK32 ) \
		_builder_( Depth16,				VK_FORMAT_D16_UNORM ) \
		_builder_( Depth24,				VK_FORMAT_X8_D24_UNORM_PACK32 ) \
		/*_builder_( Depth32,			VK_FORMAT_D32_SFLOAT ) */ \
		_builder_( Depth32F,			VK_FORMAT_D32_SFLOAT ) \
		_builder_( Depth16_Stencil8,	VK_FORMAT_D16_UNORM_S8_UINT ) \
		_builder_( Depth24_Stencil8,	VK_FORMAT_D24_UNORM_S8_UINT ) \
		_builder_( Depth32F_Stencil8,	VK_FORMAT_D32_SFLOAT_S8_UINT ) \
		_builder_( BC1_RGB8_UNorm,		VK_FORMAT_BC1_RGB_UNORM_BLOCK ) \
		_builder_( BC1_RGB8_A1_UNorm,	VK_FORMAT_BC1_RGBA_UNORM_BLOCK ) \
		_builder_( BC2_RGBA8_UNorm,		VK_FORMAT_BC2_UNORM_BLOCK ) \
		_builder_( BC3_RGBA8_UNorm,		VK_FORMAT_BC3_UNORM_BLOCK ) \
		_builder_( BC4_RED8_SNorm,		VK_FORMAT_BC4_SNORM_BLOCK ) \
		_builder_( BC4_RED8_UNorm,		VK_FORMAT_BC4_UNORM_BLOCK ) \
		_builder_( BC5_RG8_SNorm,		VK_FORMAT_BC5_SNORM_BLOCK ) \
		_builder_( BC5_RG8_UNorm,		VK_FORMAT_BC5_UNORM_BLOCK ) \
		_builder_( BC7_RGBA8_UNorm,		VK_FORMAT_BC7_UNORM_BLOCK ) \
		_builder_( BC7_SRGB8_A8_UNorm,	VK_FORMAT_BC7_SRGB_BLOCK ) \
		_builder_( BC6H_RGB16F,			VK_FORMAT_BC6H_SFLOAT_BLOCK ) \
		_builder_( BC6H_RGB16F_Unsigned,VK_FORMAT_BC6H_UFLOAT_BLOCK ) \
		_builder_( ETC2_RGB8_UNorm,		VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK ) \
		_builder_( ECT2_SRGB8_UNorm,	VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK ) \
		_builder_( ETC2_RGB8_A1_UNorm,	VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK ) \
		_builder_( ETC2_SRGB8_A1_UNorm,	VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK ) \
		_builder_( ETC2_RGBA8_UNorm,	VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK ) \
		_builder_( ETC2_SRGB8_A8_UNorm,	VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK ) \
		_builder_( EAC_R11_SNorm,		VK_FORMAT_EAC_R11_SNORM_BLOCK ) \
		_builder_( EAC_R11_UNorm,		VK_FORMAT_EAC_R11_UNORM_BLOCK ) \
		_builder_( EAC_RG11_SNorm,		VK_FORMAT_EAC_R11G11_SNORM_BLOCK ) \
		_builder_( EAC_RG11_UNorm,		VK_FORMAT_EAC_R11G11_UNORM_BLOCK ) \
		_builder_( ASTC_RGBA_4x4,		VK_FORMAT_ASTC_4x4_UNORM_BLOCK ) \
		_builder_( ASTC_RGBA_5x4,		VK_FORMAT_ASTC_5x4_UNORM_BLOCK ) \
		_builder_( ASTC_RGBA_5x5,		VK_FORMAT_ASTC_5x5_UNORM_BLOCK ) \
		_builder_( ASTC_RGBA_6x5,		VK_FORMAT_ASTC_6x5_UNORM_BLOCK ) \
		_builder_( ASTC_RGBA_6x6,		VK_FORMAT_ASTC_6x6_UNORM_BLOCK ) \
		_builder_( ASTC_RGBA_8x5,		VK_FORMAT_ASTC_8x5_UNORM_BLOCK ) \
		_builder_( ASTC_RGBA_8x6,		VK_FORMAT_ASTC_8x6_UNORM_BLOCK ) \
		_builder_( ASTC_RGBA_8x8,		VK_FORMAT_ASTC_8x8_UNORM_BLOCK ) \
		_builder_( ASTC_RGBA_10x5,		VK_FORMAT_ASTC_10x5_UNORM_BLOCK ) \
		_builder_( ASTC_RGBA_10x6,		VK_FORMAT_ASTC_10x6_UNORM_BLOCK ) \
		_builder_( ASTC_RGBA_10x8,		VK_FORMAT_ASTC_10x8_UNORM_BLOCK ) \
		_builder_( ASTC_RGBA_10x10,		VK_FORMAT_ASTC_10x10_UNORM_BLOCK ) \
		_builder_( ASTC_RGBA_12x10,		VK_FORMAT_ASTC_12x10_UNORM_BLOCK ) \
		_builder_( ASTC_RGBA_12x12,		VK_FORMAT_ASTC_12x12_UNORM_BLOCK ) \
		_builder_( ASTC_SRGB8_A8_4x4,	VK_FORMAT_ASTC_4x4_SRGB_BLOCK ) \
		_builder_( ASTC_SRGB8_A8_5x4,	VK_FORMAT_ASTC_5x4_SRGB_BLOCK ) \
		_builder_( ASTC_SRGB8_A8_5x5,	VK_FORMAT_ASTC_5x5_SRGB_BLOCK ) \
		_builder_( ASTC_SRGB8_A8_6x5,	VK_FORMAT_ASTC_6x5_SRGB_BLOCK ) \
		_builder_( ASTC_SRGB8_A8_6x6,	VK_FORMAT_ASTC_6x6_SRGB_BLOCK ) \
		_builder_( ASTC_SRGB8_A8_8x5,	VK_FORMAT_ASTC_8x5_SRGB_BLOCK ) \
		_builder_( ASTC_SRGB8_A8_8x6,	VK_FORMAT_ASTC_8x6_SRGB_BLOCK ) \
		_builder_( ASTC_SRGB8_A8_8x8,	VK_FORMAT_ASTC_8x8_SRGB_BLOCK ) \
		_builder_( ASTC_SRGB8_A8_10x5,	VK_FORMAT_ASTC_10x5_SRGB_BLOCK ) \
		_builder_( ASTC_SRGB8_A8_10x6,	VK_FORMAT_ASTC_10x6_SRGB_BLOCK ) \
		_builder_( ASTC_SRGB8_A8_10x8,	VK_FORMAT_ASTC_10x8_SRGB_BLOCK ) \
		_builder_( ASTC_SRGB8_A8_10x10,	VK_FORMAT_ASTC_10x10_SRGB_BLOCK ) \
		_builder_( ASTC_SRGB8_A8_12x10,	VK_FORMAT_ASTC_12x10_SRGB_BLOCK ) \
		_builder_( ASTC_SRGB8_A8_12x12,	VK_FORMAT_ASTC_12x12_SRGB_BLOCK )
			
/*
=================================================
	Format
=================================================
*/
	ND_ inline vk::VkFormat  Vk1Enum (EPixelFormat::type value)
	{
		using namespace vk;

#		define FMT_BUILDER( _engineFmt_, _vkFormat_ ) \
			case EPixelFormat::_engineFmt_ : return _vkFormat_;

		switch ( value )
		{
			VK_PIXEL_FORMATS( FMT_BUILDER )
		}

#		undef FMT_BUILDER

		RETURN_ERR( "invalid pixel format", VK_FORMAT_MAX_ENUM );
	}

/*
=================================================
	Format
=================================================
*/
	ND_ inline EPixelFormat::type  Vk1Enum (vk::VkFormat value)
	{
		using namespace vk;
		
#		define FMT_BUILDER( _engineFmt_, _vkFormat_ ) \
			case _vkFormat_ : return EPixelFormat::_engineFmt_;

		switch ( value )
		{
			VK_PIXEL_FORMATS( FMT_BUILDER )
		}

#		undef FMT_BUILDER

		RETURN_ERR( "invalid pixel format" );
	}

#	undef VK_PIXEL_FORMATS


}	// PlatformVK
}	// Engine

#endif	// GRAPHICS_API_VULKAN
