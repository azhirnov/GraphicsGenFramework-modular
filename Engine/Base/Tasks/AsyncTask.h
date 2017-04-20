// Copyright � 2014-2017  Zhirnov Andrey. All rights reserved.

#pragma once

#include "Engine/Base/Tasks/AsyncMessage.h"
#include "Engine/Base/Modules/Module.h"
#include "Engine/Base/Tasks/TaskModule.h"
#include "Engine/Base/Main/MainSystem.h"

namespace Engine
{
namespace Base
{

	//
	// Async Task
	//

	template <	typename ResultT	= UninitializedType,
				typename ProgressT	= UninitializedType
			 >
	class AsyncTask : public BaseObject
	{
	// types
	public:
		using ResultType	= ResultT;
		using ProgressType	= ProgressT;
		using Self			= AsyncTask< ResultType, ProgressType >;


	// variables
	private:
		ModulePtr		_currentThreadModule;		// module in thread where waiting task result and updates progress 
		ModulePtr		_targetThreadModule;		// module in thread where task will be schedule

		OS::SyncEvent	_event;
		Atomic<bool>	_isCanceled;
		Atomic<bool>	_onCanceledCalled;
		Atomic<bool>	_isSync;					// if current and target threads are same

		ResultType		_result;


	// methods
	public:
		bool  Wait ();
		Self* Execute ();
		void  Cancel ();
		bool  IsCanceled ();
		
		SubSystemsRef	GlobalSystems () const	= delete;


	protected:
		AsyncTask (const ModulePtr &currentThreadModule, const ModulePtr &targetThreadModule);
		~AsyncTask ();

		void PublishProgress (ProgressType &&);

		ModulePtr const&	CurrentThreadModule ()		{ return _currentThreadModule; }
		ModulePtr const&	TargetThreadModule ()		{ return _targetThreadModule; }


	private:
		void _RunSync ();
		void _RunAsync (const TaskModulePtr &);
		void _OnCanceled (const TaskModulePtr & = null);
		void _UpdateProgress (const TaskModulePtr &, ProgressType &&);
		void _PostExecute (const TaskModulePtr &);


	// interface
	protected:
		// in current thread //
		virtual void PreExecute (const ModulePtr &)							{}
		virtual void UpdateProgress (const ModulePtr &, ProgressType &&)	{}
		virtual void PostExecute (const ModulePtr &, ResultType &&)			{}
		virtual void OnCanceled (const ModulePtr &)							{}
		
		// in target thread //
		virtual void ExecuteInBackground (const ModulePtr &, OUT ResultType &) = 0;
	};

	
/*
=================================================
	constructor
=================================================
*/
	template <typename R, typename P>
	inline AsyncTask<R,P>::AsyncTask (const ModulePtr &currentThreadModule, const ModulePtr &targetThreadModule) :
		BaseObject( currentThreadModule->GlobalSystems() ),
		_currentThreadModule( currentThreadModule ),
		_targetThreadModule( targetThreadModule ),
		_event( OS::SyncEvent::MANUAL_RESET ),
		_isCanceled( false ),
		_onCanceledCalled( false ),
		_isSync( _targetThreadModule->GetThreadID() == _currentThreadModule->GetThreadID() )
	{
	}
	
/*
=================================================
	destructor
=================================================
*/
	template <typename R, typename P>
	inline AsyncTask<R,P>::~AsyncTask ()
	{
		_OnCanceled();

		CHECK( Wait() );
	}
	
/*
=================================================
	Wait
=================================================
*/
	template <typename R, typename P>
	inline bool AsyncTask<R,P>::Wait ()
	{
		return _event.Wait( TimeU::FromSeconds( 60 ) );
	}
		
/*
=================================================
	Execute
=================================================
*/
	template <typename R, typename P>
	inline AsyncTask<R,P>*  AsyncTask<R,P>::Execute ()
	{
		ASSERT( _currentThreadModule->GetThreadID() == ThreadID::GetCurrent() );

		if ( IsCanceled() )
		{
			_OnCanceled();
			return this;
		}

		PreExecute( _currentThreadModule );

		if ( IsCanceled() )
		{
			_OnCanceled();
			return this;
		}

		// target thread is current
		if ( _isSync )
		{
			_RunSync();
			return this;
		}

		CHECK( _currentThreadModule->GlobalSystems()->Get< TaskModule >()->
			Send( Message< ModuleMsg::PushAsyncMessage >{
				this,
				AsyncMessage{ &AsyncTask::_RunAsync, this },
				_targetThreadModule->GetThreadID()
			} )
		);
		return this;
	}
	
/*
=================================================
	Cancel
=================================================
*/
	template <typename R, typename P>
	inline void AsyncTask<R,P>::Cancel ()
	{
		_isCanceled = true;
	}
	
/*
=================================================
	IsCanceled
=================================================
*/
	template <typename R, typename P>
	inline bool AsyncTask<R,P>::IsCanceled ()
	{
		return _isCanceled;
	}

/*
=================================================
	PublishProgress
=================================================
*/
	template <typename R, typename P>
	inline void AsyncTask<R,P>::PublishProgress (ProgressType &&value)
	{
		ASSERT( _targetThreadModule->GetThreadID() == ThreadID::GetCurrent() );
		
		// target thread is current
		if ( _isSync )
		{
			UpdateProgress( _currentThreadModule, RVREF( value ) );
			return;
		}

		CHECK( _targetThreadModule->GlobalSystems()->Get< TaskModule >()->
			Send( Message< ModuleMsg::PushAsyncMessage >{
				this,
				AsyncMessage{ &AsyncTask::_UpdateProgress, this, RVREF(value) },
				_currentThreadModule->GetThreadID()
			} )
		);
	}
	
/*
=================================================
	_RunSync
=================================================
*/
	template <typename R, typename P>
	inline void AsyncTask<R,P>::_RunSync ()
	{
		if ( IsCanceled() )
		{
			_OnCanceled();
			return;
		}

		ExecuteInBackground( _targetThreadModule, OUT _result );
		
		if ( IsCanceled() )
		{
			_OnCanceled();
			return;
		}

		PostExecute( _currentThreadModule, RVREF( _result ) );

		_event.Signal();
	}
	
/*
=================================================
	_RunAsync
=================================================
*/
	template <typename R, typename P>
	inline void AsyncTask<R,P>::_RunAsync (const TaskModulePtr &)
	{
		ASSERT( _targetThreadModule->GetThreadID() == ThreadID::GetCurrent() );
		
		if ( IsCanceled() )
		{
			CHECK( _targetThreadModule->GlobalSystems()->Get< TaskModule >()->
				Send( Message< ModuleMsg::PushAsyncMessage >{
					this,
					AsyncMessage{ &AsyncTask::_OnCanceled, this },
					_currentThreadModule->GetThreadID()
				} )
			);
			return;
		}

		ExecuteInBackground( _targetThreadModule, OUT _result );
		
		if ( IsCanceled() )
		{
			CHECK( _targetThreadModule->GlobalSystems()->Get< TaskModule >()->
				Send( Message< ModuleMsg::PushAsyncMessage >{
					this,
					AsyncMessage{ &AsyncTask::_OnCanceled, this },
					_currentThreadModule->GetThreadID()
				} )
			);
			return;
		}

		CHECK( _targetThreadModule->GlobalSystems()->Get< TaskModule >()->
			Send( Message< ModuleMsg::PushAsyncMessage >{
				this,
				AsyncMessage{ &AsyncTask::_PostExecute, this },
				_currentThreadModule->GetThreadID()
			} )
		);
	}
	
/*
=================================================
	_UpdateProgress
=================================================
*/
	template <typename R, typename P>
	inline void AsyncTask<R,P>::_UpdateProgress (const TaskModulePtr &, ProgressType &&value)
	{
		ASSERT( _currentThreadModule->GetThreadID() == ThreadID::GetCurrent() );

		UpdateProgress( _currentThreadModule, RVREF( value ) );
	}
		
/*
=================================================
	_PostExecute
=================================================
*/
	template <typename R, typename P>
	inline void AsyncTask<R,P>::_PostExecute (const TaskModulePtr &)
	{
		ASSERT( _currentThreadModule->GetThreadID() == ThreadID::GetCurrent() );

		PostExecute( _currentThreadModule, RVREF( _result ) );

		_event.Signal();
	}
	
/*
=================================================
	_OnCanceled
=================================================
*/
	template <typename R, typename P>
	inline void AsyncTask<R,P>::_OnCanceled (const TaskModulePtr &)
	{
		ASSERT( _currentThreadModule->GetThreadID() == ThreadID::GetCurrent() );

		if ( _onCanceledCalled )
			return;

		_onCanceledCalled = true;

		OnCanceled( _currentThreadModule );

		_event.Signal();
	}


}	// Base
}	// Engine
