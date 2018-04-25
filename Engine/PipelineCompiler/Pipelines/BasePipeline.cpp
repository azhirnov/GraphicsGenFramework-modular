// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "Engine/PipelineCompiler/Pipelines/BasePipeline.h"
#include "Engine/PipelineCompiler/Pipelines/PipelineManager.h"
#include "Engine/PipelineCompiler/Common/ToGLSL.h"
#include "Engine/PipelineCompiler/glsl/glsl_source_vfs.h"

namespace PipelineCompiler
{

/*
=================================================
	constructor
=================================================
*/
	BasePipeline::BasePipeline (StringCRef name) :
		_path( name ),
		_name( FileAddress::GetNameAndExt(name) ),
		shaderFormat{ EShaderSrcFormat::GLSL }
	{
		usize	pos = 0;
		if ( _name.Find( '.', OUT pos ) )
		{
			_name.Erase( pos, _name.Length() - pos );
		}

		FOR( i, _name ) {
			_name[i] = StringUtils::ToLower( _name[i] );
		}

		_lastEditTime = OS::FileSystem::GetFileLastModificationTime( _path ).ToTime();

		PipelineManager::Instance()->Add( this );
	}
	
/*
=================================================
	constructor
=================================================
*/
	BasePipeline::BasePipeline (StringCRef path, StringCRef name) :
		_path( path ),
		_name( name ),
		shaderFormat{ EShaderSrcFormat::GLSL }
	{
		_lastEditTime = OS::FileSystem::GetFileLastModificationTime( _path ).ToTime();

		PipelineManager::Instance()->Add( this );
	}
	
/*
=================================================
	destructor
=================================================
*/
	BasePipeline::~BasePipeline ()
	{
		PipelineManager::Instance()->Remove( this );
	}
	
/*
=================================================
	Depends
=================================================
*/
	void BasePipeline::Depends (StringCRef filename)
	{
		CHECK_ERR( OS::FileSystem::IsFileExist( filename ), void() );

		_lastEditTime = Max( _lastEditTime, OS::FileSystem::GetFileLastModificationTime( filename ).ToTime() );
	}

/*
=================================================
	Path
=================================================
*/
	String BasePipeline::Path () const
	{
		if ( not OS::FileSystem::IsAbsolutePath( _path ) )
		{
			String	str;
			OS::FileSystem::GetCurrentDirectory( OUT str );

			str.ReplaceChars( '\\', '/' );
			str = FileAddress::BuildPath( str, _path );

			ASSERT( OS::FileSystem::IsFileExist( str ) );
			return FileAddress::ToShortPath( str, 3 );
		}

		return FileAddress::ToShortPath( _path, 3 );
	}

/*
=================================================
	_VaryingsToString
----
	pass 1 & 2
=================================================
*/
	void BasePipeline::_VaryingsToString (const Array<Varying> &varyings, OUT String &str)
	{
		const BytesU	vec4_align	= EShaderVariable::SizeOf( EShaderVariable::Float4, 0_b );

		FOR( i, varyings )
		{
			const auto&		var = varyings[i];

			if ( EShaderVariable::IsStruct( var.type ) )
			{
				if ( var.location != UMax )
					str << "layout(location=" << var.location << ") ";
			
				str << ToStringGLSL( var.qualifier ) << " " << var.typeName;
			}
			else
			{
				if ( var.location != UMax )
					str << "layout(location=" << var.location << ") ";
			
				str << ToStringGLSL( var.qualifier ) << " "
					<< ToStringGLSL( var.precision ) << " "
					<< ToStringGLSL( var.type );
			}

			str << " " << var.name;

			if ( var.arraySize == 0 )
				str << "[]";
			else
			if ( var.arraySize > 1 )
				str << "[" << var.arraySize << "]";

			str << ";\n";
		}
	}
	
/*
=================================================
	_AddStructType
----
	pass 1 & 2
=================================================
*/
	bool BasePipeline::_AddStructType (const _StructField &structType, INOUT StructTypes &currTypes)
	{
		// temp
		ASSERT( not structType.typeName.StartsWith("gl_") );
		ASSERT( not structType.name.StartsWith("gl_") );

		StructTypes::iterator	iter;

		if ( not currTypes.Find( structType.typeName, OUT iter ) )
		{
			_StructField	st_type = structType;
			st_type.arraySize = 1;

			currTypes.Add( structType.typeName, st_type );
			return true;
		}

		Array<StringCRef>	tokens_left;
		Array<StringCRef>	tokens_right;

		// compare struct info
		CHECK_ERR(	iter->second.precision	== structType.precision	and
					iter->second.qualifier	== structType.qualifier );

		iter->second.stride		= Max( structType.stride, iter->second.stride );
		iter->second.align		= Max( structType.align, iter->second.align );
		iter->second.packing	|= structType.packing;
		iter->second.arraySize	= 1;
		iter->second.memoryModel = EShaderMemoryModel::Default;

		// compare fields
		if ( structType.fields.Count() == iter->second.fields.Count() )
		{
			FOR( j, structType.fields )
			{
				auto const&		left	= structType.fields[j];
				auto&			right	= iter->second.fields[j];
				
				// temp
				ASSERT( not left.name.StartsWith("gl_") );

				if ( left != right )
				{
					// merge fields
					CHECK_ERR(	left.name			== right.name		and
								left.typeName		== right.typeName	and
								left.type			== right.type		and
								left.arraySize		== right.arraySize	and
								left.precision		== right.precision	and
								left.qualifier		== right.qualifier	and
								left.memoryModel	== right.memoryModel );

					right.packing		|= left.packing;
					right.offset		= Max( left.offset, right.offset );
					right.align			= Max( left.align,  right.align  );
					right.stride		= Max( left.stride, right.stride );
				}
			}
			return true;
		}

		RETURN_ERR( "incompatible fields in structures with same typename" );
	}
	
/*
=================================================
	_GetVersionGLSL
----
	pass 1 & 2
=================================================
*/
	String BasePipeline::_GetVersionGLSL ()
	{
		return "#version "_str << ShaderCompiler::GLSL_VERSION << " core\n";
	}

/*
=================================================
	_GetDefaultHeaderGLSL
----
	pass 1 & 2
=================================================
*/
	StringCRef BasePipeline::_GetDefaultHeaderGLSL ()
	{
		static const char	header[] = R"#(
#define SH_VERTEX           (1<<0)
#define SH_TESS_CONTROL     (1<<1)
#define SH_TESS_EVALUATION  (1<<2)
#define SH_GEOMETRY         (1<<3)
#define SH_FRAGMENT         (1<<4)
#define SH_COMPUTE          (1<<5)

#ifdef GL_ARB_gpu_shader_int64
#extension GL_ARB_gpu_shader_int64 : require
//#define ARB_gpu_shader_int64_enabled  1
#endif

// for vulkan compatibility
#ifdef GL_ARB_separate_shader_objects
#extension GL_ARB_separate_shader_objects : enable
#define ARB_separate_shader_objects_enabled  1
#endif

#define bool2		bvec2
#define bool3		bvec3
#define bool4		bvec4

#define int2		ivec2
#define int3		ivec3
#define int4		ivec4

#define uint2		uvec2
#define uint3		uvec3
#define uint4		uvec4

#ifdef ARB_gpu_shader_int64_enabled
# define ilong		int64_t
# define ilong2		i64vec2
# define ilong3		i64vec3
# define ilong4		i64vec4
# define ulong		uint64_t
# define ulong2		u64vec2
# define ulong3		u64vec3
# define ulong4		u64vec4
#endif // ARB_gpu_shader_int64_enabled

#define float2		vec2
#define float3		vec3
#define float4		vec4
			
#define float2x2	mat2x2
#define float2x3	mat2x3
#define float2x4	mat2x4
#define float3x2	mat3x2
#define float3x3	mat3x3
#define float3x4	mat3x4
#define float4x2	mat4x2
#define float4x3	mat4x3
#define float4x4	mat4x4

#define double2		dvec2
#define double3		dvec3
#define double4		dvec4
			
#define double2x2	dmat2x2
#define double2x3	dmat2x3
#define double2x4	dmat2x4
#define double3x2	dmat3x2
#define double3x3	dmat3x3
#define double3x4	dmat3x4
#define double4x2	dmat4x2
#define double4x3	dmat4x3
#define double4x4	dmat4x4

#ifdef VULKAN
#define PUSH_CONSTANT( _name_ )	layout (std140, push_constant) uniform _name_
#else
#define PUSH_CONSTANT( _name_ )	layout (std140) uniform pushConst_##_name_
#endif
		)#";
		return header;
	}
	
/*
=================================================
	_GetPerShaderHeaderGLSL
----
	pass 1 & 2
=================================================
*/
	StringCRef  BasePipeline::_GetPerShaderHeaderGLSL (EShader::type type)
	{
		switch ( type )
		{
			case EShader::Vertex :
				return	"#define SHADER	SH_VERTEX\n"
						"#ifdef VULKAN\n"
						"# define gl_VertexID    gl_VertexIndex\n"
						"# define gl_InstanceID  gl_InstanceIndex\n"
						"#else\n"
						"# define gl_VertexIndex    gl_VertexID\n"
						"# define gl_InstanceIndex  gl_InstanceID\n"
						"#endif\n"
						"out gl_PerVertex {\n"
						"	vec4 gl_Position;\n"
						"	float gl_PointSize;\n"
						"	float gl_ClipDistance[];\n"
						"	float gl_CullDistance[];\n"
						"};\n";

			case EShader::TessControl :
				return	"#define SHADER	SH_TESS_CONTROL\n";

			case EShader::TessEvaluation :
				return	"#define SHADER	SH_TESS_EVALUATION\n";

			case EShader::Geometry :
				return	"#define SHADER	SH_GEOMETRY\n";

			case EShader::Fragment :
				return	"#define SHADER	SH_FRAGMENT\n";

			case EShader::Compute :
				return	"#define SHADER	SH_COMPUTE\n";
		}
		RETURN_ERR( "unsupported shader type!" );
	}
	
/*
=================================================
	_OnCompilationFailed
----
	pass 1 & 2
=================================================
*/
	bool BasePipeline::_OnCompilationFailed (EShader::type shaderType, EShaderSrcFormat::type fmt, ArrayCRef<StringCRef> source, StringCRef log) const
	{
		String	str;

		str << EShader::ToString( shaderType ) << " shader \"" << Name() << "\" in \""
			<< Path() << "\" compilation error\n---------------\n" << log;
		
		LOG( str, ELog::Error | ELog::OpenSpoilerFlag );
		return false;
	}
//=========================================================


	
/*
=================================================
	Bindings::Texture
=================================================
*/
	BasePipeline::Bindings&  BasePipeline::Bindings::Texture (EImage::type imageType, StringCRef name, EShader::bits shaderUsage, EPixelFormatClass::type format)
	{
		TextureUniform	tex;
		tex.name		= name;
		tex.imageType	= imageType;
		tex.format		= format;
		tex.shaderUsage	= shaderUsage;

		uniforms.PushBack( _Uniform( RVREF(tex) ) );
		return *this;
	}
	
/*
=================================================
	Bindings::Sampler
=================================================
*/
	BasePipeline::Bindings&  BasePipeline::Bindings::Sampler (StringCRef texName, const SamplerDescriptor &descr, bool canBeOverridden)
	{
		FOR( i, uniforms )
		{
			auto&	un = uniforms[i];

			if ( un.Is<TextureUniform>() )
			{
				auto&	tex = un.Get<TextureUniform>();

				if ( tex.name == texName )
				{
					tex.defaultSampler			= descr;
					tex.samplerCanBeOverridden	= canBeOverridden;
					return *this;
				}
			}
		}

		RETURN_ERR( "Can't find texture uniform with name '" << texName << "'", *this );
	}
	
/*
=================================================
	Bindings::Image
=================================================
*/
	BasePipeline::Bindings&  BasePipeline::Bindings::Image (EImage::type imageType, StringCRef name, EPixelFormat::type format,
															EShader::bits shaderUsage, EShaderMemoryModel::type access)
	{
		ImageUniform	img;
		img.name		= name;
		img.imageType	= imageType;
		img.format		= format;
		img.memoryModel	= access;
		img.shaderUsage	= shaderUsage;
		
		uniforms.PushBack( _Uniform( RVREF(img) ) );
		return *this;
	}
	
/*
=================================================
	Bindings::UniformBuffer
=================================================
*/
	BasePipeline::Bindings&  BasePipeline::Bindings::UniformBuffer (StringCRef name, StringCRef typeName, EShader::bits shaderUsage)
	{
		BasePipeline::UniformBuffer	buf;
		buf.name		= name;
		buf.typeName	= typeName;
		buf.shaderUsage	= shaderUsage;

		uniforms.PushBack( _Uniform( RVREF(buf) ) );
		return *this;
	}
	
/*
=================================================
	Bindings::StorageBuffer
=================================================
*/
	BasePipeline::Bindings&  BasePipeline::Bindings::StorageBuffer (StringCRef name, StringCRef typeName, EShader::bits shaderUsage,
																	EShaderMemoryModel::type access)
	{
		BasePipeline::StorageBuffer	buf;
		buf.name		= name;
		buf.typeName	= typeName;
		buf.shaderUsage	= shaderUsage;
		buf.memoryModel	= access;

		uniforms.PushBack( _Uniform( RVREF(buf) ) );
		return *this;
	}
//=========================================================

	
	
/*
=================================================
	Location::BindingToStringGLSL
=================================================
*/
	String  BasePipeline::Location::BindingToStringGLSL (EShaderType shaderApi) const
	{
		String	str;

		switch ( shaderApi )
		{
			case EShaderType::None :
				break;

			case EShaderType::GLSL :
				if ( index != UMax ) {
					str << "layout(binding=" << index << ") ";
				}
				break;
				
			case EShaderType::CL :
			case EShaderType::HLSL :
			case EShaderType::Software :
				if ( uniqueIndex != UMax ) {
					str << "layout(binding=" << uniqueIndex << ") ";
				}
				break;

			case EShaderType::SPIRV :
				if ( uniqueIndex != UMax ) {
					str << "layout(binding=" << uniqueIndex << (descriptorSet != UMax ? ", set="_str << descriptorSet : "") << ") ";
				}
				break;

			case EShaderType::GLSL_ES_2 :
			case EShaderType::GLSL_ES_3 :
			default :
				WARNING( "not supported" );
		}
		return str;
	}

/*
=================================================
	Location::LocationToStringGLSL
=================================================
*/
	String  BasePipeline::Location::LocationToStringGLSL (EShaderType shaderApi) const
	{
		String	str;
		
		switch ( shaderApi )
		{
			case EShaderType::None :
				break;

			case EShaderType::GLSL :
			case EShaderType::GLSL_ES_3 :
				if ( index != UMax ) {
					str << "layout(location=" << index << ") ";
				}
				break;

			default :
				WARNING( "not supported" );
		}
		return str;
	}
//=========================================================



/*
=================================================
	TextureUniform::ToStringGLSL
=================================================
*/
	String  BasePipeline::TextureUniform::ToStringGLSL (EShaderType shaderApi) const
	{
		bool	is_shadow = defaultSampler.IsDefined() and defaultSampler->CompareOp() != ECompareFunc::None;
		
		return	location.BindingToStringGLSL( shaderApi ) << "uniform " <<
				PipelineCompiler::ToStringGLSL( EShaderVariable::ToSampler( imageType, is_shadow, format ) ) << " " <<
				name << ";\n";
	}
//=========================================================

	

/*
=================================================
	ImageUniform::ToStringGLSL
=================================================
*/
	String  BasePipeline::ImageUniform::ToStringGLSL (EShaderType shaderApi) const
	{
		return	location.BindingToStringGLSL( shaderApi ) << "layout(" << PipelineCompiler::ToStringGLSL( format ) << ") " <<
				PipelineCompiler::ToStringGLSL( memoryModel ) << " uniform " <<
				PipelineCompiler::ToStringGLSL( EShaderVariable::ToImage( imageType, format ) ) << " " << name << ";\n";
	}
//=========================================================

	
	
/*
=================================================
	_StructField::operator ==
=================================================
*/
	bool BasePipeline::_StructField::operator == (const _StructField &right) const
	{
		return	this->name			== right.name			and
				this->type			== right.type			and
				this->typeName		== right.typeName		and
				this->arraySize		== right.arraySize		and
				this->offset		== right.offset			and
				this->align			== right.align			and
				this->stride		== right.stride			and
				this->memoryModel	== right.memoryModel	and
				this->packing		== right.packing		and
				this->precision		== right.precision		and
				this->qualifier		== right.qualifier;
	}
//=========================================================



/*
=================================================
	UniformBuffer::ToStringGLSL
=================================================
*/
	String  BasePipeline::UniformBuffer::ToStringGLSL (StringCRef fields, EShaderType shaderApi) const
	{
		return	location.BindingToStringGLSL( shaderApi ) << "layout(" << PipelineCompiler::ToStringGLSL( packing ) << ") " <<
				"uniform " << typeName << " {\n" << fields << "\n} " << name << ";\n";
	}
//=========================================================

	

/*
=================================================
	StorageBuffer::ToStringGLSL
=================================================
*/
	String  BasePipeline::StorageBuffer::ToStringGLSL (StringCRef fields, EShaderType shaderApi) const
	{
		String	str;
		str << location.BindingToStringGLSL( shaderApi ) << "layout(" << PipelineCompiler::ToStringGLSL( packing ) << ") ";

		if ( memoryModel != EShaderMemoryModel::Default )
			str << PipelineCompiler::ToStringGLSL( memoryModel );

		str << " buffer " << typeName << " {\n" << fields << "\n} " << name << ";\n";
		return str;
	}
//=========================================================
	

	
/*
=================================================
	constructor
=================================================
*/
	BasePipeline::ShaderModule::ShaderModule (const ShaderModule &other) :
		_source{other._source}, _io{other._io}, entry{other.entry}, type{other.type}
	{}

/*
=================================================
	ShaderModule::Load
=================================================
*/
	BasePipeline::ShaderModule&  BasePipeline::ShaderModule::Load (StringCRef path, StringCRef defines)
	{
		CHECK_ERR( not path.Empty(), *this );

		String	src;

		// load shader from standart library
		if ( path.Length() > 2 and path.Front() == '<' and path.Back() == '>' )
		{
			CHECK_ERR( glsl_vfs::LoadFile( path.SubString( 1, path.Length()-2 ), OUT src ), *this );
		}
		else
		// load from file system
		{
			File::RFilePtr	file;
			CHECK_ERR( (file = File::HddRFile::New( path )), *this );
		
			const usize	len	= usize(file->RemainingSize());
			src.Resize( len );

			CHECK_ERR( file->Read( src.ptr(), src.LengthInBytes() ), *this );

			_maxEditTime = Max( _maxEditTime, OS::FileSystem::GetFileLastModificationTime( path ).ToTime() );
		}

		defines >> src;

		this->_source.PushBack( RVREF(src) );
		return *this;
	}
	
/*
=================================================
	ShaderModule::Load
=================================================
*/
	BasePipeline::ShaderModule&  BasePipeline::ShaderModule::Load (StringCRef path)
	{
		return Load( path, "" );
	}

	BasePipeline::ShaderModule&  BasePipeline::ShaderModule::Load (StringCRef path, StringCRef filename, StringCRef defines)
	{
		String	fname = FileAddress::BuildPath( FileAddress::GetPath( path ), FileAddress::GetNameAndExt( filename ) );
		return Load( fname, defines );
	}
	
/*
=================================================
	ShaderModule::Source
=================================================
*/
	BasePipeline::ShaderModule&  BasePipeline::ShaderModule::Source (StringCRef src)
	{
		this->_source.PushBack( src );
		return *this;
	}
	
/*
=================================================
	ShaderModule::Depends
=================================================
*/
	BasePipeline::ShaderModule&  BasePipeline::ShaderModule::Depends (StringCRef filename)
	{
		CHECK_ERR( OS::FileSystem::IsFileExist( filename ), *this );

		_maxEditTime = Max( _maxEditTime, OS::FileSystem::GetFileLastModificationTime( filename ).ToTime() );
		return *this;
	}

/*
=================================================
	ShaderModule::IsEnabled
=================================================
*/
	bool BasePipeline::ShaderModule::IsEnabled () const
	{
		return not _source.Empty();
	}
	
/*
=================================================
	ShaderModule::LastEditTime
=================================================
*/
	TimeL BasePipeline::ShaderModule::LastEditTime () const
	{
		return _maxEditTime;
	}

/*
=================================================
	ShaderModule::operator =
=================================================
*/
	BasePipeline::ShaderModule&  BasePipeline::ShaderModule::operator = (const ShaderModule &right)
	{
		ASSERT( type == right.type );

		_source = right._source;
		_io		= right._io;
		entry	= right.entry;

		return *this;
	}
//=========================================================


	
/*
=================================================
	Varying::operator ==
=================================================
*/
	bool BasePipeline::Varying::operator == (const Varying &right) const
	{
		return	name		== right.name		and
				type		== right.type		and
				precision	== right.precision	and
				qualifier	== right.qualifier	and
				location	== right.location;
	}

}	// PipelineCompiler
