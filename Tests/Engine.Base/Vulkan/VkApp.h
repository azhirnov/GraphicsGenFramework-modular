// Copyright �  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "../Common.h"


class VkApp final : public StaticRefCountedObject
{
// types
private:
	using RenderPassMsgList_t = CompileTime::TypeListFrom< Message<GpuMsg::GetRenderPassDescriptor> >;


// variables
public:
	Ptr< Module >		ms;

private:
	bool				looping		= true;
	uint				cmdBufIndex	= 0;

	// triangle
	ModulePtr			gpipeline1;
	ModulePtr			pipelineTemplate1;
	ModulePtr			resourceTable1;

	// quad
	ModulePtr			gpipeline2;
	ModulePtr			pipelineTemplate2;
	ModulePtr			resourceTable2;

	ModulePtr			framebuffer;
	ModulePtr			fbColorImage;

	ModulePtr			texture;
	ModulePtr			sampler;

	ModulePtr			vbuffer;
	ModulePtr			ibuffer;
	ModulePtr			ubuffer;

	Array<ModulePtr>	cmdBuffers;
	ModulePtr			cmdBuilder;


// methods
public:
	VkApp ();
	void Initialize ();
	void Quit ();
	bool Update ();


private:
	bool _OnWindowClosed (const Message<OSMsg::WindowAfterDestroy> &);
	bool _OnKey (const Message< ModuleMsg::InputKey > &);
	bool _OnMotion (const Message< ModuleMsg::InputMotion > &);
	bool _Draw (const Message< ModuleMsg::Update > &);
	bool _VkInit (const Message< GpuMsg::DeviceCreated > &);
	bool _VkDelete (const Message< GpuMsg::DeviceBeforeDestroy > &);
	
	bool _CreatePipeline1 ();
	bool _CreatePipeline2 ();
	bool _CreateCmdBuffers ();
};
