// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "PApp.h"
#include "Pipelines/all_pipelines.h"

bool PApp::_Test_Include ()
{
	const BytesU	buf_size = SizeOf< Pipelines::Include_SSBO >;


	// create resources
	auto	factory	= ms->GlobalSystems()->modulesFactory;

	GpuMsg::CreateFence		fence_ctor;
	syncManager->Send( fence_ctor );

	ModulePtr	cmd_buffer;
	CHECK_ERR( factory->Create(
					gpuIDs.commandBuffer,
					gpuThread->GlobalSystems(),
					CreateInfo::GpuCommandBuffer{},
					OUT cmd_buffer ) );
	cmdBuilder->Send( ModuleMsg::AttachModule{ cmd_buffer });

	ModulePtr	buffer;
	CHECK_ERR( factory->Create(
					gpuIDs.buffer,
					gpuThread->GlobalSystems(),
					CreateInfo::GpuBuffer{
						BufferDescription{ buf_size, EBufferUsage::Storage },
						EGpuMemory::CoherentWithCPU },
					OUT buffer ) );

	CreateInfo::PipelineTemplate	pt_ci;
	Pipelines::Create_include( OUT pt_ci.descr );
	
	ModulePtr	pipeline_template;
	CHECK_ERR( factory->Create(
					PipelineTemplateModuleID,
					gpuThread->GlobalSystems(),
					pt_ci,
					OUT pipeline_template ) );
	ModuleUtils::Initialize({ pipeline_template });

	GpuMsg::CreateComputePipeline	cppl_ctor{ gpuIDs.pipeline, gpuThread };
	pipeline_template->Send( cppl_ctor );

	ModulePtr	pipeline	= *cppl_ctor.result;

	ModulePtr	resource_table;
	CHECK_ERR( factory->Create(
					gpuIDs.resourceTable,
					gpuThread->GlobalSystems(),
					CreateInfo::PipelineResourceTable{},
					OUT resource_table ) );
	
	resource_table->Send( ModuleMsg::AttachModule{ "pipeline", pipeline });
	resource_table->Send( GpuMsg::PipelineAttachBuffer{ "ssb", buffer, 0_b, buf_size });

	ModuleUtils::Initialize({ cmd_buffer, buffer, pipeline, resource_table });


	// build command buffer
	cmdBuilder->Send( GpuMsg::CmdBegin{ cmd_buffer });

	cmdBuilder->Send( GpuMsg::CmdBindComputePipeline{ pipeline });
	cmdBuilder->Send( GpuMsg::CmdBindComputeResourceTable{ resource_table });
	cmdBuilder->Send( GpuMsg::CmdDispatch{ uint3(1) });
	
	GpuMsg::CmdEnd	cmd_end;
	cmdBuilder->Send( cmd_end );


	// submit and sync
	gpuThread->Send( GpuMsg::SubmitCommands{ *cmd_end.result }.SetFence( *fence_ctor.result ));

	syncManager->Send( GpuMsg::ClientWaitFence{ *fence_ctor.result });


	// read from buffer
	BinaryArray	dst_data;	dst_data.Resize( usize(buf_size) );

	GpuMsg::ReadFromGpuMemory	read_cmd{ dst_data };
	buffer->Send( read_cmd );

	auto* st = Cast<const Pipelines::Include_SSBO *>( read_cmd.result->ptr() );

	CHECK_ERR(Equals( st->results[0], 1.0f ));
	CHECK_ERR(Equals( st->results[1], 2.0f ));

	LOG( "Include - OK", ELog::Info );
	return true;
}
