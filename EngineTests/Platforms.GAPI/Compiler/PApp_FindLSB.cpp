// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "PApp.h"
#include "Pipelines/all_pipelines.h"

bool PApp::_Test_FindLSB ()
{
	const BytesU	buf_size = SizeOf< Pipelines::FindLSB_SSBO >;


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
	Pipelines::Create_findlsb( OUT pt_ci.descr );
	
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

	auto* st = Cast<const Pipelines::FindLSB_SSBO *>( read_cmd.result->ptr() );

	CHECK_ERR( st->results[0] == 0 );
	CHECK_ERR( st->results[1] == 2 );
	CHECK_ERR( st->results[2] == 4 );
	CHECK_ERR( st->results[3] == 6 );
	CHECK_ERR( st->results[4] == 7 );
	CHECK_ERR( st->results[5] == 8 );
	CHECK_ERR( st->results[6] == 9 );
	CHECK_ERR( st->results[7] == 10 );
	CHECK_ERR( st->results[8] == 11 );
	CHECK_ERR( st->results[9] == 12 );
	CHECK_ERR( st->results[10] == 12 );
	CHECK_ERR( st->results[11] == 10 );
	CHECK_ERR( st->results[12] == 8 );
	CHECK_ERR( st->results[13] == 6 );
	CHECK_ERR( st->results[14] == 4 );
	CHECK_ERR( st->results[15] == 2 );

	LOG( "FindLSB - OK", ELog::Info );
	return true;
}
