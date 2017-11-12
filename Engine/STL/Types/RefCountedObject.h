// Copyright �  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "ReferenceCounter.h"
#include "Engine/STL/ThreadSafe/Atomic.h"
#include "Engine/STL/OS/OSLowLevel.h"
#include "Engine/STL/Containers/Set.h"

namespace GX_STL
{
namespace GXTypes
{

#	define SHARED_POINTER( _name_ )		using _name_##Ptr = ::GX_STL::GXTypes::SharedPointerType< _name_ >;



	//
	// Reference Counted Object
	//

	class _STL_EXPORT_ RefCountedObject : public CompileTime::TypeQualifier< CompileTime::ETypeQualifier::Def_Noncopyable >
	{
		friend class  StaticRefCountedObject;
		friend struct SharedPointerStrategy< RefCountedObject >;

	// types
	private:
		typedef RefCountedObject	Self;


	// variables
	private:
		Atomic< int >		_atomicCounter;


	// methods
	private:
		void operator = (Self &&) = delete;
		void operator = (const Self &) = delete;

	private:
		forceinline void _DebugAddRef ()
		{
			DEBUG_ONLY(
				SCOPELOCK( _GetMutex() );
				_GetObjectRefs().Add( this );
			)
		}

		forceinline void _DebugRemoveRef ()
		{
			DEBUG_ONLY(
				SCOPELOCK( _GetMutex() );

				usize	idx;
				if ( _GetObjectRefs().FindIndex( this, idx ) ) {
					_GetObjectRefs().EraseByIndex( idx );
				}
			)
		}

		DEBUG_ONLY(
		static Set< Self *>& _GetObjectRefs () noexcept
		{
			static Set< Self *>		s_instance;
			return s_instance;
		}

		static OS::Mutex& _GetMutex () noexcept
		{
			static OS::Mutex	s_mutex;
			return s_mutex;
		})

	protected:
		RefCountedObject (): _atomicCounter(0)
		{
			_DebugAddRef();
		}
		
		explicit
		RefCountedObject (const Self &) : _atomicCounter(0)
		{
			_DebugAddRef();
		}
		
		RefCountedObject (Self &&other) : _atomicCounter( other._atomicCounter )
		{
			other._atomicCounter = 0;
		}

		virtual ~RefCountedObject ()
		{
			_DebugRemoveRef();
		}

		forceinline  void _AddRef ()				{ _atomicCounter.Inc(); }
		forceinline  uint _GetRefCount ()	const	{ return _atomicCounter; }
		forceinline  void _ReleaseRef ()			{ ASSERT( int(_atomicCounter) > 0 );  if ( _atomicCounter.Dec() == 0 ) _Release(); }
		virtual		 void _Release ()				{ delete this; }
		

	public:
		DEBUG_ONLY(
		static void s_ChenckNotReleasedObjects (uint refValue = 0)
		{
			SCOPELOCK( _GetMutex() );

			auto& objects = _GetObjectRefs();

			if ( objects.Count() != refValue )
			{
				GX_BREAK_POINT();
			}
		})
	};
	
	
		
	template <>
	struct SharedPointerStrategy< RefCountedObject >
	{
		forceinline static void IncRef (RefCountedObject *ptr)	{ ptr->_AddRef(); }
		forceinline static void DecRef (RefCountedObject *ptr)	{ ptr->_ReleaseRef(); }
		forceinline static int  Count  (RefCountedObject *ptr)	{ return ptr->_GetRefCount(); }
	};
	

	template <typename T>
	using SharedPointerType = ReferenceCounter< T, RefCountedObject, SharedPointerStrategy< RefCountedObject > >;


}	// GXTypes
}	// GX_STL
