// This is generated file
// Origin file: 'Compiler/Pipelines/UnnamedBuffer.ppln'
#include "all_pipelines.h"
// C++ shader
#ifdef GRAPHICS_API_SOFT
namespace SWShaderLang {
namespace {

#	define INOUT
#	define IN
#	define OUT

	struct UnnamedBuffer_SSBO
	{
		SArr<UInt,2> data;
		UInt result;
	
		UnnamedBuffer_SSBO () {}
		UnnamedBuffer_SSBO (UnnamedBuffer_SSBO &&) = default;
		UnnamedBuffer_SSBO (const UnnamedBuffer_SSBO &) = default;
		explicit UnnamedBuffer_SSBO(const SArr<UInt,2> &data, const UInt &result) : data{data}, result{result} {}
	
		UnnamedBuffer_SSBO& operator = (UnnamedBuffer_SSBO &&) = default;
		UnnamedBuffer_SSBO& operator = (const UnnamedBuffer_SSBO &) = default;
		Bool operator == (const UnnamedBuffer_SSBO &right) const {
			return	all( data == right.data ) &&
					all( result == right.result );
		}
		Bool operator != (const UnnamedBuffer_SSBO &right) const { return !(*this == right); }
	
	};
	
	
	//---------------------------------
	
	static void sw_unnamedbuffer_comp (const Impl::SWShaderHelper &_helper_)
	{
		// prepare externals
		Impl::StorageBuffer< UnnamedBuffer_SSBO, Impl::EStorageAccess::Coherent >  external_0;    _helper_.GetStorageBuffer( 0, external_0 );
	
		// shader
		{
			external_0->result = ((external_0->data[Int(1)]) + (external_0->data[Int(0)]));
		}
	}
	
}		// anonymous namespace
}		// SWShaderLang
#endif	// GRAPHICS_API_SOFT


namespace Pipelines
{

void Create_unnamedbuffer (PipelineTemplateDescriptor& descr)
{
	descr = PipelineTemplateDescriptor();
	descr.supportedShaders = EShader::Compute;

	descr.localGroupSize = uint3(1, 1, 1);
	descr.layout = PipelineLayoutDescriptor::Builder()
			.AddStorageBuffer( "", 12_b, 0_b, EShaderMemoryModel::Default, 0u, 0u, EShader::Compute )
			.Finish();

	descr.Compute().StringGLSL( 
R"#(#version 450 core
layout (local_size_x=1, local_size_y=1, local_size_z=1) in;

layout(binding=0) layout(std430) buffer UnnamedBuffer_SSBO{
	readonly uint data[2];
	writeonly uint result;
} ;

//---------------------------------

void main ()
{
	result = ((data[int( 1 )]) + (data[int( 0 )]));
}


)#"_str );
	descr.Compute().ArraySPIRV({ 
0x07230203, 0x00010000, 0x00080006, 0x00000017, 0x00000000, 0x00020011, 0x00000001, 0x0006000B, 0x00000002, 0x4C534C47, 0x6474732E, 0x3035342E, 
0x00000000, 0x0003000E, 0x00000000, 0x00000001, 0x0005000F, 0x00000005, 0x00000005, 0x6E69616D, 0x00000000, 0x00060010, 0x00000005, 0x00000011, 
0x00000001, 0x00000001, 0x00000001, 0x00030007, 0x00000001, 0x00000000, 0x002D0003, 0x00000002, 0x000001C2, 0x00000001, 0x4F202F2F, 0x646F4D70, 
0x50656C75, 0x65636F72, 0x64657373, 0x746E6520, 0x702D7972, 0x746E696F, 0x69616D20, 0x2F2F0A6E, 0x4D704F20, 0x6C75646F, 0x6F725065, 0x73736563, 
0x63206465, 0x6E65696C, 0x706F2074, 0x6C676E65, 0x0A303031, 0x4F202F2F, 0x646F4D70, 0x50656C75, 0x65636F72, 0x64657373, 0x72617420, 0x2D746567, 
0x20766E65, 0x6E65706F, 0x2F0A6C67, 0x704F202F, 0x75646F4D, 0x7250656C, 0x7365636F, 0x20646573, 0x72746E65, 0x6F702D79, 0x20746E69, 0x6E69616D, 
0x696C230A, 0x3120656E, 0x0000000A, 0x00040005, 0x00000005, 0x6E69616D, 0x00000000, 0x00070005, 0x0000000A, 0x616E6E55, 0x4264656D, 0x65666675, 
0x53535F72, 0x00004F42, 0x00050006, 0x0000000A, 0x00000000, 0x61746164, 0x00000000, 0x00050006, 0x0000000A, 0x00000001, 0x75736572, 0x0000746C, 
0x00030005, 0x0000000C, 0x00000000, 0x00040047, 0x00000009, 0x00000006, 0x00000004, 0x00050048, 0x0000000A, 0x00000000, 0x00000023, 0x00000000, 
0x00050048, 0x0000000A, 0x00000001, 0x00000023, 0x00000008, 0x00030047, 0x0000000A, 0x00000003, 0x00040047, 0x0000000C, 0x00000022, 0x00000000, 
0x00040047, 0x0000000C, 0x00000021, 0x00000000, 0x00020013, 0x00000003, 0x00030021, 0x00000004, 0x00000003, 0x00040015, 0x00000007, 0x00000020, 
0x00000000, 0x0004002B, 0x00000007, 0x00000008, 0x00000002, 0x0004001C, 0x00000009, 0x00000007, 0x00000008, 0x0004001E, 0x0000000A, 0x00000009, 
0x00000007, 0x00040020, 0x0000000B, 0x00000002, 0x0000000A, 0x0004003B, 0x0000000B, 0x0000000C, 0x00000002, 0x00040015, 0x0000000D, 0x00000020, 
0x00000001, 0x0004002B, 0x0000000D, 0x0000000E, 0x00000001, 0x0004002B, 0x0000000D, 0x0000000F, 0x00000000, 0x00040020, 0x00000010, 0x00000002, 
0x00000007, 0x00050036, 0x00000003, 0x00000005, 0x00000000, 0x00000004, 0x000200F8, 0x00000006, 0x00040008, 0x00000001, 0x0000000D, 0x00000000, 
0x00060041, 0x00000010, 0x00000011, 0x0000000C, 0x0000000F, 0x0000000E, 0x0004003D, 0x00000007, 0x00000012, 0x00000011, 0x00060041, 0x00000010, 
0x00000013, 0x0000000C, 0x0000000F, 0x0000000F, 0x0004003D, 0x00000007, 0x00000014, 0x00000013, 0x00050080, 0x00000007, 0x00000015, 0x00000012, 
0x00000014, 0x00050041, 0x00000010, 0x00000016, 0x0000000C, 0x0000000E, 0x0003003E, 0x00000016, 0x00000015, 0x000100FD, 0x00010038 });
	descr.Compute().StringCL( 
R"#(#define FORMAT( _fmt_ )
#define INOUT
#define OUT
// Functions for GLSL compatibility

#define Gen_IntTemplates( _gen_ ) \
	_gen_( int ) \
	_gen_( int2 ) \
	_gen_( int3 ) \
	_gen_( int4 )

#define Gen_UIntTemplates( _gen_ ) \
	_gen_( uint ) \
	_gen_( uint2 ) \
	_gen_( uint3 ) \
	_gen_( uint4 )
	
#define Gen_LongTemplates( _gen_ ) \
	_gen_( long ) \
	_gen_( long2 ) \
	_gen_( long3 ) \
	_gen_( long4 )
	
#define Gen_ULongTemplates( _gen_ ) \
	_gen_( ulong ) \
	_gen_( ulong2 ) \
	_gen_( ulong3 ) \
	_gen_( ulong4 )

#define Gen_FloatTemplates( _gen_ ) \
	_gen_( float ) \
	_gen_( float2 ) \
	_gen_( float3 ) \
	_gen_( float4 )

#define Gen_DoubleTemplates( _gen_ ) \
	_gen_( double ) \
	_gen_( double2 ) \
	_gen_( double3 ) \
	_gen_( double4 )


// Fract
#define GenTemplate_Fract( _type_ ) \
	_type_ fractTempl_##_type_ (_type_ x) { \
		_type_	ipart; \
		return fract( x, &ipart ); \
	}
	Gen_FloatTemplates( GenTemplate_Fract )
	Gen_DoubleTemplates( GenTemplate_Fract )
#undef GenTemplate_Fract


// findMSB 32
#define GenTemplate_FindMSB32( _type_ ) \
	_type_ findMSB_##_type_ (_type_ x) { \
		return 31 - clz( x ); \
	}
	Gen_IntTemplates( GenTemplate_FindMSB32 )
	Gen_UIntTemplates( GenTemplate_FindMSB32 )
#undef GenTemplate_FindMSB32
	

// findMSB 64
#define GenTemplate_FindMSB64( _type_ ) \
	_type_ findMSB_##_type_ (_type_ x) { \
		return 63 - clz( x ); \
	}
	Gen_LongTemplates( GenTemplate_FindMSB64 )
	Gen_ULongTemplates( GenTemplate_FindMSB64 )
#undef GenTemplate_FindMSB64


// findLSB 32
#define GenTemplate_FindLSB32( _type_ ) \
	_type_ findLSB_##_type_ (_type_ x) { \
		return	(convert_##_type_(!!( x >> 31 )) & convert_##_type_(!( x & (_type_)(0x7FFFFFFF) ))) + \
				(convert_##_type_(!!( x >> 30 )) & convert_##_type_(!( x & (_type_)(0x3FFFFFFF) ))) + \
				(convert_##_type_(!!( x >> 29 )) & convert_##_type_(!( x & (_type_)(0x1FFFFFFF) ))) + \
				(convert_##_type_(!!( x >> 28 )) & convert_##_type_(!( x & (_type_)(0xFFFFFFF) ))) + \
				(convert_##_type_(!!( x >> 27 )) & convert_##_type_(!( x & (_type_)(0x7FFFFFF) ))) + \
				(convert_##_type_(!!( x >> 26 )) & convert_##_type_(!( x & (_type_)(0x3FFFFFF) ))) + \
				(convert_##_type_(!!( x >> 25 )) & convert_##_type_(!( x & (_type_)(0x1FFFFFF) ))) + \
				(convert_##_type_(!!( x >> 24 )) & convert_##_type_(!( x & (_type_)(0xFFFFFF) ))) + \
				(convert_##_type_(!!( x >> 23 )) & convert_##_type_(!( x & (_type_)(0x7FFFFF) ))) + \
				(convert_##_type_(!!( x >> 22 )) & convert_##_type_(!( x & (_type_)(0x3FFFFF) ))) + \
				(convert_##_type_(!!( x >> 21 )) & convert_##_type_(!( x & (_type_)(0x1FFFFF) ))) + \
				(convert_##_type_(!!( x >> 20 )) & convert_##_type_(!( x & (_type_)(0xFFFFF) ))) + \
				(convert_##_type_(!!( x >> 19 )) & convert_##_type_(!( x & (_type_)(0x7FFFF) ))) + \
				(convert_##_type_(!!( x >> 18 )) & convert_##_type_(!( x & (_type_)(0x3FFFF) ))) + \
				(convert_##_type_(!!( x >> 17 )) & convert_##_type_(!( x & (_type_)(0x1FFFF) ))) + \
				(convert_##_type_(!!( x >> 16 )) & convert_##_type_(!( x & (_type_)(0xFFFF) ))) + \
				(convert_##_type_(!!( x >> 15 )) & convert_##_type_(!( x & (_type_)(0x7FFF) ))) + \
				(convert_##_type_(!!( x >> 14 )) & convert_##_type_(!( x & (_type_)(0x3FFF) ))) + \
				(convert_##_type_(!!( x >> 13 )) & convert_##_type_(!( x & (_type_)(0x1FFF) ))) + \
				(convert_##_type_(!!( x >> 12 )) & convert_##_type_(!( x & (_type_)(0xFFF) ))) + \
				(convert_##_type_(!!( x >> 11 )) & convert_##_type_(!( x & (_type_)(0x7FF) ))) + \
				(convert_##_type_(!!( x >> 10 )) & convert_##_type_(!( x & (_type_)(0x3FF) ))) + \
				(convert_##_type_(!!( x >>  9 )) & convert_##_type_(!( x & (_type_)(0x1FF) ))) + \
				(convert_##_type_(!!( x >>  8 )) & convert_##_type_(!( x & (_type_)(0xFF) ))) + \
				(convert_##_type_(!!( x >>  7 )) & convert_##_type_(!( x & (_type_)(0x7F) ))) + \
				(convert_##_type_(!!( x >>  6 )) & convert_##_type_(!( x & (_type_)(0x3F) ))) + \
				(convert_##_type_(!!( x >>  5 )) & convert_##_type_(!( x & (_type_)(0x1F) ))) + \
				(convert_##_type_(!!( x >>  4 )) & convert_##_type_(!( x & (_type_)(0xF) ))) + \
				(convert_##_type_(!!( x >>  3 )) & convert_##_type_(!( x & (_type_)(0x7) ))) + \
				(convert_##_type_(!!( x >>  2 )) & convert_##_type_(!( x & (_type_)(0x3) ))) + \
				(convert_##_type_(!!( x >>  1 )) & convert_##_type_(!( x & (_type_)(0x1) ))) + \
				convert_##_type_(!!( x )) - 1; \
	}
	Gen_IntTemplates( GenTemplate_FindLSB32 )
	Gen_UIntTemplates( GenTemplate_FindLSB32 )
#undef GenTemplate_FindLSB32
	
/*
// findLSB 64
#define GenTemplate_FindLSB64( _type_ ) \
	_type_ findLSB_##_type_ (_type_ x) { \
		return 63 - clz( rotate( x, 63 ) ); \
	}
	Gen_LongTemplates( GenTemplate_FindLSB64 )
	Gen_ULongTemplates( GenTemplate_FindLSB64 )
#undef GenTemplate_FindLSB64



// bitfieldReverse 32
#define GenTemplate_BitfieldReverse32( _type_ ) \
	_type_ bitfieldReverse_##_type_ (_type_ x) { \
		return ; \
	}
	Gen_IntTemplates( GenTemplate_BitfieldReverse32 )
	Gen_UIntTemplates( GenTemplate_BitfieldReverse32 )
#undef GenTemplate_BitfieldReverse32
*/

#undef Gen_IntTemplates
#undef Gen_UIntTemplates
#undef Gen_LongTemplates
#undef Gen_ULongTemplates
#undef Gen_FloatTemplates
#undef Gen_DoubleTemplates

struct UnnamedBuffer_SSBO
{
	uint data [2];
	uint result;
};


//---------------------------------

kernel void Main (
	/*0*/__global  struct UnnamedBuffer_SSBO* external_0)
{

	{
		external_0->result = ((external_0->data[((int)( 1 ))]) + (external_0->data[((int)( 0 ))]));
	}
}

)#"_str );
	descr.Compute().StringHLSL( 
R"#(cbuffer ComputeBuiltins : register(b0)
{
	uint3		dx_NumWorkGroups;
};

struct UnnamedBuffer_SSBO{
	uint data[2];
	uint result;
};
RWByteAddressBuffer<UnnamedBuffer_SSBO> external_0 : register(u0);

//---------------------------------

[numthreads(1, 1, 1)]
void main (uint3 dx_DispatchThreadID : SV_DispatchThreadID, uint3 dx_GroupThreadID : SV_GroupThreadID, uint3 dx_GroupID : SV_GroupID)
{
	((external_0).Load1(/*result*/8)) = ((((external_0).Load1(/*data*/0))[int( 1 )]) + (((external_0).Load1(/*data*/0))[int( 0 )]));
}


)#"_str );
#ifdef GRAPHICS_API_SOFT
	descr.Compute().FuncSW( &SWShaderLang::sw_unnamedbuffer_comp );
#endif

};
};
