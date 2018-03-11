
#ifdef __cplusplus
#include "Engine/PipelineCompiler/Pipelines/ComputePipeline.h"

using namespace PipelineCompiler;

DECL_PIPELINE( SharedMemoryTest, ComputePipeline,
{
	shader.Load( __FILE__ );

	shaderFormat = EShaderSrcFormat::GXSL;

	//localGroupSize = uint3( 16, 16, 1 );
})
#endif


#ifdef SHADER
#if SHADER & SH_COMPUTE

layout (local_size_x=16, local_size_y=16, local_size_z=1) in;

layout(rgba32f)	readonly  uniform image2D  un_SrcImage;
layout(rgba32f)	writeonly uniform image2D  un_DstImage;

float MaxMag (const float a, const float b)
{
	if ( abs(a) > abs(b) )	return a;
	if ( abs(a) < abs(b) )	return b;
	return max( a, b );
}

float4 MaxMag (const float4 a, const float4 b)
{
	return float4(
			MaxMag( a.x, b.x ),
			MaxMag( a.y, b.y ),
			MaxMag( a.z, b.z ),
			MaxMag( a.w, b.w )
		);
}

shared float4 sharedMemory[gl_WorkGroupSize.x*gl_WorkGroupSize.y];

void main ()
{
	int2 coord = int3(gl_LocalInvocationID).xy;
	int idx = int(gl_LocalInvocationIndex);

	sharedMemory[idx] = imageLoad(un_SrcImage, coord);

	barrier();

	float4 value = float4(0.0);

	for (int y = 0; y < gl_WorkGroupSize.y; ++y)
	{
		for (int x = 0; x < gl_WorkGroupSize.x; ++x)
		{
			value = MaxMag( value, sharedMemory[x + y * gl_WorkGroupSize.x] );
		}
	}

	imageStore(un_DstImage, coord, value);
}

#endif
#endif	// SHADER
