// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "Core/STL/Math/Mathematics.h"
#include "Core/STL/Math/Vec.h"
#include "Core/STL/Math/Color/Half.h"

namespace GX_STL
{
namespace GXMath
{

#	define _VEC_OPERATOR( _op_ ) \
		template <typename T> Self&		operator _op_##= (const T& right)		{ Set( Vec3_t(*this) _op_ right );  return *this; } \
		template <typename T> Self		operator _op_  (const T& right) const	{ return Self( Vec3_t(*this) _op_ right ); }



	//
	// R11 G11 B10 Float Vector
	//

	template <typename FT>
	struct TR11G11B10F : public CompileTime::CopyQualifiers< FT >
	{
	// types
	public:
		typedef TR11G11B10F<FT>		Self;
		typedef Vec<FT,3>			Vec3_t;
		typedef FT					Float_t;

		struct TBits
		{
			// Blue //
			uint	b_m	: 5;
			uint	b_e	: 5;
			// Green //
			uint	g_m	: 6;
			uint	g_e	: 5;
			// Red //
			uint	r_m	: 6;
			uint	r_e	: 5;
		};

		struct TRGB
		{
			uint	b	: 10;
			uint	g	: 11;
			uint	r	: 11;
		};


	// variables
	private:
		union {
			TBits	_bits;
			TRGB	_rgb;
			uint	_value;
		};


	// methods
	public:
		// constructors
		TR11G11B10F (GX_DEFCTOR): _value(0) {}

		template <typename RT, typename GT, typename BT>
		TR11G11B10F (const RT& R, const GT& G, const BT& B)
		{
			Set( Float_t(R), Float_t(G), Float_t(B) );
		}

		template <typename VT>
		TR11G11B10F (const VT& vec)
		{
			Set( Float_t(vec.x), Float_t(vec.y), Float_t(vec.z) );
		}


		// set/get
		void  Set (const Vec3_t &v);
		void  Set (Float_t R, Float_t G, Float_t B);
		void  Get (OUT Float_t& R, OUT Float_t& G, OUT Float_t& B) const;
		void  Get (OUT Vec3_t &v) const;

		void  SetR (Float_t R);
		void  SetG (Float_t G);
		void  SetB (Float_t B);

		Float_t R () const;
		Float_t G () const;
		Float_t B () const;

		uint  RBits () const				{ return _rgb.r; }
		uint  GBits () const				{ return _rgb.g; }
		uint  BBits () const				{ return _rgb.b; }


		// type cast
		operator const Vec3_t () const		{ Vec3_t  ret;  Get(ret);  return ret; }


		// unary operators
		Self	operator - () const;
		Self	operator ~ () const;

		// binary operators
		_VEC_OPERATOR( +  );
		_VEC_OPERATOR( -  );
		_VEC_OPERATOR( *  );
		_VEC_OPERATOR( /  );
		_VEC_OPERATOR( %  );
		_VEC_OPERATOR( &  );
		_VEC_OPERATOR( |  );
		_VEC_OPERATOR( ^  );
		_VEC_OPERATOR( >> );
		_VEC_OPERATOR( << );


		// check
		STATIC_ASSERT( sizeof(uint) == 4 and sizeof(TRGB) == sizeof(uint), "incorrect types" );
	};


	typedef TR11G11B10F< half >		r11g11b10f_t;

	
	template <typename FT>
	inline void TR11G11B10F<FT>::Set (const Vec3_t &v)
	{
		Set( v.x, v.y, v.z );
	}

	
	template <typename FT>
	inline void TR11G11B10F<FT>::Set (Float_t R, Float_t G, Float_t B)
	{
#	if 0
		Float_t	f;
		STATIC_ASSERT( sizeof(f) >= sizeof(float) );

		f = R;
		ASSERT( f._bits.s == 0 and "only unsigned value supported" );
		_bits.r_e = f._bits.e - (127 - 15);
		_bits.r_m = f._bits.m >> (23-6);

		f = G;
		ASSERT( f._bits.s == 0 and "only unsigned value supported" );
		_bits.g_e = f._bits.e - (127 - 15);
		_bits.g_m = f._bits.m >> (23-6);

		f = B;
		ASSERT( f._bits.s == 0 and "only unsigned value supported" );
		_bits.b_e = f._bits.e - (127 - 15);
		_bits.b_m = f._bits.m >> (23-5);
#	else
		// with NaN & Inf

		half	h;

		h = R;
		ASSERT( h._bits.s == 0 and "only unsigned value supported" );
		_bits.r_e = h._bits.e;
		_bits.r_m = h._bits.m >> (10-6);
	
		h = G;
		ASSERT( h._bits.s == 0 and "only unsigned value supported" );
		_bits.g_e = h._bits.e;
		_bits.g_m = h._bits.m >> (10-6);

		h = B;
		ASSERT( h._bits.s == 0 and "only unsigned value supported" );
		_bits.b_e = h._bits.e;
		_bits.b_m = h._bits.m >> (10-5);
#	endif
	}

	
	template <typename FT>
	inline void TR11G11B10F<FT>::Get (Float_t& R, Float_t& G, Float_t& B) const
	{
#	if 0
		Float_t	f;
		STATIC_ASSERT( sizeof(f) >= sizeof(float) );
	
		f._bits.m = _bits.r_m << (23-6);
		f._bits.e = _bits.r_e + (127 - 15);
		R = f;

		f._bits.m = _bits.g_m << (23-6);
		f._bits.e = _bits.g_e + (127 - 15);
		G = f;

		f._bits.m = _bits.b_m << (23-5);
		f._bits.e = _bits.b_e + (127 - 15);
		B = f;
#	else
		// with NaN & Inf
		half	h;

		h._bits.m = _bits.r_m << (10-6);
		h._bits.e = _bits.r_e;
		R = h;
	
		h._bits.m = _bits.g_m << (10-6);
		h._bits.e = _bits.g_e;
		G = h;
	
		h._bits.m = _bits.b_m << (10-5);
		h._bits.e = _bits.b_e;
		B = h;
#	endif
	}

	
	template <typename FT>
	inline void TR11G11B10F<FT>::Get (Vec3_t &v) const
	{
		Get( v.x, v.y, v.z );
	}

	
	template <typename FT>
	inline FT TR11G11B10F<FT>::R () const
	{
		Float_t	f;
		f._bits.m = _bits.r_m << (23-6);
		f._bits.e = _bits.r_e + (127 - 15);
		return f;
	}

	
	template <typename FT>
	inline FT TR11G11B10F<FT>::G () const
	{
		Float_t	f;
		f._bits.m = _bits.g_m << (23-6);
		f._bits.e = _bits.g_e + (127 - 15);
		return f;
	}

	
	template <typename FT>
	inline FT TR11G11B10F<FT>::B () const
	{
		Float_t	f;
		f._bits.m = _bits.b_m << (23-5);
		f._bits.e = _bits.b_e + (127 - 15);
		return f;
	}
	
	
	template <typename FT>
	inline void TR11G11B10F<FT>::SetR (Float_t R)
	{
		Float_t	f(R);

		ASSERT( f._bits.s == 0 and "only unsigned value supported" );
		_bits.r_e = f._bits.e - (127 - 15);
		_bits.r_m = f._bits.m >> (23-6);
	}

	
	template <typename FT>
	inline void TR11G11B10F<FT>::SetG (Float_t G)
	{
		Float_t	f(G);

		ASSERT( f._bits.s == 0 and "only unsigned value supported" );
		_bits.g_e = f._bits.e - (127 - 15);
		_bits.g_m = f._bits.m >> (23-6);
	}

	
	template <typename FT>
	inline void TR11G11B10F<FT>::SetB (Float_t B)
	{
		Float_t	f(B);

		ASSERT( f._bits.s == 0 and "only unsigned value supported" );
		_bits.b_e = f._bits.e - (127 - 15);
		_bits.b_m = f._bits.m >> (23-5);
	}

	
	template <typename FT>
	inline TR11G11B10F<FT> TR11G11B10F<FT>::operator - () const
	{
		WARNING( "only unsigned values supported" );
		return *this;
	}

	
	template <typename FT>
	inline TR11G11B10F<FT> TR11G11B10F<FT>::operator ~ () const
	{
		Self	ret;
		ret._value = ~_value;
		return ret;
	}


#	undef _VEC_OPERATOR


}	// GXMath

namespace GXTypes
{
	
	template <typename T>
	struct Hash< GXMath::TR11G11B10F<T> > : public Hash< typename GXMath::TR11G11B10F<T>::Vec3_t >
	{};

}	// GXTypes
}	// GX_STL
