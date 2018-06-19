// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "Core/STL/Common/Platforms.h"

#if defined( PLATFORM_WINDOWS ) and not defined( PLATFORM_SDL )

#include "Core/STL/OS/Base/ScopeLock.h"
#include "Core/STL/OS/Windows/OSWindows.h"

namespace GX_STL
{
namespace OS
{
	
	struct CriticalSection;
	struct ReadWriteSync;
	struct ConditionVariable;


	//
	// Critical Section
	//

	struct CriticalSection final : public Noncopyable
	{
		friend struct ConditionVariable;

	// types
	public:
		static const uint	_CSsize = sizeof(void*) * 4 + 8;

		typedef CriticalSection							Self;
		typedef DeferredType< _CSsize, alignof(void*) >	Handle_t;	// CRITICAL_SECTION


	// variables
	private:
		Handle_t	_crSection;
		bool		_inited;

		
	// methods
	private:
		void _Delete ();

	public:
		CriticalSection ();
		~CriticalSection ();

		ND_ bool IsValid () const	{ return _inited; }

			void Lock ();
		ND_ bool TryLock ();
			void Unlock ();

		ND_ ScopeLock  GetScopeLock ()
		{
			struct Util {
				static void Lock (void *p)		{ ((Self *)p)->Lock(); }
				static void Unlock (void *p)	{ ((Self *)p)->Unlock(); }
			};

			return ScopeLock( this, &Util::Lock, &Util::Unlock, false );
		}

		ND_ operator ScopeLock ()
		{
			return GetScopeLock();
		}
	};



	//
	// Single Read, Multiple Write (with WinXP support)
	//

	struct ReadWriteSync final : public Noncopyable
	{
		friend struct ConditionVariable;

	// types
	public:
		typedef ReadWriteSync				Self;
		typedef DeferredTypeFrom< void* >	Handle_t;	// SRWLOCK


	// variables
	private:
		Handle_t	_srw;
		

	private:
		static bool _isInitialized;
		static void _InitSRWFuncPointers ();


	// methods
	public:
		ReadWriteSync ();
		~ReadWriteSync ();


			void LockWrite ();
		ND_ bool TryLockWrite ();
			void UnlockWrite ();

			void LockRead ();
		ND_ bool TryLockRead ();
			void UnlockRead ();

		ND_ ScopeLock  GetScopeWriteLock ()
		{
			struct Util {
				static void Lock (void *p)		{ ((Self *)p)->LockWrite(); }
				static void Unlock (void *p)	{ ((Self *)p)->UnlockWrite(); }
			};
			return ScopeLock( this, &Util::Lock, &Util::Unlock, false );
		}

		ND_ ScopeLock  GetScopeReadLock ()
		{
			struct Util {
				static void Lock (void *p)		{ ((Self *)p)->LockRead(); }
				static void Unlock (void *p)	{ ((Self *)p)->UnlockRead(); }
			};
			return ScopeLock( this, &Util::Lock, &Util::Unlock, false );
		}
	};



	//
	// Condition Variable
	//

	struct ConditionVariable final : public Noncopyable
	{
	// types
	public:
		typedef ConditionVariable			Self;
		typedef DeferredTypeFrom< void* >	Handle_t;	// CONDITION_VARIABLE


	// variables
	private:
		Handle_t	_cv;
		bool		_inited;


	// methods
	private:
		static bool _isInitialized;
		static void _InitCondVarFuncPointers ();

		bool _Create ();
		void _Destroy ();

	public:
		ConditionVariable ();
		~ConditionVariable ();

		bool IsValid () const	{ return _inited; }

		void Signal ();
		void Broadcast ();

		bool Wait (CriticalSection &cs, TimeL time);
	};



	//
	// Synchronization Event
	//
	
	struct SyncEvent final : public Noncopyable
	{
	// types
	public:
		typedef SyncEvent					Self;
		typedef DeferredTypeFrom< void* >	Handle_t;	// HANDLE

		enum EFlags {
			MANUAL_RESET		= 0,
			AUTO_RESET			= 0x1,
			INIT_STATE_SIGNALED = 0x2,
		};


	// variables
	private:
		Handle_t	_event;


	// methods
	public:
		explicit
		SyncEvent (EFlags flags = AUTO_RESET);
		~SyncEvent ();

		bool IsValid () const	{ return _event.IsDefined(); }

		bool Signal ();
		bool Reset ();

		bool Wait ();
		bool Wait (TimeL time);

		static int WaitEvents (ArrayRef<Self *> events, bool waitAll, TimeL time);
	
	private:
		bool _Create (EFlags flags);
		bool _Delete();
	};



	//
	// Semaphore
	//

	struct Semaphore final : public Noncopyable
	{
	// types
	public:
		typedef Semaphore					Self;
		typedef DeferredTypeFrom< void* >	Handle_t;	// HANDLE


	// variables
	private:
		Handle_t	_sem;


	// methods
	private:
		bool _Create (uint initialValue);
		void _Destroy ();

	public:
		explicit Semaphore (uint initialValue);
		~Semaphore ();

		bool IsValid () const	{ return _sem.IsDefined(); }

		void Lock ();
		bool TryLock ();
		void Unlock ();

		uint GetValue ();
		
		/*ScopeLock GetScopeLock()
		{
			struct Util {
				static void Lock (void *p)		{ ((Self *)p)->Lock(); }
				static void Unlock (void *p)	{ ((Self *)p)->Unlock(); }
			};

			return ScopeLock( this, &Util::Lock, &Util::Unlock, false );
		}

		operator ScopeLock ()
		{
			return GetScopeLock();
		}*/
	};


}	// OS
}	// GX_STL

#endif	// PLATFORM_WINDOWS