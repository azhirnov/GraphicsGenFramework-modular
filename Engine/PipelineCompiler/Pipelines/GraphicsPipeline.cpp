// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "Engine/PipelineCompiler/Pipelines/GraphicsPipeline.h"
#include "Engine/PipelineCompiler/Common/ToGLSL.h"

namespace PipelineCompiler
{
	
/*
=================================================
	constructor
=================================================
*/
	GraphicsPipeline::GraphicsPipeline (StringCRef name) : BasePipeline(name)
	{
	}
		
/*
=================================================
	Prepare
----
	pass 1
=================================================
*/
	bool GraphicsPipeline::Prepare (const ConverterConfig &cfg)
	{
		ShaderDisasemblies	disasm;

		CHECK_ERR( shaders.vertex.IsEnabled() );
		CHECK_ERR( shaders.fragment.IsEnabled() );
		CHECK_ERR( shaders.tessControl.IsEnabled() == shaders.tessEvaluation.IsEnabled() );

		// build shader source and extract attribs/uniforms/buffers
		{
			if ( shaders.vertex.IsEnabled() )
				CHECK_ERR( _DisasembleShader( cfg, INOUT shaders.vertex, OUT disasm[ shaders.vertex.type ] ) );
			
			if ( shaders.tessControl.IsEnabled() )
				CHECK_ERR( _DisasembleShader( cfg, INOUT shaders.tessControl, OUT disasm[ shaders.tessControl.type ] ) );
			
			if ( shaders.tessEvaluation.IsEnabled() )
				CHECK_ERR( _DisasembleShader( cfg, INOUT shaders.tessEvaluation, OUT disasm[ shaders.tessEvaluation.type ] ) );
			
			if ( shaders.geometry.IsEnabled() )
				CHECK_ERR( _DisasembleShader( cfg, INOUT shaders.geometry, OUT disasm[ shaders.geometry.type ] ) );
			
			if ( shaders.fragment.IsEnabled() )
				CHECK_ERR( _DisasembleShader( cfg, INOUT shaders.fragment, OUT disasm[ shaders.fragment.type ] ) );
		}

		// check compatibility between shaders
		CHECK_ERR( _CheckShaderCompatibility( INOUT disasm ) );

		// unite struct and block types from shaders
		_structTypes.Clear();
		FOR( i, disasm ) {
			CHECK_ERR( _MergeStructTypes( disasm[i].structTypes, INOUT _structTypes ) );
		}

		CHECK_ERR( _AttribsToStructTypes( Name(), attribs, INOUT _structTypes ) );
		
		// update offsets by packing
		CHECK_ERR( _CalculateOffsets( INOUT _structTypes ) );

		_originTypes = _structTypes;

		CHECK_ERR( _UpdateBindings() );
		CHECK_ERR( _UpdateDescriptorSets() );

		if ( renderState.inputAssembly.topology != EPrimitive::Unknown )
			supportedTopology |= renderState.inputAssembly.topology;
		
		_lastEditTime = Max( _lastEditTime, shaders.vertex.LastEditTime() );
		_lastEditTime = Max( _lastEditTime, shaders.tessControl.LastEditTime() );
		_lastEditTime = Max( _lastEditTime, shaders.tessEvaluation.LastEditTime() );
		_lastEditTime = Max( _lastEditTime, shaders.geometry.LastEditTime() );
		_lastEditTime = Max( _lastEditTime, shaders.fragment.LastEditTime() );
		return true;
	}
	
/*
=================================================
	_CheckShaderCompatibility
----
	pass 1
=================================================
*/
	bool GraphicsPipeline::_CheckShaderCompatibility (INOUT ShaderDisasemblies &disasm)
	{
		Array<Varying>	empty;
		Array<Varying>*	varyings = &empty;

		if ( disasm[ EShader::Vertex ].IsEnabled() )
		{
			//CHECK_ERR( _CompareVaryings( *varyings, disasm[ EShader::Vertex ].input ) ); 
			varyings = &disasm[ EShader::Vertex ].output;
		}

		if ( disasm[ EShader::TessControl ].IsEnabled() )
		{
			CHECK_ERR( _CompareVaryings( *varyings, disasm[ EShader::TessControl ].input ) ); 
			varyings = &disasm[ EShader::TessControl ].output;
		}

		if ( disasm[ EShader::TessEvaluation ].IsEnabled() )
		{
			CHECK_ERR( _CompareVaryings( *varyings, disasm[ EShader::TessEvaluation ].input ) ); 
			varyings = &disasm[ EShader::TessEvaluation ].output;
		}

		if ( disasm[ EShader::Geometry ].IsEnabled() )
		{
			CHECK_ERR( _CompareVaryings( *varyings, disasm[ EShader::Geometry ].input ) ); 
			varyings = &disasm[ EShader::Geometry ].output;
		}

		if ( disasm[ EShader::Fragment ].IsEnabled() )
		{
			CHECK_ERR( _CompareVaryings( *varyings, disasm[ EShader::Fragment ].input ) ); 
			//varyings = &disasm[ EShader::Fragment ].output;
		}

		return true;
	}
	
/*
=================================================
	_CompareVaryings
----
	pass 1
=================================================
*/
	bool GraphicsPipeline::_CompareVaryings (const Array<Varying> &prevOutput, INOUT Array<Varying> &nextInput) const
	{
		FOR( i, prevOutput )
		{
			Varying		prev	= prevOutput[i];
			bool		found	= false;

			prev.qualifier.Reset( EVariableQualifier::Out );
			prev.qualifier.Set( EVariableQualifier::In );

			FOR( j, nextInput )
			{
				Varying&	next = nextInput[j];

				if ( prev.type == EShaderVariable::VaryingsBlock	and
					 next.type == EShaderVariable::VaryingsBlock	and
					 prev.typeName	== next.typeName				and
					 //prev.location	== next.location			and
					 prev.qualifier	== next.qualifier )
				{
					found			= true;
					next.location	= prev.location;
					break;
				}

				if ( prev.name		== next.name		and
					 prev.type		== next.type		and 
					 prev.typeName	== next.typeName	and
					 prev.qualifier	== next.qualifier )
				{
					found			= true;
					next.location	= prev.location;
					CHECK_ERR( prev == next );
					break;
				}
			}

			if ( not found )
				nextInput.PushBack( prev );
		}

		CHECK_ERR( prevOutput.Count() == nextInput.Count() );
		return true;
	}

/*
=================================================
	Convert
----
	pass 2
=================================================
*/
	bool GraphicsPipeline::Convert (OUT String &src, Ptr<ISerializer> ser, const ConverterConfig &constCfg) const
	{
		ConverterConfig		cfg = constCfg;

		src.Clear();
		src << ser->Comment( "This is generated file" );
		src << ser->Comment( "Origin file: '"_str << Path() << "'" );
		//src << ser->Comment( "Created at: "_str << ToString( Date().Now() ) ) << '\n';
		src << ser->BeginFile( false );

		// add c++ types localy
		if ( not cfg.searchForSharedTypes )
		{
			/*cfg._glslTypes = "";
			
			CHECK_ERR( _CalculateOffsets( INOUT _structTypes ) );
		
			if ( cfg.addPaddingToStructs )
			{
				CHECK_ERR( _AddPaddingToStructs( INOUT _structTypes ) );
				CHECK_ERR( _CalculateOffsets( INOUT _structTypes ) );
			}

			String	ser_str;
			CHECK_ERR( _AllStructsToString( _structTypes, ser, OUT ser_str, OUT cfg._glslTypes ) );

			src << ser_str;*/
			TODO( "" );
		}

		FOR( i, cfg.includings ) {
			src << ser->Include( cfg.includings[i] );
		}
		src << '\n' << ser->BeginNamespace( cfg.nameSpace ) << '\n';

		EShader::bits	shader_bits;
		if ( shaders.vertex.IsEnabled() )			shader_bits |= EShader::Vertex;
		if ( shaders.tessControl.IsEnabled() )		shader_bits |= EShader::TessControl;
		if ( shaders.tessEvaluation.IsEnabled() )	shader_bits |= EShader::TessEvaluation;
		if ( shaders.geometry.IsEnabled() )			shader_bits |= EShader::Geometry;
		if ( shaders.fragment.IsEnabled() )			shader_bits |= EShader::Fragment;


		// serialize descriptor
		src << ser->DeclFunction( "void", "Create_"_str << Name(), {{"PipelineTemplateDescription&", "descr"}} );
		src << ser->BeginScope();
		src << ser->AssignVariable( "\tdescr", "PipelineTemplateDescription()" );
		src << ser->ToString( "\tdescr.renderState", renderState );
		src << ser->ToString( "\tdescr.dynamicStates", dynamicStates );
		src << ser->ToString( "\tdescr.supportedShaders", shader_bits );
		src << ser->ToString( "\tdescr.supportedPrimitives", supportedTopology ) << '\n';

		CHECK_ERR( _ConvertLayout( "\tdescr.layout", INOUT src, ser ) );

		CHECK_ERR( _ConvertVertexShader( INOUT src, ser, cfg ) );
		CHECK_ERR( _ConvertTessControlShader( INOUT src, ser, cfg ) );
		CHECK_ERR( _ConvertTessEvaluationShader( INOUT src, ser, cfg ) );
		CHECK_ERR( _ConvertGeometryShader( INOUT src, ser, cfg ) );
		CHECK_ERR( _ConvertFragmentShader( INOUT src, ser, cfg ) );

		src << ser->EndScope();	// function
		src << ser->EndNamespace();
		src << ser->EndFile( false );
		return true;
	}
	
/*
=================================================
	_ConvertVertexShader
----
	pass 2
=================================================
*/
	bool GraphicsPipeline::_ConvertVertexShader (INOUT String &src, Ptr<ISerializer> ser, const ConverterConfig &cfg) const
	{
		CHECK_ERR( shaders.vertex.IsEnabled() );

		if ( attribs.Count() > 0 )
			src << ser->ToString( "\tdescr.attribs", attribs ) << '\n';

		CompiledShader_t	compiled;
		CHECK_ERR( _CompileShader( shaders.vertex, cfg, OUT compiled ) );

		const String	name = "\t" + ser->CallFunction( "descr.Vertex", Uninitialized );

		for (auto& comp : compiled) {
			src << ser->ShaderToString( comp.first, name, comp.second );
		}

		if ( cfg.validation ) {
			CHECK_ERR( _ValidateShader( EShader::Vertex, compiled ) );
		}
		return true;
	}
	
/*
=================================================
	_ConvertTessControlShader
----
	pass 2
=================================================
*/
	bool GraphicsPipeline::_ConvertTessControlShader (INOUT String &src, Ptr<ISerializer> ser, const ConverterConfig &cfg) const
	{
		if ( not shaders.tessControl.IsEnabled() )
			return true;
		
		CompiledShader_t	compiled;
		CHECK_ERR( _CompileShader( shaders.tessControl, cfg, OUT compiled ) );
		
		ASSERT( patchControlPoints > 0 );
		src << '\t' << ser->DeclVariable( "uint", "patchControlPoints", ToString(patchControlPoints) ) << '\n';

		const String	name = "\t" + ser->CallFunction( "descr.TessControl", Uninitialized );
		
		for (auto& comp : compiled) {
			src << ser->ShaderToString( comp.first, name, comp.second );
		}

		if ( cfg.validation ) {
			CHECK_ERR( _ValidateShader( EShader::TessControl, compiled ) );
		}
		return true;
	}
	
/*
=================================================
	_ConvertTessEvaluationShader
----
	pass 2
=================================================
*/
	bool GraphicsPipeline::_ConvertTessEvaluationShader (INOUT String &src, Ptr<ISerializer> ser, const ConverterConfig &cfg) const
	{
		if ( not shaders.tessEvaluation.IsEnabled() )
			return true;
		
		CompiledShader_t	compiled;
		CHECK_ERR( _CompileShader( shaders.tessEvaluation, cfg, OUT compiled ) );
		
		const String	name = "\t" + ser->CallFunction( "descr.TessEvaluation", Uninitialized );
		
		for (auto& comp : compiled) {
			src << ser->ShaderToString( comp.first, name, comp.second );
		}

		if ( cfg.validation ) {
			CHECK_ERR( _ValidateShader( EShader::TessEvaluation, compiled ) );
		}
		return true;
	}
	
/*
=================================================
	_ConvertGeometryShader
----
	pass 2
=================================================
*/
	bool GraphicsPipeline::_ConvertGeometryShader (INOUT String &src, Ptr<ISerializer> ser, const ConverterConfig &cfg) const
	{
		if ( not shaders.geometry.IsEnabled() )
			return true;
		
		CompiledShader_t	compiled;
		CHECK_ERR( _CompileShader( shaders.geometry, cfg, OUT compiled ) );
		
		const String	name = "\t" + ser->CallFunction( "descr.Geometry", Uninitialized );
		
		for (auto& comp : compiled) {
			src << ser->ShaderToString( comp.first, name, comp.second );
		}
		
		if ( cfg.validation ) {
			CHECK_ERR( _ValidateShader( EShader::Geometry, compiled ) );
		}
		return true;
	}
	
/*
=================================================
	_ConvertFragmentShader
----
	pass 2
=================================================
*/
	bool GraphicsPipeline::_ConvertFragmentShader (INOUT String &src, Ptr<ISerializer> ser, const ConverterConfig &cfg) const
	{
		CHECK_ERR( shaders.fragment.IsEnabled() );
		
		//src << ser->ToString( "\tdescr.earlyFragmentTests", earlyFragmentTests );

		if ( fragOutput.Count() > 0 )
			src << ser->ToString( "\tdescr.fragOutput", fragOutput ) << '\n';

		CompiledShader_t	compiled;
		CHECK_ERR( _CompileShader( shaders.fragment, cfg, OUT compiled ) );

		const String	name = "\t" + ser->CallFunction( "descr.Fragment", Uninitialized );
		
		for (auto& comp : compiled) {
			src << ser->ShaderToString( comp.first, name, comp.second );
		}
		
		if ( cfg.validation ) {
			CHECK_ERR( _ValidateShader( EShader::Fragment, compiled ) );
		}
		return true;
	}
	
}	// PipelineCompiler
