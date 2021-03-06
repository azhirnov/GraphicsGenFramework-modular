// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "Engine/Base/Common/Common.h"

#ifdef COMPUTE_API_OPENCL

namespace cl
{

#	include "External/opencl/cl_platform.h"

# if COMPUTE_API_OPENCL == 110
#	include "External/opencl/opencl11_core.h"
# elif COMPUTE_API_OPENCL == 120
#	include "External/opencl/opencl12_core.h"
# elif COMPUTE_API_OPENCL == 200
#	include "External/opencl/opencl20_core.h"
# else
#	error Unsupported OpenCL version!
# endif
	
# if defined( GRAPHICS_API_OPENGL )
#  if COMPUTE_API_OPENCL == 110
#	include "External/opencl/opencl11_gl.h"
#  elif COMPUTE_API_OPENCL == 120
#	include "External/opencl/opencl12_gl.h"
#  elif COMPUTE_API_OPENCL == 200
#	include "External/opencl/opencl20_gl.h"
#  endif
# else
#	define CL1_GL_FUNCTIONS( ... )
# endif
	

	// typedefs //
#	define CL1_BUILDTYPEDEF( _retType_, _funcName_, _funcParams_, _retValue_ ) \
		extern "C" typedef _retType_ (CL_API_CALL * PFNCL##_funcName_##PROC)  _funcParams_;


	// pointers to functions //
#	define CL1_BUILDFUNC( _retType_, _funcName_, _funcParams_, _retValue_ ) \
		extern PFNCL##_funcName_##PROC	cl##_funcName_;
	

	CL1_CORE_FUNCTIONS( CL1_BUILDTYPEDEF )
	CL1_CORE_FUNCTIONS( CL1_BUILDFUNC )
	
	CL1_GL_FUNCTIONS( CL1_BUILDTYPEDEF )
	CL1_GL_FUNCTIONS( CL1_BUILDFUNC )
	

	// API
	bool CL1_Init ();
	void CL1_Delete ();
	
	bool CL1_CheckErrors (cl_int errorCode, const char *clcall, const char *func, const char *file, int line);
	

	// macro
#if !defined( __GX_COMPUTE_API_ERROR_CHECKS__ )
	
#	define CL_CALL( ... )		{ __VA_ARGS__; }
#	define CL_CHECK( ... )		{ __VA_ARGS__; }

#else

#	define CL_CALL( ... ) \
	{ \
		const ::cl::cl_int __cl_err__ =  (__VA_ARGS__); \
		::cl::CL1_CheckErrors( __cl_err__, TOSTRING( __VA_ARGS__ ), GX_FUNCTION_NAME, __FILE__, __LINE__ ); \
	}

#	define __CL_CALL_R( _func_, _ret_, ... ) \
	{ \
		const ::cl::cl_int __cl_err__ =  (_func_); \
		if ( not ::cl::CL1_CheckErrors( __cl_err__, TOSTRING( _func_ ), GX_FUNCTION_NAME, __FILE__, __LINE__ ) ) \
			return _ret_; \
	}
	
	// Warning: different behavior on Debug and Release!
#	define CL_CHECK( ... ) \
		__CL_CALL_R( AUXDEF_GETARG_0( __VA_ARGS__ ), AUXDEF_GETARG_1( __VA_ARGS__, Uninitialized ) )

#endif

}	// cl

#endif	// COMPUTE_API_OPENCL
