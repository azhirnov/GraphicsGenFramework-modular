// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "Engine/Platforms/Public/GPU/BufferEnums.h"
#include "Engine/Platforms/Public/GPU/MemoryEnums.h"
#include "Engine/Platforms/Public/GPU/IDs.h"

namespace Engine
{
namespace Platforms
{

	//
	// Buffer Descriptor
	//

	struct BufferDescriptor : CompileTime::PODStruct
	{
	// variables
		BytesUL				size;
		EBufferUsage::bits	usage;

	// methods
		BufferDescriptor (GX_DEFCTOR) {}
		BufferDescriptor (Bytes<ulong> size, EBufferUsage::bits usage) : size(size), usage(usage) {}
		BufferDescriptor (Bytes<uint> size, EBufferUsage::bits usage) : size(size), usage(usage) {}
	};

}	// Platforms


namespace CreateInfo
{

	//
	// Buffer Create Info
	//
	struct GpuBuffer
	{
	// types
		using BufferDescriptor	= Platforms::BufferDescriptor;
		using EGpuMemory		= Platforms::EGpuMemory;
		using EMemoryAccess		= Platforms::EMemoryAccess;

	// variables
		ModulePtr				gpuThread;			// can be null
		ModulePtr				memManager;			// can be null
		BufferDescriptor		descr;
		EGpuMemory::bits		memFlags;
		EMemoryAccess::bits		access;
		bool					allocMem = true;	// if true then you don't need to attach memory module to buffer

	// methods
		GpuBuffer (GX_DEFCTOR) {}

		explicit GpuBuffer (const BufferDescriptor &descr) : descr{descr}, allocMem{false} {}

		GpuBuffer (const BufferDescriptor &descr, EGpuMemory::bits memFlags, EMemoryAccess::bits access) :
			descr{descr}, memFlags{memFlags}, access{access}, allocMem{true} {}

		GpuBuffer (const BufferDescriptor &descr, const ModulePtr &memMngr, EGpuMemory::bits memFlags, EMemoryAccess::bits access) :
			memManager{memMngr}, descr{descr}, memFlags{memFlags}, access{access}, allocMem{true} {}
	};


	//
	// Shared Buffer Create Info
	//
	struct GpuSharedBuffer
	{
	// types
		using EMemoryAccess		= Platforms::EMemoryAccess;

	// variables
		ModulePtr				gpuThread;		// can be null
		ModulePtr				sharedBuffer;
		EMemoryAccess::bits		access;

	// methods
		GpuSharedBuffer (GX_DEFCTOR) {}

		GpuSharedBuffer (const ModulePtr &buf, EMemoryAccess::bits access) : sharedBuffer{buf}, access{access} {}
	};

}	// CreateInfo


namespace GpuMsg
{

	//
	// Get Buffer Descriptor
	//
	struct GetBufferDescriptor
	{
		Out< Platforms::BufferDescriptor >	result;
	};

	struct SetBufferDescriptor
	{
		Platforms::BufferDescriptor			descr;
	};


}	// GpuMsg
}	// Engine
