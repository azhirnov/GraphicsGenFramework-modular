// Copyright �  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "Engine/Platforms/Shared/GPU/RenderState.h"
#include "Engine/Platforms/Shared/GPU/VertexAttribs.h"
#include "Engine/Platforms/Shared/GPU/VertexInputState.h"
#include "Engine/Platforms/Shared/GPU/FragmentOutputState.h"
#include "Engine/Platforms/Shared/GPU/PipelineLayout.h"
#include "Engine/Platforms/Shared/GPU/IDs.h"

namespace SWShaderLang
{
	class SWShaderHelper;
}

namespace Engine
{
namespace Platforms
{

	//
	// Graphics Pipeline Descriptor
	//

	struct _ENGINE_PLATFORMS_EXPORT_ GraphicsPipelineDescriptor final : CompileTime::FastCopyable
	{
	// variables
		VertexInputState				vertexInput;
		RenderState						renderState;
		EPipelineDynamicState::bits		dynamicStates;
		FragmentOutputState				fragOutput;
		PipelineLayoutDescriptor		layout;
		uint							patchControlPoints;
		uint							subpass;
		// TODO: specialization constants, viewports

	// methods
		GraphicsPipelineDescriptor (GX_DEFCTOR);

		GraphicsPipelineDescriptor (const VertexInputState &vertexInput,
									const RenderState &renderState,
									const PipelineLayoutDescriptor &layout,
									EPipelineDynamicState::bits dynamicStates,
									const FragmentOutputState &fragOutput,
									uint patchControlPoints = 0,
									uint subpass = 0);
	};



	//
	// Compute Pipeline Descriptor
	//

	struct _ENGINE_PLATFORMS_EXPORT_ ComputePipelineDescriptor final : CompileTime::FastCopyable
	{
	// variables
		PipelineLayoutDescriptor		layout;
		uint3							localGroupSize;
		// TODO: specialization constants

	// methods
		ComputePipelineDescriptor (GX_DEFCTOR);

		explicit
		ComputePipelineDescriptor (const PipelineLayoutDescriptor &layout,
								   const uint3 &localGroupSize);
	};



	//
	// Pipeline Template Descriptor
	//

	struct _ENGINE_PLATFORMS_EXPORT_ PipelineTemplateDescriptor final : CompileTime::FastCopyable
	{
	// types
		struct _ENGINE_PLATFORMS_EXPORT_ ShaderSource
		{
		// types
			struct ESource { enum type {
				GLSL,
				SPIRV,
				OpenCL,
				SoftRenderer,
				_Count
			}; };

			using SWInvoke	= void (*) (const SWShaderLang::SWShaderHelper &);
			using Data		= Union< uint, String, BinaryArray, Array<uint>, SWInvoke >;
			using Sources	= StaticArray< Data, ESource::_Count >;
			
		// variables
			Sources		src;

		// methods
			ShaderSource () {}

			void StringGLSL (StringCRef data);
			void StringSPIRV (StringCRef data);
			void StringCL (StringCRef data);
			void ArraySPIRV (ArrayCRef<uint> data);
			bool FileSPIRV (const RFilePtr &file);
			void FuncSW (const SWInvoke &func);

			StringCRef		GetGLSL ()	const;
			ArrayCRef<uint>	GetSPIRV ()	const;
			StringCRef		GetCL ()	const;
			SWInvoke		GetSW ()	const;
		};

		using Sources = StaticArray< ShaderSource, EShader::_Count >;


	// variables
		Sources							shaders;
		VertexAttribs					attribs;
		RenderState						renderState;
		EPipelineDynamicState::bits		dynamicStates;
		FragmentOutputState				fragOutput;
		PipelineLayoutDescriptor		layout;
		uint							patchControlPoints;
		uint							subpass;
		uint3							localGroupSize;
		EPrimitive::bits				supportedPrimitives;
		EShader::bits					supportedShaders;


	// methods
		PipelineTemplateDescriptor (GX_DEFCTOR);

		ShaderSource& Vertex ()				{ return shaders[ EShader::Vertex ]; }
		ShaderSource& TessControl ()		{ return shaders[ EShader::TessControl ]; }
		ShaderSource& TessEvaluation ()		{ return shaders[ EShader::TessEvaluation ]; }
		ShaderSource& Geometry ()			{ return shaders[ EShader::Geometry ]; }
		ShaderSource& Fragment ()			{ return shaders[ EShader::Fragment ]; }
		ShaderSource& Compute ()			{ return shaders[ EShader::Compute ]; }
	};

}	// Platforms


namespace CreateInfo
{

	//
	// Graphics Pipeline Create Info
	//
	struct GraphicsPipeline
	{
	// types
		using Descriptor	= Platforms::GraphicsPipelineDescriptor;

	// variables
		ModulePtr		gpuThread;
		Descriptor		descr;
		ModulePtr		shaders;
		ModulePtr		renderPass;
	};


	//
	// Compute Pipeline Create Info
	//
	struct ComputePipeline
	{
	// types
		using Descriptor	= Platforms::ComputePipelineDescriptor;
		
	// variables
		ModulePtr		gpuThread;
		Descriptor		descr;
		ModulePtr		shaders;
	};


	//
	// Pipeline Template Create Info
	//
	struct PipelineTemplate
	{
	// types
		using Descriptor	= Platforms::PipelineTemplateDescriptor;

	// variables
		Descriptor		descr;
	};


	//
	// Pipeline Resource Table Create Info
	//
	struct PipelineResourceTable
	{
	// variables
		ModulePtr		gpuThread;

	// methods
		//PipelineResourceTable () {}
	};

}	// CreateInfo


namespace GpuMsg
{
	//
	// Get Graphics Pipeline Descriptor
	//
	struct GetGraphicsPipelineDescriptor
	{
		Out< Platforms::GraphicsPipelineDescriptor >	result;
	};


	//
	// Get Compute Pipeline Descriptor
	//
	struct GetComputePipelineDescriptor
	{
		Out< Platforms::ComputePipelineDescriptor >		result;
	};


	//
	// Get Pipeline Layout Descriptor
	//
	struct GetPipelineLayoutDescriptor
	{
		Out< Platforms::PipelineLayoutDescriptor >	result;
	};


	//
	// Update Resources in Pipeline Resource Table
	//
	struct PipelineResourceTableUpdate
	{};

	
	//
	// Create GraphicsPipelineDescriptor from PipelineTemplate
	//
	struct CreateGraphicsPipelineDescriptor
	{
	// variables
		Platforms::VertexInputState						vertexInput;
		Platforms::EPrimitive::type						topology	= Uninitialized;
		Out< Platforms::GraphicsPipelineDescriptor >	result;

	// methods
		CreateGraphicsPipelineDescriptor (const Platforms::VertexInputState &vertexInput, Platforms::EPrimitive::type topology) :
			vertexInput(vertexInput), topology(topology)
		{}
	};


	//
	// Create ComputePipelineDescriptor from PipelineTemplate
	//
	struct CreateComputePipelineDescriptor
	{
		Out< Platforms::ComputePipelineDescriptor >		result;
	};


	//
	// Create GraphicsPipeline Module from PipelineTemplate
	//
	struct CreateGraphicsPipeline
	{
	// variables
		Out< ModulePtr >				result;
		UntypedID_t						moduleID	= 0;
		ModulePtr						gpuThread;
		ModulePtr						renderPass;
		Platforms::VertexInputState		vertexInput;
		Platforms::EPrimitive::type		topology	= Uninitialized;

	// methods
		CreateGraphicsPipeline (GX_DEFCTOR) {}

		CreateGraphicsPipeline (UntypedID_t moduleID,
								const ModulePtr &gpuThread,
								const ModulePtr &renderPass,
								const Platforms::VertexInputState &vertexInput,
								Platforms::EPrimitive::type topology) :
			moduleID(moduleID),			gpuThread(gpuThread),
			renderPass(renderPass),		vertexInput(vertexInput),
			topology(topology)
		{}
	};


	//
	// Create ComputePipeline Module from PipelineTemplate
	//
	struct CreateComputePipeline
	{
	// variables
		Out< ModulePtr >		result;
		UntypedID_t				moduleID	= 0;
		ModulePtr				gpuThread;
		
	// methods
		CreateComputePipeline (GX_DEFCTOR) {}

		explicit CreateComputePipeline (UntypedID_t moduleID,
										const ModulePtr &gpuThread = null) :
			moduleID(moduleID),		gpuThread(gpuThread)
		{}
	};


}	// GpuMsg
}	// Engine
