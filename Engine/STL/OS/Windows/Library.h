// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "Engine/STL/Common/Platforms.h"

#if defined( PLATFORM_WINDOWS ) and not defined( PLATFORM_SDL )

#include "Engine/STL/OS/Windows/OSWindows.h"

namespace GX_STL
{
	using SharedLibFunction_t 	= void (CALLBACK *) ();

namespace OS
{

	//
	// Library
	//

	struct Library final : public Noncopyable
	{
	// types
	public:
		using Handle_t	= DeferredTypeFrom<void *>;	// HMODULE
		using Self		= Library;
		using Func_t	= SharedLibFunction_t;


	// variables
	private:
		Handle_t	_library;
		String		_name;
		bool		_freeWhenDelete;


	// methods
	public:
		Library ();

		explicit
		Library (StringCRef name, bool canFree = true);

		~Library ();
		
		//void * Handle () const;
		bool IsValid () const;

		const String & GetName () const		{ return _name; }

		bool FindAndLoad (StringCRef name, uint searchDepth, bool canFree = true);
		bool LoadFromHandle (const Handle_t &lib, bool canFree = false);
		bool Load (StringCRef name, bool canFree = true);
		bool LoadSelf ();
		bool Unload ();

		template <typename T>
		bool GetProc (OUT T &proc, StringCRef procName) const
		{
			ASSERT( IsValid() );
			ASSERT( not procName.Empty() );

			T tmp = ReferenceCast<T>( GetProc( procName.cstr() ) );

			if ( tmp != null )
			{
				proc = tmp;
				return true;
			}
			return false;
		}

		Func_t GetProc (StringCRef procName, Func_t defProc = null) const;
	};

	
}	// OS
}	// GX_STL

#endif	// PLATFORM_WINDOWS