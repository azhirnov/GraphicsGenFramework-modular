// Copyright �  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "Engine/PipelineCompiler/Shaders/ShaderCompiler_Translator.h"

namespace PipelineCompiler
{

#	define CHECK_COMP2( _func_ ) \
	{ \
		String&	log = translator.log; \
		CHECK_COMP( _func_ ); \
	}

	static bool RecursiveProcessAggregateNode (TIntermNode* node, const uint uid, Translator &translator);
	static bool RecursiveProcessNode (TIntermNode* node, const uint uid, Translator &translator);
	static bool RecursiveProcessBranchNode (TIntermNode* node, const uint uid, Translator &translator);
	static bool RecursiveProcessSwitchNode (TIntermNode* node, const uint uid, Translator &translator);
	static bool RecursiveProcessConstUnionNode (TIntermNode* node, const uint uid, Translator &translator);
	static bool RecursiveProcessSelectionNode (TIntermNode* node, const uint uid, Translator &translator);
	static bool RecursiveProcessMethodNode (TIntermNode* node, const uint uid, Translator &translator);
	static bool RecursiveProcessSymbolNode (TIntermNode* node, const uint uid, Translator &translator);
	static bool RecursiveProcessTypedNode (TIntermNode* node, const uint uid, Translator &translator);
	static bool RecursiveProcessOperatorNode (TIntermNode* node, const uint uid, Translator &translator);
	static bool RecursiveProcessUnaryNode (TIntermNode* node, const uint uid, Translator &translator);
	static bool RecursiveProcessBinaryNode (TIntermNode* node, const uint uid, Translator &translator);
	static bool RecursiveProcessLoopNode (TIntermNode* node, const uint uid, Translator &translator);

	static bool TranslateFunction (glslang::TIntermAggregate* aggr, const uint uid, Translator &translator);
	static bool TranslateExternalObjects (glslang::TIntermAggregate* aggr, const uint uid, Translator &translator);
	static bool ConvertType (TIntermNode* node, glslang::TType const &type, glslang::TSourceLoc const &loc, const Translator::TypeInfo *parent, OUT Translator::TypeInfo &typeInfo);
	static bool TranslateVectorSwizzle (glslang::TIntermOperator* node, const uint uid, Translator &translator);
	static bool TranslateIndexDirectStruct (glslang::TIntermOperator* node, const uint uid, Translator &translator);
	static bool TranslateOperatorCall (TIntermNode* node, const uint uid, Translator &translator);
	static bool TranslateFunctionCall (TIntermNode* node, const uint uid, Translator &translator);
	static bool TranslateInlineFunctionCall (TIntermNode* node, const uint uid, TIntermNode* func, Translator &translator);
	static bool TranslateMain (TIntermNode* root, const uint uid, bool skipExternals, Translator &translator);
	static bool TranslateConstUnionAccess (TIntermNode* node, const uint uid, Translator &translator);
	static bool TranslateCustomType (glslang::TIntermTyped* node, const Translator &translator, INOUT String &src);
	static bool TranslateFunctionForwardDecl (glslang::TIntermAggregate* node, const Translator &translator, INOUT String &src);

	static bool GXCheckAccessToExternal (const Translator &translator, const Translator::Node &node);
	
		
/*
=================================================
	Main
=================================================
*/
	bool Translator::Main (TIntermNode* root, const uint uid, bool skipExternals)
	{
		return TranslateMain( root, uid, skipExternals, *this );
	}

/*
=================================================
	GXCheckAccessToExternal
=================================================
*/
	static bool GXCheckAccessToExternal (const Translator &translator, const Translator::Node &node)
	{
		if ( translator.useGXrules )
		{
			// can't access to global objects directly from non-main function.
			// this rule used for compatibility with OpenCL.
			if ( not translator.isMain ) {
				return not node.typeInfo.isGlobal;
			}
		}
		return true;
	}

/*
=================================================
	TranslateMain
=================================================
*/
	static bool TranslateMain (TIntermNode* root, const uint uid, bool skipExternals, Translator &translator)
	{
		using DeclFunction_t	= Set< glslang::TIntermAggregate* >;

		glslang::TIntermAggregate* aggr = root->getAsAggregate();
		CHECK_ERR( aggr and aggr->getOp() == glslang::TOperator::EOpSequence );
		
		// get external objects
		const uint		ext_uid	= ++translator.uid;
		DeclFunction_t	funcs;

		for (size_t i = 0; i < aggr->getSequence().size(); ++i)
		{
			TIntermNode *				node = aggr->getSequence()[i];
			glslang::TIntermAggregate*	aggr2 = node->getAsAggregate();

			if ( aggr2 and aggr2->getOp() == glslang::TOperator::EOpLinkerObjects )
			{
				CHECK_ERR( TranslateExternalObjects( aggr2, ext_uid, translator ) );
			}
			else
			if ( aggr2 and aggr2->getOp() == glslang::TOperator::EOpFunction )
			{
				funcs.Add( aggr2 );
			}
		}

		// source
		String	src;
		for (size_t i = 0; i < aggr->getSequence().size(); ++i)
		{
			TIntermNode*	node = aggr->getSequence()[i];

			if ( node->getAsAggregate() and node->getAsAggregate()->getOp() == glslang::TOperator::EOpLinkerObjects )
				continue;

			const uint	arg_uid = ++translator.uid;
			CHECK_ERR( RecursiveProcessNode( node, arg_uid, translator ) );
			
			// node may not exist if it is a function declaration
			if ( not translator.nodes.IsExist( arg_uid ) )
				continue;

			const auto&	arg = translator.nodes( arg_uid );
			CHECK_ERR( not arg.src.Empty() );

			src << arg.src << "\n";
		}

		if ( not skipExternals )
		{
			translator.src	<< translator.nodes( ext_uid ).src
							<< "\n//---------------------------------\n\n";
		}

		// custom types declaration
		FOR( i, translator.types.globalTypes )
		{
			CHECK_ERR( TranslateCustomType( translator.types.globalTypes[i].second, translator, INOUT translator.src ) );
		}

		// function forward declaration
		FOR( i, funcs )
		{
			CHECK_ERR( TranslateFunctionForwardDecl( funcs[i], translator, INOUT translator.src ) );
		}

		translator.src << "\n//---------------------------------\n" << src;
		return true;
	}
	
/*
=================================================
	TranslateCustomType
=================================================
*/
	static bool TranslateCustomType (glslang::TIntermTyped* node, const Translator &translator, INOUT String &src)
	{
		Translator::TypeInfo	info;
		CHECK_ERR( ConvertType( node, node->getType(), node->getLoc(), null, OUT info ) );
		
		CHECK_ERR( not info.fields.Empty() );

		src << "struct " << info.typeName << "{\n";

		FOR( i, info.fields )
		{
			src << "\t";
			CHECK_ERR( translator.language->TranslateLocalVar( info.fields[i], INOUT src ) );
			src << ";\n";
		}

		src << "};\n\n";
		return true;
	}
	
/*
=================================================
	TranslateFunctionForwardDecl
=================================================
*/
	static bool TranslateFunctionForwardDecl (glslang::TIntermAggregate* aggr, const Translator &translator, INOUT String &src)
	{
		ASSERT( aggr->getOp() == glslang::TOperator::EOpFunction );
		
		Translator::TypeInfo			ret_type;
		Array< Translator::TypeInfo >	func_args;
		String							func_name;
		bool							will_inlined = false;

		// get result type
		CHECK_ERR( ConvertType( aggr, aggr->getType(), aggr->getLoc(), null, OUT ret_type ) );
		
		// get name
		{
			String	signature	= aggr->getName().c_str();
			usize	pos			= 0;

			if ( signature.Find( '(', OUT pos ) )
				func_name = signature.SubString( 0, pos );
			else
				func_name = signature;
		}
		
		if (func_name == translator.entryPoint)
			return true;
		
		// find arguments
		for (size_t i = 0; i < aggr->getSequence().size(); ++i)
		{
			TIntermNode* n = aggr->getSequence()[i];

			if ( n->getAsAggregate() and 
				 n->getAsAggregate()->getOp() == glslang::TOperator::EOpParameters )
			{
				// get arguemnts
				glslang::TIntermAggregate*	args_node = n->getAsAggregate();
				
				func_args.Reserve( func_args.Count() + args_node->getSequence().size() );

				for (size_t j = 0; j < args_node->getSequence().size(); ++j)
				{
					Translator::TypeInfo		arg;
					glslang::TIntermTyped *		nn = args_node->getSequence()[j]->getAsTyped();

					CHECK_ERR( ConvertType( nn, nn->getType(), nn->getLoc(), null, OUT arg ) );

					// function with dynamic array must be inlined
					if ( arg.arraySize == UMax )
						will_inlined = true;

					func_args.PushBack( RVREF(arg) );
				}
			}
		}
		
		src << (will_inlined ? "// " : "");
		CHECK_ERR( translator.language->TranslateType( ret_type, INOUT src ) );
		src << " " << func_name << " (";

		FOR( i, func_args )
		{
			src << (i ? ", " : "");
			CHECK_ERR( translator.language->TranslateArg( func_args[i], INOUT src ) );
		}
		src << ");" << (will_inlined ? "\t// will be inlined\n" : "\n");
		return true;
	}

/*
=================================================
	RecursiveProcessAggregateNode
=================================================
*/
	static bool RecursiveProcessAggregateNode (TIntermNode* node, const uint uid, Translator &translator)
	{
		glslang::TIntermAggregate* aggr = node->getAsAggregate();
		
		// continue
		if ( aggr->getOp() == glslang::TOperator::EOpSequence )
		{
			// search loop node
			for (size_t i = 0; i < aggr->getSequence().size(); ++i)
			{
				if ( aggr->getSequence()[i]->getAsLoopNode() )
				{
					CHECK_ERR( RecursiveProcessLoopNode( aggr, uid, translator ) );
					return true;
				}
			}

			Translator::Node	dst_node;
			dst_node.uid = uid;

			for (size_t i = 0; i < aggr->getSequence().size(); ++i)
			{
				const uint	arg_uid = ++translator.uid;
				CHECK_ERR( RecursiveProcessNode( aggr->getSequence()[i], arg_uid, translator ) );

				// add local variable declaration
				FOR( j, translator.fwd.pendingVars )
				{
					Translator::TypeInfo	info = translator.fwd.pendingVars[j].second;
					translator.inl.prefixStack.Get() >> info.name;

					CHECK_ERR( translator.language->TranslateLocalVar( info, INOUT dst_node.src ) );
					dst_node.src << ";\n";
				}
				// add strings
				FOR( j, translator.fwd.addBeforeLine ) {
					dst_node.src << translator.fwd.addBeforeLine[j] << ";\n";
				}
				translator.fwd.pendingVars.Clear();
				translator.fwd.addBeforeLine.Clear();

				const auto&	arg = translator.nodes( arg_uid );
				CHECK_ERR( not arg.src.Empty() );

				dst_node.src << arg.src << (arg.src.EndsWith(";\n") ? "" : ";\n");
			}

			translator.nodes.Add( uid, RVREF(dst_node) );
			return true;
		}

		// function definition
		if ( aggr->getOp() == glslang::TOperator::EOpFunction )
		{
			CHECK_ERR( TranslateFunction( aggr, uid, translator ) );
			return true;
		}

		// uniforms, buffers, images, ...
		if ( aggr->getOp() == glslang::TOperator::EOpLinkerObjects )
		{
			RETURN_ERR( "not supported here, must be used in 'TranslateMain'" );
		}

		// function call
		if ( aggr->getOp() == glslang::TOperator::EOpFunctionCall )
		{
			CHECK_ERR( TranslateFunctionCall( node, uid, translator ) );
			return true;
		}

		// builtin function call
		if ( (aggr->getOp() >= glslang::TOperator::EOpRadians and aggr->getOp() <= glslang::TOperator::EOpAll) or
			 (aggr->getOp() >= glslang::TOperator::EOpAddCarry and aggr->getOp() <= glslang::TOperator::EOpFindMSB) or
			 (aggr->getOp() >= glslang::TOperator::EOpEqual and aggr->getOp() <= glslang::TOperator::EOpGreaterThanEqual) or
			 (aggr->getOp() >= glslang::TOperator::EOpTextureGuardBegin and aggr->getOp() <= glslang::TOperator::EOpTextureGuardEnd) or
			 (aggr->getOp() > glslang::TOperator::EOpImageGuardBegin and aggr->getOp() < glslang::TOperator::EOpImageGuardEnd) or
			 (aggr->getOp() > glslang::TOperator::EOpConstructGuardStart and aggr->getOp() < glslang::TOperator::EOpConstructGuardEnd) )
		{
			CHECK_ERR( TranslateOperatorCall( node, uid, translator ) );
			return true;
		}

		TODO( "" );
		return true;
	}

/*
=================================================
	TranslateFunction
=================================================
*/
	static bool TranslateFunction (glslang::TIntermAggregate* aggr, const uint uid, Translator &translator)
	{
		ASSERT( aggr->getOp() == glslang::TOperator::EOpFunction );

		Translator::Node				dst_node;
		Array< Translator::TypeInfo >	func_args;
		String							func_name;
		const uint						body_uid	= ++translator.uid;

		// get result type
		CHECK_ERR( ConvertType( aggr, aggr->getType(), aggr->getLoc(), null, OUT dst_node.typeInfo ) );
		
		// get name
		{
			String	signature	= aggr->getName().c_str();
			usize	pos			= 0;

			if ( signature.Find( '(', OUT pos ) )
				func_name = signature.SubString( 0, pos );
			else
				func_name = signature;
		}
		const bool	is_entry = (func_name == translator.entryPoint);

		
		// find arguments
		for (size_t i = 0; i < aggr->getSequence().size(); ++i)
		{
			TIntermNode* n = aggr->getSequence()[i];

			if ( n->getAsAggregate() and 
				 n->getAsAggregate()->getOp() == glslang::TOperator::EOpParameters )
			{
				// get arguemnts
				glslang::TIntermAggregate*	args_node = n->getAsAggregate();
				
				func_args.Reserve( func_args.Count() + args_node->getSequence().size() );

				for (size_t j = 0; j < args_node->getSequence().size(); ++j)
				{
					Translator::TypeInfo		arg;
					glslang::TIntermTyped *		nn = args_node->getSequence()[j]->getAsTyped();

					CHECK_ERR( ConvertType( nn, nn->getType(), nn->getLoc(), null, OUT arg ) );

					// function with dynamic array must be inlined
					if ( arg.arraySize == UMax )
					{
						translator.inl.functions.Add( aggr->getName().c_str(), aggr );
						return true;
					}

					// TODO: check rules?

					func_args.PushBack( RVREF(arg) );
				}
			}
			else
			{
				ASSERT( not translator.isMain );
				translator.isMain = is_entry;
	
				CHECK_ERR( RecursiveProcessNode( n, body_uid, translator ) );
				
				translator.isMain = false;
			}
		}

		String	body = translator.nodes( body_uid ).src;

		CHECK_ERR( not body.Empty() );
		CHECK_ERR( translator.language->TranslateType( dst_node.typeInfo, INOUT dst_node.src ) );
		dst_node.src << " " << func_name << " (";

		FOR( i, func_args )
		{
			dst_node.src << (i ? ", " : "");
			CHECK_ERR( translator.language->TranslateArg( func_args[i], INOUT dst_node.src ) );
		}

		StringParser::IncreaceIndent( INOUT body );
		dst_node.src << ")\n{\n" << body << "}\n\n";

		translator.nodes.Add( uid, RVREF(dst_node) );
		return true;
	}
	
/*
=================================================
	ConvertType
=================================================
*/
	static bool ConvertType (TIntermNode* node, glslang::TType const &type, glslang::TSourceLoc const &loc, const Translator::TypeInfo *parent, OUT Translator::TypeInfo &result)
	{
		auto const&		qual = type.getQualifier();

		// copy name
		if ( &type.getFieldName() != null )
		{
			result.name = type.getFieldName().c_str();
		}
		else
		if ( node and node->getAsSymbolNode() )
		{
			result.name = node->getAsSymbolNode()->getName().c_str();

			if ( result.name.StartsWithIC( "anon@" ) )
				result.name.Clear();
		}

		// copy typename
		if ( &type.getTypeName() != null )
		{
			result.typeName = type.getTypeName().c_str();
		}

		// arrays
		if ( type.isArray() )
		{
			result.arraySize = type.isExplicitlySizedArray() ? Max( 1, type.getOuterArraySize() ) : -1;
		}

		// binding
		result.binding = qual.hasBinding() ? qual.layoutBinding : UMax;

		// buffer
		if ( type.getBasicType() == glslang::TBasicType::EbtBlock )
		{
			// do nothing
		}
		else
		if ( type.getBasicType() == glslang::TBasicType::EbtSampler and not type.isSubpass() )
		{
			glslang::TSampler const&	samp = type.getSampler();
			EShaderVariable::type		samp_type;

			switch ( samp.dim )
			{
				case glslang::TSamplerDim::Esd1D :
				{
					if ( samp.isShadow() and samp.isArrayed() )		samp_type = EShaderVariable::Sampler1DArrayShadow;	else
					if ( samp.isShadow() )							samp_type = EShaderVariable::Sampler1DShadow;		else
					if ( samp.isArrayed() )							samp_type = EShaderVariable::Sampler1DArray;		else
																	samp_type = EShaderVariable::Sampler1D;
					break;
				}
				case glslang::TSamplerDim::Esd2D :
				{
					if ( samp.isShadow() and samp.isArrayed() )			samp_type = EShaderVariable::Sampler2DArrayShadow;	else
					if ( samp.isShadow() )								samp_type = EShaderVariable::Sampler2DShadow;		else
					if ( samp.isMultiSample() and samp.isArrayed() )	samp_type = EShaderVariable::Sampler2DMSArray;		else
					if ( samp.isArrayed() )								samp_type = EShaderVariable::Sampler2DArray;		else
					if ( samp.isMultiSample() )							samp_type = EShaderVariable::Sampler2DMS;			else
																		samp_type = EShaderVariable::Sampler2D;
					break;
				}
				case glslang::TSamplerDim::Esd3D :
				{
					samp_type = EShaderVariable::Sampler3D;
					break;
				}
				case glslang::TSamplerDim::EsdCube :
				{
					if ( samp.isShadow() )		samp_type = EShaderVariable::SamplerCubeShadow;	else
					if ( samp.isArrayed() )		samp_type = EShaderVariable::SamplerCubeArray;	else
												samp_type = EShaderVariable::SamplerCube;
					break;
				}
				case glslang::TSamplerDim::EsdBuffer :
				{
					samp_type = EShaderVariable::SamplerBuffer;
					break;
				}
				default :
					RETURN_ERR( "unknown sampler dimension type!" );
			}
					
			// sampler
			if ( samp.isCombined() )
			{
				result.type = EShaderVariable::ToSampler( samp_type, ConvertBasicType( samp.type, samp.vectorSize ) );
			}
			else
			// image
			if ( samp.isImage() )
			{
				result.type   = EShaderVariable::ToImage( samp_type, ConvertBasicType( samp.type, samp.vectorSize ) );
				result.format = ConvertImageLayoutFormat( qual.layoutFormat );
			}
			else
				RETURN_ERR( "unsupported sampler type!" );
		}
		else
		// subpass
		if ( type.isSubpass() )
		{
			// TODO
		}
		else
		// for scalar, vector and matrix types only
		if ( type.getBasicType() == glslang::TBasicType::EbtVoid	or
				type.getBasicType() == glslang::TBasicType::EbtFloat	or
				type.getBasicType() == glslang::TBasicType::EbtDouble	or
				type.getBasicType() == glslang::TBasicType::EbtInt		or
				type.getBasicType() == glslang::TBasicType::EbtUint	or
				type.getBasicType() == glslang::TBasicType::EbtInt64	or
				type.getBasicType() == glslang::TBasicType::EbtUint64	or
			#ifdef AMD_EXTENSIONS
				type.getBasicType() == glslang::TBasicType::EbtFloat16	or
				type.getBasicType() == glslang::TBasicType::EbtInt16	or
				type.getBasicType() == glslang::TBasicType::EbtUint16	or
			#endif
				type.getBasicType() == glslang::TBasicType::EbtBool	or
				type.getBasicType() == glslang::TBasicType::EbtUint64 )
		{
			result.type = ConvertBasicType( type.getBasicType(), type.getVectorSize(), type.getMatrixCols(), type.getMatrixRows() );
		}

		// qualifiers
		{
			auto const&	q = type.getQualifier();
			
			if ( type.isBuiltIn()					or
					result.typeName.StartsWith("gl_")	or
					result.name.StartsWith("gl_")		or
					(parent and parent->qualifier[ EVariableQualifier::BuiltIn ]) )
			{
				result.qualifier |= EVariableQualifier::BuiltIn;
			}

			if ( q.invariant )
				result.qualifier |= EVariableQualifier::Invariant;

			if ( q.noContraction )
				result.qualifier |= EVariableQualifier::Precise;

			if ( q.centroid )
				result.qualifier |= EVariableQualifier::Centroid;

			if ( q.smooth )
				result.qualifier |= EVariableQualifier::Smooth;

			if ( q.flat )
				result.qualifier |= EVariableQualifier::Flat;

			if ( q.nopersp )
				result.qualifier |= EVariableQualifier::NoPerspective;

			if ( q.patch )
				result.qualifier |= EVariableQualifier::Patch;

			if ( q.sample )
				result.qualifier |= EVariableQualifier::Sample;

			if ( q.coherent )
				result.memoryModel |= EGpuMemoryModel::Coherent;

			if ( q.volatil )
				result.memoryModel |= EGpuMemoryModel::Volatile;

			if ( q.restrict )
				result.memoryModel |= EGpuMemoryModel::Restrict;

			if ( q.readonly )
				result.memoryModel |= EGpuMemoryModel::ReadOnly;

			if ( q.writeonly )
				result.memoryModel |= EGpuMemoryModel::WriteOnly;

			if ( q.specConstant )
				result.qualifier |= EVariableQualifier::Specialization;

			switch ( q.storage )
			{
				case glslang::TStorageQualifier::EvqTemporary :			result.qualifier |= EVariableQualifier::Local;		break;
				//case glslang::TStorageQualifier::EvqGlobal :			break;
				case glslang::TStorageQualifier::EvqConst :				result.qualifier |= EVariableQualifier::Constant;	break;
				case glslang::TStorageQualifier::EvqVaryingIn :			result.qualifier |= EVariableQualifier::In;			result.isGlobal = true;		break;
				case glslang::TStorageQualifier::EvqVaryingOut :		result.qualifier |= EVariableQualifier::Out;		result.isGlobal = true;		break;
				case glslang::TStorageQualifier::EvqUniform :			result.qualifier |= EVariableQualifier::Uniform;	result.isGlobal = true;		break;
				case glslang::TStorageQualifier::EvqBuffer :			result.isGlobal = true;		break;
				case glslang::TStorageQualifier::EvqShared :			result.qualifier |= EVariableQualifier::Shared;		result.isGlobal = true;		break;

				// function arguments
				case glslang::TStorageQualifier::EvqIn :				result.qualifier |= EVariableQualifier::InArg;		break;
				case glslang::TStorageQualifier::EvqOut :				result.qualifier |= EVariableQualifier::OutArg;		break;
				case glslang::TStorageQualifier::EvqInOut :				result.qualifier |= EVariableQualifier::InArg;
																		result.qualifier |= EVariableQualifier::OutArg;		break;
				case glslang::TStorageQualifier::EvqConstReadOnly :		result.qualifier |= EVariableQualifier::InArg;
																		result.qualifier |= EVariableQualifier::Constant;	break;
			}

			switch ( q.precision )
			{
				case glslang::TPrecisionQualifier::EpqNone :			result.precision = EPrecision::Default;	break;
				case glslang::TPrecisionQualifier::EpqLow :				result.precision = EPrecision::Low;		break;
				case glslang::TPrecisionQualifier::EpqMedium :			result.precision = EPrecision::Medium;	break;
				case glslang::TPrecisionQualifier::EpqHigh :			result.precision = EPrecision::High;	break;
				default :												RETURN_ERR( "unknown precision type!" );
			}
		}

		// copy from parent
		if ( parent )
		{
			result.qualifier.Or( EVariableQualifier::In, parent->qualifier[EVariableQualifier::In] );
			result.qualifier.Or( EVariableQualifier::Out, parent->qualifier[EVariableQualifier::Out] );
			result.qualifier.Or( EVariableQualifier::InArg, parent->qualifier[EVariableQualifier::InArg] );
			result.qualifier.Or( EVariableQualifier::OutArg, parent->qualifier[EVariableQualifier::OutArg] );
			result.qualifier.Or( EVariableQualifier::Shared, parent->qualifier[EVariableQualifier::Shared] );
			result.qualifier.Or( EVariableQualifier::BuiltIn, parent->qualifier[EVariableQualifier::BuiltIn] );
			result.qualifier.Or( EVariableQualifier::Local, parent->qualifier[EVariableQualifier::Local] );
			result.qualifier.Or( EVariableQualifier::Specialization, parent->qualifier[EVariableQualifier::Specialization] );

			if ( result.memoryModel == EGpuMemoryModel::None )
				result.memoryModel = parent->memoryModel;

			if ( result.precision == EPrecision::Default )
				result.precision = parent->precision;
		}

		if ( type.isStruct() )
		{
			auto const&	type_list = *type.getStruct();
			
			result.type		= EShaderVariable::Struct;
			result.typeName = type.getTypeName().c_str();
			result.fields.Reserve( result.fields.Count() + type_list.size() );

			for (size_t i = 0; i < type_list.size(); ++i)
			{
				Translator::TypeInfo	arg;
				CHECK_ERR( ConvertType( null, *type_list[i].type, type_list[i].loc, &result, OUT arg ) );
				result.fields.PushBack( RVREF(arg) );
			}
		}
		return true;
	}

/*
=================================================
	TranslateExternalObjects
=================================================
*/
	static bool TranslateConst (glslang::TIntermTyped* typed, Translator::TypeInfo const& info, OUT String &str);

	static bool TranslateExternalObjects (glslang::TIntermAggregate* aggr, const uint uid, Translator &translator)
	{
		CHECK_ERR( aggr and aggr->getOp() == glslang::TOperator::EOpLinkerObjects );

		Translator::Node	dst_node;
		String &			str = dst_node.src;

		dst_node.uid = uid;

		for (size_t i = 0; i < aggr->getSequence().size(); ++i)
		{
			TIntermNode*	node = aggr->getSequence()[i];
			CHECK_ERR( node->getAsTyped() );

			glslang::TIntermTyped*		typed	= node->getAsTyped();
			glslang::TType const&		type	= typed->getType();
			Translator::TypeInfo		info;

			CHECK_ERR( ConvertType( node, type, typed->getLoc(), null, OUT info ) );
			
			if ( type.isBuiltIn() or info.typeName.StartsWithIC("gl_") ) {
				continue;
			}
			
			if ( type.getQualifier().storage == glslang::TStorageQualifier::EvqConst ) {
				translator.constNodes.Add( node->getAsSymbolNode() );
			}

			CHECK_ERR( translator.language->TranslateExternal( typed, info, OUT str ) );
		}

		translator.nodes.Add( uid, RVREF(dst_node) );
		return true;
	}

/*
=================================================
	RecursiveProcessNode
=================================================
*/
	static bool RecursiveProcessNode (TIntermNode* node, const uint uid, Translator &translator)
	{
		if ( not node )
			return false;

		if ( node->getAsAggregate() )
		{
			CHECK_ERR( RecursiveProcessAggregateNode( node, uid, translator ) );
			return true;
		}

		if ( node->getAsUnaryNode() )
		{
			CHECK_ERR( RecursiveProcessUnaryNode( node, uid, translator ) );
			return true;
		}

		if ( node->getAsBinaryNode() )
		{
			CHECK_ERR( RecursiveProcessBinaryNode( node, uid, translator ) );
			return true;
		}

		if ( node->getAsOperator() )
		{
			CHECK_ERR( RecursiveProcessOperatorNode( node, uid, translator ) );
			return true;
		}

		if ( node->getAsBranchNode() )
		{
			CHECK_ERR( RecursiveProcessBranchNode( node, uid, translator ) );
			return true;
		}

		if ( node->getAsSwitchNode() )
		{
			CHECK_ERR( RecursiveProcessSwitchNode( node, uid, translator ) );
			return true;
		}

		if ( node->getAsConstantUnion() )
		{
			CHECK_ERR( RecursiveProcessConstUnionNode( node, uid, translator ) );
			return true;
		}

		if ( node->getAsSelectionNode() )
		{
			CHECK_ERR( RecursiveProcessSelectionNode( node, uid, translator ) );
			return true;
		}

		if ( node->getAsMethodNode() )
		{
			CHECK_ERR( RecursiveProcessMethodNode( node, uid, translator ) );
			return true;
		}

		if ( node->getAsSymbolNode() )
		{
			CHECK_ERR( RecursiveProcessSymbolNode( node, uid, translator ) );
			return true;
		}

		if ( node->getAsTyped() )
		{
			CHECK_ERR( RecursiveProcessTypedNode( node, uid, translator ) );
			return true;
		}

		if ( node->getAsLoopNode() )
		{
			CHECK_ERR( RecursiveProcessLoopNode( node, uid, translator ) );
			return true;
		}
		
		TODO( "" );
		return false;
	}
	
/*
=================================================
	RecursiveProcessBranchNode
=================================================
*/
	static bool RecursiveProcessBranchNode (TIntermNode* node, const uint uid, Translator &translator)
	{
		glslang::TIntermBranch*		branch	= node->getAsBranchNode();
		Translator::Node			dst_node;					dst_node.uid = uid;
		bool						replaced_return = false;

		switch ( branch->getFlowOp() )
		{
			case glslang::TOperator::EOpKill :		dst_node.src << "discard";	break;
			case glslang::TOperator::EOpBreak :		dst_node.src << "break";	break;
			case glslang::TOperator::EOpContinue :	dst_node.src << "continue";	break;
			case glslang::TOperator::EOpCase :		dst_node.src << "case";		break;
			case glslang::TOperator::EOpDefault :	dst_node.src << "default";	break;
			case glslang::TOperator::EOpReturn :
			{
				if ( translator.inl.prefixStack.Get().Empty() ) {
					dst_node.src << "return";
					break;
				}
				replaced_return = true;
				break;
			}
			default :								RETURN_ERR( "unknown operator!" );
		}

		if ( branch->getExpression() )
		{
			const uint	arg_uid = ++translator.uid;
			CHECK_ERR( RecursiveProcessNode( branch->getExpression(), arg_uid, translator ) );

			const auto&	arg = translator.nodes( arg_uid );
			CHECK_ERR( not arg.src.Empty() );

			dst_node.src << " " << arg.src;
			
			// inside inline function 'return' must be replaced by exit of cycle
			if ( replaced_return ) {
				("{\n\t"_str << translator.inl.prefixStack.Get() << "return = ") >> dst_node.src;
				dst_node.src << ";\n\tbreak;\n}";
			}
		}
		else
		// function return void, but in inline function need to break loop
		if ( replaced_return )
		{
			dst_node.src << "break";
		}

		translator.nodes.Add( uid, RVREF(dst_node) );
		return true;
	}
	
/*
=================================================
	RecursiveProcessSwitchNode
=================================================
*/
	static bool RecursiveProcessSwitchNode (TIntermNode* node, const uint uid, Translator &translator)
	{
		glslang::TIntermSwitch*		sw = node->getAsSwitchNode();

		CHECK_ERR( RecursiveProcessNode( sw->getCondition(), ++translator.uid, translator ) );
		CHECK_ERR( RecursiveProcessNode( sw->getBody(), ++translator.uid, translator ) );

		TODO( "" );
		return true;
	}
	
/*
=================================================
	RecursiveProcessConstUnionNode
=================================================
*/
	static bool RecursiveProcessConstUnionNode (TIntermNode* node, const uint uid, Translator &translator)
	{
		glslang::TIntermConstantUnion*		cu		= node->getAsConstantUnion();
		const glslang::TConstUnionArray&	cu_arr	= cu->getConstArray();
		Translator::Node					dst_node;
		
		dst_node.uid = uid;
		CHECK_ERR( ConvertType( node, cu->getType(), cu->getLoc(), null, OUT dst_node.typeInfo ) );
		CHECK_COMP2( GXCheckAccessToExternal( translator, dst_node ) );

		DeserializedShader::Constant::ValueArray_t	values;
		CHECK_ERR( DeserializeConstant::Process( dst_node.typeInfo.type, cu_arr, OUT values ) );
		
		CU_ToString_Func	func( dst_node.src );

		FOR( i, values )
		{
			dst_node.src << (i ? ", " : "");
			CHECK_ERR( translator.language->TranslateType( dst_node.typeInfo, INOUT dst_node.src ) );
			values[i].Apply( func );
		}

		translator.nodes.Add( uid, RVREF(dst_node) );
		return true;
	}

/*
=================================================
	RecursiveProcessSelectionNode
=================================================
*/
	static bool RecursiveProcessSelectionNode (TIntermNode* node, const uint uid, Translator &translator)
	{
		glslang::TIntermSelection*	selection = node->getAsSelectionNode();
		Translator::Node			dst_node;		dst_node.uid = uid;
		
		CHECK_ERR( ConvertType( selection, selection->getType(), selection->getLoc(), null, OUT dst_node.typeInfo ) );

		const bool	is_if_else_block = (dst_node.typeInfo.type == EShaderVariable::Void);

		String	cond_src;
		if ( selection->getCondition() )
		{
			const uint	cond_uid = ++translator.uid;
			CHECK_ERR( RecursiveProcessNode( selection->getCondition(), cond_uid, translator ) );
			cond_src = translator.nodes( cond_uid ).src;
		}

		String	true_src;
		if ( selection->getTrueBlock() )
		{
			const uint	true_uid = ++translator.uid;
			CHECK_ERR( RecursiveProcessNode( selection->getTrueBlock(), true_uid, translator ) );
			true_src = translator.nodes( true_uid ).src;
		}

		String	false_src;
		if ( selection->getFalseBlock() )
		{
			const uint	false_uid = ++translator.uid;
			CHECK_ERR( RecursiveProcessNode( selection->getFalseBlock(), false_uid, translator ) );
			false_src = translator.nodes( false_uid ).src;
		}

		if ( is_if_else_block )
		{
			CHECK_ERR( not cond_src.Empty() and (not true_src.Empty() or not false_src.Empty()) );

			dst_node.src << "if ( " << cond_src << ")\n";
			
			if ( not true_src.Empty() )
			{
				StringParser::IncreaceIndent( true_src );
				dst_node.src << "{\n" << true_src << ";\n}\n";
			}
			else
				dst_node.src << "{}\n";
			
			if ( not false_src.Empty() )
			{
				StringParser::IncreaceIndent( false_src );
				dst_node.src << "else\n{\n" << false_src << ";\n}\n";
			}
		}
		else
		{
			CHECK_ERR( not cond_src.Empty() and not true_src.Empty() and not false_src.Empty() );

			dst_node.src << "( (" << cond_src << ") ? (" << true_src << ") : (" << false_src << ") )";
		}

		translator.nodes.Add( uid, RVREF(dst_node) );
		return true;
	}
	
/*
=================================================
	RecursiveProcessMethodNode
=================================================
*/
	static bool RecursiveProcessMethodNode (TIntermNode* node, const uint uid, Translator &translator)
	{
		glslang::TIntermMethod*		method = node->getAsMethodNode();

		CHECK_ERR( RecursiveProcessNode( method->getObject(), ++translator.uid, translator ) );
		
		TODO( "" );
		return true;
	}
	
/*
=================================================
	RecursiveProcessSymbolNode
=================================================
*/
	static bool RecursiveProcessSymbolNode (TIntermNode* node, const uint uid, Translator &translator)
	{
		glslang::TIntermSymbol*		symbol = node->getAsSymbolNode();
		Translator::Node			dst_node;

		CHECK_ERR( ConvertType( symbol, symbol->getType(), symbol->getLoc(), null, OUT dst_node.typeInfo ) );
		CHECK_COMP2( GXCheckAccessToExternal( translator, dst_node ) );

		// if inside inline function
		if ( dst_node.typeInfo.qualifier[ EVariableQualifier::InArg ] or
			 dst_node.typeInfo.qualifier[ EVariableQualifier::OutArg ] )
		{
			translator.inl.prefixStack.Get() >> dst_node.typeInfo.name;
			
			// replace some function arguments to external objects to support GLSL specification.
			Translator::LocalReplacer_t::iterator	iter;
			if ( translator.inl.localsReplacer.Find( dst_node.typeInfo.name, OUT iter ) ) {
				dst_node.typeInfo.name = iter->second;
			}
		}

		dst_node.uid	= uid;
		dst_node.src	= dst_node.typeInfo.name;

		if ( symbol->getQualifier().storage == glslang::TStorageQualifier::EvqTemporary and
			 not translator.fwd.definedLocalVars.IsExist( symbol->getId() ) )
		{
			translator.fwd.definedLocalVars.Add( symbol->getId() );
			translator.fwd.pendingVars.Add( symbol->getId(), dst_node.typeInfo );
		}

		if ( dst_node.typeInfo.type == EShaderVariable::Struct and
			 not dst_node.typeInfo.isGlobal )
		{
			translator.types.globalTypes.Add( dst_node.typeInfo.typeName, symbol );
		}

		translator.nodes.Add( uid, RVREF(dst_node) );
		return true;
	}
	
/*
=================================================
	RecursiveProcessTypedNode
=================================================
*/
	static bool RecursiveProcessTypedNode (TIntermNode* node, const uint uid, Translator &translator)
	{
		glslang::TIntermTyped*		typed = node->getAsTyped();

		TODO( "" );
		return true;
	}
	
/*
=================================================
	RecursiveProcessOperatorNode
=================================================
*/
	static bool RecursiveProcessOperatorNode (TIntermNode* node, const uint uid, Translator &translator)
	{
		glslang::TIntermOperator*	op = node->getAsOperator();
		
		TODO( "" );
		return true;
	}
	
/*
=================================================
	RecursiveProcessUnaryNode
=================================================
*/
	static bool RecursiveProcessUnaryNode (TIntermNode* node, const uint uid, Translator &translator)
	{
		glslang::TIntermUnary*		unary	= node->getAsUnaryNode();
		const uint					arg_uid	= ++translator.uid;
		Translator::Node			dst_node;	dst_node.uid = uid;	

		CHECK_ERR( ConvertType( node, unary->getType(), unary->getLoc(), null, OUT dst_node.typeInfo ) );
		CHECK_COMP2( GXCheckAccessToExternal( translator, dst_node ) );

		CHECK_ERR( RecursiveProcessNode( unary->getOperand(), arg_uid, translator ) );

		const auto&		arg_node = translator.nodes( arg_uid );
		CHECK_ERR( not arg_node.src.Empty() );

		CHECK_ERR( translator.language->TranslateOperator( unary->getOp(), dst_node.typeInfo, { arg_node.src }, { &arg_node.typeInfo }, OUT dst_node.src ) );

		translator.nodes.Add( uid, RVREF(dst_node) );
		return true;
	}
	
/*
=================================================
	RecursiveProcessBinaryNode
=================================================
*/
	static bool RecursiveProcessBinaryNode (TIntermNode* node, const uint uid, Translator &translator)
	{
		glslang::TIntermBinary*		binary = node->getAsBinaryNode();

		if ( binary->getOp() == glslang::TOperator::EOpVectorSwizzle or
			(binary->getOp() == glslang::TOperator::EOpIndexDirect and not binary->getLeft()->isArray() and
			(binary->getLeft()->isScalar() or binary->getLeft()->isVector())) )
		{
			CHECK_ERR( TranslateVectorSwizzle( binary, uid, translator ) );
			return true;
		}

		if ( binary->getOp() == glslang::TOperator::EOpIndexDirectStruct )
		{
			CHECK_ERR( TranslateIndexDirectStruct( binary, uid, translator ) );
			return true;
		}

		if ( binary->getLeft()->getAsConstantUnion() and
			(binary->getOp() == glslang::TOperator::EOpIndexDirect or binary->getOp() == glslang::TOperator::EOpIndexIndirect) )
		{
			CHECK_ERR( TranslateConstUnionAccess( node, uid, translator ) );
			return true;
		}

		const uint	lhs_uid	= ++translator.uid;
		const uint	rhs_uid	= ++translator.uid;

		CHECK_ERR( RecursiveProcessNode( binary->getLeft(), lhs_uid, translator ) );
		CHECK_ERR( RecursiveProcessNode( binary->getRight(), rhs_uid, translator ) );

		const auto &		lhs = translator.nodes( lhs_uid );
		const auto &		rhs = translator.nodes( rhs_uid );
		Translator::Node	dst_node;

		dst_node.uid = uid;
		CHECK_ERR( ConvertType( node, binary->getType(), binary->getLoc(), null, OUT dst_node.typeInfo ) );
		CHECK_ERR( not lhs.src.Empty() and not rhs.src.Empty() );
		
		CHECK_COMP2( GXCheckAccessToExternal( translator, dst_node ) );

		CHECK_ERR( translator.language->TranslateOperator( binary->getOp(), dst_node.typeInfo, { lhs.src, rhs.src }, { &lhs.typeInfo, &rhs.typeInfo }, INOUT dst_node.src ) );

		translator.nodes.Add( uid, RVREF(dst_node) );
		return true;
	}
	
/*
=================================================
	RecursiveProcessLoopNode
=================================================
*/
	static bool RecursiveProcessLoopNode (TIntermNode* node, const uint uid, Translator &translator)
	{
		glslang::TIntermLoop*	loop = node->getAsLoopNode();
		Translator::Node		dst_node;
		dst_node.uid = uid;

		// initialize variables in 'for' loop
		if ( node->getAsAggregate() )
		{
			glslang::TIntermAggregate*	aggr = node->getAsAggregate();

			for (size_t i = 0; i < aggr->getSequence().size(); ++i)
			{
				glslang::TIntermLoop*	nloop = aggr->getSequence()[i]->getAsLoopNode();

				if ( nloop )
				{
					CHECK_ERR( not loop );
					loop = nloop;
					continue;
				}
				
				const uint	arg_uid = ++translator.uid;
				CHECK_ERR( RecursiveProcessNode( aggr->getSequence()[i], arg_uid, translator ) );

				// add local variable declaration
				FOR( j, translator.fwd.pendingVars )
				{
					Translator::TypeInfo	info = translator.fwd.pendingVars[j].second;
					translator.inl.prefixStack.Get() >> info.name;

					CHECK_ERR( translator.language->TranslateLocalVar( info, OUT dst_node.src ) );
					dst_node.src << ";\n";
				}
				// add strings
				FOR( j, translator.fwd.addBeforeLine ) {
					dst_node.src << translator.fwd.addBeforeLine[j] << ";\n";
				}
				translator.fwd.pendingVars.Clear();
				translator.fwd.addBeforeLine.Clear();

				const auto&	arg = translator.nodes( arg_uid );
				CHECK_ERR( not arg.src.Empty() );

				dst_node.src << arg.src << (arg.src.EndsWith(";\n") ? "" : ";\n");
			}
			StringParser::IncreaceIndent( dst_node.src );
		}

		String		loop_src;
		String		test_src;
		String		terminal_src;

		const uint	body_uid = ++translator.uid;
		CHECK_ERR( RecursiveProcessNode( loop->getBody(), body_uid, translator ) );

		loop_src = translator.nodes( body_uid ).src;
		CHECK_ERR( not loop_src.Empty() );
		
		// loop exit condition
		if ( loop->getTest() )
		{
			const uint	test_uid = ++translator.uid;
			CHECK_ERR( RecursiveProcessNode( loop->getTest(), test_uid, translator ) );

			test_src = translator.nodes( test_uid ).src;
			CHECK_ERR( not test_src.Empty() );
		}

		// loop ...
		if ( loop->getTerminal() )
		{
			const uint	terminal_uid = ++translator.uid;
			CHECK_ERR( RecursiveProcessNode( loop->getTerminal(), terminal_uid, translator ) );

			terminal_src = translator.nodes( terminal_uid ).src;
			CHECK_ERR( not terminal_src.Empty() );
		}

		StringParser::IncreaceIndent( loop_src );

		if ( loop->testFirst() )
		{
			// 'for'
			if ( not terminal_src.Empty() )
			{
				loop_src = "for(; "_str << test_src << "; " << terminal_src << ")\n{\n" << loop_src << "}\n";
			}
			else
			// 'while'
			{
				loop_src = "while( "_str << test_src << ")\n{\n" << loop_src << "}\n";
			}
		}
		else
		{
			// 'do-while'
			CHECK_ERR( terminal_src.Empty() );
			loop_src = "do {\n"_str << loop_src << "} while( " << test_src << ")\n";
		}

		// unite with variables
		if ( not dst_node.src.Empty() )
		{
			StringParser::IncreaceIndent( loop_src );
			dst_node.src = "{\n"_str << dst_node.src << loop_src << "}\n";
		}
		else
			dst_node.src = RVREF(loop_src);

		translator.nodes.Add( uid, RVREF(dst_node) );
		return true;
	}

/*
=================================================
	TranslateVectorSwizzle
=================================================
*/
	static bool TranslateVectorSwizzle (glslang::TIntermOperator* node, const uint uid, Translator &translator)
	{
		glslang::TIntermBinary*	binary = node->getAsBinaryNode();
		Translator::Node		dst_node;		dst_node.uid = uid;
		String					sw_mask;

		CHECK_ERR( binary and (binary->getOp() == glslang::TOperator::EOpVectorSwizzle or binary->getOp() == glslang::TOperator::EOpIndexDirect) );

		const uint	lhs_uid	= ++translator.uid;
		CHECK_ERR( RecursiveProcessNode( binary->getLeft(), lhs_uid, translator ) );
		
		const auto&	lhs = translator.nodes( lhs_uid );
		CHECK_ERR( not lhs.src.Empty() );

		const auto ProcessUnion = LAMBDA(&sw_mask) (TIntermNode *node) -> bool
		{
			glslang::TIntermConstantUnion*		cu		= node->getAsConstantUnion();
			glslang::TConstUnionArray const&	cu_arr	= cu->getConstArray();

			CHECK_ERR( cu_arr.size() == 1 and cu->getType().getBasicType() == glslang::EbtInt );
			CHECK_ERR( cu_arr[0].getType() == glslang::EbtInt and cu_arr[0].getIConst() >= 0 and cu_arr[0].getIConst() < 4 );

			const char xyzw[] = "xyzw";

			sw_mask << xyzw[ cu_arr[0].getIConst() ];
			return true;
		};

		if ( binary->getRight()->getAsAggregate() )
		{
			glslang::TIntermAggregate*	aggr = binary->getRight()->getAsAggregate();
			CHECK_ERR( aggr->getOp() == glslang::TOperator::EOpSequence );

			for (size_t i = 0; i < aggr->getSequence().size(); ++i)
			{
				CHECK_ERR( ProcessUnion( aggr->getSequence()[i] ) );
			}
		}
		else
		if ( binary->getRight()->getAsConstantUnion() )
		{
			CHECK_ERR( ProcessUnion( binary->getRight() ) );
		}
		else
			RETURN_ERR( "not supported!" );

		CHECK_ERR( translator.language->TranslateSwizzle( lhs.typeInfo, lhs.src, sw_mask, INOUT dst_node.src ) );

		translator.nodes.Add( uid, RVREF(dst_node) );
		return true;
	}
	
/*
=================================================
	TranslateIndexDirectStruct
=================================================
*/
	static bool TranslateIndexDirectStruct (glslang::TIntermOperator* node, const uint uid, Translator &translator)
	{
		glslang::TIntermBinary*	binary = node->getAsBinaryNode();
		Translator::Node		dst_node;
		
		CHECK_ERR( ConvertType( node, binary->getType(), binary->getLoc(), null, OUT dst_node.typeInfo ) );

		CHECK_ERR( binary and binary->getOp() == glslang::TOperator::EOpIndexDirectStruct );
		CHECK_ERR( binary->getLeft()->getType().isStruct() and binary->getRight()->getAsConstantUnion() );
		
		const auto&	st_type = *binary->getLeft()->getType().getStruct();

		const uint	lhs_uid	= ++translator.uid;
		CHECK_ERR( RecursiveProcessNode( binary->getLeft(), lhs_uid, translator ) );
		
		const auto&	lhs = translator.nodes( lhs_uid );

		glslang::TIntermConstantUnion*		cu		= binary->getRight()->getAsConstantUnion();
		glslang::TConstUnionArray const&	cu_arr	= cu->getConstArray();
		
		CHECK_ERR( cu_arr.size() == 1 and (cu->getType().getBasicType() == glslang::EbtInt or cu->getType().getBasicType() == glslang::EbtUint) );
		CHECK_ERR( (cu_arr[0].getType() == glslang::EbtInt or cu_arr[0].getType() == glslang::EbtUint) and
				   cu_arr[0].getIConst() >= 0 and cu_arr[0].getIConst() < int(st_type.size()) );
		
		dst_node.uid = uid;
		dst_node.src << (lhs.src.Empty() ? "" : lhs.src + ".") << st_type[ cu_arr[0].getIConst() ].type->getFieldName().c_str();

		translator.nodes.Add( uid, RVREF(dst_node) );
		return true;
	}
	
/*
=================================================
	TranslateConstUnionAccess
=================================================
*/
	static bool TranslateConstUnionAccess (TIntermNode* node, const uint uid, Translator &translator)
	{
		glslang::TIntermBinary *			binary	= node->getAsBinaryNode();
		glslang::TIntermConstantUnion*		cu		= binary->getLeft()->getAsConstantUnion();
		glslang::TConstUnionArray const&	cu_arr	= cu->getConstArray();
		Translator::Node					dst_node;

		dst_node.uid = uid;
		CHECK_ERR( ConvertType( cu, cu->getType(), cu->getLoc(), null, OUT dst_node.typeInfo ) );

		FOR( i, translator.constNodes )
		{
			glslang::TConstUnionArray const&	arr = translator.constNodes[i]->getConstArray();

			if ( cu_arr == arr )
			{
				dst_node.src << translator.constNodes[i]->getName().c_str();
				break;
			}
		}
		CHECK_ERR( not dst_node.src.Empty() );

		const uint		rhs_uid	= ++translator.uid;
		CHECK_ERR( RecursiveProcessNode( binary->getRight(), rhs_uid, translator ) );

		const auto&		rhs = translator.nodes( rhs_uid ).src;
		CHECK_ERR( not rhs.Empty() );

		dst_node.src << '[' << rhs << ']';

		translator.nodes.Add( uid, RVREF(dst_node) );
		return true;
	}
	
/*
=================================================
	TranslateFunctionCall
=================================================
*/
	static bool TranslateFunctionCall (TIntermNode* node, const uint uid, Translator &translator)
	{
		glslang::TIntermAggregate*	aggr = node->getAsAggregate();
		CHECK_ERR( aggr and aggr->getOp() == glslang::TOperator::EOpFunctionCall );

		String	name = aggr->getName().c_str();
		usize	pos	 = 0;

		if ( translator.useGXrules )
		{
			Translator::InlFunctionsMap_t::iterator	iter;
			if ( translator.inl.functions.Find( name, OUT iter ) )
				return TranslateInlineFunctionCall( node, uid, iter->second, translator );
		}

		if ( name.Find( '(', OUT pos ) ) {
			name = name.SubString( 0, pos );
		}

		Translator::Node	dst_node;

		// get result type
		CHECK_ERR( ConvertType( node, aggr->getType(), aggr->getLoc(), null, OUT dst_node.typeInfo ) );

		dst_node.src << name << "(";
		
		for (size_t i = 0; i < aggr->getSequence().size(); ++i)
		{
			const uint	arg_uid = ++translator.uid;

			CHECK_ERR( RecursiveProcessNode( aggr->getSequence()[i], arg_uid, translator ) );

			const auto&	arg = translator.nodes( arg_uid ).src;
			CHECK_ERR( not arg.Empty() );

			dst_node.src << (i ? ", " : " ") << arg;
		}

		dst_node.uid = uid;
		dst_node.src << " )";

		translator.nodes.Add( uid, RVREF(dst_node) );
		return true;
	}

/*
=================================================
	UidToPrefix
=================================================
*/
	static String UidToPrefix (const uint uid, Translator &translator)
	{
		const usize	az		= usize('Z' - 'A');
		const usize	hash1	= HashOf( uid ).Get();
		usize		hash	= hash1;
		String		result;

		if_constexpr ( sizeof(hash) > sizeof(uint) ) {
			hash = ((hash >> 32) ^ hash) & 0xFFFFFFFF;
		}
		hash = ((hash >> 16) ^ hash) & 0xFFFF;

		for (uint i = 1; i < 4; ++i)
		{
			result << 'x';
			for (uint j = 1; j <= i; ++j)
			{
				const char	c = ('A' + char( (hash >> (j*4)) % az ));
				result << c;
			}
			result << '_';

			if ( not translator.inl.localNames.IsExist( result ) )
			{
				translator.inl.localNames.Add( result );
				return result;
			}
		}

		RETURN_ERR( "bad hash function!", (String("x") << String().FormatAlignedI( hash1, sizeof(hash1)*2, '0', 16 ) << '_') );
	}
	
/*
=================================================
	TranslateInlineFunctionCall
=================================================
*/
	static bool TranslateInlineFunctionCall (TIntermNode* node, const uint uid, TIntermNode* funcNode, Translator &translator)
	{
		glslang::TIntermAggregate*	aggr = node->getAsAggregate();
		glslang::TIntermAggregate*	func = funcNode->getAsAggregate();

		CHECK_ERR( aggr and aggr->getOp() == glslang::TOperator::EOpFunctionCall );
		CHECK_ERR( func and func->getOp() == glslang::TOperator::EOpFunction and func->getSequence().size() == 2 );
		
		const String			prefix	= UidToPrefix( uid, translator );
		Translator::Node		dst_node;
		String					temp;
	
		dst_node.uid = uid;
		
		temp << "\n// " << aggr->getName().c_str() << "\n";

		// decl args
		{
			Translator::TypeInfo			func_result;
			Array< Translator::TypeInfo >	func_args;
			glslang::TIntermAggregate*		params	= func->getSequence()[0]->getAsAggregate();

			CHECK_ERR( ConvertType( func, func->getType(), func->getLoc(), null, OUT func_result ) );
			CHECK_ERR( params and params->getOp() == glslang::TOperator::EOpParameters );
			CHECK_ERR( params->getSequence().size() == aggr->getSequence().size() );

			for (size_t i = 0; i < aggr->getSequence().size(); ++i)
			{
				// get argument type
				Translator::TypeInfo		param_type;
				glslang::TIntermTyped *		nn = params->getSequence()[i]->getAsTyped();

				CHECK_ERR( ConvertType( nn, nn->getType(), nn->getLoc(), null, OUT param_type ) );

				// get argument value
				const uint	arg_uid = ++translator.uid;
				CHECK_ERR( RecursiveProcessNode( aggr->getSequence()[i], arg_uid, translator ) );

				const auto&	arg = translator.nodes( arg_uid );
				CHECK_ERR( not arg.src.Empty() );
				
				// external types must be replaced, no temporary variables
				if ( param_type.arraySize > 0 or
					 arg.typeInfo.qualifier[ EVariableQualifier::Shared ] or
					 EShaderVariable::IsStruct( param_type.type ) or
					 EShaderVariable::IsTexture( param_type.type ) or
					 EShaderVariable::IsImage( param_type.type ) )
				{
					translator.inl.localsReplacer.Add( prefix + param_type.name, arg.src );
					continue;
				}
				
				// build source
				if ( not param_type.qualifier[ EVariableQualifier::OutArg ] )
					param_type.qualifier |= EVariableQualifier::Constant;

				prefix >> param_type.name;
				CHECK_ERR( translator.language->TranslateLocalVar( param_type, INOUT temp ) );
				temp << " = " << arg.src << ";\n";
			}

			// add result type
			func_result.name = prefix + "return";
			CHECK_ERR( translator.language->TranslateLocalVar( func_result, INOUT temp ) );
			temp << ";\n"; 
			dst_node.src << func_result.name;
		}


		// place function in scope
		{
			translator.inl.prefixStack.Push( prefix );

			glslang::TIntermAggregate*		body		= func->getSequence()[1]->getAsAggregate();
			const uint						body_uid	= ++translator.uid;
			
			CHECK_ERR( RecursiveProcessNode( body, body_uid, translator ) );

			String		body_src = translator.nodes( body_uid ).src;
			CHECK_ERR( not body_src.Empty() );
			
			StringParser::IncreaceIndent( INOUT body_src );

			temp << "for (int " << prefix << " = 0; " << prefix << " == 0; ++" << prefix << ")\n{\n" << body_src << "}";

			ASSERT( prefix == translator.inl.prefixStack.Get() );
			translator.inl.prefixStack.Pop();
		}

		translator.fwd.addBeforeLine.PushBack( RVREF(temp) );
		translator.nodes.Add( uid, RVREF(dst_node) );
		return true;
	}

/*
=================================================
	TranslateOperatorCall
=================================================
*/
	static bool TranslateOperatorCall (TIntermNode* node, const uint uid, Translator &translator)
	{
		glslang::TIntermOperator*				op = node->getAsOperator();
		Translator::Node						dst_node;
		Array< String >							args_src;
		Array< Translator::TypeInfo const* >	arg_types;

		// get result type
		dst_node.uid = uid;
		CHECK_ERR( ConvertType( node, op->getType(), op->getLoc(), null, OUT dst_node.typeInfo ) );
		
		// convert args
		if ( node->getAsAggregate() )
		{
			glslang::TIntermAggregate* aggr = node->getAsAggregate();
			
			for (size_t i = 0; i < aggr->getSequence().size(); ++i)
			{
				const uint	arg_uid = ++translator.uid;

				CHECK_ERR( RecursiveProcessNode( aggr->getSequence()[i], arg_uid, translator ) );

				const auto&	arg = translator.nodes( arg_uid );
				CHECK_ERR( not arg.src.Empty() );

				args_src.PushBack( arg.src );
				arg_types.PushBack( &arg.typeInfo );
			}
		}

		CHECK_ERR( translator.language->TranslateOperator( op->getOp(), dst_node.typeInfo, args_src, arg_types, OUT dst_node.src ) );

		translator.nodes.Add( uid, RVREF(dst_node) );
		return true;
	}

}	// PipelineCompiler