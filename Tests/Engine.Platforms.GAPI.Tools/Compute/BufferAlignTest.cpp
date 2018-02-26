
#ifdef __cplusplus
#include "Engine/PipelineCompiler/Pipelines/ComputePipeline.h"

using namespace PipelineCompiler;

DECL_PIPELINE( BufferAlignTest, ComputePipeline,
{
	shader.Load( __FILE__ );

	shaderFormat = EShaderSrcFormat::GXSL;
})
#endif


#ifdef SHADER
#if SHADER & SH_COMPUTE

struct BufferAlign_Struct
{
	int4		i4;
	float2		f2;
	bool		b1;
	uint3		u3;
	//ulong2	l2;
	//double2	d2;
	int			i1;
	bool3		b3;
	//ulong3	l3;
	//double3	d3;
	float		f1;
};

layout(binding=0, std140) buffer BufferAlign_SSBO
{
	readonly BufferAlign_Struct		src;
	writeonly BufferAlign_Struct	dst1;
	writeonly BufferAlign_Struct	dst2;

} ssb;

void main ()
{
	ssb.dst1 = ssb.src;

	ssb.dst2.i4 = int4( 1, -2, 3, -4 );
	ssb.dst2.f2 = float2( 3.1, 5.5 );
	ssb.dst2.b1 = true;
	ssb.dst2.u3 = uint3( 9, 8, 7 );
	//ssb.dst2.l2 = ulong2( 7823493482ul, 9189183109ul );
	//ssb.dst2.d2 = double2( 4.987, 9.1234 );
	ssb.dst2.i1 = 0x123456;
	ssb.dst2.b3 = bool3( false, true, false );
	//ssb.dst2.d3 = double3( 5.543, 2.655e+10, -7.2345 );
	//ssb.dst2.l3 = ulong3( 8349234892ul, 172381ul, 98510910293i12ul );
	ssb.dst2.f1 = 1.4335;
}

#endif
#endif	// SHADER
