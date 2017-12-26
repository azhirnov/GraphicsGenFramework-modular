// Copyright �  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "Engine/PipelineCompiler/Pipelines/GraphicsPipeline.h"
using namespace PipelineCompiler;


//
// Shader Toy Template
//

class ShadertoyTemplate : public GraphicsPipeline
{
protected:
	explicit ShadertoyTemplate (StringCRef name) : GraphicsPipeline( name )
	{
		shaderFormat = EShaderSrcFormat::GLSL;

		shaders.vertex.Source( R"#(
const float2	g_Positions[] = {
{ -1.0,  1.0 },
{ -1.0, -1.0 },
{  1.0,  1.0 },
{  1.0, -1.0 }
};

void main() {
	gl_Position	= float4( g_Positions[gl_VertexIndex], 0.0, 1.0 );
}
		)#");

		shaders.fragment.Source( R"#(
layout(std140) uniform ShadertoyUB
{
	float3	iResolution;			// viewport resolution (in pixels)
	float	iTime;					// shader playback time (in seconds)
	float	iTimeDelta;				// render time (in seconds)
	int		iFrame;					// shader playback frame
	float	iChannelTime[4];		// channel playback time (in seconds)
	float3	iChannelResolution[4];	// channel resolution (in pixels)
	float4	iMouse;					// mouse pixel coords. xy: current (if MLB down), zw: click
	float4	iDate;					// (year, month, day, time in seconds)
	float	iSampleRate;			// sound sample rate (i.e., 44100)
};

// input channel, TODO: 2D and Cube support
//uniform sampler2D	iChannel0;
//uniform sampler2D	iChannel1;
//uniform sampler2D	iChannel2;
//uniform sampler2D	iChannel3;

layout(location=0) out float4	out_Color0;

void mainImage (out float4 fragColor, in float2 fragCoord);

void main ()
{
	mainImage( out_Color0, gl_FragCoord.xy );
}
		)#");

		renderState.inputAssembly.topology	= EPrimitive::TriangleStrip;
		dynamicStates = EPipelineDynamicState::Viewport | EPipelineDynamicState::Scissor;
	}
};