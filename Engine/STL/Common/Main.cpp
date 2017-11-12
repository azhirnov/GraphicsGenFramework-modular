// Copyright �  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "Engine/STL/Engine.STL.h"

#include "Engine/STL/Defines/PublicMacro.h"
#include "Engine/STL/CompileTime/CompileTimeChecks.h"
#include "Engine/STL/Math/Rand/Random.h"

namespace GX_STL
{
namespace GXMath
{

/*
=================================================
	InitializeAllRandomGenerators_Op
=================================================
*/
	struct InitializeAllRandomGenerators_Op
	{
		usize result;

		InitializeAllRandomGenerators_Op () : result(0)
		{}

		template <typename T, usize Index>
		void Process ()
		{
			result += (uint) GXMath::TRandomGenerator<T>::Initialize();
		}
	};

/*
=================================================
	InitializeAllRandomGenerators
=================================================
*/
	static void InitializeAllRandomGenerators ()
	{
		using engine_list	= CompileTime::TypeListFrom< GXMath::DefaultRandomEngine, GXMath::PlatformRandomEngine >;

		InitializeAllRandomGenerators_Op	op;
		engine_list::RuntimeForEach( op );

		return void(op.result);
	}
	
/*
=================================================
	InitializeSTLMath
=================================================
*/
	void InitializeSTLMath ()
	{
		InitializeAllRandomGenerators();
	}

}	// GXMath
	
	
namespace GXTypes
{

#if not defined( GX_THREADSAFE_STATIC_SUPPORTED )
	OS::Mutex			SingletonMultiThread::_mutex;
#endif

}	// GXTypes
}	// GX_STL

using namespace GX_STL;
using namespace GXTypes;
using namespace GXMath;
using namespace CompileTime;


// check hidden namespaces intersection
using namespace _types_hidden_;
using namespace _ctime_hidden_;
using namespace _math_hidden_;
