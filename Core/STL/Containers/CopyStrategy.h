// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'
/*
	Construct	- call contructor
	Destruct	- call destructor
	Create		- call default constructors for elements
	Destroy		- call destructors for elements
	Copy		- copy elements (copy-ctor)
	Move		- move external objects (move-ctor)
	Replace		- move internal elements (move-ctor + dtor)
*/

#pragma once

#include "Core/STL/Memory/MemFunc.h"
#include "Core/STL/Memory/PlacementNew.h"
#include "Core/STL/Dimensions/ByteAndBit.h"

namespace GX_STL
{
namespace GXTypes
{

	struct CopyStrategy
	{
	
		//
		// Copy Strategy with MemMove
		//

		template <typename T>
		struct MemCopyWithCtor : public Noninstancable
		{
			STATIC_ASSERT(	CompileTime::IsMemCopyAvailable<T> and
							not CompileTime::IsNoncopyable<T> );

			// create default elements
			static void Create (T *ptr, const usize count) noexcept
			{
				for (usize i = 0; i < count; ++i) {
					UnsafeMem::PlacementNew<T>( ptr + i );
				}
			}

			static void Destroy (T *ptr, const usize count) noexcept
			{
				for (usize i = 0; i < count; ++i) {
					PlacementDelete( ptr[i] );
				}
			}

			// copy elements from one memblock to other memblock
			static void Copy (T *to, const T * const from, const usize count) noexcept
			{
				for (usize i = 0; i < count; ++i) {
					UnsafeMem::PlacementNew<T>( to+i, from[i] );
				}
			}

			// move elements from one memblock to other memblock
			static void Move (T *to, T *from, const usize count) noexcept
			{
				for (usize i = 0; i < count; ++i) {
					UnsafeMem::PlacementNew<T>( to+i, RVREF( from[i] ) );
				}
			}
			
			// replace elements inside memory block
			static void Replace (T *to, T *from, const usize count, bool inSingleMemBlock = false) noexcept
			{
				UnsafeMem::MemMove( to, from, SizeOf<T> * count );

				// clear old values after replace
				GX_UNUSED( inSingleMemBlock );
				DEBUG_ONLY(
				if ( inSingleMemBlock ) {
					for (T* t = from; t < from + count; ++t) {
						if (t < to or t >= to + count) {
							UnsafeMem::ZeroMem( t, SizeOf<T> );
						}
					}
				})
			}

			static void ReplaceRev (T *to, T *from, const usize count, bool inSingleMemBlock = false) noexcept
			{
				// if used memmove then memory blocks can intersects
				return Replace( to, from, count, inSingleMemBlock );
			}
		};



		//
		// Copy Strategy with Copy Ctor and Move Ctor
		//

		template <typename T>
		struct CopyAndMoveCtor : public Noninstancable
		{
			STATIC_ASSERT( (CompileTime::IsCtorAvailable<T> or
							CompileTime::IsDtorAvailable<T>) and
							not CompileTime::IsNoncopyable<T> );
			
			// create default elements
			static void Create (T *ptr, const usize count) noexcept
			{
				for (usize i = 0; i < count; ++i) {
					UnsafeMem::PlacementNew<T>( ptr + i );
				}
			}

			static void Destroy (T *ptr, const usize count) noexcept
			{
				for (usize i = 0; i < count; ++i) {
					PlacementDelete( ptr[i] );
				}
			}
			
			// copy elements from one memblock to other memblock
			static void Copy (T *to, const T * const from, const usize count) noexcept
			{
				for (usize i = 0; i < count; ++i) {
					UnsafeMem::PlacementNew<T>( to+i, from[i] );
				}
			}
			
			// move elements from one memblock to other memblock
			static void Move (T *to, T *from, const usize count) noexcept
			{
				for (usize i = 0; i < count; ++i) {
					UnsafeMem::PlacementNew<T>( to+i, RVREF( from[i] ) );
				}
			}
			
			// replace elements inside memory block
			static void Replace (T *to, T *from, const usize count, bool inSingleMemBlock = false) noexcept
			{
				GX_UNUSED( inSingleMemBlock );

				for (usize i = 0; i < count; ++i) {
					UnsafeMem::PlacementNew<T>( to+i, RVREF( from[i] ) );
					PlacementDelete( from[i] );	// TODO: is it needed?
				}
			}
			
			static void ReplaceRev (T *to, T *from, const usize count, bool inSingleMemBlock = false) noexcept
			{
				GX_UNUSED( inSingleMemBlock );

				for (usize i = count-1; i < count; --i) {
					UnsafeMem::PlacementNew<T>( to+i, RVREF( from[i] ) );
					PlacementDelete( from[i] );	// TODO: is it needed?
				}
			}
		};



		//
		// Copy Strategy with MemCopy & MemMove without Ctors
		//

		template <typename T>
		struct MemCopyWithoutCtor : public Noninstancable
		{
			STATIC_ASSERT(	CompileTime::IsMemCopyAvailable<T>	 and
							CompileTime::IsZeroMemAvailable<T>	 and
							not CompileTime::IsCtorAvailable<T>  and
							not CompileTime::IsDtorAvailable<T>  and
							not CompileTime::IsNoncopyable<T> );
			
			// create default elements
			static void Create (T *ptr, const usize count) noexcept
			{
				UnsafeMem::ZeroMem( ptr, SizeOf<T> * count );
			}

			static void Destroy (T *ptr, const usize count) noexcept
			{
				DEBUG_ONLY( UnsafeMem::ZeroMem( ptr, SizeOf<T> * count ) );
			}
			
			// copy elements from one memblock to other memblock
			static void Copy (T *to, const T * const from, const usize count) noexcept
			{
				UnsafeMem::MemCopy( to, from, SizeOf<T> * count );
			}
			
			// move elements from one memblock to other memblock
			static void Move (T *to, T * from, const usize count) noexcept
			{
				UnsafeMem::MemMove( to, from, SizeOf<T> * count );
			}
			
			// replace elements inside memory block
			static void Replace (T *to, T *from, const usize count, bool inSingleMemBlock = false) noexcept
			{
				Move( to, from, count );

				// clear old values after replace
				GX_UNUSED( inSingleMemBlock );
				DEBUG_ONLY(
				if ( inSingleMemBlock ) {
					for (T* t = from; t < from + count; ++t) {
						if (t < to or t >= to + count) {
							UnsafeMem::ZeroMem( t, SizeOf<T> );
						}
					}
				})
			}

			static void ReplaceRev (T *to, T *from, const usize count, bool inSingleMemBlock = false) noexcept
			{
				// if used memmove then memory blocks can intersects
				return Replace( to, from, count, inSingleMemBlock );
			}
		};

	};

	
	//
	// Auto Detect Copy Strategy
	//

	template <typename T>
	struct AutoDetectCopyStrategy final : public Noninstancable
	{
		STATIC_ASSERT( not CompileTime::IsNoncopyable<T> );

		static const bool _is_fastcopy	= CompileTime::IsMemCopyAvailable<T>;
		static const bool _is_pod		= _is_fastcopy and (not CompileTime::IsCtorAvailable<T>) and (not CompileTime::IsDtorAvailable<T>);

		typedef CompileTime::SwitchType< _is_pod, CopyStrategy::template MemCopyWithoutCtor<T>,
							CompileTime::SwitchType< _is_fastcopy, CopyStrategy::template MemCopyWithCtor<T>,
																	CopyStrategy::template CopyAndMoveCtor<T> > >		type;
	};


}	// GXTypes
}	// GX_STL
