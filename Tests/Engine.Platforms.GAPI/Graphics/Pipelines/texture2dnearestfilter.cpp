// This is generated file
// Origin file: 'Graphics/Pipelines/Texture2DNearestFilter.ppln'
#include "all_pipelines.h"
// C++ shader
#ifdef GRAPHICS_API_SOFT
namespace SWShaderLang {
namespace {

#	define INOUT
#	define IN
#	define OUT

	static void sw_texture2dnearestfilter_comp (const Impl::SWShaderHelper &_helper_)
	{
		// prepare externals
		Impl::Texture2D< vec4 >  un_SrcTexture;    _helper_.GetTexture( 0, un_SrcTexture );
		Impl::Image2D< vec4, Impl::EStorageAccess::WriteOnly >  un_DstImage;    _helper_.GetImage( 1, un_DstImage );
		auto& gl_GlobalInvocationID = _helper_.GetComputeShaderState().inGlobalInvocationID;
		auto& gl_NumWorkGroups = _helper_.GetComputeShaderState().inNumWorkGroups;
	
		// shader
		{
			Int2 coord = Int3(gl_GlobalInvocationID).xy;
			;
			Int2 size = Int3((gl_NumWorkGroups * UInt3(1u))).xy;
			;
			Float2 point = (Float2(coord) / Float2((size - Int(1))));
			;
			Float4 color = textureOffset(un_SrcTexture, point, Int2(-2, 0));
			;
			imageStore(un_DstImage, coord, color);
		}
	}
	
}		// anonymous namespace
}		// SWShaderLang
#endif	// GRAPHICS_API_SOFT


namespace Pipelines
{

void Create_texture2dnearestfilter (PipelineTemplateDescriptor& descr)
{
	descr = PipelineTemplateDescriptor();
	descr.supportedShaders = EShader::Compute;

	descr.localGroupSize = uint3(1, 1, 1);
	descr.layout = PipelineLayoutDescriptor::Builder()
			.AddTexture( "un_SrcTexture", EImage::Tex2D, EPixelFormatClass::AnyColorChannels | EPixelFormatClass::LinearColorSpace | EPixelFormatClass::AnyFloat | EPixelFormatClass::AnyNorm, 0u, 0u, EShader::Compute )
			.AddImage( "un_DstImage", EImage::Tex2D, EPixelFormat::RGBA8_UNorm, EShaderMemoryModel::WriteOnly, 0u, 1u, EShader::Compute )
			.Finish();

	descr.Compute().StringGLSL( 
R"#(#version 450 core
layout (local_size_x=1, local_size_y=1, local_size_z=1) in;

layout(binding=0) uniform sampler2D un_SrcTexture;
layout(binding=0) layout(rgba8) writeonly uniform image2D un_DstImage;

//---------------------------------

void main ()
{
	ivec2 coord = ivec3( gl_GlobalInvocationID ).xy;
	;
	ivec2 size = ivec3( (gl_NumWorkGroups * uvec3( 1u )) ).xy;
	;
	vec2 point = (vec2( coord ) / vec2( (size - int( 1 )) ));
	;
	vec4 color = textureOffset( un_SrcTexture, point, ivec2( -2, 0 ) );
	;
	imageStore( un_DstImage, coord, color );
}


)#"_str );
	descr.Compute().ArraySPIRV({ 
0x07230203, 0x00010000, 0x00080006, 0x0000003B, 0x00000000, 0x00020011, 0x00000001, 0x0006000B, 0x00000002, 0x4C534C47, 0x6474732E, 0x3035342E, 
0x00000000, 0x0003000E, 0x00000000, 0x00000001, 0x0007000F, 0x00000005, 0x00000005, 0x6E69616D, 0x00000000, 0x0000000E, 0x00000014, 0x00060010, 
0x00000005, 0x00000011, 0x00000001, 0x00000001, 0x00000001, 0x00030007, 0x00000001, 0x00000000, 0x002D0003, 0x00000002, 0x000001C2, 0x00000001, 
0x4F202F2F, 0x646F4D70, 0x50656C75, 0x65636F72, 0x64657373, 0x746E6520, 0x702D7972, 0x746E696F, 0x69616D20, 0x2F2F0A6E, 0x4D704F20, 0x6C75646F, 
0x6F725065, 0x73736563, 0x63206465, 0x6E65696C, 0x706F2074, 0x6C676E65, 0x0A303031, 0x4F202F2F, 0x646F4D70, 0x50656C75, 0x65636F72, 0x64657373, 
0x72617420, 0x2D746567, 0x20766E65, 0x6E65706F, 0x2F0A6C67, 0x704F202F, 0x75646F4D, 0x7250656C, 0x7365636F, 0x20646573, 0x72746E65, 0x6F702D79, 
0x20746E69, 0x6E69616D, 0x696C230A, 0x3120656E, 0x0000000A, 0x00040005, 0x00000005, 0x6E69616D, 0x00000000, 0x00040005, 0x0000000A, 0x726F6F63, 
0x00000064, 0x00080005, 0x0000000E, 0x475F6C67, 0x61626F6C, 0x766E496C, 0x7461636F, 0x496E6F69, 0x00000044, 0x00040005, 0x00000013, 0x657A6973, 
0x00000000, 0x00070005, 0x00000014, 0x4E5F6C67, 0x6F576D75, 0x72476B72, 0x7370756F, 0x00000000, 0x00040005, 0x0000001E, 0x6E696F70, 0x00000074, 
0x00040005, 0x00000029, 0x6F6C6F63, 0x00000072, 0x00060005, 0x0000002D, 0x535F6E75, 0x65546372, 0x72757478, 0x00000065, 0x00050005, 0x00000037, 
0x445F6E75, 0x6D497473, 0x00656761, 0x00040047, 0x0000000E, 0x0000000B, 0x0000001C, 0x00040047, 0x00000014, 0x0000000B, 0x00000018, 0x00040047, 
0x0000002D, 0x00000022, 0x00000000, 0x00040047, 0x0000002D, 0x00000021, 0x00000000, 0x00040047, 0x00000037, 0x00000022, 0x00000000, 0x00040047, 
0x00000037, 0x00000021, 0x00000001, 0x00030047, 0x00000037, 0x00000019, 0x00020013, 0x00000003, 0x00030021, 0x00000004, 0x00000003, 0x00040015, 
0x00000007, 0x00000020, 0x00000001, 0x00040017, 0x00000008, 0x00000007, 0x00000002, 0x00040020, 0x00000009, 0x00000007, 0x00000008, 0x00040015, 
0x0000000B, 0x00000020, 0x00000000, 0x00040017, 0x0000000C, 0x0000000B, 0x00000003, 0x00040020, 0x0000000D, 0x00000001, 0x0000000C, 0x0004003B, 
0x0000000D, 0x0000000E, 0x00000001, 0x00040017, 0x00000010, 0x00000007, 0x00000003, 0x0004003B, 0x0000000D, 0x00000014, 0x00000001, 0x0004002B, 
0x0000000B, 0x00000016, 0x00000001, 0x0006002C, 0x0000000C, 0x00000017, 0x00000016, 0x00000016, 0x00000016, 0x00030016, 0x0000001B, 0x00000020, 
0x00040017, 0x0000001C, 0x0000001B, 0x00000002, 0x00040020, 0x0000001D, 0x00000007, 0x0000001C, 0x0004002B, 0x00000007, 0x00000022, 0x00000001, 
0x00040017, 0x00000027, 0x0000001B, 0x00000004, 0x00040020, 0x00000028, 0x00000007, 0x00000027, 0x00090019, 0x0000002A, 0x0000001B, 0x00000001, 
0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x0003001B, 0x0000002B, 0x0000002A, 0x00040020, 0x0000002C, 0x00000000, 0x0000002B, 
0x0004003B, 0x0000002C, 0x0000002D, 0x00000000, 0x0004002B, 0x00000007, 0x00000030, 0xFFFFFFFE, 0x0004002B, 0x00000007, 0x00000031, 0x00000000, 
0x0005002C, 0x00000008, 0x00000032, 0x00000030, 0x00000031, 0x0004002B, 0x0000001B, 0x00000033, 0x00000000, 0x00090019, 0x00000035, 0x0000001B, 
0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000002, 0x00000004, 0x00040020, 0x00000036, 0x00000000, 0x00000035, 0x0004003B, 0x00000036, 
0x00000037, 0x00000000, 0x00050036, 0x00000003, 0x00000005, 0x00000000, 0x00000004, 0x000200F8, 0x00000006, 0x0004003B, 0x00000009, 0x0000000A, 
0x00000007, 0x0004003B, 0x00000009, 0x00000013, 0x00000007, 0x0004003B, 0x0000001D, 0x0000001E, 0x00000007, 0x0004003B, 0x00000028, 0x00000029, 
0x00000007, 0x00040008, 0x00000001, 0x0000000B, 0x00000000, 0x0004003D, 0x0000000C, 0x0000000F, 0x0000000E, 0x0004007C, 0x00000010, 0x00000011, 
0x0000000F, 0x0007004F, 0x00000008, 0x00000012, 0x00000011, 0x00000011, 0x00000000, 0x00000001, 0x0003003E, 0x0000000A, 0x00000012, 0x00040008, 
0x00000001, 0x0000000D, 0x00000000, 0x0004003D, 0x0000000C, 0x00000015, 0x00000014, 0x00050084, 0x0000000C, 0x00000018, 0x00000015, 0x00000017, 
0x0004007C, 0x00000010, 0x00000019, 0x00000018, 0x0007004F, 0x00000008, 0x0000001A, 0x00000019, 0x00000019, 0x00000000, 0x00000001, 0x0003003E, 
0x00000013, 0x0000001A, 0x00040008, 0x00000001, 0x0000000F, 0x00000000, 0x0004003D, 0x00000008, 0x0000001F, 0x0000000A, 0x0004006F, 0x0000001C, 
0x00000020, 0x0000001F, 0x0004003D, 0x00000008, 0x00000021, 0x00000013, 0x00050050, 0x00000008, 0x00000023, 0x00000022, 0x00000022, 0x00050082, 
0x00000008, 0x00000024, 0x00000021, 0x00000023, 0x0004006F, 0x0000001C, 0x00000025, 0x00000024, 0x00050088, 0x0000001C, 0x00000026, 0x00000020, 
0x00000025, 0x0003003E, 0x0000001E, 0x00000026, 0x00040008, 0x00000001, 0x00000011, 0x00000000, 0x0004003D, 0x0000002B, 0x0000002E, 0x0000002D, 
0x0004003D, 0x0000001C, 0x0000002F, 0x0000001E, 0x00080058, 0x00000027, 0x00000034, 0x0000002E, 0x0000002F, 0x0000000A, 0x00000033, 0x00000032, 
0x0003003E, 0x00000029, 0x00000034, 0x00040008, 0x00000001, 0x00000013, 0x00000000, 0x0004003D, 0x00000035, 0x00000038, 0x00000037, 0x0004003D, 
0x00000008, 0x00000039, 0x0000000A, 0x0004003D, 0x00000027, 0x0000003A, 0x00000029, 0x00040063, 0x00000038, 0x00000039, 0x0000003A, 0x000100FD, 
0x00010038 });
#ifdef GRAPHICS_API_SOFT
	descr.Compute().FuncSW( &SWShaderLang::sw_texture2dnearestfilter_comp );
#endif

};
};