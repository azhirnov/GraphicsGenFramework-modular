// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "Engine/Profilers/Public/GpuStatistic.h"
#include "Engine/Platforms/Public/OS/Window.h"
#include "Engine/Platforms/Public/GPU/Thread.h"
#include "Engine/Platforms/Public/GPU/VR.h"
#include "Engine/Profilers/Impl/ProfilerObjectsConstructor.h"
#include "Engine/Platforms/Public/Tools/GPUThreadHelper.h"

namespace Engine
{
namespace Profilers
{

	//
	// FPS Counter
	//

	class FPSCounter : public Module
	{
	// types
	protected:
		using SupportedMessages_t	= MessageListFrom<
											ModuleMsg::AttachModule,
											ModuleMsg::DetachModule,
											ModuleMsg::OnModuleAttached,
											ModuleMsg::OnModuleDetached,
											ModuleMsg::Link,
											ModuleMsg::Delete
										>;

		using SupportedEvents_t		= MessageListFrom<
											ModuleMsg::Delete
										>;

		using WindowMsgList_t		= MessageListFrom<
											OSMsg::WindowGetDescription,
											OSMsg::WindowSetDescription >;

		
	// constants
	private:
		static const TypeIdList		_eventTypes;


	// variables
	private:
		const TimeD				_interval;
		TimeProfilerD			_timer;
		uint					_frameCounter;


	// methods
	public:
		FPSCounter (UntypedID_t, GlobalSystemsRef gs, const CreateInfo::FPSCounter &);
		~FPSCounter ();


	// message handlers
	private:
		bool _Link (const ModuleMsg::Link &);
		bool _Delete (const ModuleMsg::Delete &);

		// event handlers
		bool _OnThreadEndFrame (const GpuMsg::ThreadEndFrame &);
		bool _OnThreadEndVRFrame (const GpuMsg::ThreadEndVRFrame &);

		void _Update ();
	};
//-----------------------------------------------------------------------------


	
	const TypeIdList	FPSCounter::_eventTypes{ UninitializedT< SupportedEvents_t >() };

/*
=================================================
	constructor
=================================================
*/
	FPSCounter::FPSCounter (UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::FPSCounter &ci) :
		Module( gs, ModuleConfig{ id, 1 }, &_eventTypes ),
		_interval{ ci.interval },	_frameCounter{ 0 }
	{
		SetDebugName( "FPSCounter" );

		_SubscribeOnMsg( this, &FPSCounter::_AttachModule_Impl );
		_SubscribeOnMsg( this, &FPSCounter::_DetachModule_Impl );
		_SubscribeOnMsg( this, &FPSCounter::_OnModuleAttached_Impl );
		_SubscribeOnMsg( this, &FPSCounter::_OnModuleDetached_Impl );
		_SubscribeOnMsg( this, &FPSCounter::_Link );
		_SubscribeOnMsg( this, &FPSCounter::_Delete );
	}
	
/*
=================================================
	destructor
=================================================
*/
	FPSCounter::~FPSCounter ()
	{
	}
	
/*
=================================================
	_Link
=================================================
*/
	bool FPSCounter::_Link (const ModuleMsg::Link &msg)
	{
		if ( _IsComposedOrLinkedState( GetState() ) )
			return true;	// already linked

		CHECK_ERR( _IsInitialState( GetState() ) );

		// must be attached to window
		CHECK_LINKING( _GetParents().Count() == 1 );
		CHECK_LINKING( _GetParents().Front()->SupportsAllMessages< WindowMsgList_t >() );

		// find gpu thread
		ModulePtr	gthread;
		//if ( not (gthread = GlobalSystems()->parallelThread->GetModuleByMsg< VRThreadMsgList_t >()) )
		if ( not (gthread = PlatformTools::GPUThreadHelper::FindVRThread( GlobalSystems() )) )
		{
			CHECK_ERR(( gthread = PlatformTools::GPUThreadHelper::FindGraphicsThread( GlobalSystems() ) ));
			gthread->Subscribe( this, &FPSCounter::_OnThreadEndFrame );
		}
		else
			gthread->Subscribe( this, &FPSCounter::_OnThreadEndVRFrame );

		_timer.Start();

		return Module::_Link_Impl( msg );
	}
	
/*
=================================================
	_Delete
=================================================
*/
	bool FPSCounter::_Delete (const ModuleMsg::Delete &msg)
	{
		return Module::_Delete_Impl( msg );
	}
	
/*
=================================================
	_OnThreadEndFrame
=================================================
*/
	bool FPSCounter::_OnThreadEndFrame (const GpuMsg::ThreadEndFrame &)
	{
		_Update();
		return true;
	}
	
	bool FPSCounter::_OnThreadEndVRFrame (const GpuMsg::ThreadEndVRFrame &)
	{
		_Update();
		return true;
	}

/*
=================================================
	_Update
=================================================
*/
	void FPSCounter::_Update ()
	{
		double	dt = _timer.GetTimeDelta().Seconds();

		++_frameCounter;

		if ( dt > _interval.Seconds() )
		{
			ModulePtr	window = _GetParents().Front();

			OSMsg::WindowGetDescription		req_descr;
			window->Send( req_descr );

			String&	str = req_descr.result->caption;
			usize	pos = 0;
			uint	fps	= uint(GXMath::Round(double(_frameCounter) / dt));

			if ( str.FindIC( " [FPS", OUT pos, 0 ) )
			{
				usize	end = 0;
				str.Find( ']', OUT end, pos );

				str.Erase( pos, end - pos + 1 );
			}

			str << " [FPS: " << fps << "]";

			window->Send( OSMsg::WindowSetDescription{ *req_descr.result }); 

			_timer.Start();
			_frameCounter = 0;
		}
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	CreateFPSCounter
=================================================
*/
	ModulePtr ProfilerObjectsConstructor::CreateFPSCounter (ModuleMsg::UntypedID_t id, GlobalSystemsRef gs, const CreateInfo::FPSCounter &ci)
	{
		return New< FPSCounter >( id, gs, ci );
	}

}	// Profilers
}	// Engine
