// This is generated file
// Created at: 2018/04/26 - 11:17:14

#pragma once

#include "Engine/Platforms/Engine.Platforms.h"

#ifdef GRAPHICS_API_SOFT
#include "Engine/Platforms/Soft/ShaderLang/SWLang.h"
#endif

namespace Pipelines
{
	using namespace GX_STL;
	using namespace GX_STL::GXTypes;
	using namespace GX_STL::GXMath;

	using namespace Engine::Platforms;
}

#include "shared_types.h"

namespace Pipelines
{
// From file 'default2.cpp'
void Create_default2 (PipelineTemplateDescriptor& descr);

// From file 'defaultcompute2.cpp'
void Create_defaultcompute2 (PipelineTemplateDescriptor& descr);

// From file 'default.cpp'
void Create_default (PipelineTemplateDescriptor& descr);

};
