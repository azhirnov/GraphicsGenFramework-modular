// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'
/*
	Translate from GXSL/GLSL to CL.
*/

#include "Engine/PipelineCompiler/Shaders/ShaderCompiler_Translator.h"
//#include "Engine/PipelineCompiler/Common/ToGLSL.h"
#include "Engine/PipelineCompiler/cl/cl_source_vfs.h"

namespace PipelineCompiler
{

	//
	// CL Language
	//

	class CL_DstLanguage : public Translator::IDstLanguage
	{
	// types
	private:
		using TypeInfo		= Translator::TypeInfo;
		using StringMap_t	= HashMap< String, String >;
		using Externals_t	= Array< TypeInfo >;

		struct FuncInfo {
			String			name;
			Bitfield<64>	ptrArgs;	// setted if argument is pointer
		};

		using FuncSignMap_t	= HashMap< String, FuncInfo >;
		using StringSet_t	= HashSet< String >;


	// variables
	private:
		Translator&		_translator;
		StringMap_t		_builtinMap;
		Externals_t		_externals;

		FuncSignMap_t	_definedFuncs;
		StringSet_t		_existingFuncs;


	// methods
	public:
		explicit CL_DstLanguage (Translator &translator);
		~CL_DstLanguage () {}

		bool TranslateLocalVar (const TypeInfo &, INOUT String &src) override;
		bool TranslateStruct (const TypeInfo &, INOUT String &src) override;
		bool TranslateType (const TypeInfo &, INOUT String &src) override;
		bool TranslateName (const TypeInfo &, INOUT String &src) override;

		bool TranslateFunctionDecl (StringCRef sign, const TypeInfo &resultType, ArrayCRef<TypeInfo> args, INOUT String &src) override;
		bool TranslateFunctionCall (StringCRef sign, const TypeInfo &resultType, ArrayCRef<String> args, ArrayCRef<TypeInfo const*> argTypes, INOUT String &src) override;
		
		bool TranslateConstant (const glslang::TConstUnionArray &, const TypeInfo &, INOUT String &src) override;
		bool TranslateExternal (glslang::TIntermTyped*, const TypeInfo &, INOUT String &src) override;
		bool TranslateOperator (glslang::TOperator op, const TypeInfo &resultType, ArrayCRef<String> args, ArrayCRef<TypeInfo const*> argTypes, INOUT String &src) override;
		bool TranslateSwizzle (const TypeInfo &type, StringCRef val, StringCRef swizzle, INOUT String &src) override;

		bool TranslateEntry (const TypeInfo &ret, StringCRef sign, ArrayCRef<TypeInfo> args, INOUT String &src) override;
		bool TranslateStructAccess (const TypeInfo &stType, StringCRef objName, const TypeInfo &fieldType, INOUT String &src) override;
		
		bool TranslateValue (VariantCRef value, INOUT String &src) const override;

		bool DeclExternalTypes () const	override	{ return true; }

	private:
		String _TranslateFunctionName (StringCRef sign);
		bool _TranslateArg (const TypeInfo &t, INOUT String &res);

		bool _TranslateBuffer (Translator::TypeInfo const& info, OUT String &str);
		bool _TranslateImage (Translator::TypeInfo const& info, OUT String &str);

		bool _TranslateShared (Translator::TypeInfo const& info, OUT String &str);
		bool _TranslateConst (glslang::TIntermTyped* typed, Translator::TypeInfo const& info, OUT String &str);
		
		bool _RecursiveInitConstStruct (const Array<Translator::TypeInfo> &fields, const glslang::TConstUnionArray& cu_arr, INOUT int &index, OUT String &src);

		static String ToStringCL (EShaderMemoryModel::type value);
		static String ToStringCL (EPixelFormat::type value);
		static String ToStringCL (EShaderVariable::type value);
	};

	

/*
=================================================
	_TranslateGXSLtoGLSL
=================================================
*/
	static bool TranslateShaderInfo (const glslang::TIntermediate* intermediate, Translator &translator);

	bool ShaderCompiler::_TranslateGXSLtoCL (const Config &cfg, const _GLSLangResult &glslangData, OUT String &log, OUT BinaryArray &result) const
	{
		CHECK_ERR(	cfg.source == EShaderSrcFormat::GXSL or
					cfg.source == EShaderSrcFormat::GLSL or
					cfg.source == EShaderSrcFormat::GXSL_Vulkan or
					cfg.source == EShaderSrcFormat::GLSL_Vulkan );
		CHECK_ERR(	cfg.target == EShaderDstFormat::CL_Source );
	
		// not supported here
		//ASSERT( not cfg.filterInactive );
		ASSERT( not cfg.optimize );
		ASSERT( not cfg.skipExternals );

		const glslang::TIntermediate* intermediate = glslangData.prog.getIntermediate( glslangData.shader->getStage() );
		CHECK_ERR( intermediate );
		CHECK_ERR( intermediate->getStage() == EShLanguage::EShLangCompute );

		TIntermNode*	root	= intermediate->getTreeRoot();
		Translator		translator;

		translator.states.useGXrules	= intermediate->getSource() == glslang::EShSourceGxsl;
		translator.states.inlineAll		= cfg.inlineAll;
		translator.entryPoint			= intermediate->getEntryPointName().c_str();
		translator.language				= new CL_DstLanguage( translator );

		CHECK_ERR( TranslateShaderInfo( intermediate, translator ) );

		CHECK_ERR( translator.Main( root, false ) );

		translator.src << "}\n";

		log		<< translator.log;
		result	= BinArrayCRef::From( translator.src );
		return true;
	}
//-----------------------------------------------------------------------------



/*
=================================================
	TranslateShaderInfo
=================================================
*/
	static bool TranslateShaderInfo (const glslang::TIntermediate*, Translator &translator)
	{
		String&	str = translator.src;
		
		str << "#define FORMAT( _fmt_ )\n";

		str << "#define INOUT\n"
			<< "#define OUT\n";

		CHECK_ERR( cl_vfs::LoadFile( "funcs.cl", OUT str ) );

		str << "\n";

		// TODO
		return true;
	}
	
/*
=================================================
	constructor
=================================================
*/
	CL_DstLanguage::CL_DstLanguage (Translator &translator) : _translator{translator}
	{
		_builtinMap.Add( "gl_LocalInvocationIndex",	"((uint)(get_local_linear_id()))" );
		_builtinMap.Add( "gl_GlobalInvocationID",	"((uint3)(get_global_id(0),  get_global_id(1),  get_global_id(2)))" );
		_builtinMap.Add( "gl_LocalInvocationID",	"((uint3)(get_local_id(0),   get_local_id(1),   get_local_id(2)))" );
		_builtinMap.Add( "gl_NumWorkGroups",		"((uint3)(get_num_groups(0), get_num_groups(1), get_num_groups(2)))" );
		_builtinMap.Add( "gl_WorkGroupID",			"((uint3)(get_global_id(0),  get_global_id(1),  get_global_id(2)))" );
		_builtinMap.Add( "gl_WorkGroupSize",		"((uint3)(get_local_size(0), get_local_size(1), get_local_size(2)))" );
	}

/*
=================================================
	TranslateLocalVar
=================================================
*/
	bool CL_DstLanguage::TranslateLocalVar (const TypeInfo &t, INOUT String &res)
	{
		if ( t.qualifier[ EVariableQualifier::Volatile ] )
			res << "volatile ";

		// read-only
		if ( t.qualifier[ EVariableQualifier::Constant ] )
			res << "const ";

		// image format
		if ( t.format != EPixelFormat::Unknown )
			res << "FORMAT(" << ToStringCL( t.format ) << ") ";

		// type
		if ( not t.typeName.Empty() ) {
			res << "struct " << t.typeName;
		} else {
			res << ToStringCL( t.type );
		}
		res << " ";
		
		if ( t.arraySize == 0 )		res << t.name;				else
		if ( t.arraySize == UMax )	res << "* " << t.name;		else
									res << t.name << " [" << t.arraySize << "]";
		return true;
	}
	
/*
=================================================
	TranslateStruct
=================================================
*/
	bool CL_DstLanguage::TranslateStruct (const TypeInfo &info, INOUT String &src)
	{
		src << "struct " << info.typeName << "\n{\n";

		FOR( j, info.fields )
		{
			src << "\t";

			auto const& t = info.fields[j];
			{
				if ( t.qualifier[ EVariableQualifier::Volatile ] )
					src << "volatile ";

				// read-only
				if ( t.qualifier[ EVariableQualifier::Constant ] )
					src << "const ";

				// image format
				if ( t.format != EPixelFormat::Unknown )
					src << "FORMAT(" << ToStringCL( t.format ) << ") ";

				// type
				if ( not t.typeName.Empty() ) {
					src << "struct " << t.typeName;
				} else {
					src << ToStringCL( t.type );
				}
				src << " ";
		
				if ( t.arraySize == 0 )		src << t.name;				else
				if ( t.arraySize == UMax )	src << t.name << " []";		else			// array memory placed in struct
											src << t.name << " [" << t.arraySize << "]";
			}

			src << ";\n";
		}

		src << "};\n\n";
		return true;
	}
	
/*
=================================================
	TranslateType
=================================================
*/
	bool CL_DstLanguage::TranslateType (const TypeInfo &t, INOUT String &res)
	{
		if ( t.qualifier[ EVariableQualifier::Volatile ] )
			res << "volatile ";

		// image format
		if ( t.format != EPixelFormat::Unknown )
			res << "FORMAT(" << ToStringCL( t.format ) << ") ";

		// type
		if ( not t.typeName.Empty() ) {
			res << "struct " << t.typeName;
		} else {
			res << ToStringCL( t.type );
		}
		
		if ( t.arraySize == 0 )		{}				else
		if ( t.arraySize == UMax )	res << "* ";	else
									res << " [" << t.arraySize << "]";

		return true;
	}
	
/*
=================================================
	TranslateName
=================================================
*/
	bool CL_DstLanguage::TranslateName (const TypeInfo &t, INOUT String &src)
	{
		// convert builtin names
		StringMap_t::iterator	iter;

		if ( _builtinMap.Find( t.name, OUT iter ) )
		{
			src << iter->second;
			return true;
		}

		src << t.name;
		return true;
	}
	
/*
=================================================
	_TranslateFunctionName
=================================================
*/
	String CL_DstLanguage::_TranslateFunctionName (StringCRef signature)
	{
		usize	pos  = 0;
		String	name = signature;

		if ( name.Find( '(', OUT pos ) )
		{
			FuncSignMap_t::iterator	iter;

			if ( _definedFuncs.Find( signature, OUT iter ) )
			{
				name = iter->second.name;
				return name;
			}
			
			for (usize i = 0;; ++i)
			{
				name = name.SubString( 0, pos );

				if ( i > 0 )
					name << '_' << i;

				if ( not _existingFuncs.IsExist( name ) )
					break;
			}
		}

		_definedFuncs.Add( signature, { name, Uninitialized } );
		_existingFuncs.Add( name );
		return name;
	}

/*
=================================================
	TranslateExternal
=================================================
*/
	bool CL_DstLanguage::TranslateExternal (glslang::TIntermTyped* typed, const TypeInfo &info, INOUT String &str)
	{
		if ( EShaderVariable::IsBuffer( info.type ) ) {
			_externals.PushBack( info );
		}
		else
		if ( EShaderVariable::IsImage( info.type ) or EShaderVariable::IsTexture( info.type ) ) {
			_externals.PushBack( info );
		}
		else
		if ( info.qualifier[ EVariableQualifier::Shared ] ) {
			_externals.PushBack( info );
		}
		else
		if ( info.qualifier[ EVariableQualifier::Constant ] ) {
			CHECK_ERR( _TranslateConst( typed, info, INOUT str ) );
		}
		else {
			RETURN_ERR( "not supported!" );
		}
		return true;
	}
	
/*
=================================================
	TranslateOperator
=================================================
*/
	bool CL_DstLanguage::TranslateOperator (glslang::TOperator op, const TypeInfo &resultType, ArrayCRef<String> inArgs, ArrayCRef<TypeInfo const*> argTypes, INOUT String &src)
	{
		using vinfo = _platforms_hidden_::EValueTypeInfo;

		Array<String>	args = inArgs;
		
		// pointer dereference
		FOR( i, args )
		{
			if ( argTypes[i]->qualifier[ EVariableQualifier::OutArg ] )
				'*' >> args[i];
		}

		CHECK_ERR( args.Count() == argTypes.Count() );

		String	all_args;
		FOR( i, args ) {
			all_args << (i ? ", " : "( ") << args[i];
		}
		all_args << " )";

		if ( op >= glslang::TOperator::EOpConvIntToBool and op <= glslang::TOperator::EOpConvUint16ToUint64 )
		{
			String	tname;
			CHECK_ERR( args.Count() == 1 );
			CHECK_ERR( TranslateType( resultType, OUT tname ) );
			src <<"convert_"<<tname<< all_args;
			return true;
		}

		if ( op >= glslang::TOperator::EOpConstructGuardStart and op < glslang::TOperator::EOpConstructGuardEnd )
		{
			String	tname;
			CHECK_ERR( TranslateType( resultType, OUT tname ) );
			src <<"(("<<tname<<')'<< all_args << ')';
			return true;
		}

		if ( args.Count() == 0 )
		{
			switch ( op )
			{
				case glslang::TOperator::EOpBarrier :					src << "barrier(CLK_LOCAL_MEM_FENCE)";		break;
				//case glslang::TOperator::EOpMemoryBarrier :				src << "memoryBarrier()";					break;
				//case glslang::TOperator::EOpMemoryBarrierAtomicCounter:	src << "memoryBarrierAtomicCounter()";		break;
				//case glslang::TOperator::EOpMemoryBarrierBuffer :		src << "memoryBarrierBuffer()";				break;
				//case glslang::TOperator::EOpMemoryBarrierImage :		src << "memoryBarrierImage()";				break;
				//case glslang::TOperator::EOpMemoryBarrierShared :		src << "memoryBarrierShared()";				break;
				//case glslang::TOperator::EOpGroupMemoryBarrier :		src << "groupMemoryBarrier()";				break;
				default :												RETURN_ERR( "unknown operator!" );
			}
			return true;
		}
		
		if ( args.Count() == 1 )
		{
			const uint  vsize = EShaderVariable::VecSize( argTypes[0]->type );

			switch ( op )
			{
				case glslang::TOperator::EOpNegative :				src << "-" << all_args;				break;
				case glslang::TOperator::EOpLogicalNot :			src << "!" << all_args;				break;
				case glslang::TOperator::EOpVectorLogicalNot :		src << "!" << all_args;				break;
				case glslang::TOperator::EOpBitwiseNot :			src << "~" << all_args;				break;
				case glslang::TOperator::EOpPostIncrement :			src << all_args << "++";			break;
				case glslang::TOperator::EOpPostDecrement :			src << all_args << "--";			break;
				case glslang::TOperator::EOpPreIncrement :			src << "++" << all_args;			break;
				case glslang::TOperator::EOpPreDecrement :			src << "--" << all_args;			break;
				//case glslang::TOperator::EOpArrayLength :			src << all_args << ".length()";		break;
				case glslang::TOperator::EOpAny :					src << "any" << all_args;			break;
				case glslang::TOperator::EOpAll :					src << "all" << all_args;			break;
				case glslang::TOperator::EOpRadians :				src << "radians" << all_args;		break;
				case glslang::TOperator::EOpDegrees :				src << "degrees" << all_args;		break;
				case glslang::TOperator::EOpSin :					src << "sin" << all_args;			break;
				case glslang::TOperator::EOpCos :					src << "cos" << all_args;			break;
				case glslang::TOperator::EOpTan :					src << "tan" << all_args;			break;
				case glslang::TOperator::EOpAsin :					src << "asin" << all_args;			break;
				case glslang::TOperator::EOpAcos :					src << "acos" << all_args;			break;
				case glslang::TOperator::EOpAtan :					src << "atan" << all_args;			break;
				case glslang::TOperator::EOpSinh :					src << "sinh" << all_args;			break;
				case glslang::TOperator::EOpCosh :					src << "cosh" << all_args;			break;
				case glslang::TOperator::EOpTanh :					src << "tanh" << all_args;			break;
				case glslang::TOperator::EOpAsinh :					src << "asinh" << all_args;			break;
				case glslang::TOperator::EOpAcosh :					src << "acosh" << all_args;			break;
				case glslang::TOperator::EOpAtanh :					src << "atanh" << all_args;			break;
				case glslang::TOperator::EOpExp :					src << "exp" << all_args;			break;
				case glslang::TOperator::EOpLog :					src << "log" << all_args;			break;
				case glslang::TOperator::EOpExp2 :					src << "exp2" << all_args;			break;
				case glslang::TOperator::EOpLog2 :					src << "log2" << all_args;			break;
				case glslang::TOperator::EOpSqrt :					src << "sqrt" << all_args;			break;
				case glslang::TOperator::EOpInverseSqrt :			src << "rsqrt" << all_args;			break;
				case glslang::TOperator::EOpAbs :
					if ( EShaderVariable::IsFloat( argTypes[0]->type ) )	src << "fabs" << all_args;
					else													src << "abs" << all_args;
					break;
				case glslang::TOperator::EOpSign :					src << "sign" << all_args;			break;
				case glslang::TOperator::EOpFloor :					src << "floor" << all_args;			break;
				case glslang::TOperator::EOpTrunc :					src << "trunc" << all_args;			break;
				case glslang::TOperator::EOpRound :					src << "round" << all_args;			break;
				case glslang::TOperator::EOpRoundEven :				src << "rint" << all_args;			break;
				case glslang::TOperator::EOpCeil :					src << "ceil" << all_args;			break;
				case glslang::TOperator::EOpIsNan :					src << "isnan" << all_args;			break;
				case glslang::TOperator::EOpIsInf :					src << "isinf" << all_args;			break;

				case glslang::TOperator::EOpFract :					src << "fractTempl_" << ToStringCL( argTypes[0]->type ) << all_args;	break;

				case glslang::TOperator::EOpFloatBitsToInt :		src << "as_int" << vsize << all_args;		break;
				case glslang::TOperator::EOpFloatBitsToUint :		src << "as_uint" << vsize << all_args;		break;
				case glslang::TOperator::EOpIntBitsToFloat :		src << "as_float" << vsize << all_args;		break;
				case glslang::TOperator::EOpUintBitsToFloat :		src << "as_float" << vsize << all_args;		break;
				case glslang::TOperator::EOpDoubleBitsToInt64 :		src << "as_ulong" << vsize << all_args;		break;
				case glslang::TOperator::EOpDoubleBitsToUint64 :	src << "as_ulong" << vsize << all_args;		break;
				case glslang::TOperator::EOpInt64BitsToDouble :		src << "as_double" << vsize << all_args;	break;
				case glslang::TOperator::EOpUint64BitsToDouble :	src << "as_double" << vsize << all_args;	break;
				#ifdef AMD_EXTENSIONS
				//case glslang::TOperator::EOpFloat16BitsToInt16 :	src << "float16BitsToInt16" << all_args;		break;
				//case glslang::TOperator::EOpFloat16BitsToUint16 :	src << "float16BitsToUint16" << all_args;		break;
				//case glslang::TOperator::EOpInt16BitsToFloat16 :	src << "int16BitsToFloat16" << all_args;		break;
				//case glslang::TOperator::EOpUint16BitsToFloat16 :	src << "uint16BitsToFloat16" << all_args;		break;
				#endif
				//case glslang::TOperator::EOpPackSnorm2x16 :		src << "packSnorm2x16" << all_args;				break;	// TODO
				//case glslang::TOperator::EOpUnpackSnorm2x16 :		src << "unpackSnorm2x16" << all_args;			break;
				//case glslang::TOperator::EOpPackUnorm2x16 :		src << "packUnorm2x16" << all_args;				break;
				//case glslang::TOperator::EOpUnpackUnorm2x16 :		src << "unpackUnorm2x16" << all_args;			break;
				//case glslang::TOperator::EOpPackSnorm4x8 :		src << "packSnorm4x8" << all_args;				break;
				//case glslang::TOperator::EOpUnpackSnorm4x8 :		src << "unpackSnorm4x8" << all_args;			break;
				//case glslang::TOperator::EOpPackUnorm4x8 :		src << "packUnorm4x8" << all_args;				break;
				//case glslang::TOperator::EOpUnpackUnorm4x8 :		src << "unpackUnorm4x8" << all_args;			break;
				//case glslang::TOperator::EOpPackHalf2x16 :		src << "packHalf2x16" << all_args;				break;
				//case glslang::TOperator::EOpUnpackHalf2x16 :		src << "unpackHalf2x16" << all_args;			break;
				//case glslang::TOperator::EOpPackDouble2x32 :		src << "packDouble2x32" << all_args;			break;
				//case glslang::TOperator::EOpUnpackDouble2x32 :	src << "unpackDouble2x32" << all_args;			break;
				//case glslang::TOperator::EOpPackInt2x32 :			src << "packInt2x32" << all_args;				break;
				//case glslang::TOperator::EOpUnpackInt2x32 :		src << "unpackInt2x32" << all_args;				break;
				//case glslang::TOperator::EOpPackUint2x32 :		src << "packUint2x32" << all_args;				break;
				//case glslang::TOperator::EOpUnpackUint2x32 :		src << "unpackUint2x32" << all_args;			break;
				#ifdef AMD_EXTENSIONS
				//case glslang::TOperator::EOpPackFloat2x16 :		src << "packFloat2x16" << all_args;				break;
				//case glslang::TOperator::EOpUnpackFloat2x16 :		src << "unpackFloat2x16" << all_args;			break;
				//case glslang::TOperator::EOpPackInt2x16 :			src << "packInt2x16" << all_args;				break;
				//case glslang::TOperator::EOpUnpackInt2x16 :		src << "unpackInt2x16" << all_args;				break;
				//case glslang::TOperator::EOpPackUint2x16 :		src << "packUint2x16" << all_args;				break;
				//case glslang::TOperator::EOpUnpackUint2x16 :		src << "unpackUint2x16" << all_args;			break;
				//case glslang::TOperator::EOpPackInt4x16 :			src << "packInt4x16" << all_args;				break;
				//case glslang::TOperator::EOpUnpackInt4x16 :		src << "unpackInt4x16" << all_args;				break;
				//case glslang::TOperator::EOpPackUint4x16 :		src << "packUint4x16" << all_args;				break;
				//case glslang::TOperator::EOpUnpackUint4x16 :		src << "unpackUint4x16" << all_args;			break;
				#endif
				case glslang::TOperator::EOpLength :				src << "length" << all_args;					break;
				case glslang::TOperator::EOpNormalize :				src << "normalize" << all_args;					break;
				//case glslang::TOperator::EOpDPdx :				src << "dFdx" << all_args;						break;
				//case glslang::TOperator::EOpDPdy :				src << "dFdy" << all_args;						break;
				//case glslang::TOperator::EOpFwidth :				src << "fwidth" << all_args;					break;
				//case glslang::TOperator::EOpDPdxFine :			src << "dFdxFine" << all_args;					break;
				//case glslang::TOperator::EOpDPdyFine :			src << "dFdyFine" << all_args;					break;
				//case glslang::TOperator::EOpFwidthFine :			src << "fwidthFine" << all_args;				break;
				//case glslang::TOperator::EOpDPdxCoarse :			src << "dFdxCoarse" << all_args;				break;
				//case glslang::TOperator::EOpDPdyCoarse :			src << "dFdyCoarse" << all_args;				break;
				//case glslang::TOperator::EOpFwidthCoarse :		src << "fwidthCoarse" << all_args;				break;
				//case glslang::TOperator::EOpInterpolateAtCentroid :src << "interpolateAtCentroid" << all_args;	break;
				case glslang::TOperator::EOpBitCount :				src << "popcount" << all_args;					break;

				case glslang::TOperator::EOpBitFieldReverse :		src << "bitfieldReverse_" << ToStringCL( argTypes[0]->type ) << all_args;	break;
				case glslang::TOperator::EOpFindMSB :				src << "findMSB_" << ToStringCL( argTypes[0]->type ) << all_args;			break;
				case glslang::TOperator::EOpFindLSB :				src << "findLSB_" << ToStringCL( argTypes[0]->type ) << all_args;			break;

				#ifdef AMD_EXTENSIONS
				//case glslang::TOperator::EOpInterpolateAtVertex :	src << "atanh" << all_args;						break;
				#endif
				//case glslang::TOperator::EOpDeterminant :			src << "determinant" << all_args;				break;
				//case glslang::TOperator::EOpMatrixInverse :		src << "inverse" << all_args;					break;
				//case glslang::TOperator::EOpTranspose :			src << "transpose" << all_args;					break;

				case glslang::TOperator::EOpImageQuerySize :		src << "get_image_dim" << all_args;				break;
				case glslang::TOperator::EOpImageQuerySamples :		src << "get_image_num_samples" << all_args;		break;
				//case glslang::TOperator::EOpNoise :				src << "noise" << all_args;						break;	// deprecated
					
				//case glslang::TOperator::EOpAtomicCounter :		src << "atomicCounter" << all_args;				break;
				//case glslang::TOperator::EOpAtomicCounterIncrement:src << "atomicCounterIncrement" << all_args;	break;
				//case glslang::TOperator::EOpAtomicCounterDecrement:src << "atomicCounterDecrement" << all_args;	break;

				//case glslang::TOperator::EOpTextureQuerySize :	src << "textureSize" << all_args;				break;
				//case glslang::TOperator::EOpTextureQueryLevels :	src << "textureQueryLevels" << all_args;		break;
				//case glslang::TOperator::EOpTextureQuerySamples :	src << "textureSamples" << all_args;			break;
				default :											RETURN_ERR( "unknown operator!" );
			}
			return true;
		}
		
		if ( args.Count() == 2 )
		{
			const bool	is_vec	=	(resultType.arraySize == 0 and EShaderVariable::VecSize( resultType.type ) > 1)		and
									(argTypes[0]->arraySize == 0 and EShaderVariable::VecSize( argTypes[0]->type ) > 1)	and
									(argTypes[1]->arraySize == 0 and EShaderVariable::VecSize( argTypes[1]->type ) > 1);
			
			const bool	is_float = (EShaderVariable::IsFloat( argTypes[0]->type ) or EShaderVariable::IsFloat( argTypes[1]->type ));

			if ( is_vec and is_float )
			{
				const auto &	lhs_type = *argTypes[0];
				const auto &	rhs_type = *argTypes[1];
				const uint		vec_size = Max( EShaderVariable::VecSize( resultType.type ), EShaderVariable::VecSize( lhs_type.type ), EShaderVariable::VecSize( rhs_type.type ) );
				const auto		sh_type  = EShaderVariable::ToVec( lhs_type.type, vec_size );
				String			temp;
			
				if ( lhs_type.type != sh_type )		temp << ToStringCL( sh_type ) << '(' << args[0] << ')';
				else								temp << args[0];
				temp << ", ";
				if ( rhs_type.type != sh_type )		temp << ToStringCL( sh_type ) << '(' << args[1] << ')';
				else								temp << args[1];
			
				switch ( op )
				{
					case glslang::TOperator::EOpVectorEqual :		src<<"isequal("<<temp<<')';			return true;
					case glslang::TOperator::EOpVectorNotEqual :	src<<"isnotequal("<<temp<<')';		return true;
					case glslang::TOperator::EOpLessThan :			src<<"isless("<<temp<<')';			return true;
					case glslang::TOperator::EOpGreaterThan :		src<<"isgreater("<<temp<<')';		return true;
					case glslang::TOperator::EOpLessThanEqual :		src<<"islessequal("<<temp<<')';		return true;
					case glslang::TOperator::EOpGreaterThanEqual :	src<<"isgreaterequal("<<temp<<')';	return true;
				}
			}

			if ( is_float )
			{
				switch ( op )
				{
					case glslang::TOperator::EOpMod :					src << "fmod("<<args[0]<<", "<<args[1]<<')';	return true;
				}
			}

			switch ( op )
			{
				case glslang::TOperator::EOpAdd :						src << '('<<args[0] <<" + " << args[1]<<')';	break;
				case glslang::TOperator::EOpSub :						src << '('<<args[0] <<" - " << args[1]<<')';	break;
				case glslang::TOperator::EOpMul :						src << '('<<args[0] <<" * " << args[1]<<')';	break;
				case glslang::TOperator::EOpDiv :						src << '('<<args[0] <<" / " << args[1]<<')';	break;
				case glslang::TOperator::EOpMod :						src << '('<<args[0] <<" % " << args[1]<<')';	break;
				case glslang::TOperator::EOpRightShift :				src << '('<<args[0] <<" >> "<< args[1]<<')';	break;
				case glslang::TOperator::EOpLeftShift :					src << '('<<args[0] <<" << "<< args[1]<<')';	break;
				case glslang::TOperator::EOpAnd :						src << '('<<args[0] <<" & " << args[1]<<')';	break;
				case glslang::TOperator::EOpInclusiveOr :				src << '('<<args[0] <<" | " << args[1]<<')';	break;
				case glslang::TOperator::EOpExclusiveOr :				src << '('<<args[0] <<" ^ " << args[1]<<')';	break;
				case glslang::TOperator::EOpVectorEqual :
				case glslang::TOperator::EOpEqual :						src << '('<<args[0] <<" == "<< args[1]<<')';	break;
				case glslang::TOperator::EOpVectorNotEqual :
				case glslang::TOperator::EOpNotEqual :					src << '('<<args[0] <<" != "<< args[1]<<')';	break;
				case glslang::TOperator::EOpLessThan :					src << '('<<args[0] <<" < " << args[1]<<')';	break;
				case glslang::TOperator::EOpGreaterThan :				src << '('<<args[0] <<" > " << args[1]<<')';	break;
				case glslang::TOperator::EOpLessThanEqual :				src << '('<<args[0] <<" <= "<< args[1]<<')';	break;
				case glslang::TOperator::EOpGreaterThanEqual :			src << '('<<args[0] <<" >= "<< args[1]<<')';	break;
				case glslang::TOperator::EOpVectorTimesScalar :			src << '('<<args[0] <<" * " << args[1]<<')';	break;
				case glslang::TOperator::EOpVectorTimesMatrix :			src << '('<<args[0] <<" * " << args[1]<<')';	break;
				case glslang::TOperator::EOpMatrixTimesVector :			src << '('<<args[0] <<" * " << args[1]<<')';	break;
				case glslang::TOperator::EOpMatrixTimesScalar :			src << '('<<args[0] <<" * " << args[1]<<')';	break;
				case glslang::TOperator::EOpLogicalOr :					src << '('<<args[0] <<" || "<< args[1]<<')';	break;
				case glslang::TOperator::EOpLogicalXor :				src << '('<<args[0] <<" ^^ "<< args[1]<<')';	break;
				case glslang::TOperator::EOpLogicalAnd :				src << '('<<args[0] <<" && "<< args[1]<<')';	break;
				case glslang::TOperator::EOpIndexDirect :				src << '('<<args[0] <<'[' << args[1]<<"])";		break;
				case glslang::TOperator::EOpIndexIndirect :				src << '('<<args[0] <<'[' << args[1]<<"])";		break;
				//case glslang::TOperator::EOpMethod :					break;
				//case glslang::TOperator::EOpScoping :					break;
				case glslang::TOperator::EOpAssign :					src <<      args[0] <<" = " << args[1];			break;
				case glslang::TOperator::EOpAddAssign :					src << '('<<args[0] <<" += "<< args[1]<<')';	break;
				case glslang::TOperator::EOpSubAssign :					src << '('<<args[0] <<" -= "<< args[1]<<')';	break;
				case glslang::TOperator::EOpMulAssign :					src << '('<<args[0] <<" *= "<< args[1]<<')';	break;
				case glslang::TOperator::EOpVectorTimesMatrixAssign :	src << '('<<args[0] <<" *= "<< args[1]<<')';	break;
				case glslang::TOperator::EOpVectorTimesScalarAssign :	src << '('<<args[0] <<" *= "<< args[1]<<')';	break;
				case glslang::TOperator::EOpMatrixTimesScalarAssign :	src << '('<<args[0] <<" *= "<< args[1]<<')';	break;
				case glslang::TOperator::EOpMatrixTimesMatrixAssign :	src << '('<<args[0] <<" *= "<< args[1]<<')';	break;
				case glslang::TOperator::EOpDivAssign :					src << '('<<args[0] <<" /= "<< args[1]<<')';	break;
				case glslang::TOperator::EOpModAssign :					src << '('<<args[0] <<" %= "<< args[1]<<')';	break;
				case glslang::TOperator::EOpAndAssign :					src << '('<<args[0] <<" &= "<< args[1]<<')';	break;
				case glslang::TOperator::EOpInclusiveOrAssign :			src << '('<<args[0] <<" |= "<< args[1]<<')';	break;
				case glslang::TOperator::EOpExclusiveOrAssign :			src << '('<<args[0] <<" ^= "<< args[1]<<')';	break;
				case glslang::TOperator::EOpLeftShiftAssign :			src << '('<<args[0] <<" <<= "<< args[1]<<')';	break;
				case glslang::TOperator::EOpRightShiftAssign :			src << '('<<args[0] <<" >>= "<< args[1]<<')';	break;

				case glslang::TOperator::EOpAtan :						src << "atan2" << all_args;						break;
				case glslang::TOperator::EOpPow :						src << "pow" << all_args;						break;
				case glslang::TOperator::EOpMin :						src << "min" << all_args;						break;
				case glslang::TOperator::EOpMax :						src << "max" << all_args;						break;
				case glslang::TOperator::EOpStep :						src << "step" << all_args;						break;
				case glslang::TOperator::EOpFrexp :						src << "frexp" << all_args;						break;
				case glslang::TOperator::EOpLdexp :						src << "ldexp" << all_args;						break;
				case glslang::TOperator::EOpDistance :					src << "distance" << all_args;					break;
				case glslang::TOperator::EOpDot :						src << "dot" << all_args;						break;
				case glslang::TOperator::EOpCross :						src << "cross" << all_args;						break;
				case glslang::TOperator::EOpReflect :					src << "reflect" << all_args;					break;
				case glslang::TOperator::EOpInterpolateAtSample :		src << "interpolateAtSample" << all_args;		break;
				case glslang::TOperator::EOpInterpolateAtOffset :		src << "interpolateAtOffset" << all_args;		break;
				case glslang::TOperator::EOpOuterProduct :				src << "outerProduct" << all_args;				break;
				case glslang::TOperator::EOpMatrixTimesMatrix :			src << "matrixCompMult" << all_args;			break;

				case glslang::TOperator::EOpModf :
					src << "modf(" << args[0] << ", &" << args[1] << ")";
					break;

				case glslang::TOperator::EOpImageLoad : {
					String	fmt = "f";
					switch ( argTypes[0]->type & vinfo::_TYPE_FLAG_MASK )
					{
						case vinfo::_HALF :			fmt = "h";	break;
						case vinfo::_INT :			fmt = "ui";	break;
						case vinfo::_UINT :			fmt = "i";	break;
					}
					switch ( argTypes[0]->type & vinfo::_SAMP_MASK )
					{
						case vinfo::_SAMP_1D :		src << "read_image"<<fmt<<"("<<args[0]<<", "<<args[1]<<")"; 				break;
						case vinfo::_SAMP_1DA :		src << "read_image"<<fmt<<"("<<args[0]<<", "<<args[1]<<")"; 				break;
						case vinfo::_SAMP_2D :		src << "read_image"<<fmt<<"("<<args[0]<<", "<<args[1]<<")"; 				break;
						case vinfo::_SAMP_2DA :		src << "read_image"<<fmt<<"("<<args[0]<<", convert_int4("<<args[1]<<"))"; 	break;
						case vinfo::_SAMP_3D :		src << "read_image"<<fmt<<"("<<args[0]<<", "<<args[1]<<")"; 				break;
						case vinfo::_SAMP_BUF :		src << "read_image"<<fmt<<"("<<args[0]<<", "<<args[1]<<")"; 				break;
						default :					RETURN_ERR( "not supported!" );
					}
					break;
				}
				case glslang::TOperator::EOpAtomicAdd :			src << "atomic_add(&("<<args[0]<<"), "<<args[1]<<")";	break;
				case glslang::TOperator::EOpAtomicMin :			src << "atomic_min(&("<<args[0]<<"), "<<args[1]<<")";	break;
				case glslang::TOperator::EOpAtomicMax :			src << "atomic_max(&("<<args[0]<<"), "<<args[1]<<")";	break;
				case glslang::TOperator::EOpAtomicAnd :			src << "atomic_and(&("<<args[0]<<"), "<<args[1]<<")";	break;
				case glslang::TOperator::EOpAtomicOr :			src << "atomic_or(&("<<args[0]<<"), "<<args[1]<<")";	break;
				case glslang::TOperator::EOpAtomicXor :			src << "atomic_xor(&("<<args[0]<<"), "<<args[1]<<")";	break;
				case glslang::TOperator::EOpAtomicExchange :	src << "atomic_xchg(&("<<args[0]<<"), "<<args[1]<<")";	break;

				//case glslang::TOperator::EOpAtomicCounterAdd :		src << "atomicCounterAdd" << all_args;			break;
				//case glslang::TOperator::EOpAtomicCounterSubtract :	src << "atomicCounterSub" << all_args;			break;
				//case glslang::TOperator::EOpAtomicCounterMin :		src << "atomicCounterMin" << all_args;			break;
				//case glslang::TOperator::EOpAtomicCounterMax :		src << "atomicCounterMax" << all_args;			break;
				//case glslang::TOperator::EOpAtomicCounterAnd :		src << "atomicCounterAnd" << all_args;			break;
				//case glslang::TOperator::EOpAtomicCounterOr :			src << "atomicCounterOr" << all_args;			break;
				//case glslang::TOperator::EOpAtomicCounterXor :		src << "atomicCounterXor" << all_args;			break;
				//case glslang::TOperator::EOpAtomicCounterExchange :	src << "atomicCounterExchange" << all_args;		break;

				//case glslang::TOperator::EOpTextureQuerySize :		src << "textureSize" << all_args;				break;
				//case glslang::TOperator::EOpTextureQueryLod :			src << "textureQueryLod" << all_args;			break;
				//case glslang::TOperator::EOpTexture :					src << "texture" << all_args;					break;
				//case glslang::TOperator::EOpTextureProj :				src << "textureProj" << all_args;				break;
				//case glslang::TOperator::EOpTextureFetch :			src << "texelFetch" << all_args;				break;
				//case glslang::TOperator::EOpTextureGather :			src << "textureGather" << all_args;				break;
				default :												RETURN_ERR( "unknown operator!" );
			}
			return true;
		}

		switch ( op )
		{
			case glslang::TOperator::EOpImageLoad : {				// 3 args
				switch ( argTypes[0]->type & vinfo::_SAMP_MASK )
				{
					case vinfo::_SAMP_2DMS :	src << "read_imagef("<<args[0]<<", "<<args[1]<<", "<<args[2]<<")"; 					break;
					case vinfo::_SAMP_2DAMS :	src << "read_imagef("<<args[0]<<", convert_int4("<<args[1]<<", "<<args[2]<<"))"; 	break;
					default :					RETURN_ERR( "not supported!" );
				}
				break;
			}

			case glslang::TOperator::EOpImageStore : {				// 3,4 args
				String	fmt = "f";
				switch ( argTypes[0]->type & vinfo::_TYPE_FLAG_MASK )
				{
					case vinfo::_HALF :			fmt = "h";	break;
					case vinfo::_INT :			fmt = "ui";	break;
					case vinfo::_UINT :			fmt = "i";	break;
				}
				switch ( argTypes[0]->type & vinfo::_SAMP_MASK )
				{
					case vinfo::_SAMP_1D :		src << "write_image"<<fmt<<"("<<args[0]<<", "<<args[1]<<", "<<args[2]<<")"; 				break;
					case vinfo::_SAMP_1DA :		src << "write_image"<<fmt<<"("<<args[0]<<", "<<args[1]<<", "<<args[2]<<")"; 				break;
					case vinfo::_SAMP_2D :		src << "write_image"<<fmt<<"("<<args[0]<<", "<<args[1]<<", "<<args[2]<<")"; 				break;
					case vinfo::_SAMP_2DA :		src << "write_image"<<fmt<<"("<<args[0]<<", convert_int4("<<args[1]<<"), "<<args[2]<<")"; 	break;
					case vinfo::_SAMP_3D :		src << "write_image"<<fmt<<"("<<args[0]<<", "<<args[1]<<", "<<args[2]<<")"; 				break;
					case vinfo::_SAMP_BUF :		src << "write_image"<<fmt<<"("<<args[0]<<", "<<args[1]<<", "<<args[2]<<")"; 				break;
					case vinfo::_SAMP_2DMS :	src << "write_image"<<fmt<<"("<<args[0]<<", "<<args[1]<<", "<<args[2]<<", "<<args[3]<<")"; 					break;
					case vinfo::_SAMP_2DAMS :	src << "write_image"<<fmt<<"("<<args[0]<<", convert_int4("<<args[1]<<"), "<<args[2]<<", "<<args[3]<<")"; 	break;
					default :					RETURN_ERR( "not supported!" );
				}
				break;
			}
			#ifdef AMD_EXTENSIONS
			//case glslang::TOperator::EOpImageLoadLod :			src << "imageLoadLod" << all_args;			break;
			//case glslang::TOperator::EOpImageStoreLod :			src << "imageStoreLod" << all_args;			break;
			#endif
			//case glslang::TOperator::EOpImageAtomicAdd :			src << "imageAtomicAdd" << all_args;		break;	// 3,4 args
			//case glslang::TOperator::EOpImageAtomicMin :			src << "imageAtomicMin" << all_args;		break;	// 3,4 args
			//case glslang::TOperator::EOpImageAtomicMax :			src << "imageAtomicMax" << all_args;		break;	// 3,4 args
			//case glslang::TOperator::EOpImageAtomicAnd :			src << "imageAtomicAnd" << all_args;		break;	// 3,4 args
			//case glslang::TOperator::EOpImageAtomicOr :			src << "imageAtomicOr" << all_args;			break;	// 3,4 args
			//case glslang::TOperator::EOpImageAtomicXor :			src << "imageAtomicXor" << all_args;		break;	// 3,4 args
			//case glslang::TOperator::EOpImageAtomicExchange :		src << "imageAtomicExchange" << all_args;	break;	// 3,4 args
			//case glslang::TOperator::EOpImageAtomicCompSwap :		src << "imageAtomicCompSwap" << all_args;	break;	// 4,5 args
			//case glslang::TOperator::EOpSubpassLoad :				src << "image(";	break;
			//case glslang::TOperator::EOpSubpassLoadMS :			src << "image(";	break;
			//case glslang::TOperator::EOpSparseImageLoad :			src << "image(";	break;
			#ifdef AMD_EXTENSIONS
			//case glslang::TOperator::EOpSparseImageLoadLod :		src << "image(";	break;
			#endif
			case glslang::TOperator::EOpClamp :						src << "clamp" << all_args;					break;	// 3 args
			case glslang::TOperator::EOpMix :						src << "mix" << all_args;					break;	// 3 args
			case glslang::TOperator::EOpSmoothStep :				src << "smoothstep" << all_args;			break;	// 3 args
			case glslang::TOperator::EOpFma :						src << "fma" << all_args;					break;	// 3 args
			#ifdef AMD_EXTENSIONS
			//case glslang::TOperator::EOpFloat16BitsToInt16,
			//case glslang::TOperator::EOpFloat16BitsToUint16,
			//case glslang::TOperator::EOpInt16BitsToFloat16,
			//case glslang::TOperator::EOpUint16BitsToFloat16,
			#endif
			#ifdef AMD_EXTENSIONS
			//case glslang::TOperator::EOpPackFloat2x16,
			//case glslang::TOperator::EOpUnpackFloat2x16,
			//case glslang::TOperator::EOpPackInt2x16,
			//case glslang::TOperator::EOpUnpackInt2x16,
			//case glslang::TOperator::EOpPackUint2x16,
			//case glslang::TOperator::EOpUnpackUint2x16,
			//case glslang::TOperator::EOpPackInt4x16,
			//case glslang::TOperator::EOpUnpackInt4x16,
			//case glslang::TOperator::EOpPackUint4x16,
			//case glslang::TOperator::EOpUnpackUint4x16,
			#endif
			//case glslang::TOperator::EOpFaceForward :				src << "faceForward" << all_args;		break;	// 3 args
			//case glslang::TOperator::EOpRefract :					src << "refract" << all_args;			break;	// 3 args
			#ifdef AMD_EXTENSIONS
			//case glslang::TOperator::EOpMin3,
			//case glslang::TOperator::EOpMax3,
			//case glslang::TOperator::EOpMid3,
			#endif
			#ifdef AMD_EXTENSIONS
			//case glslang::TOperator::EOpInterpolateAtVertex,
			#endif
			//case glslang::TOperator::EOpFtransform :				src << "ftransform" << all_args;			break;	// WTF?
			//case glslang::TOperator::EOpBallot :					src << "ballotARB" << all_args;				break;	// WTF?
			//case glslang::TOperator::EOpReadInvocation :			src << "readInvocation" << all_args;		break;	// WTF?
			//case glslang::TOperator::EOpReadFirstInvocation :		src << "readFirstInvocatin" << all_args;	break;	// WTF?
			//case glslang::TOperator::EOpAnyInvocation :			src << "anyInvocation" << all_args;			break;	// WTF?
			//case glslang::TOperator::EOpAllInvocations :			src << "allInvocations" << all_args;		break;	// WTF?
			//case glslang::TOperator::EOpAllInvocationsEqual :		src << "allInvocationsEqual" << all_args;	break;	// WTF?
			#ifdef AMD_EXTENSIONS
			//case glslang::TOperator::EOpMinInvocations,
			//case glslang::TOperator::EOpMaxInvocations,
			//case glslang::TOperator::EOpAddInvocations,
			//case glslang::TOperator::EOpMinInvocationsNonUniform,
			//case glslang::TOperator::EOpMaxInvocationsNonUniform,
			//case glslang::TOperator::EOpAddInvocationsNonUniform,
			//case glslang::TOperator::EOpMinInvocationsInclusiveScan,
			//case glslang::TOperator::EOpMaxInvocationsInclusiveScan,
			//case glslang::TOperator::EOpAddInvocationsInclusiveScan,
			//case glslang::TOperator::EOpMinInvocationsInclusiveScanNonUniform,
			//case glslang::TOperator::EOpMaxInvocationsInclusiveScanNonUniform,
			//case glslang::TOperator::EOpAddInvocationsInclusiveScanNonUniform,
			//case glslang::TOperator::EOpMinInvocationsExclusiveScan,
			//case glslang::TOperator::EOpMaxInvocationsExclusiveScan,
			//case glslang::TOperator::EOpAddInvocationsExclusiveScan,
			//case glslang::TOperator::EOpMinInvocationsExclusiveScanNonUniform,
			//case glslang::TOperator::EOpMaxInvocationsExclusiveScanNonUniform,
			//case glslang::TOperator::EOpAddInvocationsExclusiveScanNonUniform,
			//case glslang::TOperator::EOpSwizzleInvocations,
			//case glslang::TOperator::EOpSwizzleInvocationsMasked,
			//case glslang::TOperator::EOpWriteInvocation,
			//case glslang::TOperator::EOpMbcnt,
			//case glslang::TOperator::EOpCubeFaceIndex,
			//case glslang::TOperator::EOpCubeFaceCoord,
			//case glslang::TOperator::EOpTime,
			#endif
			case glslang::TOperator::EOpAtomicCompSwap :			src << "atomic_cmpxchg(&("<<args[0]<<"), "<<args[1]<<", "<<args[2]<<")";	break;	// 3 args

			//case glslang::TOperator::EOpAtomicCounterCompSwap :	src << "atomicCounterCompSwap" << all_args;		break;	// 3 args

			//case glslang::TOperator::EOpAddCarry :				src << "uaddCarry" << all_args;					break;	// 3 args
			//case glslang::TOperator::EOpSubBorrow :				src << "usubBorrow" << all_args;				break;	// 3 args
			//case glslang::TOperator::EOpUMulExtended :			src << "umulExtended" << all_args;				break;	// 4 args
			//case glslang::TOperator::EOpIMulExtended :			src << "imulExtended" << all_args;				break;	// 4 args
			//case glslang::TOperator::EOpBitfieldExtract :			src << "bitfieldExtract" << all_args;			break;	// 3 args
			//case glslang::TOperator::EOpBitfieldInsert :			src << "bitfieldInsert" << all_args;			break;	// 4 args

			//case glslang::TOperator::EOpTexture :					src << "texture" << all_args;					break;	// 3 args
			//case glslang::TOperator::EOpTextureProj :				src << "textureProj" << all_args;				break;	// 3 args
			//case glslang::TOperator::EOpTextureLod :				src << "textureLod" << all_args;				break;	// 3 args
			//case glslang::TOperator::EOpTextureOffset :			src << "textureOffset" << all_args;				break;	// 3,4 args
			//case glslang::TOperator::EOpTextureFetch :			src << "texelFetch" << all_args;				break;	// 3 args
			//case glslang::TOperator::EOpTextureFetchOffset :		src << "texelFetchOffset" << all_args;			break;	// 3,4 args
			//case glslang::TOperator::EOpTextureProjOffset :		src << "textureProjOffset" << all_args;			break;	// 3,4 args
			//case glslang::TOperator::EOpTextureLodOffset :		src << "textureLodOffset" << all_args;			break;	// 4 args
			//case glslang::TOperator::EOpTextureProjLod :			src << "textureProjLod" << all_args;			break;	// 3 args
			//case glslang::TOperator::EOpTextureProjLodOffset :	src << "textureProjLodOffset" << all_args;		break;	// 4 args
			//case glslang::TOperator::EOpTextureGrad :				src << "textureGrad" << all_args;				break;	// 4 args
			//case glslang::TOperator::EOpTextureGradOffset :		src << "textureGradOffset" << all_args;			break;	// 5 args
			//case glslang::TOperator::EOpTextureProjGrad :			src << "textureProjGrad" << all_args;			break;	// 4 args
			//case glslang::TOperator::EOpTextureProjGradOffset :	src << "textureProjGradOffset" << all_args;		break;	// 5 args
			//case glslang::TOperator::EOpTextureGather :			src << "textureGather" << all_args;				break;	// 3 args
			//case glslang::TOperator::EOpTextureGatherOffset :		src << "textureGatherOffset" << all_args;		break;	// 3,4 args
			//case glslang::TOperator::EOpTextureGatherOffsets :	src << "textureGatherOffsets" << all_args;		break;	// 3,4 args
			//case glslang::TOperator::EOpTextureClamp :			break;
			//case glslang::TOperator::EOpTextureOffsetClamp :		break;
			//case glslang::TOperator::EOpTextureGradClamp :		break;
			//case glslang::TOperator::EOpTextureGradOffsetClamp :	break;
			#ifdef AMD_EXTENSIONS
			//case glslang::TOperator::EOpTextureGatherLod :			break;
			//case glslang::TOperator::EOpTextureGatherLodOffset :		break;
			//case glslang::TOperator::EOpTextureGatherLodOffsets :		break;
			#endif
			//case glslang::TOperator::EOpSparseTexture :				break;
			//case glslang::TOperator::EOpSparseTextureLod :			break;
			//case glslang::TOperator::EOpSparseTextureOffset :			break;
			//case glslang::TOperator::EOpSparseTextureFetch :			break;
			//case glslang::TOperator::EOpSparseTextureFetchOffset :	break;
			//case glslang::TOperator::EOpSparseTextureLodOffset :		break;
			//case glslang::TOperator::EOpSparseTextureGrad :			break;
			//case glslang::TOperator::EOpSparseTextureGradOffset :		break;
			//case glslang::TOperator::EOpSparseTextureGather :			break;
			//case glslang::TOperator::EOpSparseTextureGatherOffset :	break;
			//case glslang::TOperator::EOpSparseTextureGatherOffsets :	break;
			//case glslang::TOperator::EOpSparseTexelsResident :		break;
			//case glslang::TOperator::EOpSparseTextureClamp :			break;
			//case glslang::TOperator::EOpSparseTextureOffsetClamp :	break;
			//case glslang::TOperator::EOpSparseTextureGradClamp :		break;
			//case glslang::TOperator::EOpSparseTextureGradOffsetClamp :	break;
			#ifdef AMD_EXTENSIONS
			//case glslang::TOperator::EOpSparseTextureGatherLod :			break;
			//case glslang::TOperator::EOpSparseTextureGatherLodOffset :	break;
			//case glslang::TOperator::EOpSparseTextureGatherLodOffsets :	break;
			#endif

			default :												RETURN_ERR( "unknown builtin function!" );
		}
		return true;
	}
	
/*
=================================================
	TranslateFunctionCall
=================================================
*/
	bool CL_DstLanguage::TranslateFunctionCall (StringCRef signature, const TypeInfo &, ArrayCRef<String> args, ArrayCRef<TypeInfo const*> argTypes, INOUT String &src)
	{
		FuncSignMap_t::iterator	iter;
		CHECK_ERR( _definedFuncs.Find( signature, OUT iter ) );

		src << iter->second.name << '(';

		FOR( i, args )
		{
			src << (i ? ", " : "");

			if ( (EShaderVariable::IsStruct( argTypes[i]->type ) or iter->second.ptrArgs[i]) and
				 not (argTypes[i]->qualifier[ EVariableQualifier::InArg ] or argTypes[i]->qualifier[ EVariableQualifier::OutArg ]) )
			{
				src << '&';
			}

			src << args[i];
		}

		src << ')';
		return true;
	}

/*
=================================================
	_TranslateArg
=================================================
*/
	bool CL_DstLanguage::_TranslateArg (const TypeInfo &t, INOUT String &res)
	{
		// qualifies
		if ( t.qualifier[ EVariableQualifier::InArg ] and t.qualifier[ EVariableQualifier::OutArg ] )
			res << "INOUT ";
		else
		if ( t.qualifier[ EVariableQualifier::OutArg ] )
			res << "OUT ";
		else
		if ( t.qualifier[ EVariableQualifier::Constant ] )
			res << "const ";
		else
		if ( t.qualifier[ EVariableQualifier::InArg ] and EShaderVariable::IsStruct( t.type ) )
			res << "const ";

		// image format
		if ( t.format != EPixelFormat::Unknown )
			res << "FORMAT(" << ToStringCL( t.format ) << ") ";

		// type
		if ( not t.typeName.Empty() ) {
			res << "struct " << t.typeName;
		} else {
			res << ToStringCL( t.type );
		}
		res << " ";
		
		if ( t.qualifier[ EVariableQualifier::OutArg ] or EShaderVariable::IsStruct( t.type ) )
			res << "*";

		if ( t.arraySize == 0 )		res << t.name;				else
		if ( t.arraySize == UMax )	res << "* " << t.name;		else
									res << t.name << " [" << t.arraySize << "]";
		return true;
	}
	
/*
=================================================
	TranslateSwizzle
=================================================
*/
	bool CL_DstLanguage::TranslateFunctionDecl (StringCRef signature, const TypeInfo &resultType, ArrayCRef<TypeInfo> args, INOUT String &src)
	{
		CHECK_ERR( TranslateType( resultType, INOUT src ) );

		src << ' ' << _TranslateFunctionName( signature ) << '(';
		
		FuncSignMap_t::iterator	iter;
		CHECK_ERR( _definedFuncs.Find( signature, OUT iter ) );

		FOR( i, args )
		{
			src << (i ? ", " : "");

			CHECK_ERR( _TranslateArg( args[i], OUT src ) );

			iter->second.ptrArgs.SetAt( i, args[i].qualifier[ EVariableQualifier::OutArg ] );
		}

		src << ')';
		return true;
	}

/*
=================================================
	TranslateSwizzle
=================================================
*/
	bool CL_DstLanguage::TranslateSwizzle (const TypeInfo &, StringCRef val, StringCRef swizzle, INOUT String &src)
	{
		src << val << '.' << swizzle;
		return true;
	}
	
/*
=================================================
	TranslateEntry
=================================================
*/
	bool CL_DstLanguage::TranslateEntry (const TypeInfo &ret, StringCRef signature, ArrayCRef<TypeInfo> args, INOUT String &src)
	{
		CHECK_ERR( args.Empty() );
		CHECK_ERR( ret.type == EShaderVariable::Void );

		src.Clear();
		src << "kernel void " << _TranslateFunctionName( signature ) << " (";

		Sort( _externals, LAMBDA() (auto& left, auto& right) { return left.binding > right.binding; });

		bool	has_shared = false;

		FOR( i, _externals )
		{
			auto const&	obj = _externals[i];
			String		binding;

			binding << (i ? "," : "")
					<< "\n\t/*" << obj.binding << "*/";

			if ( EShaderVariable::IsBuffer( obj.type ) ) {
				src << binding;
				CHECK_ERR( _TranslateBuffer( obj, INOUT src ) );
			}
			else
			if ( EShaderVariable::IsImage( obj.type ) or EShaderVariable::IsTexture( obj.type ) ) {
				src << binding;
				CHECK_ERR( _TranslateImage( obj, INOUT src ) );
			}
			else
			if ( obj.qualifier[ EVariableQualifier::Shared ] ) {
				has_shared = true;
				continue;
			}
			else {
				RETURN_ERR( "unknown external object!" );
			}
		}

		src << ")\n{\n";
		
		if ( has_shared )
		{
			FOR( i, _externals )
			{
				auto const&	obj = _externals[i];

				if ( obj.qualifier[ EVariableQualifier::Shared ] )
				{
					CHECK_ERR( _TranslateShared( obj, INOUT src ) );
				}
			}
		}
		return true;
	}
	
/*
=================================================
	TranslateStructAccess
=================================================
*/
	bool CL_DstLanguage::TranslateStructAccess (const TypeInfo &stType, StringCRef objName, const TypeInfo &fieldType, INOUT String &src)
	{
		if ( not objName.Empty() )
		{
			if ( EShaderVariable::IsBuffer( stType.type ) or stType.qualifier[ EVariableQualifier::OutArg ] or stType.qualifier[ EVariableQualifier::InArg ] )
				src << objName << "->";
			else
				src << objName << '.';
		}

		src << fieldType.name;
		return true;
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	_TranslateBuffer
=================================================
*/
	bool CL_DstLanguage::_TranslateBuffer (Translator::TypeInfo const& info, OUT String &str)
	{
		CHECK_ERR( EShaderVariable::IsBuffer( info.type ) );
		CHECK_ERR( info.arraySize == 0 );

		str << "__global " << ToStringCL( info.memoryModel )
			<< " struct " << (info.typeName.Empty() ? ToStringCL( info.type ) : info.typeName)
			<< "* " << info.name;

		return true;
	}

/*
=================================================
	_TranslateImage
=================================================
*/
	bool CL_DstLanguage::_TranslateImage (Translator::TypeInfo const& info, OUT String &str)
	{
		CHECK_ERR( info.arraySize == 0 );

		str << ToStringCL( info.memoryModel ) << " FORMAT(" << ToStringCL( info.format ) << ") "
			<< ToStringCL( info.type ) << " " << info.name;

		return true;
	}

/*
=================================================
	_TranslateShared
=================================================
*/
	bool CL_DstLanguage::_TranslateShared (Translator::TypeInfo const& info, OUT String &str)
	{
		str << "__local ";
		CHECK_ERR( TranslateLocalVar( info, INOUT str ) );
		str << ";\n";
		return true;
	}
	
/*
=================================================
	_RecursiveInitConstStruct
=================================================
*/
	bool CL_DstLanguage::_RecursiveInitConstStruct (const Array<Translator::TypeInfo> &fields, const glslang::TConstUnionArray& cu_arr, INOUT int &index, OUT String &src)
	{
		DeserializedShader::Constant::ValueArray_t	values;

		src << "( ";

		FOR( i, fields )
		{
			const auto&	field	= fields[i];
			const uint	count	= field.arraySize == 0 ? 1 : field.arraySize;
			const bool	is_arr	= field.arraySize != 0;

			CHECK_ERR( field.arraySize != UMax );
			
			src << (i ? ", " : "");

			if ( is_arr )
				src << "{ ";

			if ( EShaderVariable::IsStruct( field.type ) )
			{
				for (uint j = 0; j < count; ++j) {
					CHECK_ERR( _RecursiveInitConstStruct( field.fields, cu_arr, INOUT index, INOUT src ) );
				}
			}
			else
			{
				for (uint j = 0; j < count; ++j)
				{
					values.Clear();
					CHECK_ERR( DeserializeConstant::Process( field.type, cu_arr, index, true, OUT values, OUT index ) );

					CU_ToArray_Func	func{ this };
					
					src << (j ? ", " : "");
					values.Front().Apply( func );

					CHECK_ERR( TranslateOperator( glslang::TOperator::EOpConstructGuardStart,
												  field, func.GetStrings(), func.GetTypes(), INOUT src ) );
				}
			}
			
			if ( is_arr )
				src << " }";
		}
		src << " )";
		return true;
	}
	
/*
=================================================
	TranslateConstant
=================================================
*/
	bool CL_DstLanguage::TranslateConstant (const glslang::TConstUnionArray &cu_arr, const TypeInfo &info, INOUT String &str)
	{
		// array
		if ( info.arraySize > 0 )
		{
			Translator::TypeInfo	scalar_info = info;		scalar_info.arraySize = 0;

			str << "{ ";
			
			DeserializedShader::Constant::ValueArray_t	values;
			CHECK_ERR( DeserializeConstant::Process( scalar_info.type, cu_arr, OUT values ) );

			FOR( i, values )
			{
				CU_ToArray_Func	func{ this };

				str << (i ? ", " : "");
				values[i].Apply( func );

				CHECK_ERR( TranslateOperator( glslang::TOperator::EOpConstructGuardStart,
												scalar_info, func.GetStrings(), func.GetTypes(), INOUT str ) );
			}

			str << " }";
		}
		else
		// struct
		if ( EShaderVariable::IsStruct( info.type ) )
		{
			str << info.typeName;

			int index = 0;
			CHECK_ERR( _RecursiveInitConstStruct( info.fields, cu_arr, INOUT index, INOUT str ) );
		}
		else
		// scalar
		{
			DeserializedShader::Constant::ValueArray_t	values;
			CHECK_ERR( DeserializeConstant::Process( info.type, cu_arr, OUT values ) );

			FOR( i, values )
			{
				CU_ToArray_Func	func{ this };

				str << (i ? ", " : "");
				values[i].Apply( func );
				
				CHECK_ERR( TranslateOperator( glslang::TOperator::EOpConstructGuardStart,
												info, func.GetStrings(), func.GetTypes(), INOUT str ) );
			}
		}
		return true;
	}

/*
=================================================
	_TranslateConst
=================================================
*/
	bool CL_DstLanguage::_TranslateConst (glslang::TIntermTyped* typed, Translator::TypeInfo const& info, OUT String &str)
	{
		CHECK_ERR( typed and typed->getAsSymbolNode() );

		glslang::TConstUnionArray const&	cu_arr	= typed->getAsSymbolNode()->getConstArray();
		
		
		// read-only
		if ( info.qualifier[ EVariableQualifier::Constant ] )
			str << "__constant ";

		// image format
		if ( info.format != EPixelFormat::Unknown )
			str << "FORMAT(" << ToStringCL( info.format ) << ") ";

		// type
		if ( not info.typeName.Empty() ) {
			str << "struct " << info.typeName;
		} else {
			str << ToStringCL( info.type );
		}
		str << " ";
		
		if ( info.arraySize == 0 )		str << info.name;				else
		if ( info.arraySize == UMax )	str << "* " << info.name;		else
										str << info.name << " [" << info.arraySize << "]";
		
		str << " = ";
		
		CHECK_ERR( TranslateConstant( cu_arr, info, INOUT str ) );
			
		str << ";\n";
		return true;
	}
	
/*
=================================================
	TranslateValue
=================================================
*/
	bool CL_DstLanguage::TranslateValue (VariantCRef value, INOUT String &src) const
	{
		src.Clear();

		// bool
		if ( value.GetValueTypeId() == TypeIdOf<bool>() )
		{
			src << ToString( value.Get<bool>() );
		}
		else
		// int
		if ( value.GetValueTypeId() == TypeIdOf<int>() )
		{
			const int	val = value.Get<int>();

			if ( val != MinValue(val) )
				src << ToString( val );
			else
				src << "(" << (MinValue(val)+1) << " - 1)";
		}
		else
		// long
		if ( value.GetValueTypeId() == TypeIdOf<ilong>() )
		{
			const ilong	val = value.Get<ilong>();

			if ( val != MinValue(val) )
				src << ToString( val ) << "L";
			else
				src << "(" << (MinValue(val)+1) << "L - 1)";
		}
		else
		// uint
		if ( value.GetValueTypeId() == TypeIdOf<uint>() )
		{
			src << ToString( value.Get<uint>() ) << "U";
		}
		else
		// ulong
		if ( value.GetValueTypeId() == TypeIdOf<ulong>() )
		{
			src << ToString( value.Get<ulong>() ) << "UL";
		}
		else
		// float
		if ( value.GetValueTypeId() == TypeIdOf<float>() )
		{
			src << String().FormatF( value.Get<float>(), StringFormatF().Fmt(0,8).CutZeros() ) << "f";
		}
		else
		// double
		if ( value.GetValueTypeId() == TypeIdOf<double>() )
		{
			src << String().FormatF( value.Get<double>(), StringFormatF().Fmt(0,16).CutZeros() );
		}
		else
			RETURN_ERR( "unsupported scalar type" );

		return true;
	}

/*
=================================================
	ToStringCL (EShaderMemoryModel)
=================================================
*/
	String CL_DstLanguage::ToStringCL (EShaderMemoryModel::type value)
	{
		if ( value == EShaderMemoryModel::Default )
			value = EShaderMemoryModel::Coherent;

		bool	read_access		= EShaderMemoryModel::HasReadAccess( value );
		bool	write_access	= EShaderMemoryModel::HasWriteAccess( value );

		if ( read_access and write_access )
			return "read_write";

		if ( write_access )
			return "write_only";
		
		return "read_only";
	}
	
/*
=================================================
	ToStringCL (EPixelFormat)
=================================================
*/
	String CL_DstLanguage::ToStringCL (EPixelFormat::type value)
	{
		switch ( value )
		{
			case EPixelFormat::RGBA32F			: return "rgba32f";
			case EPixelFormat::RGBA16F			: return "rgba16f";
			case EPixelFormat::RG32F			: return "rg32f";
			case EPixelFormat::RG16F			: return "rg16f";
			case EPixelFormat::RGB_11_11_10F	: return "r11f_g11f_b10f";
			case EPixelFormat::R32F				: return "r32f";
			case EPixelFormat::R16F				: return "r16f";
			case EPixelFormat::RGBA32U			: return "rgba32ui";
			case EPixelFormat::RGBA16U			: return "rgba16ui";
			case EPixelFormat::RGB10_A2U		: return "rgb10_a2ui";
			case EPixelFormat::RGBA8U			: return "rgba8ui";
			case EPixelFormat::RG32U			: return "rg32ui";
			case EPixelFormat::RG16U			: return "rg16ui";
			case EPixelFormat::RG8U				: return "rg8ui";
			case EPixelFormat::R32U				: return "r32ui";
			case EPixelFormat::R16U				: return "r16ui";
			case EPixelFormat::R8U				: return "r8ui";
			case EPixelFormat::RGBA32I			: return "rgba32i";
			case EPixelFormat::RGBA16I			: return "rgba16i";
			case EPixelFormat::RGBA8I			: return "rgba8i";
			case EPixelFormat::RG32I			: return "rg32i";
			case EPixelFormat::RG16I			: return "rg16i";
			case EPixelFormat::RG8I				: return "rg8i";
			case EPixelFormat::R32I				: return "r32i";
			case EPixelFormat::R16I				: return "r16i";
			case EPixelFormat::R8I				: return "r8i";
			case EPixelFormat::RGBA16_UNorm		: return "rgba16_unorm";
			case EPixelFormat::RGB10_A2_UNorm	: return "rgb10_a2_unorm";
			case EPixelFormat::RGBA8_UNorm		: return "rgba8_unorm";
			case EPixelFormat::RG16_UNorm		: return "rg16_unorm";
			case EPixelFormat::RG8_UNorm		: return "rg8_unorm";
			case EPixelFormat::R16_UNorm		: return "r16_unorm";
			case EPixelFormat::R8_UNorm			: return "r8_unorm";
			case EPixelFormat::RGBA16_SNorm		: return "rgba16_snorm";
			case EPixelFormat::RGBA8_SNorm		: return "rgba8_snorm";
			case EPixelFormat::RG16_SNorm		: return "rg16_snorm";
			case EPixelFormat::RG8_SNorm		: return "rg8_snorm";
			case EPixelFormat::R16_SNorm		: return "r16_snorm";
			case EPixelFormat::R8_SNorm			: return "r8_snorm";
		}
		RETURN_ERR( "unsupported format" );
	}
	
/*
=================================================
	ToStringCL (EShaderVariable)
=================================================
*/
	String CL_DstLanguage::ToStringCL (EShaderVariable::type value)
	{
		switch ( value )
		{
			case EShaderVariable::Void :		return "void";
			case EShaderVariable::Bool :		return "int";
			case EShaderVariable::Bool2 :		return "int2";
			case EShaderVariable::Bool3 :		return "int3";
			case EShaderVariable::Bool4 :		return "int4";
			case EShaderVariable::Int :			return "int";
			case EShaderVariable::Int2 :		return "int2";
			case EShaderVariable::Int3 :		return "int3";
			case EShaderVariable::Int4 :		return "int4";
			case EShaderVariable::UInt :		return "uint";
			case EShaderVariable::UInt2 :		return "uint2";
			case EShaderVariable::UInt3 :		return "uint3";
			case EShaderVariable::UInt4 :		return "uint4";
			case EShaderVariable::Long :		return "long";
			case EShaderVariable::Long2 :		return "long2";
			case EShaderVariable::Long3 :		return "long3";
			case EShaderVariable::Long4 :		return "long4";
			case EShaderVariable::ULong :		return "ulong";
			case EShaderVariable::ULong2 :		return "ulong2";
			case EShaderVariable::ULong3 :		return "ulong3";
			case EShaderVariable::ULong4 :		return "ulong4";
			case EShaderVariable::Float :		return "float";
			case EShaderVariable::Float2 :		return "float2";
			case EShaderVariable::Float3 :		return "float3";
			case EShaderVariable::Float4 :		return "float4";
			case EShaderVariable::Double :		return "double";
			case EShaderVariable::Double2 :		return "double2";
			case EShaderVariable::Double3 :		return "double3";
			case EShaderVariable::Double4 :		return "double4";

			// software matrix types
			case EShaderVariable::Float2x2 :	return "__float2x2";
			case EShaderVariable::Float2x3 :	return "__float2x3";
			case EShaderVariable::Float2x4 :	return "__float2x4";
			case EShaderVariable::Float3x2 :	return "__float3x2";
			case EShaderVariable::Float3x3 :	return "__float3x3";
			case EShaderVariable::Float3x4 :	return "__float3x4";
			case EShaderVariable::Float4x2 :	return "__float4x2";
			case EShaderVariable::Float4x3 :	return "__float4x3";
			case EShaderVariable::Float4x4 :	return "__float4x4";
			case EShaderVariable::Double2x2 :	return "__double2x2";
			case EShaderVariable::Double2x3 :	return "__double2x3";
			case EShaderVariable::Double2x4 :	return "__double2x4";
			case EShaderVariable::Double3x2 :	return "__double3x2";
			case EShaderVariable::Double3x3 :	return "__double3x3";
			case EShaderVariable::Double3x4 :	return "__double3x4";
			case EShaderVariable::Double4x2 :	return "__double4x2";
			case EShaderVariable::Double4x3 :	return "__double4x3";
			case EShaderVariable::Double4x4 :	return "__double4x4";
			/*
			case EShaderVariable::FloatSampler1D :				return "sampler1D";
			case EShaderVariable::FloatSampler1DShadow :		return "sampler1DShadow";
			case EShaderVariable::FloatSampler1DArray :			return "sampler1DArray";
			case EShaderVariable::FloatSampler1DArrayShadow :	return "sampler1DArrayShadow";
			case EShaderVariable::FloatSampler2D :				return "sampler2D";
			case EShaderVariable::FloatSampler2DShadow :		return "sampler2DShadow";
			case EShaderVariable::FloatSampler2DMS :			return "sampler2DMS";
			case EShaderVariable::FloatSampler2DArray :			return "sampler2DArray";
			case EShaderVariable::FloatSampler2DArrayShadow :	return "sampler2DArrayShadow";
			case EShaderVariable::FloatSampler2DMSArray :		return "sampler2DMSArray";
			case EShaderVariable::FloatSamplerCube :			return "samplerCube";
			case EShaderVariable::FloatSamplerCubeShadow :		return "samplerCubeShadow";
			case EShaderVariable::FloatSamplerCubeArray :		return "samplerCubeArray";
			case EShaderVariable::FloatSampler3D :				return "sampler3D";
			case EShaderVariable::FloatSamplerBuffer :			return "samplerBuffer";
			case EShaderVariable::IntSampler1D :				return "isampler1D";
			case EShaderVariable::IntSampler1DArray :			return "isampler1DArray";
			case EShaderVariable::IntSampler2D :				return "isampler2D";
			case EShaderVariable::IntSampler2DMS :				return "isampler2DMS";
			case EShaderVariable::IntSampler2DArray :			return "isampler2DArray";
			case EShaderVariable::IntSampler2DMSArray :			return "isampler2DMSArray";
			case EShaderVariable::IntSamplerCube :				return "isamplerCube";
			case EShaderVariable::IntSamplerCubeArray :			return "isamplerCubeArray";
			case EShaderVariable::IntSampler3D :				return "isampler3D";
			case EShaderVariable::IntSamplerBuffer :			return "isamplerBuffer";
			case EShaderVariable::UIntSampler1D :				return "uisampler1D";
			case EShaderVariable::UIntSampler1DArray :			return "uisampler1DArray";
			case EShaderVariable::UIntSampler2D :				return "uisampler2D";
			case EShaderVariable::UIntSampler2DMS :				return "uisampler2DMS";
			case EShaderVariable::UIntSampler2DArray :			return "uisampler2DArray";
			case EShaderVariable::UIntSampler2DMSArray :		return "uisampler2DMSArray";
			case EShaderVariable::UIntSamplerCube :				return "uisamplerCube";
			case EShaderVariable::UIntSamplerCubeArray :		return "uisamplerCubeArray";
			case EShaderVariable::UIntSampler3D :				return "uisampler3D";
			case EShaderVariable::UIntSamplerBuffer :			return "uisamplerBuffer";
			*/
			case EShaderVariable::IntImage1D :
			case EShaderVariable::UIntImage1D :
			case EShaderVariable::FloatImage1D :		return "image1d_t";
			case EShaderVariable::IntImage1DArray :
			case EShaderVariable::UIntImage1DArray :
			case EShaderVariable::FloatImage1DArray :	return "image1d_array_t";
			case EShaderVariable::IntImage2D :
			case EShaderVariable::UIntImage2D :
			case EShaderVariable::FloatImage2D :		return "image2d_t";
			case EShaderVariable::IntImage2DMS :
			case EShaderVariable::UIntImage2DMS :
			case EShaderVariable::FloatImage2DMS :		return "image2d_msaa_t";
			case EShaderVariable::IntImage2DArray :
			case EShaderVariable::UIntImage2DArray :
			case EShaderVariable::FloatImage2DArray :	return "image2d_array_t";
			case EShaderVariable::IntImage2DMSArray :
			case EShaderVariable::UIntImage2DMSArray :
			case EShaderVariable::FloatImage2DMSArray :	return "image2d_array_msaa_t";
			//case EShaderVariable::IntImageCube :
			//case EShaderVariable::UIntImageCube :
			//case EShaderVariable::FloatImageCube :		return "imageCube";
			//case EShaderVariable::IntImageCubeArray :
			//case EShaderVariable::UIntImageCubeArray :
			//case EShaderVariable::FloatImageCubeArray :	return "imageCubeArray";
			case EShaderVariable::IntImage3D :
			case EShaderVariable::UIntImage3D :
			case EShaderVariable::FloatImage3D :		return "image3d_t";
			case EShaderVariable::IntImageBuffer :
			case EShaderVariable::UIntImageBuffer :
			case EShaderVariable::FloatImageBuffer :	return "image1d_buffer_t";
		}

		RETURN_ERR( "invalid variable type", "unknown" );
	}

}	// PipelineCompiler
