// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "Engine/PipelineCompiler/Pipelines/BasePipeline.h"
#include "Engine/PipelineCompiler/Common/ToGLSL.h"
#include "Engine/PipelineCompiler/Common/Packing.h"

namespace PipelineCompiler
{
	
/*
=================================================
	_StructToString
=================================================
*/
	String  BasePipeline::_StructToString (const StructTypes &types, StringCRef typeName, bool skipLayouts)
	{
		String						str;
		StructTypes::const_iterator	iter;

		if ( types.Find( typeName, OUT iter ) )
		{
			const bool	is_block	= iter->second.type == EShaderVariable::UniformBlock or
									  iter->second.type == EShaderVariable::StorageBlock;

			for (auto& fld : iter->second.fields)
			{
				str << "\t";

				if ( not skipLayouts and is_block )
				{
					str << "layout(offset=" << usize(fld.offset)
						<< ", align=" << usize(fld.align) << ") ";

					if ( fld.memoryModel != EShaderMemoryModel::Default )
						str << ToStringGLSL( fld.memoryModel ) << ' ';
				}

				if ( EShaderVariable::IsStruct( fld.type ) )
					str << fld.typeName;
				else
					str << ToStringGLSL( fld.type );

				str << "  " << fld.name
					<< (fld.arraySize.IsNotArray() ? "" : fld.arraySize.IsDynamicArray() ? " []" : " ["_str << fld.arraySize.Size() << "]")
					<< ';';
				
				if ( skipLayouts or not is_block )
				{
					str << "	// offset: " << usize(fld.offset) << ", align: " << usize(fld.align);
				}
				str << "\n";
			}
		}
		return str;
	}

/*
=================================================
	_BindingsToString_Func
=================================================
*/
	struct BasePipeline::_BindingsToString_Func
	{
	// variables
		BasePipeline const *		_pp;
		String	&					_str;
		EShader::type				_shaderType;
		EShaderFormat::type		_shaderApi;
		bool						_useOriginTypes;
		bool						_skipBufferLayouts;


	// methods
		_BindingsToString_Func (BasePipeline const *pp, EShader::type shaderType, EShaderFormat::type shaderApi, bool useOriginTypes, OUT String &str) :
			_pp(pp),							_str(str),
			_shaderType(shaderType),			_shaderApi(shaderApi),				
			_useOriginTypes(useOriginTypes),	_skipBufferLayouts(shaderApi == EShaderFormat::Vulkan)
		{}


		void operator () (const TextureUniform &tex) const
		{
			if ( not tex.shaderUsage[ _shaderType ] )
				return;

			_str << tex.ToStringGLSL( _shaderApi );
		}


		void operator () (const ImageUniform &img) const
		{
			if ( not img.shaderUsage[ _shaderType ] )
				return;

			_str << img.ToStringGLSL( _shaderApi );
		}


		void operator () (const SubpassInput &spi) const
		{
			if ( not spi.shaderUsage[ _shaderType ] )
				return;

			_str << spi.ToStringGLSL( _shaderApi );
		}


		void operator () (const UniformBuffer &ub) const
		{
			if ( not ub.shaderUsage[ _shaderType ] )
				return;

			_str << ub.ToStringGLSL( _StructToString( ub.typeName ), _shaderApi );
		}


		void operator () (const StorageBuffer &ssb) const
		{
			if ( not ssb.shaderUsage[ _shaderType ] )
				return;

			_str << ssb.ToStringGLSL( _StructToString( ssb.typeName ), _shaderApi );
		}

		String _StructToString (StringCRef typeName) const
		{
			return BasePipeline::_StructToString( (_useOriginTypes ? _pp->_originTypes : _pp->_structTypes), typeName, _skipBufferLayouts );
		}
	};
	
/*
=================================================
	_BindingsToString
=================================================
*/
	void BasePipeline::_BindingsToString (EShader::type shaderType, EShaderFormat::type shaderApi, bool useOriginTypes, OUT String &str) const
	{
		_BindingsToString_Func	func( this, shaderType, shaderApi, useOriginTypes, OUT str );

		FOR( i, bindings.uniforms )
		{
			const auto&		un = bindings.uniforms[i];

			un.Accept( func );
			str << '\n';
		}
	}

/*
=================================================
	_MergeStructTypes
=================================================
*/
	bool BasePipeline::_MergeStructTypes (const StructTypes &newTypes, INOUT StructTypes &currTypes)
	{
		FOR( i, newTypes )
		{
			const auto&		st = newTypes[i];

			CHECK_ERR( _AddStructType( st.second, INOUT currTypes ) );
		}
		return true;
	}
	
/*
=================================================
	_StructsToString
=================================================
*/
	bool BasePipeline::_StructsToString (const StructTypes &structTypes, OUT String &glslSource)
	{
		using StArray_t	= Array< StructTypes::CPair_t const *>;

		HashSet<StringCRef>		defined;	defined.Reserve( structTypes.Count() );
		StArray_t				sorted;		sorted.Reserve( structTypes.Count() );
		StArray_t				pending;	pending.Resize( structTypes.Count() );
		
		FOR( i, structTypes ) {
			pending[i] = &structTypes[i];
		}

		// sort by dependencies of other types
		for (bool changed = true; changed;)
		{
			changed = false;

			FOR( i, pending )
			{
				bool	all_deps_defined = true;

				// check fields
				for (auto& fd : pending[i]->second.fields)
				{
					if ( EShaderVariable::IsStruct( fd.type ) )
					{
						all_deps_defined &= defined.IsExist( fd.typeName );
					}
				}

				if ( all_deps_defined )
				{
					changed = true;
					sorted  << pending[i];
					defined << pending[i]->first; 

					pending.Erase( i );
					--i;
				}
			}
		}
		CHECK_ERR( pending.Empty() );

		// serialize
		FOR( i, sorted )
		{
			auto const&	st = *sorted[i];
			
			// keep struct, varying
			if ( st.second.type != EShaderVariable::Struct )
				continue;

			glslSource << "struct " << st.second.typeName << "\n{\n";

			// staticaly sized UB or SSB
			FOR( j, st.second.fields )
			{
				const auto&		fld			= st.second.fields[j];
				const String	type_name	= EShaderVariable::IsStruct( fld.type ) ? StringCRef(fld.typeName) : ToStringGLSL( fld.type );
				const uint		align		= uint(fld.align);
				const uint		offset		= uint(fld.offset);

				glslSource	<< "\t" << type_name << "  " << fld.name
							<< (fld.arraySize.IsNotArray() ? "" : fld.arraySize.IsDynamicArray() ? " []" : " ["_str << fld.arraySize.Size() << "]")
							<< ";  // offset: " << offset << ", align: " << align << "\n";
			}

			glslSource << "};\n\n";
		}

		return true;
	}
	
	
/*
=================================================
	_SerializeStructs
=================================================
*/
	bool BasePipeline::_SerializeStructs (const StructTypes &structTypes, Ptr<ISerializer> ser, OUT String &serialized)
	{
		using StArray_t	= Array< StructTypes::CPair_t const *>;

		HashSet<StringCRef>		defined;	defined.Reserve( structTypes.Count() );
		StArray_t				sorted;		sorted.Reserve( structTypes.Count() );
		StArray_t				pending;	pending.Resize( structTypes.Count() );
		String &				str		= serialized;
		
		FOR( i, structTypes ) {
			pending[i] = &structTypes[i];
		}

		// sort by dependencies of other types
		for (bool changed = true; changed;)
		{
			changed = false;

			FOR( i, pending )
			{
				bool	all_deps_defined = true;

				// check fields
				for (auto& fd : pending[i]->second.fields)
				{
					if ( EShaderVariable::IsStruct( fd.type ) )
					{
						all_deps_defined &= defined.IsExist( fd.typeName );
					}
				}

				if ( all_deps_defined )
				{
					changed = true;
					sorted  << pending[i];
					defined << pending[i]->first; 

					pending.Erase( i );
					--i;
				}
			}
		}
		CHECK_ERR( pending.Empty() );

		// serialize
		FOR( i, sorted )
		{
			auto const&	st = *sorted[i];

			// keep struct, buffer, vertex
			if ( st.second.packing[ EVariablePacking::Varying ] )
				continue;
			
			str << ser->Comment( "Packing: "_str << EVariablePacking::ToString(st.second.packing) )
				<< ser->BeginStruct( st.second.typeName, uint(st.second.stride), true );

			// staticaly sized UB or SSB
			FOR( j, st.second.fields )
			{
				const auto&		fld			= st.second.fields[j];
				const String	type_name	= EShaderVariable::IsStruct( fld.type ) ? fld.typeName : ser->ToString( fld.type );
				const uint		align		= uint(fld.align);
				const uint		offset		= uint(fld.offset);
				const uint		size		= j+1 < st.second.fields.Count() ?
												uint(st.second.fields[j+1].offset - st.second.fields[j].offset) :
												fld.arraySize.Size() * (EShaderVariable::IsStruct( fld.type ) ?
													uint(fld.stride) :
													uint(EShaderVariable::SizeOf( fld.type, 0_b )));

				if ( fld.arraySize.IsDynamicArray() ) {
					ASSERT( j == st.second.fields.LastIndex() );
				} else
				if ( fld.arraySize.IsNotArray() ) {
					str << ser->StructField( fld.name, type_name, fld.arraySize, offset, align, size );
				} else {
					str << ser->StructField( fld.name, type_name, fld.arraySize, offset, align, size );
				}
			}
				
			// SSB with dynamic array
			if ( st.second.fields.Back().arraySize.IsDynamicArray() )
			{
				const auto&		arr			= st.second.fields.Back();
				const String	type_name	= EShaderVariable::IsStruct( arr.type ) ? arr.typeName : ser->ToString( arr.type );
				ASSERT( arr.arraySize.IsDynamicArray() and arr.stride > 0_b );

				str << "\n\t";
				str << ser->Comment( "Element  "_str << arr.name << "[];   offset: " << uint(arr.offset) << ", align: " << uint(arr.align) );
				str << ser->BeginStruct( "Element", uint(arr.stride), true );
				str << ser->StructField( arr.name, type_name, ArraySize(), 0, uint(arr.align), uint(arr.stride) );
				str << ser->EndStruct();
			}

			if ( st.second.packing[ EVariablePacking::VertexAttrib ] )
			{
				str << ser->StructCtorForInitializerList()
					<< ser->StructVertexAttribBinding();
			}
			
			str << ser->EndStruct() << '\n';
		}

		return true;
	}

/*
=================================================
	_BindingsToLayout_Func
=================================================
*/
	struct BasePipeline::_BindingsToLayout_Func
	{
	// types
		using Uniform	= PipelineLayoutDescription::Uniform_t;
		

	// variables
		PipelineLayoutDescription::Builder&	layout;
		

	// methods
		explicit _BindingsToLayout_Func (PipelineLayoutDescription::Builder &layout) :
			layout(layout)
		{}

		void operator () (const TextureUniform &src) const
		{
			layout.AddTexture( src.name, src.imageType, src.format, src.location.index, src.location.uniqueIndex, src.shaderUsage );
		}

		void operator () (const ImageUniform &src) const
		{
			layout.AddImage( src.name, src.imageType, src.format, src.memoryModel, src.location.index, src.location.uniqueIndex, src.shaderUsage );
		}

		void operator () (const SubpassInput &src) const
		{
			layout.AddSubpass( src.name, src.attachmentIndex, src.isMultisample, src.location.index, src.location.uniqueIndex, src.shaderUsage );
		}

		void operator () (const UniformBuffer &src) const
		{
			layout.AddUniformBuffer( src.name, src.size, src.location.index, src.location.uniqueIndex, src.shaderUsage );
		}

		void operator () (const StorageBuffer &src) const
		{
			layout.AddStorageBuffer( src.name, src.staticSize, src.arrayStride, src.memoryModel, src.location.index, src.location.uniqueIndex, src.shaderUsage );
		}

		// TODO:
		//	PipelineLayoutDescription::SamplerUniform
		//	PipelineLayoutDescription::PushConstants
	};
	
/*
=================================================
	_ConvertLayout
=================================================
*/
	bool BasePipeline::_ConvertLayout (StringCRef name, INOUT String &src, Ptr<ISerializer> ser) const
	{
		PipelineLayoutDescription::Builder	builder;

		_BindingsToLayout_Func		func( OUT builder );

		FOR( i, bindings.uniforms ) {
			bindings.uniforms[i].Accept( func );
		}

		src << ser->ToString( name, builder.Finish() ) << '\n';
		return true;
	}
	
/*
=================================================
	_TypeReplacer
=================================================
*/
	bool BasePipeline::_TypeReplacer (StringCRef typeName, INOUT ShaderCompiler::FieldTypeInfo &field) const
	{
		StructTypes::iterator	iter;

		if ( _structTypes.Find( typeName, OUT iter ) )
		{
			FOR( i, iter->second.fields )
			{
				const auto&		fl = iter->second.fields[i];

				if ( fl.name == field.name )
				{
					//field.index	= i;	// not needed, index will be replaced later
					field.type	= fl.type;
					return true;
				}
			}
		}
		return false;
	}
	
/*
=================================================
	_CalculateOffsets_Helper
=================================================
*/
	struct BasePipeline::_CalculateOffsets_Helper final : CalculateOffsets_Helper<BasePipeline::_StructField>
	{
	// variables
		StructTypes&	structTypes;


	// methods
		explicit _CalculateOffsets_Helper (StructTypes& structTypes) :
			structTypes(structTypes)
		{}


		bool ProcessStruct (INOUT BytesU &offset, INOUT Variable &var, EVariablePacking::type packing) override
		{
			if ( var.fields.Empty() )
			{
				StructTypes::iterator	iter;
				CHECK_ERR( structTypes.Find( var.typeName, OUT iter ) );
				
				Variable	st_type	= iter->second;
				BytesU		off		= offset;
				CHECK_ERR( Self::ProcessStruct( INOUT off, INOUT st_type, packing ) );

				var.offset	= st_type.offset;
				var.align	= st_type.align;
				var.stride	= st_type.stride;
				offset		= var.offset + var.stride * var.arraySize.Size();
				return true;
			}

			CHECK_ERR( Self::ProcessStruct( INOUT offset, INOUT var, packing ) );
			return true;
		}
	};

/*
=================================================
	_CalculateOffsets
=================================================
*/
	bool BasePipeline::_CalculateOffsets (INOUT StructTypes &structTypes)
	{
		_CalculateOffsets_Helper	helper( structTypes );

		FOR( i, structTypes )
		{
			auto&	var = structTypes[i].second;

			// skip varyings and vertices
			if ( var.packing == EVariablePacking::Varying or var.packing == EVariablePacking::VertexAttrib )
				continue;

			BytesU	offset;
			CHECK_ERR( helper.ProcessStruct( INOUT offset, INOUT var, EVariablePacking::GetMaxPacking( var.packing ) ) );
		}
		return true;
	}
	
/*
=================================================
	_ReplaceTypes_Helper
=================================================
*/
	struct BasePipeline::_ReplaceTypes_Helper final : ReplaceTypes_Helper<BasePipeline::_StructField>
	{
	// variables
		StructTypes&	structTypes;
	
	// methods
		explicit _ReplaceTypes_Helper (StructTypes& structTypes) :
			structTypes(structTypes)
		{}

		void ReplaceVector (INOUT Array<Variable> &fields, INOUT usize &i, BytesU sizeOf) override
		{
			Self::ReplaceVector( INOUT fields, INOUT i, sizeOf );
		}
		
		void ReplaceMatrix (INOUT Array<Variable> &fields, INOUT usize &i, BytesU sizeOf) override
		{
			Self::ReplaceMatrix( INOUT fields, INOUT i, sizeOf );
		}
		
		void ReplaceArray (INOUT Array<Variable> &fields, INOUT usize &i, BytesU sizeOf) override
		{
			Self::ReplaceArray( INOUT fields, INOUT i, sizeOf );
		}
		
		bool ProcessType (StringCRef typeName) override
		{
			StructTypes::iterator	iter;
			CHECK_ERR( structTypes.Find( typeName, OUT iter ) );
			
			BytesU	offset, align;
			CHECK_ERR( RecursiveProcess( iter->second.typeName, INOUT iter->second.fields, OUT offset, OUT align ) );
			return true;
		}
	};
	
/*
=================================================
	_AddPaddingToStructs
=================================================
*/
	bool BasePipeline::_AddPaddingToStructs (INOUT StructTypes &structTypes)
	{
		_ReplaceTypes_Helper	helper( structTypes );

		FOR( i, structTypes )
		{
			auto&	var = structTypes[i].second;
				
			if ( var.packing == EVariablePacking::Varying or var.packing == EVariablePacking::VertexAttrib )
				continue;

			BytesU	offset;
			BytesU	align	= var.align;

			CHECK_ERR( helper.RecursiveProcess( var.typeName, INOUT var.fields, OUT offset, INOUT align ) );

			var.align	= Max( var.align, align );
			var.stride	= Max( var.align, var.stride );
		}
		return true;
	}
	
/*
=================================================
	_CompileShader
=================================================
*/
	bool BasePipeline::_CompileShader (const ShaderModule &shader, const ConverterConfig &convCfg, OUT CompiledShader_t &compiled) const
	{
		Array<StringCRef>		source;
		String					str;
		const String			version	= _GetVersionGLSL( shaderFormat );

		if ( convCfg.searchForSharedTypes and convCfg.addPaddingToStructs )
		{
			// replace types to aligned types and padding
			CHECK_ERR( _AddPaddingToStructs( _structTypes ) );

			// update offsets by packing
			CHECK_ERR( _CalculateOffsets( _structTypes ) );
		}

		// replace types
		BinaryArray		glsl_source;
		{
			ShaderCompiler::Config	cfg;
			cfg.skipExternals	= true;
			cfg.optimize		= false;
			cfg.source			= shaderFormat;
			cfg.target			= EShaderFormat::IntermediateSrc;
			cfg.typeReplacer	= DelegateBuilder( this, &BasePipeline::_TypeReplacer );
			
			if ( shader.type == EShader::Compute ) {
				str << _LocalGroupSizeToStringGLSL( localGroupSize );
			}

			source	<< version
					<< _GetDefaultHeaderGLSL()
					<< _GetTypeRedefinitionGLSL()
					<< _GetPerShaderHeaderGLSL( shader.type )
					<< str;

			FOR( i, shader._source ) {
				source << StringCRef(shader._source[i]);
			}
			
			String	log;
			if ( not ShaderCompiler::Instance()->Translate( shader.type, source, shader.entry, _path, cfg, OUT log, OUT glsl_source ) )
			{
				CHECK_ERR( _OnCompilationFailed( shader.type, cfg.source, source, log ) );
			}
		}
		
		String	glsl_types;
		CHECK_ERR( _StructsToString( _structTypes, OUT glsl_types ) );

		String	varyings;
		_VaryingsToString( shader._io, OUT varyings );


		const auto	TranslateToHL = LAMBDA(&) (const ShaderCompiler::Config &cfg, OUT BinaryArray &result) -> bool
		{{
			String	bindings;
			_BindingsToString( shader.type, cfg.target, false, OUT bindings );

			source.Clear();
			source	<< version
					<< _GetDefaultHeaderGLSL()
					<< _GetPerShaderHeaderGLSL( shader.type )
					<< glsl_types
					<< varyings
					<< bindings
					<< StringCRef::From( glsl_source );
			
			String	log;
			if ( not ShaderCompiler::Instance()->Translate( shader.type, source, "main", _path, cfg, OUT log, OUT result ) )
			{
				CHECK_ERR( _OnCompilationFailed( shader.type, cfg.source, source, log ) );
			}
			return true;
		}};

		const auto	CompileToBinary = LAMBDA(&) (ShaderCompiler::Config cfg, OUT BinaryArray &result) -> bool
		{{
			const EShaderFormat::type			hl_fmt  = EShaderFormat::GetApiVersion( cfg.target ) | EShaderFormat::HighLevel;
			const EShaderFormat::type			asm_fmt = EShaderFormat::GetApiVersion( cfg.target ) | EShaderFormat::Assembler;
			CompiledShader_t::const_iterator	iter;
				
			if ( compiled.Find( asm_fmt, OUT iter ) )
			{
				source.Clear();
				source << StringCRef::From( iter->second );
				cfg.source = asm_fmt;
			}
			else
			if ( compiled.Find( hl_fmt, OUT iter ) )
			{
				source.Clear();
				source << StringCRef::From( iter->second );
				cfg.source = hl_fmt;
			}
			else
			{
				// intermediate to binary
				return TranslateToHL( cfg, OUT result );
			}
				
			String	log;
			if ( not ShaderCompiler::Instance()->Translate( shader.type, source, "main", _path, cfg, OUT log, OUT result ) )
			{
				CHECK_ERR( _OnCompilationFailed( shader.type, cfg.source, source, log ) );
			}
			return true;
		}};


		for (auto fmt : convCfg.targets)
		{
			if ( EShaderFormat::IsComputeApi( fmt ) and shader.type != EShader::Compute )
				continue;

			BinaryArray		bin;

			ShaderCompiler::Config	cfg;
			cfg.optimize		= convCfg.optimizeSource;
			cfg.source			= EShaderFormat::IntermediateSrc;
			cfg.target			= fmt;
			cfg.skipExternals	= false;

			if ( EShaderFormat::GetFormat( fmt ) == EShaderFormat::HighLevel or
				 EShaderFormat::GetFormat( fmt ) == EShaderFormat::CPP_Invocable )
			{
				CHECK_ERR( TranslateToHL( cfg, OUT bin ));
			}
			else
			{
				CHECK_ERR( CompileToBinary( cfg, OUT bin ) );
			}

			compiled.Add( fmt, RVREF(bin) );
		}

		return true;
	}
	
/*
=================================================
	_ValidateShader
=================================================
*/
	bool BasePipeline::_ValidateShader (EShader::type shaderType, const CompiledShader_t &compiled)
	{
		/*if ( not compiled.glsl.Empty() ) {
			CHECK_ERR( ShaderCompiler::Instance()->Validate( EShaderFormat::GLSL_450, shaderType, compiled.glsl ) );
		}
		
		if ( not compiled.glslBinary.Empty() ) {
			CHECK_ERR( ShaderCompiler::Instance()->Validate( EShaderFormat::GLSL_450_Bin, shaderType, compiled.glslBinary ) );
		}
		
		if ( not compiled.spirv.Empty() ) {
			CHECK_ERR( ShaderCompiler::Instance()->Validate( EShaderFormat::VK_100_SPIRV, shaderType, compiled.spirv ) );
		}
		
		if ( not compiled.cl.Empty() ) {
			CHECK_ERR( ShaderCompiler::Instance()->Validate( EShaderFormat::CL_120, shaderType, compiled.cl ) );
		}

		if ( not compiled.clAsm.Empty() ) {
			CHECK_ERR( ShaderCompiler::Instance()->Validate( EShaderFormat::CL_120_Asm, shaderType, compiled.clAsm ) );
		}*/
		return true;
	}

}	// PipelineCompiler
