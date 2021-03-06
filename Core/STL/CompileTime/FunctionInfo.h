// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "Core/STL/CompileTime/TypeListHelpers.h"
#include "Core/STL/Types/Delegate.h"

namespace GX_STL
{
namespace CompileTime
{

	namespace _ctime_hidden_
	{
		template <typename T>
		struct _FunctionInfo			{};
		
		template <typename T>
		struct _FunctionInfo< T * >		{};
		
		template <typename T, typename Class>
		struct _FunctionInfo< T (Class::*) >	{};
		
		
		template <typename Result, typename ...Args>
		struct _FunctionInfo< Result (Args...) >
		{
			using args		= TypeListFrom< Args..., TypeListEnd >;
			using result	= Result;
			using type		= Result (*) (Args...);
			using clazz		= void;
		};
		
		template <typename Result, typename ...Args>
		struct _FunctionInfo< Result (*) (Args...) >
		{
			using args		= TypeListFrom< Args..., TypeListEnd >;
			using result	= Result;
			using type		= Result (*) (Args...);
			using clazz		= void;
		};
		
		template <typename Class, typename Result, typename ...Args>
		struct _FunctionInfo< Result (Class::*) (Args...) >
		{
			using args		= TypeListFrom< Args..., TypeListEnd >;
			using result	= Result;
			using type		= Result (Class::*) (Args...);
			using clazz		= Class;
		};
		
		template <typename Class, typename Result, typename ...Args>
		struct _FunctionInfo< Result (Class::*) (Args...) const >
		{
			using args		= TypeListFrom< Args..., TypeListEnd >;
			using result	= Result;
			using type		= Result (Class::*) (Args...) const;
			using clazz		= Class;
		};
		
		template <typename Class, typename Result, typename ...Args>
		struct _FunctionInfo< Result (Class::*) (Args...) volatile >
		{
			using args		= TypeListFrom< Args..., TypeListEnd >;
			using result	= Result;
			using type		= Result (Class::*) (Args...) volatile;
			using clazz		= Class;
		};

		template <typename Class, typename Result, typename ...Args>
		struct _FunctionInfo< Result (Class::*) (Args...) volatile const >
		{
			using args		= TypeListFrom< Args..., TypeListEnd >;
			using result	= Result;
			using type		= Result (Class::*) (Args...) volatile const;
			using clazz		= Class;
		};
		
		template <typename Result, typename ...Args>
		struct _FunctionInfo< Result (Args...) noexcept >
		{
			using args		= TypeListFrom< Args..., TypeListEnd >;
			using result	= Result;
			using type		= Result (*) (Args...) noexcept;
			using clazz		= void;
		};

		template <typename Result, typename ...Args>
		struct _FunctionInfo< Result (*) (Args...) noexcept >
		{
			using args		= TypeListFrom< Args..., TypeListEnd >;
			using result	= Result;
			using type		= Result (*) (Args...) noexcept;
			using clazz		= void;
		};

		template <typename Class, typename Result, typename ...Args>
		struct _FunctionInfo< Result (Class::*) (Args...) noexcept >
		{
			using args		= TypeListFrom< Args..., TypeListEnd >;
			using result	= Result;
			using type		= Result (Class::*) (Args...) noexcept;
			using clazz		= Class;
		};
		
		template <typename Class, typename Result, typename ...Args>
		struct _FunctionInfo< Result (Class::*) (Args...) const noexcept >
		{
			using args		= TypeListFrom< Args..., TypeListEnd >;
			using result	= Result;
			using type		= Result (Class::*) (Args...) const noexcept;
			using clazz		= Class;
		};

		template <typename Class, typename Result, typename ...Args>
		struct _FunctionInfo< Result (Class::*) (Args...) volatile noexcept >
		{
			using args		= TypeListFrom< Args..., TypeListEnd >;
			using result	= Result;
			using type		= Result (Class::*) (Args...) volatile noexcept;
			using clazz		= Class;
		};

		template <typename Class, typename Result, typename ...Args>
		struct _FunctionInfo< Result (Class::*) (Args...) volatile const noexcept >
		{
			using args		= TypeListFrom< Args..., TypeListEnd >;
			using result	= Result;
			using type		= Result (Class::*) (Args...) volatile const noexcept;
			using clazz		= Class;
		};

		template <typename Result, typename ...Args>
		struct _FunctionInfo< GXTypes::Delegate< Result (Args...) > >
		{
			using _del		= GXTypes::Delegate< Result (Args...) >;

			using args		= typename _del::Args_t;
			using result	= typename _del::Result_t;
			using type		= typename _del::Function_t;
			using clazz		= void;
		};

		template <typename Result, typename ...Args>
		struct _FunctionInfo< GXTypes::Event< Result (Args...) > >
		{
			using _ev		= GXTypes::Event< Result (Args...) >;

			using args		= typename _ev::Args_t;
			using result	= typename _ev::Result_t;
			using type		= typename _ev::Function_t;
			using clazz		= void;
		};
		
		template <typename Result, typename ...Args>
		struct _FunctionInfo< std::function< Result (Args...) > >
		{
			using args		= TypeListFrom< Args..., TypeListEnd >;
			using result	= Result;
			using type		= Result (*) (Args...);
			using clazz		= void;
		};

	}	// _ctime_hidden_

	
	template <typename T>
	using FunctionInfo		= _ctime_hidden_::_FunctionInfo< T >;
	
	template <typename T>
	using ResultOf			= typename FunctionInfo< T >::result;

}	// CompileTime
}	// GX_STL
