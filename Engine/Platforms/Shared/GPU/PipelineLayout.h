// Copyright �  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "Engine/Platforms/Shared/GPU/ShaderEnums.h"
#include "Engine/Platforms/Shared/GPU/ImageEnums.h"

namespace Engine
{
namespace Platforms
{

	//
	// Pipeline Layout Descriptor
	//

	struct PipelineLayoutDescriptor final : CompileTime::PODStruct
	{
	// types
	public:
		struct Builder;

		using Self		= PipelineLayoutDescriptor;
		using Name_t	= StaticString<64>;


		struct BaseUniform : CompileTime::CopyQualifiers< Name_t >
		{
			Name_t					name;
			EShader::bits			stageFlags;
			uint					binding			= ~0u;
			uint					descriptorSet	= 0;
		};


		struct TextureUniform final : BaseUniform
		{
		// variables
			EImage::type			dimension		= EImage::Unknown;
			EPixelFormatClass::type	format			= EPixelFormatClass::Any;

		// methods
			bool operator == (const TextureUniform &right) const;
			bool operator >  (const TextureUniform &right) const;
			bool operator <  (const TextureUniform &right) const;
		};


		struct SamplerUniform final : BaseUniform
		{
		// variables

		// methods
			bool operator == (const SamplerUniform &right) const;
			bool operator >  (const SamplerUniform &right) const;
			bool operator <  (const SamplerUniform &right) const;
		};


		struct SubpassInput final : BaseUniform
		{
		// variables
			// TODO

		// methods
			bool operator == (const SubpassInput &right) const;
			bool operator >  (const SubpassInput &right) const;
			bool operator <  (const SubpassInput &right) const;
		};


		struct ImageUniform final : BaseUniform
		{
		// variables
			EImage::type			dimension		= EImage::Unknown;
			EPixelFormat::type		format			= EPixelFormat::Unknown;
			bool					writeAccess		= false;
			bool					readAccess		= true;

		// methods
			bool operator == (const ImageUniform &right) const;
			bool operator >  (const ImageUniform &right) const;
			bool operator <  (const ImageUniform &right) const;
		};


		struct UniformBuffer final : BaseUniform
		{
		// variables
			// TODO: buffer typename

			BytesUL					size;

		// methods
			bool operator == (const UniformBuffer &right) const;
			bool operator >  (const UniformBuffer &right) const;
			bool operator <  (const UniformBuffer &right) const;
		};


		struct StorageBuffer final : BaseUniform
		{
		// variables
			// TODO: static buffer typename, dynamic buffer typename

			BytesUL					staticSize;
			BytesUL					arrayStride;
			bool					writeAccess		= false;
			bool					readAccess		= true;

		// methods
			bool operator == (const StorageBuffer &right) const;
			bool operator >  (const StorageBuffer &right) const;
			bool operator <  (const StorageBuffer &right) const;
		};


		struct PushConstant final
		{
		// variables
			Name_t					name;
			EShader::bits			stageFlags;
			BytesU					offset;
			BytesU					size;

		// methods
			bool operator == (const PushConstant &right) const;
			bool operator >  (const PushConstant &right) const;
			bool operator <  (const PushConstant &right) const;
		};


		using Uniform_t		= Union< TextureUniform, SamplerUniform, ImageUniform,
									 UniformBuffer, StorageBuffer, PushConstant, SubpassInput >;
		using Uniforms_t	= Array< Uniform_t >;


	// variables
	private:
		Uniforms_t		_uniforms;
		HashResult		_hash;
		uint			_maxDescriptorSet	= 0;


	// methods
	public:
		PipelineLayoutDescriptor (GX_DEFCTOR) {}

		uint					MaxDescriptorSet () const	{ return _maxDescriptorSet; }
		HashResult				GetHash ()			const	{ return _hash; }
		ArrayCRef<Uniform_t>	GetUniforms ()		const	{ return _uniforms; }

		bool operator == (const Self &right) const;
		bool operator >  (const Self &right) const;
		bool operator <  (const Self &right) const;
	};
	


	//
	// Pipeline Layout Descriptor Builder
	//

	struct PipelineLayoutDescriptor::Builder
	{
	// variables
	private:
		PipelineLayoutDescriptor	_descr;
		bool						_changed	= true;


	// methods
	public:
		Builder () {}

		explicit Builder (const PipelineLayoutDescriptor &descr) : _descr(descr) {}

		Builder& AddTexture (StringCRef name, EImage::type dimension, EPixelFormatClass::type format,
							 uint binding, uint set, EShader::bits stageFlags);

		Builder& AddImage (StringCRef name, EImage::type dimension, EPixelFormat::type format, bool writeAccess,
						   bool readAccess, uint binding, uint set, EShader::bits stageFlags);

		Builder& AddUniformBuffer (StringCRef name, BytesU size, uint binding, uint set, EShader::bits stageFlags);
		
		Builder& AddStorageBuffer (StringCRef name, BytesU size, BytesU stride, bool writeAccess, bool readAccess,
									uint binding, uint set, EShader::bits stageFlags);

		Builder& AddPushConstant (StringCRef name, BytesU offset, BytesU size, EShader::bits stageFlags);
		
		// validate, calculate hash and return
		PipelineLayoutDescriptor const& Finish ();
	};

}	// Platforms
}	// Engine


namespace GX_STL
{
namespace GXTypes
{
	//
	// Hash (PipelineLayoutDescriptor)
	//
	template <>
	struct Hash< Engine::Platforms::PipelineLayoutDescriptor > final
	{
		HashResult operator () (const Engine::Platforms::PipelineLayoutDescriptor &key) const noexcept
		{
			HashResult	res;

			res += HashOf( key.GetUniforms().Count() );

			FOR( i, key.GetUniforms() ) {
				res += HashOf( key.GetUniforms()[i] );
			}
			return res;
		}
	};
	
}	// GXTypes
}	// GX_STL