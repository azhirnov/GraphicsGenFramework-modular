// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "Engine/PipelineCompiler/Common/Common.h"
#include "Engine/PipelineCompiler/Shaders/DeserializedShader.h"

//#define GX_PIPELINECOMPILER_USE_PLATFORMS

#ifdef GX_PIPELINECOMPILER_USE_PLATFORMS
#	include "Engine/Platforms/Engine.Platforms.h"
#endif

class TIntermNode;
struct TBuiltInResource;

namespace glslang
{
	class TType;
	class TProgram;
	class TIntermediate;
	struct TSourceLoc;
}

namespace PipelineCompiler
{

	//
	// Shader Program Compiler
	//

	class ShaderCompiler
	{
	// types
	public:
		static constexpr uint	GLSL_VERSION	= 450;
		static constexpr uint	GLSL_ES_VERSION	= 320;
		static constexpr uint	HLSL_VERSION	= 1100;
		static constexpr uint	VULKAN_VERSION	= 100;	// 100, 110
		static constexpr uint	SPIRV_VERSION	= 100;	// 100, 130

		struct FieldTypeInfo
		{
			String					name;
			String					typeName;
			EShaderVariable::type	type		= EShaderVariable::Unknown;
			uint					index		= 0;		// index of field in struct, update index if 'name' was changed

			ND_ bool operator == (const FieldTypeInfo &right) const;
		};

		using ReplaceStructTypesFunc_t	= Delegate< bool (StringCRef typeName, INOUT FieldTypeInfo &field) >;


		struct Config
		{
			ReplaceStructTypesFunc_t	typeReplacer;
			EShaderSrcFormat::type		source				= EShaderSrcFormat::GXSL;
			EShaderDstFormat::type		target				= EShaderDstFormat::GLSL_Source;
			bool						skipExternals		= false;		// uniforms, buffers, in/out
			bool						optimize			= false;		// SPRIV only
			bool						inlineAll			= false;
		};


	private:
		struct _GLSLangResult;
		struct _ShaderData;
		
		#ifdef GX_PIPELINECOMPILER_USE_PLATFORMS
		class _BaseApp : public StaticRefCountedObject
		{
		private:
			ModulePtr	_glthread;
			ModulePtr	_clthread;
			bool		_looping	= true;
			bool		_clInit		= false;
			bool		_glInit		= false;
			
		private:
			bool _CLInit (const Message< GpuMsg::DeviceCreated > &);
			bool _GLInit (const Message< GpuMsg::DeviceCreated > &);

		public:
			bool IsContextInit () const;

			bool Initialize ();
			void Quit ();
			bool Update ();

			ND_ ModulePtr	CLThread ()		{ return _clthread; }
			ND_ ModulePtr	GLThread ()		{ return _glthread; }
		};

		SharedPointerType< _BaseApp >	_app;
		#endif


	// methods
	public:
		ShaderCompiler ();
		~ShaderCompiler ();

		bool Translate (EShader::type shaderType, ArrayCRef<StringCRef> source, StringCRef entryPoint,
						const Config &cfg, OUT String &log, OUT BinaryArray &result);

		bool Deserialize (EShaderSrcFormat::type shaderFmt, EShader::type shaderType, ArrayCRef<StringCRef> source, StringCRef entryPoint,
						  OUT String &log, OUT DeserializedShader &result);

		bool Validate (EShaderDstFormat::type shaderFmt, EShader::type shaderType, BinArrayCRef data);
		
		bool InitializeContext ();
		void DestroyContext ();

		static Ptr<ShaderCompiler>	Instance ();


	private:
		bool _GLSLangParse (const Config &cfg, const _ShaderData &shader, OUT String &log, OUT _GLSLangResult &result) const;

		bool _Compile (const glslang::TIntermediate* intermediate, const Config &cfg, OUT String &log, OUT BinaryArray &result) const;

		bool _CopySource (const _ShaderData &data, OUT BinaryArray &result) const;

		bool _CheckGLAErrors (OUT String *log = null) const;
		
		bool _OnCompilationFailed (EShader::type shaderType, EShaderSrcFormat::type fmt, ArrayCRef<StringCRef> source, INOUT String &log) const;

		static void _GenerateResources (OUT TBuiltInResource& resources);
		

	// Replace types
	private:
		bool _ReplaceTypes (const _GLSLangResult &data, const Config &cfg) const;


	// GLSL deserializer
	private:
		static bool _ProcessExternalObjects (TIntermNode* root, TIntermNode* node, INOUT DeserializedShader &result);
		static bool _RecursiveProcessAggregateNode (TIntermNode* root, TIntermNode* node, INOUT DeserializedShader &result);
		static bool _RecursiveProcessNode (TIntermNode* root, TIntermNode* node, INOUT DeserializedShader &result);
		
		static bool _DeserializeFunction (TIntermNode* node, INOUT DeserializedShader &result);
		
		static bool _DeserializeAtomicArg (TIntermNode* node, INOUT DeserializedShader &result);

		static bool _DeserializeFunctionArg (TIntermNode* node, glslang::TType const &ttype, glslang::TSourceLoc const &loc,
											 const DeserializedShader::FunctionArgument *parent, OUT DeserializedShader::FunctionArgument &result);

		static bool _DeserializeVariable (TIntermNode* node, glslang::TType const &ttype, glslang::TSourceLoc const &loc,
										  const DeserializedShader::_Variable *parent, OUT DeserializedShader::_Variable &result);

		static bool _DeserializeBufferVariable (TIntermNode* node, glslang::TType const &ttype, glslang::TSourceLoc const &loc,
												const DeserializedShader::BufferVariable *parent, OUT DeserializedShader::BufferVariable &result);

		static bool _DeserializeExternalObjects (TIntermNode* node, INOUT DeserializedShader &result);

		static bool _DeserializeIOVariable (TIntermNode* node, glslang::TType const &ttype, glslang::TSourceLoc const &loc,
											const DeserializedShader::IOVariable *parent, OUT DeserializedShader::IOVariable &result);

		static bool _DeserializeUniform (TIntermNode* node, glslang::TType const &ttype, glslang::TSourceLoc const &loc,
										 const DeserializedShader::Uniform *parent, OUT DeserializedShader::Uniform &result);
		
		static bool _DeserializeConstant (TIntermNode* node, glslang::TType const &ttype, glslang::TSourceLoc const &loc,
										  OUT DeserializedShader::Constant &result);

		static bool _ProcessShaderInfo (glslang::TIntermediate const* intermediate, INOUT DeserializedShader &result);


	// GXSL/GLSL to GLSL translator
	private:
		bool _TranslateGXSLtoGLSL (const Config &cfg, const _GLSLangResult &glslangData, OUT String &log, OUT BinaryArray &result) const;
		bool _CompileGLSL (const Config &cfg, const _ShaderData &shader, OUT String &log, OUT BinaryArray &result);
		bool _ValidateGLSLSource (EShader::type shaderType, StringCRef src) const;
		bool _ValidateGLSLBinary (EShader::type shaderType, BinArrayCRef bin) const;


	// GXSL/GLSL to CPP translator
	private:
		bool _TranslateGXSLtoCPP (const Config &cfg, const _GLSLangResult &glslangData, OUT String &log, OUT BinaryArray &result) const;


	// GLSL to HLSL translator
	private:
		bool _TranslateGXSLtoHLSL (const Config &cfg, const _GLSLangResult &glslangData, OUT String &log, OUT BinaryArray &result) const;
		bool _CompileHLSL (const Config &cfg, const _ShaderData &shader, OUT String &log, OUT BinaryArray &result) const;
		bool _ValidateHLSLSource (EShader::type shaderType, StringCRef src) const;
		bool _ValidateHLSLBinary (EShader::type shaderType, BinArrayCRef bin) const;


	// GXSL/GLSL to OpenCL translator
	private:
		bool _TranslateGXSLtoCL (const Config &cfg, const _GLSLangResult &glslangData, OUT String &log, OUT BinaryArray &result) const;
		bool _CompileCL (const Config &cfg, const _ShaderData &shader, OUT String &log, OUT BinaryArray &result);
		bool _ValidateCLSource (EShader::type shaderType, StringCRef src) const;
		bool _ValidateCLBinary (EShader::type shaderType, BinArrayCRef bin) const;


	// SPIRV
	private:
		bool _CompileSPIRV (const Config &cfg, const _GLSLangResult &glslangData, OUT String &log, OUT BinaryArray &result) const;
		bool _ValidateSPIRV (EShader::type shaderType, BinArrayCRef bin) const;
	};

}	// PipelineCompiler
