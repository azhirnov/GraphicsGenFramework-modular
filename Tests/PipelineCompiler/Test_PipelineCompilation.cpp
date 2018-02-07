// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "Common.h"

using namespace PipelineCompiler;


extern bool Test_PipelineCompilation ()
{
	PipelineManager::ConverterConfig	cfg;
	
	cfg.searchForSharedTypes	= true;
	cfg.addPaddingToStructs		= true;
	cfg.optimizeSource			= false;
	//cfg.optimizeBindings		= false;
	cfg.includings				<< "common.h";
	cfg.nameSpace				= "Pipelines";
	//cfg.target					|= EShaderDstFormat::GLSL_Source;
	//cfg.target					|= EShaderDstFormat::GLSL_Binary;
	//cfg.target					|= EShaderDstFormat::SPIRV_Binary;
	//cfg.target				|= EShaderDstFormat::SPIRV_Source;	// disassembled spirv
	//cfg.target					|= EShaderDstFormat::CL_Source;
	//cfg.target					|= EShaderDstFormat::CL_Binary;
	cfg.target					|= EShaderDstFormat::CPP_Module;

	PipelineManager::Instance()->Convert( "out/all_pipelines", new CppSerializer(), cfg );

	ShaderCompiler::Instance()->DestroyContext();
	return true;
}
