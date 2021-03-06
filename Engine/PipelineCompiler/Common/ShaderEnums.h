// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "Engine/PipelineCompiler/Common/Common.h"

namespace PipelineCompiler
{

	struct EShaderFormat final : EShaderLangFormat
	{
		// api
		static constexpr type	GX_API			= (9 << _ApiOffset);

		// api + version
		static constexpr type	GX_100			= (100 << _VersionOffset) | GX_API;	// == OpenGL 450	// TODO
		//static constexpr type	GX_110			= (110 << _VersionOffset) | GX_API;	// == OpenGL 450


		// api + version + format
		static constexpr type	GXSL_100		= GX_100 | HighLevel;
		//static constexpr type	GXSL_110		= GX_110 | HighLevel;
		
		static constexpr type	VKSL_100		= Vulkan_100 | HighLevel;
		static constexpr type	VKSL_110		= Vulkan_110 | HighLevel;
		static constexpr type	VK_100_SPIRV	= Vulkan_100 | SPIRV;
		static constexpr type	VK_110_SPIRV	= Vulkan_110 | SPIRV;
		static constexpr type	VK_100_Asm		= Vulkan_100 | Assembler;	// for SPIRV
		static constexpr type	VK_110_Asm		= Vulkan_110 | Assembler;	// for SPIRV

		static constexpr type	GLSL_450		= OpenGL_450 | HighLevel;
		static constexpr type	GLSL_460		= OpenGL_460 | HighLevel;
		static constexpr type	GL_450_SPIRV	= OpenGL_450 | SPIRV;
		static constexpr type	GL_460_SPIRV	= OpenGL_460 | SPIRV;
		static constexpr type	GL_450_Asm		= OpenGL_450 | Assembler;	// for SPIRV
		static constexpr type	GL_460_Asm		= OpenGL_460 | Assembler;	// for SPIRV
		static constexpr type	GLSL_450_Bin	= OpenGL_450 | GL_Binary;	// vendor specific binary format
		
		static constexpr type	ESSL_200		= OpenGLES_200 | HighLevel;
		static constexpr type	ESSL_300		= OpenGLES_300 | HighLevel;
		static constexpr type	ESSL_310		= OpenGLES_310 | HighLevel;
		static constexpr type	ESSL_320		= OpenGLES_320 | HighLevel;
		static constexpr type	ESSL_320_SPIRV	= OpenGLES_320 | SPIRV;

		static constexpr type	CL_120			= OpenCL_120 | HighLevel;
		static constexpr type	CL_210			= OpenCL_210 | HighLevel;
		static constexpr type	CL_120_Asm		= OpenCL_120 | Assembler;
		static constexpr type	CL_210_Asm		= OpenCL_210 | Assembler;
		
		static constexpr type	HLSL_11			= DirectX_11 | HighLevel;
		static constexpr type	HLSL_12			= DirectX_12 | HighLevel;
		static constexpr type	HLSL_11_BC		= DirectX_11 | DXBC;
		static constexpr type	HLSL_12_IL		= DirectX_12 | DXIL;

		//static constexpr type	Soft_100_Src	= Software_100 | HighLevel;			// intermediate C++ source
		static constexpr type	Soft_100_Exe	= Software_100 | CPP_Invocable;		// builtin program in EXE/DLL

		static constexpr type	IntermediateSrc	= VKSL_110;
		static constexpr type	DefaultSrc		= GLSL_450;
		

		// API + Format
		static constexpr type	CL_Src			= HighLevel | OpenCL;
		static constexpr type	CL_Asm			= Assembler | OpenCL;

		static constexpr type	VK_SPIRV		= SPIRV | Vulkan;
		static constexpr type	GL_SPIRV		= SPIRV | OpenGL;
		static constexpr type	ES_SPIRV		= SPIRV | OpenGLES;
		static constexpr type	CL_SPIRV		= SPIRV | OpenCL;

		static constexpr type	VK_SPIRV_Asm	= Assembler | Vulkan;
		static constexpr type	GL_SPIRV_Asm	= Assembler | OpenGL;
		static constexpr type	ES_SPIRV_Asm	= Assembler | OpenGLES;

		static constexpr type	HLSL			= HighLevel | DirectX;
		static constexpr type	HLSL_BC			= DXBC | DirectX;
		static constexpr type	HLSL_IL			= DXIL | DirectX;

		static constexpr type	VKSL			= HighLevel | Vulkan;
		static constexpr type	GXSL			= HighLevel | GX_API;

		static constexpr type	GLSL			= HighLevel | OpenGL;
		static constexpr type	GLSL_Bin		= GL_Binary | OpenGL;
		
		static constexpr type	ESSL			= HighLevel | OpenGLES;
		static constexpr type	ESSL_Bin		= GL_Binary | OpenGLES;

		// methods
		static constexpr bool	IsValid (type value);
		static String			ToString (type value);
	};


	struct ETessellationSpacing
	{
		// see https://www.khronos.org/opengl/wiki/Tessellation#Tessellation_levels

		enum type : uint
		{
			Equal,
			FractionalEven,
			FractionalOdd,

			_Count,
			Unknown		= ~0u,
		};
		
		static StringCRef ToString (type value);
	};


	struct ETessellationInputPrimitive
	{
		// see https://www.khronos.org/opengl/wiki/Tessellation#Tessellating_primitives

		enum type : uint
		{
			Points,
			Isolines,
			Triangles,
			Quads,

			_Count,
			Unknown		= ~0u,
		};
		
		static StringCRef ToString (type value);
	};


	struct EGeometryInputPrimitive
	{
		// see https://www.khronos.org/opengl/wiki/Geometry_Shader#Primitive_in.2Fout_specification

		enum type : uint
		{
			Points,
			Lines,				// lines, line_strip, line_list
			LinesAdjacency,		// lines_adjacency, line_strip_adjacency
			Triangles,			// triangles, triangle_strip, triangle_fan
			TriangleAdjacency,	// triangles_adjacency, triangle_strip_adjacency

			_Count,
			Unknown		= ~0u,
		};
		
		static StringCRef ToString (type value);
	};


	struct EGeometryOutputPrimitive
	{
		enum type : uint
		{
			Points,
			LineStrip,
			TriangleStrip,

			_Count,
			Unknown		= ~0u,
		};
		
		static StringCRef ToString (type value);
	};


	struct EVariableQualifier
	{
		enum type : uint
		{
			Patch,			// for tessellation shaders

			Flat,
			NoPerspective,
			Smooth,

			Centroid,		// fragment shader input, vertex or geometry shader output
			Sample,			// fragment shader input, vertex or geometry shader output

			Shared,			// for compute shader

			Precise,		// 'noContraction' in Vulkan, 'precise' in OpenGL
			Invariant,

			//BindlessSampler,
			//BindlessImage,

			Constant,
			In,
			Out,

			InArg,
			OutArg,
			Local,			// local variable (in function or ...)

			BuiltIn,
			Uniform,

			Specialization,	// vulkan only
			//PushConstant,

			Volatile,		// if used as atomic

			_Count
		};

		using bits	= EnumBitfield< EVariableQualifier >;

		friend bits operator | (type left, type right)		{ return bits().Set( left ).Set( right ); }
		
		static String ToString (bits value);
	};


	struct EVariablePacking
	{
		enum type : uint
		{
			Default	= 0,	// any
			Std140,			// UB, SSB
			Std430,			// SSB
			Shared,			// UB
			Packed,			// UB
			Varying,		// io
			VertexAttrib,	// attrib
			CLPack,			// OpenCL packing

			_Count,
			Unknown	= uint(-1),
		};

		using bits = EnumBitfield< EVariablePacking >;
		
		friend bits operator | (type left, type right)		{ return bits().Set( left ).Set( right ); }

		static StringCRef ToString (type value);

		static String ToString (bits value);

		static type GetMaxPacking (bits value);
	};


	struct EPrecision
	{
		enum type : uint
		{
			Default		= 0,
			Low,
			Medium,
			High,

			_Count,
			Unknown		= ~0u,
		};

		static StringCRef ToString (type value);
	};


	struct EFragmentShaderParams
	{
		enum type : uint
		{
			None						= 0,

			EarlyFragmentTests			= 1 << 1,
			OriginUpperLeft				= 1 << 2,
			PixelCenterInteger			= 1 << 3,

			ColorExport					= 1 << 4,
			DepthExport					= 1 << 5,
			StencilExport				= 1 << 6,

			ConservativeDepth_Any		= 1 << 7,
			ConservativeDepth_Greater	= 1 << 8,
			ConservativeDepth_Less		= 1 << 9,
			ConservativeDepth_Unchanged	= 1 << 10,
			_ConservativeDepth_Mask		= ConservativeDepth_Any | ConservativeDepth_Greater | ConservativeDepth_Less |ConservativeDepth_Unchanged,

			PostDepthCoverage			= 1 << 11,

			_Count						= 12,
			Unknown						= 0,
		};

		GX_ENUM_BIT_OPERATIONS( type );
	};


	struct EShaderVariable
	{
	private:
		using _vtypeinfo	= _platforms_hidden_::EValueTypeInfo;

	public:
		enum type : uint
		{
			Void			= 0,

			Bool			= _vtypeinfo::Bool,
			Bool2			= _vtypeinfo::Bool2,
			Bool3			= _vtypeinfo::Bool3,
			Bool4			= _vtypeinfo::Bool4,
			
			Int				= _vtypeinfo::Int,
			Int2			= _vtypeinfo::Int2,
			Int3			= _vtypeinfo::Int3,
			Int4			= _vtypeinfo::Int4,
			
			UInt			= _vtypeinfo::UInt,
			UInt2			= _vtypeinfo::UInt2,
			UInt3			= _vtypeinfo::UInt3,
			UInt4			= _vtypeinfo::UInt4,

			Long			= _vtypeinfo::Long,
			Long2			= _vtypeinfo::Long2,
			Long3			= _vtypeinfo::Long3,
			Long4			= _vtypeinfo::Long4,
			
			ULong			= _vtypeinfo::ULong,
			ULong2			= _vtypeinfo::ULong2,
			ULong3			= _vtypeinfo::ULong3,
			ULong4			= _vtypeinfo::ULong4,

			Float			= _vtypeinfo::Float,
			Float2			= _vtypeinfo::Float2,
			Float3			= _vtypeinfo::Float3,
			Float4			= _vtypeinfo::Float4,
			Float2x2		= _vtypeinfo::_FLOAT | _vtypeinfo::_COL2 | _vtypeinfo::_ROW2,
			Float2x3		= _vtypeinfo::_FLOAT | _vtypeinfo::_COL2 | _vtypeinfo::_ROW3,
			Float2x4		= _vtypeinfo::_FLOAT | _vtypeinfo::_COL2 | _vtypeinfo::_ROW4,
			Float3x2		= _vtypeinfo::_FLOAT | _vtypeinfo::_COL3 | _vtypeinfo::_ROW2,
			Float3x3		= _vtypeinfo::_FLOAT | _vtypeinfo::_COL3 | _vtypeinfo::_ROW3,
			Float3x4		= _vtypeinfo::_FLOAT | _vtypeinfo::_COL3 | _vtypeinfo::_ROW4,
			Float4x2		= _vtypeinfo::_FLOAT | _vtypeinfo::_COL4 | _vtypeinfo::_ROW2,
			Float4x3		= _vtypeinfo::_FLOAT | _vtypeinfo::_COL4 | _vtypeinfo::_ROW3,
			Float4x4		= _vtypeinfo::_FLOAT | _vtypeinfo::_COL4 | _vtypeinfo::_ROW4,
			
			Double			= _vtypeinfo::Double,
			Double2			= _vtypeinfo::Double2,
			Double3			= _vtypeinfo::Double3,
			Double4			= _vtypeinfo::Double4,
			Double2x2		= _vtypeinfo::_DOUBLE | _vtypeinfo::_COL2 | _vtypeinfo::_ROW2,
			Double2x3		= _vtypeinfo::_DOUBLE | _vtypeinfo::_COL2 | _vtypeinfo::_ROW3,
			Double2x4		= _vtypeinfo::_DOUBLE | _vtypeinfo::_COL2 | _vtypeinfo::_ROW4,
			Double3x2		= _vtypeinfo::_DOUBLE | _vtypeinfo::_COL3 | _vtypeinfo::_ROW2,
			Double3x3		= _vtypeinfo::_DOUBLE | _vtypeinfo::_COL3 | _vtypeinfo::_ROW3,
			Double3x4		= _vtypeinfo::_DOUBLE | _vtypeinfo::_COL3 | _vtypeinfo::_ROW4,
			Double4x2		= _vtypeinfo::_DOUBLE | _vtypeinfo::_COL4 | _vtypeinfo::_ROW2,
			Double4x3		= _vtypeinfo::_DOUBLE | _vtypeinfo::_COL4 | _vtypeinfo::_ROW3,
			Double4x4		= _vtypeinfo::_DOUBLE | _vtypeinfo::_COL4 | _vtypeinfo::_ROW4,
			
			// uniform sampler
			Sampler						= _vtypeinfo::_SAMPLER,

			// any sampler
			Sampler1D					= _vtypeinfo::_SAMPLER | _vtypeinfo::_SAMP_1D,
			Sampler1DShadow				= _vtypeinfo::_SAMPLER | _vtypeinfo::_SAMP_1DS,
			Sampler1DArray				= _vtypeinfo::_SAMPLER | _vtypeinfo::_SAMP_1DA,
			Sampler1DArrayShadow		= _vtypeinfo::_SAMPLER | _vtypeinfo::_SAMP_1DAS,
			Sampler2D					= _vtypeinfo::_SAMPLER | _vtypeinfo::_SAMP_2D,
			Sampler2DShadow				= _vtypeinfo::_SAMPLER | _vtypeinfo::_SAMP_2DS,
			Sampler2DMS					= _vtypeinfo::_SAMPLER | _vtypeinfo::_SAMP_2DMS,
			Sampler2DArray				= _vtypeinfo::_SAMPLER | _vtypeinfo::_SAMP_2DA,
			Sampler2DArrayShadow		= _vtypeinfo::_SAMPLER | _vtypeinfo::_SAMP_2DAS,
			Sampler2DMSArray			= _vtypeinfo::_SAMPLER | _vtypeinfo::_SAMP_2DAMS,
			SamplerCube					= _vtypeinfo::_SAMPLER | _vtypeinfo::_SAMP_CUBE,
			SamplerCubeShadow			= _vtypeinfo::_SAMPLER | _vtypeinfo::_SAMP_CUBES,
			SamplerCubeArray			= _vtypeinfo::_SAMPLER | _vtypeinfo::_SAMP_CUBEA,
			Sampler3D					= _vtypeinfo::_SAMPLER | _vtypeinfo::_SAMP_3D,
			SamplerBuffer				= _vtypeinfo::_SAMPLER | _vtypeinfo::_SAMP_BUF,

			// any image
			Image1D						= _vtypeinfo::_IMAGE | _vtypeinfo::_SAMP_1D,
			Image1DArray				= _vtypeinfo::_IMAGE | _vtypeinfo::_SAMP_1DA,
			Image2D						= _vtypeinfo::_IMAGE | _vtypeinfo::_SAMP_2D,
			Image2DMS					= _vtypeinfo::_IMAGE | _vtypeinfo::_SAMP_2DMS,
			Image2DArray				= _vtypeinfo::_IMAGE | _vtypeinfo::_SAMP_2DA,
			Image2DMSArray				= _vtypeinfo::_IMAGE | _vtypeinfo::_SAMP_2DAMS,
			ImageCube					= _vtypeinfo::_IMAGE | _vtypeinfo::_SAMP_CUBE,
			ImageCubeArray				= _vtypeinfo::_IMAGE | _vtypeinfo::_SAMP_CUBEA,
			Image3D						= _vtypeinfo::_IMAGE | _vtypeinfo::_SAMP_3D,
			ImageBuffer					= _vtypeinfo::_IMAGE | _vtypeinfo::_SAMP_BUF,

			// float sampler
			FloatSampler1D				= _vtypeinfo::_FLOAT | Sampler1D,
			FloatSampler1DShadow		= _vtypeinfo::_FLOAT | Sampler1DShadow,
			FloatSampler1DArray			= _vtypeinfo::_FLOAT | Sampler1DArray,
			FloatSampler1DArrayShadow	= _vtypeinfo::_FLOAT | Sampler1DArrayShadow,
			FloatSampler2D				= _vtypeinfo::_FLOAT | Sampler2D,
			FloatSampler2DShadow		= _vtypeinfo::_FLOAT | Sampler2DShadow,
			FloatSampler2DMS			= _vtypeinfo::_FLOAT | Sampler2DMS,
			FloatSampler2DArray			= _vtypeinfo::_FLOAT | Sampler2DArray,
			FloatSampler2DArrayShadow	= _vtypeinfo::_FLOAT | Sampler2DArrayShadow,
			FloatSampler2DMSArray		= _vtypeinfo::_FLOAT | Sampler2DMSArray,
			FloatSamplerCube			= _vtypeinfo::_FLOAT | SamplerCube,
			FloatSamplerCubeShadow		= _vtypeinfo::_FLOAT | SamplerCubeShadow,
			FloatSamplerCubeArray		= _vtypeinfo::_FLOAT | SamplerCubeArray,
			FloatSampler3D				= _vtypeinfo::_FLOAT | Sampler3D,
			FloatSamplerBuffer			= _vtypeinfo::_FLOAT | SamplerBuffer,
			
			// int sampler
			IntSampler1D				= _vtypeinfo::_INT | Sampler1D,
			IntSampler1DArray			= _vtypeinfo::_INT | Sampler1DArray,
			IntSampler2D				= _vtypeinfo::_INT | Sampler2D,
			IntSampler2DMS				= _vtypeinfo::_INT | Sampler2DMS,
			IntSampler2DArray			= _vtypeinfo::_INT | Sampler2DArray,
			IntSampler2DMSArray			= _vtypeinfo::_INT | Sampler2DMSArray,
			IntSamplerCube				= _vtypeinfo::_INT | SamplerCube,
			IntSamplerCubeArray			= _vtypeinfo::_INT | SamplerCubeArray,
			IntSampler3D				= _vtypeinfo::_INT | Sampler3D,
			IntSamplerBuffer			= _vtypeinfo::_INT | SamplerBuffer,
			
			// uint sampler
			UIntSampler1D				= _vtypeinfo::_UINT | Sampler1D,
			UIntSampler1DArray			= _vtypeinfo::_UINT | Sampler1DArray,
			UIntSampler2D				= _vtypeinfo::_UINT | Sampler2D,
			UIntSampler2DMS				= _vtypeinfo::_UINT | Sampler2DMS,
			UIntSampler2DArray			= _vtypeinfo::_UINT | Sampler2DArray,
			UIntSampler2DMSArray		= _vtypeinfo::_UINT | Sampler2DMSArray,
			UIntSamplerCube				= _vtypeinfo::_UINT | SamplerCube,
			UIntSamplerCubeArray		= _vtypeinfo::_UINT | SamplerCubeArray,
			UIntSampler3D				= _vtypeinfo::_UINT | Sampler3D,
			UIntSamplerBuffer			= _vtypeinfo::_UINT | SamplerBuffer,
				
			// float image
			FloatImage1D				= _vtypeinfo::_FLOAT | Image1D,
			FloatImage1DArray			= _vtypeinfo::_FLOAT | Image1DArray,
			FloatImage2D				= _vtypeinfo::_FLOAT | Image2D,
			FloatImage2DMS				= _vtypeinfo::_FLOAT | Image2DMS,
			FloatImage2DArray			= _vtypeinfo::_FLOAT | Image2DArray,
			FloatImage2DMSArray			= _vtypeinfo::_FLOAT | Image2DMSArray,
			FloatImageCube				= _vtypeinfo::_FLOAT | ImageCube,
			FloatImageCubeArray			= _vtypeinfo::_FLOAT | ImageCubeArray,
			FloatImage3D				= _vtypeinfo::_FLOAT | Image3D,
			FloatImageBuffer			= _vtypeinfo::_FLOAT | ImageBuffer,
				
			// int image
			IntImage1D					= _vtypeinfo::_INT | Image1D,
			IntImage1DArray				= _vtypeinfo::_INT | Image1DArray,
			IntImage2D					= _vtypeinfo::_INT | Image2D,
			IntImage2DMS				= _vtypeinfo::_INT | Image2DMS,
			IntImage2DArray				= _vtypeinfo::_INT | Image2DArray,
			IntImage2DMSArray			= _vtypeinfo::_INT | Image2DMSArray,
			IntImageCube				= _vtypeinfo::_INT | ImageCube,
			IntImageCubeArray			= _vtypeinfo::_INT | ImageCubeArray,
			IntImage3D					= _vtypeinfo::_INT | Image3D,
			IntImageBuffer				= _vtypeinfo::_INT | ImageBuffer,
				
			// uint image
			UIntImage1D					= _vtypeinfo::_UINT | Image1D,
			UIntImage1DArray			= _vtypeinfo::_UINT | Image1DArray,
			UIntImage2D					= _vtypeinfo::_UINT | Image2D,
			UIntImage2DMS				= _vtypeinfo::_UINT | Image2DMS,
			UIntImage2DArray			= _vtypeinfo::_UINT | Image2DArray,
			UIntImage2DMSArray			= _vtypeinfo::_UINT | Image2DMSArray,
			UIntImageCube				= _vtypeinfo::_UINT | ImageCube,
			UIntImageCubeArray			= _vtypeinfo::_UINT | ImageCubeArray,
			UIntImage3D					= _vtypeinfo::_UINT | Image3D,
			UIntImageBuffer				= _vtypeinfo::_UINT | ImageBuffer,

			_EXT2_OFF					= _vtypeinfo::_MAX,
			_EXT2_MASK					= 0x8 << _EXT2_OFF,
			Struct						= 1 << _EXT2_OFF,
			VaryingsBlock				= 2 << _EXT2_OFF,	// shader in/out block
			UniformBlock				= 3 << _EXT2_OFF,
			StorageBlock				= 4 << _EXT2_OFF,
			VertexAttribs				= 5 << _EXT2_OFF,
			Union						= 6 << _EXT2_OFF,
			SubpassInput				= 7 << _EXT2_OFF,
			
			_MAX						= CompileTime::IntLog2< uint, _EXT2_MASK > + 1,
				
			_TYPE_MASK					= _vtypeinfo::_TYPE_MASK | _vtypeinfo::_SRGB | _vtypeinfo::_NORM | _vtypeinfo::_UNSIGNED | _vtypeinfo::_REVERSE,

			Unknown						= ~0u,
		};

		static type ToVec (type basic, uint vecSize);
		static type ToMat (type basic, uint columns, uint rows);
		static type ToSampler (type samplerType, type dataType);
		static type ToImage (type samplerType, type dataType);

		static bool ToTexture (type value, OUT EImage::type &imageType, OUT bool &isShadow);

		static type ToSampler (EImage::type imageType, bool isShadow, EPixelFormatClass::type format);
		static type ToImage (EImage::type imageType, EPixelFormat::type format);
		static type ToScalar (type value);

		template <typename T>	static type ToScalar ();

		static EPixelFormat::type		ToPixelFormat (type value);
		static EPixelFormatClass::type	ToPixelFormatClass (type value);

		static EVertexAttribute::type	ToAttrib (type value);
		static EFragOutput::type		ToFragOutput (type value);

		static bool		IsStruct (type value);
		static bool		IsBuffer (type value);
		static bool		IsTexture (type value);
		static bool		IsShadowTexture (type value);
		static bool		IsImage (type value);
		static bool		IsScalar (type value);

		static bool		IsFloat (type value);
		static bool		IsFloat32 (type value);
		static bool		IsFloat64 (type value);

		static bool		IsInt (type value);
		static bool		IsInt32 (type value);
		static bool		IsInt64 (type value);
		static bool		IsUnsigned (type value);

		static bool		IsBool (type value);

		static BytesU	SizeOf (type value, BytesU rowAlign = 4_b);
		static uint		VecSize (type value);
		static uint2	MatSize (type value);	// column, row

		static type		FromString (StringCRef typeName);
	};


	
//-----------------------------------------------------------------------------//
// EShaderFormat
	
	ND_ inline constexpr bool  EShaderFormat::IsValid (type value)
	{
		return	!!( value & _ApiMask )		and
				!!( value & _VersionMask )	and
				!!( value & _StorageMask )	and
				!!( value & _FormatMask );
	}


	ND_ inline String  EShaderFormat::ToString (type value)
	{
		String	str;

		switch ( GetAPI( value ) )
		{
			case OpenGL :		str << "OpenGL";	break;
			case OpenGLES :		str << "OpenGLES";	break;
			case DirectX :		str << "DirectX";	break;
			case OpenCL :		str << "OpenCL";	break;
			case Vulkan :		str << "Vulkan";	break;
			case Metal :		str << "Metal";		break;
			case CUDA :			str << "CUDA";		break;
			case Software :		str << "Software";	break;
			case GX_API :		str << "GX_API";	break;
			default :			RETURN_ERR( "unknown api" );
		}

		str << '_' << GetVersion( value ) << '_';

		switch ( GetFormat( value ) )
		{
			case HighLevel :		str << "HighLevel";		break;
			case SPIRV :			str << "SPIRV";			break;
			case GL_Binary :		str << "GLBinary";		break;
			case DXBC :				str << "DXBC";			break;
			case DXIL :				str << "DXIL";			break;
			case Assembler :		str << "Asm";			break;
			case CPP_Invocable :	str << "CPPInvocable";	break;
			default :				RETURN_ERR( "unknown format" );
		}
		return str;
	}

	
//-----------------------------------------------------------------------------//
// ETessellationSpacing
	
	ND_ inline StringCRef  ETessellationSpacing::ToString (type value)
	{
		switch ( value )
		{
			case Equal :			return "Equal";
			case FractionalEven :	return "FractionalEven";
			case FractionalOdd :	return "FractionalOdd";
		}
		RETURN_ERR( "unsupported tessellation spacing!" );
	}
	


//-----------------------------------------------------------------------------//
// ETessellationInputPrimitive
	
	ND_ inline StringCRef  ETessellationInputPrimitive::ToString (type value)
	{
		switch ( value )
		{
			case Points :		return "Points";
			case Isolines :		return "Isolines";
			case Triangles :	return "Triangles";
			case Quads :		return "Quads";
		}
		RETURN_ERR( "unsupported tessellation input primitive!" );
	}
	


//-----------------------------------------------------------------------------//
// EGeometryInputPrimitive
	
	ND_ inline StringCRef  EGeometryInputPrimitive::ToString (type value)
	{
		switch ( value )
		{
			case Points :				return "Points";
			case Lines :				return "Lines";
			case LinesAdjacency :		return "LinesAdjacency";
			case Triangles :			return "Triangles";
			case TriangleAdjacency :	return "TriangleAdjacency";
		}
		RETURN_ERR( "unsupported geometry input primitive!" );
	}
	


//-----------------------------------------------------------------------------//
// EGeometryOutputPrimitive
	
	ND_ inline StringCRef  EGeometryOutputPrimitive::ToString (type value)
	{
		switch ( value )
		{
			case Points :			return "Points";
			case LineStrip :		return "LineStrip";
			case TriangleStrip :	return "TriangleStrip";
		}
		RETURN_ERR( "unsupported geometry output primitive!" );
	}
	

	
//-----------------------------------------------------------------------------//
// EVariableQualifier
	
	ND_ inline String  EVariableQualifier::ToString (bits value)
	{
		String str;

		FOR( i, value )
		{
			if ( not value[type(i)] )
				continue;

			if ( not str.Empty() )
				str << " | ";

			switch ( type(i) )
			{
				case Patch :			str << "Patch";			break;
				case Flat :				str << "Flat";			break;
				case NoPerspective :	str << "NoPerspective";	break;
				case Smooth :			str << "Smooth";		break;
				case Centroid :			str << "Centroid";		break;
				case Sample :			str << "Sample";		break;
				case Shared :			str << "Shared";		break;
				case Precise :			str << "Precise";		break;
				case Invariant :		str << "Invariant";		break;
				//case BindlessSampler:	str << "BindlessSampler";break;
				//case BindlessImage :	str << "BindlessImage";	break;
				case Constant :			str << "Constant";		break;
				case In :				str << "In";			break;
				case Out :				str << "Out";			break;
				case BuiltIn :			str << "BuiltIn";		break;
				case Uniform :			str << "Uniform";		break;
				case InArg :			str << "InArg";			break;
				case OutArg :			str << "OutArg";		break;
				case Local :			str << "Local";			break;
				case Specialization :	str << "Specialization"; break;
				default :				RETURN_ERR( "unknown qualifier type!" );
			}
		}
		return str;
	}


		
//-----------------------------------------------------------------------------//
// EVariablePacking
	
	ND_ inline StringCRef  EVariablePacking::ToString (type value)
	{
		switch ( value )
		{
			case Std140 :	return "std140";
			case Std430 :	return "std430";
			case Shared :	return "shared";
			case Packed :	return "packed";
		}
		RETURN_ERR( "unknown buffer packing type!" );
	}
	

	ND_ inline String  EVariablePacking::ToString (bits value)
	{
		String	str;
		
		FOR( i, value )
		{
			if ( not value[type(i)] )
				continue;

			if ( not str.Empty() )
				str << " | ";

			switch ( type(i) )
			{
				case Default :		str << "Default";	break;
				case Std140 :		str << "Std140";	break;
				case Std430 :		str << "Std430";	break;
				case Shared :		str << "Shared";	break;
				case Packed :		str << "Packed";	break;
				case Varying :		str << "Varying";	break;
				case VertexAttrib :	str << "vertex";	break;
				default :			RETURN_ERR( "unknown buffer packing type!" );
			}
		}
		return str;
	}


	ND_ inline EVariablePacking::type  EVariablePacking::GetMaxPacking (bits value)
	{
		EVariablePacking::type	max_packing = EVariablePacking::Default;

		FOR( i, value )
		{
			if ( not value[type(i)] )
				continue;

			switch ( type(i) )
			{
				case Default :		break;
				case Std140 :		max_packing = Std140;										break;
				case Std430 :		max_packing = (max_packing == Std140 ? Std140 : Std430);	break;
				case Shared :		break;
				case Packed :		break;
				case Varying :		break;
				case VertexAttrib :	break;
				default :			RETURN_ERR( "unknown buffer packing type!" );
			}
		}
		return max_packing;
	}
	


//-----------------------------------------------------------------------------//
// EPrecision
	
	ND_ inline StringCRef  EPrecision::ToString (type value)
	{
		switch ( value )
		{
			case Default :	return "Default";
			case Low :		return "Low";
			case Medium :	return "Medium";
			case High :		return "High";
		}
		RETURN_ERR( "unknown precision qualifier type!" );
	}



//-----------------------------------------------------------------------------//
// EShaderVariable
	
	ND_ inline EShaderVariable::type  EShaderVariable::ToVec (type basic, uint vecSize)
	{
		ASSERT( vecSize > 0 and vecSize <= 4 );

		vecSize = ((vecSize - 1) & 3) + 1;

		return type( (basic & ~(_vtypeinfo::_COL_MASK | _vtypeinfo::_ROW_MASK)) |
					 (vecSize << _vtypeinfo::_COL_OFF) );
	}


	ND_ inline EShaderVariable::type  EShaderVariable::ToMat (type basic, uint columns, uint rows)
	{
		ASSERT( columns > 0 and columns <= 4 );
		ASSERT( rows > 0 and rows <= 4 );

		columns	= ((columns - 1) & 3) + 1;
		rows	= ((rows - 1) & 3) + 1;

		return type( (basic & ~(_vtypeinfo::_COL_MASK | _vtypeinfo::_ROW_MASK)) | 
					 (columns << _vtypeinfo::_COL_OFF) |
					 (rows << _vtypeinfo::_ROW_OFF) );
	}
	

	ND_ inline EShaderVariable::type  EShaderVariable::ToSampler (type samplerType, type dataType)
	{
		return type( (samplerType & _vtypeinfo::_SAMP_MASK) | _vtypeinfo::_SAMPLER | (dataType & _TYPE_MASK) );
	}


	ND_ inline EShaderVariable::type  EShaderVariable::ToImage (type samplerType, type dataType)
	{
		return type( (samplerType & _vtypeinfo::_SAMP_MASK) | _vtypeinfo::_IMAGE | (dataType & _TYPE_MASK) );
	}
	

	ND_ inline EShaderVariable::type  EShaderVariable::ToScalar (type value)
	{
		return type( (value & _TYPE_MASK) | _vtypeinfo::_COL1 );
	}
	

	template <>	ND_ inline EShaderVariable::type  EShaderVariable::ToScalar<bool> ()	{ return EShaderVariable::Bool; }
	template <>	ND_ inline EShaderVariable::type  EShaderVariable::ToScalar<int> ()		{ return EShaderVariable::Int; }
	template <>	ND_ inline EShaderVariable::type  EShaderVariable::ToScalar<uint> ()	{ return EShaderVariable::UInt; }
	template <>	ND_ inline EShaderVariable::type  EShaderVariable::ToScalar<ilong> ()	{ return EShaderVariable::Long; }
	template <>	ND_ inline EShaderVariable::type  EShaderVariable::ToScalar<ulong> ()	{ return EShaderVariable::ULong; }
	template <>	ND_ inline EShaderVariable::type  EShaderVariable::ToScalar<float> ()	{ return EShaderVariable::Float; }
	template <>	ND_ inline EShaderVariable::type  EShaderVariable::ToScalar<double> ()	{ return EShaderVariable::Double; }


	inline bool EShaderVariable::ToTexture (type value, OUT EImage::type &imageType, OUT bool &isShadow)
	{
		switch ( value & _vtypeinfo::_SAMP_MASK )
		{
			case _vtypeinfo::_SAMP_1D :		imageType = EImage::Tex1D;			isShadow = false;	break;
			case _vtypeinfo::_SAMP_1DS :	imageType = EImage::Tex1D;			isShadow = true;	break;
			case _vtypeinfo::_SAMP_1DA :	imageType = EImage::Tex1DArray;		isShadow = false;	break;
			case _vtypeinfo::_SAMP_1DAS :	imageType = EImage::Tex1DArray;		isShadow = true;	break;
			case _vtypeinfo::_SAMP_2D :		imageType = EImage::Tex2D;			isShadow = false;	break;
			case _vtypeinfo::_SAMP_2DS :	imageType = EImage::Tex2D;			isShadow = true;	break;
			case _vtypeinfo::_SAMP_2DMS :	imageType = EImage::Tex2DMS;		isShadow = false;	break;
			case _vtypeinfo::_SAMP_2DA :	imageType = EImage::Tex2DArray;		isShadow = false;	break;
			case _vtypeinfo::_SAMP_2DAS :	imageType = EImage::Tex2DArray;		isShadow = true;	break;
			case _vtypeinfo::_SAMP_2DAMS :	imageType = EImage::Tex2DMSArray;	isShadow = false;	break;
			case _vtypeinfo::_SAMP_3D :		imageType = EImage::Tex3D;			isShadow = false;	break;
			case _vtypeinfo::_SAMP_CUBE :	imageType = EImage::TexCube;		isShadow = false;	break;
			case _vtypeinfo::_SAMP_CUBES :	imageType = EImage::TexCube;		isShadow = true;	break;
			case _vtypeinfo::_SAMP_CUBEA :	imageType = EImage::TexCubeArray;	isShadow = false;	break;
			case _vtypeinfo::_SAMP_BUF :	imageType = EImage::Buffer;			isShadow = false;	break;
			default :	RETURN_ERR( "Shader variable type is not a valid sampler or image type!" );
		}
		return true;
	}
	

	ND_ inline EShaderVariable::type  EShaderVariable::ToSampler (EImage::type imageType, bool isShadow, EPixelFormatClass::type format)
	{
		uint	result = _vtypeinfo::_SAMPLER;

		switch ( imageType )
		{
			case EImage::Tex1D :			result |= (isShadow ? _vtypeinfo::_SAMP_1DS : _vtypeinfo::_SAMP_1D);		break;
			case EImage::Tex1DArray :		result |= (isShadow ? _vtypeinfo::_SAMP_1DAS : _vtypeinfo::_SAMP_1DA);		break;
			case EImage::Tex2D :			result |= (isShadow ? _vtypeinfo::_SAMP_2DS : _vtypeinfo::_SAMP_2D);		break;
			case EImage::Tex2DArray :		result |= (isShadow ? _vtypeinfo::_SAMP_2DAS : _vtypeinfo::_SAMP_2DA);		break;
			case EImage::Tex2DMS :			result |= _vtypeinfo::_SAMP_2DMS;	CHECK_ERR( not isShadow );				break;
			case EImage::Tex2DMSArray :		result |= _vtypeinfo::_SAMP_2DAMS;	CHECK_ERR( not isShadow );				break;
			case EImage::TexCube :			result |= (isShadow ? _vtypeinfo::_SAMP_CUBES : _vtypeinfo::_SAMP_CUBE);	break;
			case EImage::TexCubeArray :		result |= _vtypeinfo::_SAMP_CUBEA;	CHECK_ERR( not isShadow );				break;
			case EImage::Tex3D :			result |= _vtypeinfo::_SAMP_3D;		CHECK_ERR( not isShadow );				break;
			case EImage::Buffer :			result |= _vtypeinfo::_SAMP_BUF;	CHECK_ERR( not isShadow );				break;
			default :						RETURN_ERR( "unsupported texture type!" );
		}

		if ( EnumAnyEq( format, EPixelFormatClass::AnyFloat ) )		result |= _vtypeinfo::_FLOAT;	else
		if ( EnumAnyEq( format, EPixelFormatClass::AnyInt ) )		result |= _vtypeinfo::_INT;		else
		if ( EnumAnyEq( format, EPixelFormatClass::AnyUInt ) )		result |= _vtypeinfo::_UINT;	else
		if ( EnumAnyEq( format, EPixelFormatClass::AnySNorm ) )		result |= _vtypeinfo::_FLOAT;	else
		if ( EnumAnyEq( format, EPixelFormatClass::AnyUNorm ) )		result |= _vtypeinfo::_FLOAT;	else
																	RETURN_ERR( "unsupported texture data type" );
		return EShaderVariable::type( result );
	}
	

	ND_ inline EShaderVariable::type  EShaderVariable::ToImage (EImage::type imageType, EPixelFormat::type format)
	{
		uint	result = _vtypeinfo::_IMAGE;

		switch ( imageType )
		{
			case EImage::Tex1D :			result |= _vtypeinfo::_SAMP_1D;		break;
			case EImage::Tex1DArray :		result |= _vtypeinfo::_SAMP_1DA;	break;
			case EImage::Tex2D :			result |= _vtypeinfo::_SAMP_2D;		break;
			case EImage::Tex2DArray :		result |= _vtypeinfo::_SAMP_2DA;	break;
			case EImage::Tex2DMS :			result |= _vtypeinfo::_SAMP_2DMS;	break;
			case EImage::Tex2DMSArray :		result |= _vtypeinfo::_SAMP_2DAMS;	break;
			case EImage::TexCube :			result |= _vtypeinfo::_SAMP_CUBE;	break;
			case EImage::TexCubeArray :		result |= _vtypeinfo::_SAMP_CUBEA;	break;
			case EImage::Tex3D :			result |= _vtypeinfo::_SAMP_3D;		break;
			case EImage::Buffer :			result |= _vtypeinfo::_SAMP_BUF;	break;
			default :						RETURN_ERR( "unsupported texture type!" );
		}

		if ( EPixelFormat::IsFloat( format ) or EPixelFormat::IsIntNorm( format ) )		result |= _vtypeinfo::_FLOAT;	else
		if ( EPixelFormat::IsInt( format ) )											result |= _vtypeinfo::_INT;		else
		if ( EPixelFormat::IsUInt( format ) )											result |= _vtypeinfo::_UINT;	else
																						RETURN_ERR( "unsupported format" );

		return EShaderVariable::type( result );
	}


	ND_ inline EPixelFormat::type  EShaderVariable::ToPixelFormat (type value)
	{
		bool		is_shadow	= false;
		const uint	mask		= _TYPE_MASK | _vtypeinfo::_COL_MASK;

		switch ( value & _vtypeinfo::_SAMP_MASK )
		{
			case _vtypeinfo::_SAMP_1DS :	is_shadow = true;	break;
			case _vtypeinfo::_SAMP_2DS :	is_shadow = true;	break;
			case _vtypeinfo::_SAMP_2DAS :	is_shadow = true;	break;
			case _vtypeinfo::_SAMP_CUBES :	is_shadow = true;	break;
		}

		if ( is_shadow )
		{
			switch ( value & _vtypeinfo::_TYPE_MASK )
			{
				case _vtypeinfo::_FLOAT :	return EPixelFormat::Depth32F;
				case _vtypeinfo::_INT :		return EPixelFormat::Depth32;
				case _vtypeinfo::_INT24 :	return EPixelFormat::Depth24;
				case _vtypeinfo::_SHORT :	return EPixelFormat::Depth16;
				case _vtypeinfo::_HALF :	return EPixelFormat::Depth16;
			}
			RETURN_ERR( "no suitable depth format!" );
		}
		return EPixelFormat::type( value & mask );
	}


	ND_ inline EPixelFormatClass::type  EShaderVariable::ToPixelFormatClass (type value)
	{
		using VTI	= _platforms_hidden_::EValueTypeInfo;

		uint	result = 0;

		result |= EPixelFormatClass::AnyColorChannels;

		switch ( value & (VTI::_TYPE_MASK | VTI::_UNSIGNED | VTI::_NORM) )
		{
			case VTI::_BYTE | VTI::_NORM :
			case VTI::_SHORT | VTI::_NORM :
			case VTI::_UBYTE | VTI::_NORM :
			case VTI::_USHORT | VTI::_NORM :
			case VTI::_HALF :
			case VTI::_FLOAT :
			case VTI::_FLOAT_11_11_10 :
			case VTI::_INT_10_10_10_2 | VTI::_NORM :	result |= EPixelFormatClass::AnyFloat | EPixelFormatClass::AnyNorm;	break;

			case VTI::_BYTE :
			case VTI::_SHORT :
			case VTI::_INT :
			case VTI::_INT_10_10_10_2 :
			case VTI::_INT_10_10_10 :					result |= EPixelFormatClass::AnyInt;	break;

			case VTI::_UBYTE :
			case VTI::_USHORT :
			case VTI::_UINT :
			case VTI::_USHORT_4_4_4_4 :
			case VTI::_USHORT_5_5_5_1 :
			case VTI::_USHORT_5_6_5 :
			case VTI::_USHORT_5_5_5 :					result |= EPixelFormatClass::AnyUInt;	break;

			default :									RETURN_ERR( "unsupported data type" );
		}

		if ( value & VTI::_SRGB )
			result |= EPixelFormatClass::NonLinearColorSpace;
		else
			result |= EPixelFormatClass::LinearColorSpace;

		return EPixelFormatClass::type( result );
	}
	

	ND_ inline EVertexAttribute::type  EShaderVariable::ToAttrib (type value)
	{
		return EVertexAttribute::type( value );		// TODO
	}


	ND_ inline EFragOutput::type  EShaderVariable::ToFragOutput (type value)
	{
		return EFragOutput::type( value );		// TODO
	}
	

	ND_ inline bool  EShaderVariable::IsStruct (type value)
	{
		switch ( value )
		{
			case Struct :
			case UniformBlock :
			case StorageBlock :		return true;
		}
		return false;
	}
	

	ND_ inline bool  EShaderVariable::IsBuffer (type value)
	{
		switch ( value )
		{
			case UniformBlock :
			case StorageBlock :		return true;
		}
		return false;
	}


	ND_ inline bool  EShaderVariable::IsTexture (type value)
	{
		return !!(value & _vtypeinfo::_SAMPLER) && !!(value & _vtypeinfo::_SAMP_MASK);
	}
	

	ND_ inline bool  EShaderVariable::IsShadowTexture (type value)
	{
		const uint	samp = value & _vtypeinfo::_SAMP_MASK;
		switch ( samp ) {
			case _vtypeinfo::_SAMP_1DS :
			case _vtypeinfo::_SAMP_1DAS :
			case _vtypeinfo::_SAMP_2DS :
			case _vtypeinfo::_SAMP_2DAS :
			case _vtypeinfo::_SAMP_CUBES :
				return true;
		}
		return false;
	}


	ND_ inline bool  EShaderVariable::IsImage (type value)
	{
		return !!(value & _vtypeinfo::_IMAGE);
	}
	

	ND_ inline bool  EShaderVariable::IsScalar (type value)
	{
		return VecSize( value ) == 1;
	}


	ND_ inline bool  EShaderVariable::IsFloat (type value)
	{
		return IsFloat32( value ) or IsFloat64( value );
	}

	ND_ inline bool  EShaderVariable::IsFloat32 (type value)
	{
		return (value & _vtypeinfo::_TYPE_MASK) == _vtypeinfo::_FLOAT;
	}

	ND_ inline bool  EShaderVariable::IsFloat64 (type value)
	{
		return (value & _vtypeinfo::_TYPE_MASK) == _vtypeinfo::_DOUBLE;
	}
	
	
	ND_ inline bool  EShaderVariable::IsInt (type value)
	{
		return IsInt32( value ) or IsInt64( value );
	}

	ND_ inline bool  EShaderVariable::IsInt32 (type value)
	{
		return (value & _vtypeinfo::_TYPE_MASK) == _vtypeinfo::_INT;
	}

	ND_ inline bool  EShaderVariable::IsInt64 (type value)
	{
		return (value & _vtypeinfo::_TYPE_MASK) == _vtypeinfo::_LONG;
	}
	
	ND_ inline bool  EShaderVariable::IsUnsigned (type value)
	{
		return (value & _vtypeinfo::_UNSIGNED) == _vtypeinfo::_UNSIGNED;
	}

	
	ND_ inline bool  EShaderVariable::IsBool (type value)
	{
		return (value & _vtypeinfo::_BOOL) == _vtypeinfo::_BOOL;
	}


	ND_ inline BytesU  EShaderVariable::SizeOf (type value, BytesU rowAlign)
	{
		switch ( value )
		{
			case Bool :
			case Float :
			case UInt :
			case Int :			return AlignToLarge( 4_b, rowAlign );

			case Bool2 :
			case Float2 :
			case UInt2 :
			case Int2 :			return AlignToLarge( 4_b * 2, rowAlign );

			case Bool3 :
			case Float3 :
			case UInt3 :
			case Int3 :			return AlignToLarge( 4_b * 3, rowAlign );

			case Bool4 :
			case Float4 :
			case UInt4 :
			case Int4 :			return AlignToLarge( 4_b * 4, rowAlign );

			case Double :
			case Long :
			case ULong :		return AlignToLarge( 8_b, rowAlign );

			case Double2 :
			case Long2 :
			case ULong2 :		return AlignToLarge( 8_b * 2, rowAlign );

			case Double3 :
			case Long3 :
			case ULong3 :		return AlignToLarge( 8_b * 3, rowAlign );

			case Double4 :
			case Long4 :
			case ULong4 :		return AlignToLarge( 8_b * 4, rowAlign );

			case Float2x2 :		return AlignToLarge( 4_b * 2, rowAlign ) * 2;
			case Float2x3 :		return AlignToLarge( 4_b * 3, rowAlign ) * 2;
			case Float2x4 :		return AlignToLarge( 4_b * 4, rowAlign ) * 2;
			case Float3x2 :		return AlignToLarge( 4_b * 2, rowAlign ) * 3;
			case Float3x3 :		return AlignToLarge( 4_b * 3, rowAlign ) * 3;
			case Float3x4 :		return AlignToLarge( 4_b * 4, rowAlign ) * 3;
			case Float4x2 :		return AlignToLarge( 4_b * 2, rowAlign ) * 4;
			case Float4x3 :		return AlignToLarge( 4_b * 3, rowAlign ) * 4;
			case Float4x4 :		return AlignToLarge( 4_b * 4, rowAlign ) * 4;
				
			case Double2x2 :	return AlignToLarge( 8_b * 2, rowAlign ) * 2;
			case Double2x3 :	return AlignToLarge( 8_b * 3, rowAlign ) * 2;
			case Double2x4 :	return AlignToLarge( 8_b * 4, rowAlign ) * 2;
			case Double3x2 :	return AlignToLarge( 8_b * 2, rowAlign ) * 3;
			case Double3x3 :	return AlignToLarge( 8_b * 3, rowAlign ) * 3;
			case Double3x4 :	return AlignToLarge( 8_b * 4, rowAlign ) * 3;
			case Double4x2 :	return AlignToLarge( 8_b * 2, rowAlign ) * 4;
			case Double4x3 :	return AlignToLarge( 8_b * 3, rowAlign ) * 4;
			case Double4x4 :	return AlignToLarge( 8_b * 4, rowAlign ) * 4;

			//default :			return 0_b;
		}

		RETURN_ERR( "unsized variable type!" );
	}
	

	ND_ inline uint  EShaderVariable::VecSize (type value)
	{
		switch ( value )
		{
			case Double :
			case Float :
			case Long :
			case ULong :
			case Int :
			case UInt :
			case Bool :			return 1;

			case Double2 :
			case Float2 :
			case Long2 :
			case ULong2 :
			case Int2 :
			case UInt2 :
			case Bool2 :		return 2;

			case Double3 :
			case Float3 :
			case Long3 :
			case ULong3 :
			case Int3 :
			case UInt3 :
			case Bool3 :		return 3;

			case Double4 :
			case Float4 :
			case Long4 :
			case ULong4 :
			case Int4 :
			case UInt4 :
			case Bool4 :		return 4;

			default :			return 0;
		}
		//RETURN_ERR( "type is not a vector!" );
	}


	ND_ inline uint2  EShaderVariable::MatSize (type value)
	{
		switch ( value )
		{
			case Float2x2 :
			case Double2x2 :	return uint2( 2, 2 );

			case Float2x3 :
			case Double2x3 :	return uint2( 2, 3 );

			case Float2x4 :
			case Double2x4 :	return uint2( 2, 4 );

			case Float3x2 :
			case Double3x2 :	return uint2( 3, 2 );

			case Float3x3 :
			case Double3x3 :	return uint2( 3, 3 );

			case Float3x4 :
			case Double3x4 :	return uint2( 3, 4 );

			case Float4x2 :
			case Double4x2 :	return uint2( 4, 2 );

			case Float4x3 :
			case Double4x3 :	return uint2( 4, 3 );

			case Float4x4 :
			case Double4x4 :	return uint2( 4, 4 );

			default :			return uint2();
		}
		//RETURN_ERR( "type is not a matrix!" );
	}
	

	ND_ inline EShaderVariable::type  EShaderVariable::FromString (StringCRef name)
	{
		if ( name == "void" )		return Void;
		if ( name == "bool" )		return Bool;
		if ( name == "bool2" )		return Bool2;
		if ( name == "bool3" )		return Bool3;
		if ( name == "bool4" )		return Bool4;
		if ( name == "int" )		return Int;
		if ( name == "int2" )		return Int2;
		if ( name == "int3" )		return Int3;
		if ( name == "int4" )		return Int4;
		if ( name == "uint" )		return UInt;
		if ( name == "uint2" )		return UInt2;
		if ( name == "uint3" )		return UInt3;
		if ( name == "uint4" )		return UInt4;
		if ( name == "float" )		return Float;
		if ( name == "float2" )		return Float2;
		if ( name == "float3" )		return Float3;
		if ( name == "float4" )		return Float4;
		if ( name == "float2x2" )	return Float2x2;
		if ( name == "float2x3" )	return Float2x3;
		if ( name == "float2x4" )	return Float2x4;
		if ( name == "float3x2" )	return Float3x2;
		if ( name == "float3x3" )	return Float3x3;
		if ( name == "float3x4" )	return Float3x4;
		if ( name == "float4x2" )	return Float4x2;
		if ( name == "float4x3" )	return Float4x3;
		if ( name == "float4x4" )	return Float4x4;
		if ( name == "double" )		return Double;
		if ( name == "double2" )	return Double2;
		if ( name == "double3" )	return Double3;
		if ( name == "double4" )	return Double4;
		if ( name == "double2x2" )	return Double2x2;
		if ( name == "double2x3" )	return Double2x3;
		if ( name == "double2x4" )	return Double2x4;
		if ( name == "double3x2" )	return Double3x2;
		if ( name == "double3x3" )	return Double3x3;
		if ( name == "double3x4" )	return Double3x4;
		if ( name == "double4x2" )	return Double4x2;
		if ( name == "double4x3" )	return Double4x3;
		if ( name == "double4x4" )	return Double4x4;

		RETURN_ERR( "unknown type!" );
	}


}	// PipelineCompiler
