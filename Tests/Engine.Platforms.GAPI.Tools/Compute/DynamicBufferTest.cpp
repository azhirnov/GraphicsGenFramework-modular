
#ifdef __cplusplus
#include "Engine/PipelineCompiler/Pipelines/ComputePipeline.h"

using namespace PipelineCompiler;

DECL_PIPELINE( DynamicBufferTest, ComputePipeline,
{
	shader.Load( __FILE__ );

	shaderFormat = EShaderSrcFormat::GXSL;
})
#endif


#ifdef SHADER
#if SHADER & SH_COMPUTE

struct DynamicBuffer_Struct
{
	int2	i2;
	bool	b1;
	float2	f2;
	int3	i3;
	bool2	b2;
};

void SomeFunc (DynamicBuffer_Struct arr[], const float2 f2, const int4 i4)
{
	int idx = int(gl_GlobalInvocationID.x);

	arr[idx].i2 += int2(i4.x, idx);
	arr[idx].b1 = ((idx & 1) == 0);
	arr[idx].f2 -= float2(f2);
	arr[idx].i3.xy *= 2;
	arr[idx].i3.z = ~arr[idx].i3.z;
	arr[idx].b2 = not(arr[idx].b2);
}

layout(binding=0, std140) coherent buffer DynamicBuffer_SSBO
{
	// static part
	float2	f2;
	int4	i4;

	// dynamic part
	DynamicBuffer_Struct	arr[];

} ssb;

void main ()
{
	SomeFunc(ssb.arr, ssb.f2, ssb.i4);
}

#endif
#endif	// SHADER
