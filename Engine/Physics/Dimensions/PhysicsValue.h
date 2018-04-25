// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "Engine/Physics/Dimensions/PhysicsValueUtils.h"

namespace GXPhysics
{

	//
	// Physics Value
	//

	template <typename ValueType,
			  typename Dimensions,
			  typename ValueScale
			 >
	struct PhysicsValue : public CompileTime::CopyQualifiers< ValueType >
	{
		STATIC_ASSERT( CompileTime::IsFloat<ValueType> and
					   CompileTime::IsScalar<ValueType> );

	// types
	public:	
		using Value_t		= ValueType;
		using Dimensions_t	= Dimensions;	// PhysDimList
		using ValueScale_t	= ValueScale;	// PhysicsDimensionScale::BaseConversion
		using Self			= PhysicsValue< Value_t, Dimensions_t, ValueScale_t >;


	private:
		template <typename D, typename S>
		struct _Add {
			using Right			= PhysicsValue< Value_t, D, S >;
			using conv_add_op_t	= typename ValueScale_t::template _Add4< typename Right::ValueScale_t >;
			using type			= PhysicsValue< Value_t,
										typename Dimensions_t::template Add< typename Right::Dimensions_t >,
										typename conv_add_op_t::type >;

			static Value_t Get (const Value_t &left, const Value_t &right) {
				return conv_add_op_t::Get( left, right );
			}
		};
		
		template <typename D, typename S>
		struct _Sub {
			using Right			= PhysicsValue< Value_t, D, S >;
			using conv_sub_op_t	= typename ValueScale_t::template _Sub4< typename Right::ValueScale_t >;
			using type			= PhysicsValue< Value_t,
										typename Dimensions_t::template Sub< typename Right::Dimensions_t >,
										typename conv_sub_op_t::type >;

			static Value_t Get (const Value_t &left, const Value_t &right) {
				return conv_sub_op_t::Get( left, right );
			}
		};
		
		template <typename D, typename S>
		struct _Mul {
			using Right			= PhysicsValue< Value_t, D, S >;
			using conv_mul_op_t	= typename ValueScale_t::template _Mul4< typename Right::ValueScale_t >;
			using type			= PhysicsValue< Value_t,
										typename Dimensions_t::template Mul< typename Right::Dimensions_t >,
										typename conv_mul_op_t::type >;

			static Value_t Get (const Value_t &left, const Value_t &right) {
				return conv_mul_op_t::Get( left, right );
			}
		};
		
		template <typename D, typename S>
		struct _Div {
			using Right			= PhysicsValue< Value_t, D, S >;
			using conv_div_op_t	= typename ValueScale_t::template _Div4< typename Right::ValueScale_t >;
			using type			= PhysicsValue< Value_t,
										typename Dimensions_t::template Div< typename Right::Dimensions_t >,
										typename conv_div_op_t::type >;

			static Value_t Get (const Value_t &left, const Value_t &right) {
				return conv_div_op_t::Get( left, right );
			}
		};
		
		template <isize PowNum, isize PowDenom = 1>
		struct _Pow {
			using pow_t	= typename CompileTime::Fractional32< PowNum, PowDenom >;
			using type	= PhysicsValue< Value_t,
								typename Dimensions_t::template Power< pow_t >,
								typename ValueScale_t::template Pow< pow_t > >;
		};

		template <typename NewValueType>
		struct _NewConv {
			using type	= typename PhysicsDimensionScale< NewValueType >::template 
							BaseConversion< ValueScale_t::Scale::MANTISSA, ValueScale_t::Scale::EXPONENT,
											ValueScale_t::Power::N, ValueScale_t::Power::D >;
		};

		using SelfInversed	= PhysicsValue< ValueType, typename Dimensions_t::Inverse,
											typename ValueScale_t::Inverse >;


	// variables
	private:
		Value_t		_value;


	// methods
	public:
		constexpr PhysicsValue (GX_DEFCTOR) : _value(0) {}
	
		explicit
		constexpr PhysicsValue (Value_t value) : _value(value) {}

		constexpr PhysicsValue (const Self &other) : _value(other.ref()) {}
		
		template <typename S>
		PhysicsValue (const PhysicsValue< Value_t, Dimensions_t, S > &other) :
			_value(other.template ToScale< ValueScale_t >().ref())
		{}

		//Value_t &		ref ()									{ return _value; }
		Value_t const &	ref ()							const	{ return _value; }

		CHECKRES Value_t Get ()							const	{ return ValueScale_t::Func::Get( _value ); }

		CHECKRES Self	operator -  ()					const	{ return Self( -_value ); }

				 Self &	operator =  (const Self &right)			{ _value = right.ref();  return *this; }

		CHECKRES bool	IsZero ()						const	{ return GXMath::IsZero( _value ); }

		CHECKRES bool	operator == (const Self &right)	const	{ return ( _value == right.ref() ); }
		CHECKRES bool	operator != (const Self &right)	const	{ return not ( *this == right ); }
		CHECKRES bool	operator >  (const Self &right)	const	{ return _value >  right.ref(); }
		CHECKRES bool	operator >= (const Self &right)	const	{ return _value >= right.ref(); }
		CHECKRES bool	operator <  (const Self &right)	const	{ return _value <  right.ref(); }
		CHECKRES bool	operator <= (const Self &right)	const	{ return _value <= right.ref(); }

				 Self &	operator += (const Self &right)			{ _value += right.ref();  return *this; }
				 Self &	operator -= (const Self &right)			{ _value -= right.ref();  return *this; }

		CHECKRES Self	operator +  (const Self &right)	const	{ return Self( _value + right.ref() ); }
		CHECKRES Self	operator -  (const Self &right)	const	{ return Self( _value - right.ref() ); }

				 Self &	operator *= (Value_t right)				{ _value *= right;  return *this; }
				 Self &	operator /= (Value_t right)				{ _value /= right;  return *this; }

		CHECKRES Self	operator *  (Value_t right)		const	{ return Self( _value * right ); }
		CHECKRES Self	operator /  (Value_t right)		const	{ return Self( _value / right ); }

		
		template <typename D, typename S>
		CHECKRES auto  operator +  (const PhysicsValue<Value_t,D,S> &right) const
		{
			using add_op	= _Add< D, S >;
			using Result_t	= typename add_op::type;

			return Result_t( add_op::Get( _value, right.ref() ) );
		}
		
		template <typename D, typename S>
		CHECKRES auto  operator -  (const PhysicsValue<Value_t,D,S> &right) const
		{
			using sub_op	= _Sub< D, S >;
			using Result_t	= typename sub_op::type;

			return Result_t( sub_op::Get( _value, right.ref() ) );
		}
		
		template <typename D, typename S>
		CHECKRES auto  operator *  (const PhysicsValue<Value_t,D,S> &right) const
		{
			using mul_op	= _Mul< D, S >;
			using Result_t	= typename mul_op::type;

			return Result_t( mul_op::Get( _value, right.ref() ) );
		}
		
		template <typename D, typename S>
		CHECKRES auto  operator /  (const PhysicsValue<Value_t,D,S> &right) const
		{
			using div_op	= _Div< D, S >;
			using Result_t	= typename div_op::type;

			return Result_t( div_op::Get( _value, right.ref() ) );
		}


		CHECKRES friend Self  operator * (Value_t left, const Self &right)
		{
			return Self( left * right.ref() );
		}
	

		CHECKRES friend SelfInversed  operator / (Value_t left, const Self &right)
		{
			return SelfInversed( left / right.ref() );
		}

		
		template <isize PowNum, isize PowDenom>
		CHECKRES auto  Pow () const
		{
			using Result_t	= typename _Pow< PowNum, PowDenom >::type;
			using pow_t		= typename CompileTime::Fractional32< PowNum, PowDenom >;
			using Float_t	= typename CompileTime::NearFloat::FromType<ValueType>;

			return Result_t( GXMath::Pow( _value, pow_t::template ToFloat< Float_t >() ) );
		}
		

		template <isize Power>
		CHECKRES auto  Pow () const
		{
			using Float_t	= typename CompileTime::NearFloat::FromType<ValueType>;

			return typename _Pow< Power >::type( GXMath::Pow( _value, Float_t( Power ) ) );
		}


		CHECKRES auto  Square () const
		{
			return Pow< 2 >();
		}


		CHECKRES auto  Sqrt () const
		{
			return Pow< 1, 2 >();
		}


		CHECKRES SelfInversed  Inverse () const
		{
			return Value_t(1) / (*this);
		}


		Self &	SetMax (const Self &right)
		{
			_value = Max( _value, right._value );
			return *this;
		}


		Self &	SetMin (const Self &right)
		{
			_value = Min( _value, right._value );
			return *this;
		}


		CHECKRES String ToString () const
		{
			return String().FormatF( Get() ) << '[' << Dimensions_t::ToString("*") << ']';
		}


		CHECKRES String ToDebugString () const
		{
			return String().FormatF( ref() ) << " * " << ValueScale_t::ToString() << " [" << Dimensions_t::ToString("*") << ']';
		}


		template <typename T>
		CHECKRES auto  Convert () const
		{
			return PhysicsValue< T, Dimensions_t, typename _NewConv<T>::type >( T( _value ) );
		}


		template <typename T>
		CHECKRES T  To () const
		{
			STATIC_ASSERT( Dimensions_t::template Equal< typename T::Dimensions_t >::value );

			using main_value_t	= typename CompileTime::GenType< Value_t, typename T::Value_t >;
			using scale1_t		= typename ValueScale_t::template To< main_value_t >;
			using scale2_t		= typename T::ValueScale_t::template To< main_value_t >;
			using div_op_t		= typename scale1_t::template _Div4< scale2_t >;

			return T( (typename T::Value_t) div_op_t::Get( _value, main_value_t(1) ) );
		}


		template <typename ToValueScale>
		CHECKRES PhysicsValue< Value_t, Dimensions_t, ToValueScale >  ToScale () const
		{
			return PhysicsValue< Value_t, Dimensions_t, ToValueScale >(
						Get() / ToValueScale::Func::Get( Value_t(1) ) );
		}
	};
	


	//
	// Non-Dimensional Physics Value
	//

	template <typename ValueType,
			  typename ValueScale
			 >
	struct PhysicsValue< ValueType, DefaultPhysicsDimensionsList::CreateNonDimensional, ValueScale > :
				public CompileTime::CopyQualifiers< ValueType >
	{
	// types
	public:
		using Value_t		= ValueType;
		using Dimensions_t	= DefaultPhysicsDimensionsList::CreateNonDimensional;
		using ValueScale_t	= ValueScale;
		using Self			= PhysicsValue< Value_t, Dimensions_t, ValueScale_t >;
		
		STATIC_ASSERT( Dimensions_t::IsNonDimensional::value );

	private:
		using SelfInversed	= PhysicsValue< ValueType, typename Dimensions_t::Inverse,
											typename ValueScale_t::Inverse >;


	// variables
	private:
		Value_t		_value;


	// methods
	public:
		constexpr PhysicsValue (GX_DEFCTOR) : _value(0) {}
	
		explicit
		constexpr PhysicsValue (Value_t value) : _value(value) {}

		constexpr PhysicsValue (const Self &other) : _value(other.ref()) {}
		

		//Value_t &		ref ()									{ return _value; }
		Value_t const &	ref ()							const	{ return _value; }

		CHECKRES Value_t Get ()							const	{ return ValueScale_t::Func::Get( _value ); }

		CHECKRES operator Value_t ()					const	{ return Get(); }

				 Self &	operator =  (const Self &right)			{ _value = right.ref();  return *this; }

		CHECKRES bool	operator == (const Self &right)	const	{ return ( _value == right.ref() ); }
		CHECKRES bool	operator != (const Self &right)	const	{ return not ( *this == right ); }
		CHECKRES bool	operator >  (const Self &right)	const	{ return _value >  right.ref(); }
		CHECKRES bool	operator >= (const Self &right)	const	{ return _value >= right.ref(); }
		CHECKRES bool	operator <  (const Self &right)	const	{ return _value <  right.ref(); }
		CHECKRES bool	operator <= (const Self &right)	const	{ return _value <= right.ref(); }

				 Self &	operator += (const Self &right)			{ _value += right.ref();  return *this; }
				 Self &	operator -= (const Self &right)			{ _value -= right.ref();  return *this; }
		
		CHECKRES Self	operator +  (const Self &right)	const	{ return Self( _value + right.ref() ); }
		CHECKRES Self	operator -  (const Self &right)	const	{ return Self( _value - right.ref() ); }

				 Self &	operator *= (Value_t right)				{ _value *= right;  return *this; }
				 Self &	operator /= (Value_t right)				{ _value /= right;  return *this; }
		
		CHECKRES Self	operator *  (Value_t right)		const	{ return Self( _value * right ); }
		CHECKRES Self	operator /  (Value_t right)		const	{ return Self( _value / right ); }


		CHECKRES friend Self	operator * (Value_t left, const Self &right)
		{
			return Self( left * right.ref() );
		}
	
		CHECKRES friend SelfInversed	operator / (Value_t left, const Self &right)
		{
			return SelfInversed( left / right.ref() );
		}


		template <isize PowNum, isize PowDenom>
		CHECKRES Value_t  Pow () const
		{
			using pow_t		= typename CompileTime::Fractional32< PowNum, PowDenom >;
			using Float_t	= typename CompileTime::NearFloat::FromType< Value_t >;

			return GXMath::Pow( Get(), pow_t::template ToFloat< Float_t >() );
		}
		

		template <isize Power>
		CHECKRES Value_t  Pow () const
		{
			return GXMath::Pow< Power >( Get() );
		}


		CHECKRES Value_t  Square () const
		{
			return GXMath::Square( Get() );
		}


		CHECKRES Value_t  Sqrt () const
		{
			return GXMath::Sqrt( Get() );
		}


		CHECKRES SelfInversed  Inverse () const
		{
			return SelfInversed( Value_t(1) / _value );
		}


		CHECKRES String ToString () const
		{
			return String().FormatF( Get() ) << " []";
		}


		CHECKRES String ToDebugString () const
		{
			return String().FormatF( ref() ) << " * (" << ValueScale_t::Scale::ToString()
					<< ")^" << ValueScale_t::Power::ToString() << " []";
		}
	};
	
}	// GXPhysics


namespace GX_STL
{
namespace CompileTime
{
/*
=================================================
	TypeInfo
=================================================
*/
	template <typename ValueType,
			  typename Dimensions,
			  typename ValueScale
			 >
	struct TypeInfo < GXPhysics::PhysicsValue< ValueType, Dimensions, ValueScale > >
	{
	private:
		typedef CompileTime::TypeInfo<ValueType>	_value_type_info;

	public:
		using type			= GXPhysics::PhysicsValue< ValueType, Dimensions, ValueScale >;
		using inner_type	= ValueType;
		
		template <typename OtherType>
		using CreateWith =  GXPhysics::PhysicsValue< OtherType, Dimensions, ValueScale >;

		enum {
			FLAGS	= (int)_value_type_info::FLAGS | (int)GX_STL::CompileTime::_ctime_hidden_::WRAPPER,
		};

		static constexpr type	Max()		{ return type( _value_type_info::Max() ); }
		static constexpr type	Min()		{ return type( _value_type_info::Min() ); }
		static			 type	Inf()		{ return type( _value_type_info::Inf() ); }
		static			 type	NaN()		{ return type( _value_type_info::NaN() ); }
		
		static constexpr type	Epsilon()	{ return type( _value_type_info::Epsilon() ); }
		static constexpr uint	SignBit()	{ return _value_type_info::SignBit(); }
		static constexpr uint	Count()		{ return _value_type_info::Count(); }
	};

}	// CompileTime


namespace GXTypes
{
/*
=================================================
	Hash
=================================================
*/
	template <typename ValueType,
			  typename Dimensions,
			  typename ValueScale
			 >
	struct Hash< GXPhysics::PhysicsValue< ValueType, Dimensions, ValueScale > >
	{
		CHECKRES HashResult  operator () (const GXPhysics::PhysicsValue< ValueType, Dimensions, ValueScale > &x) const
		{
			return HashOf( x.ref() );
		}
	};
	
/*
=================================================
	ToStringImpl (PhysicsValue)
=================================================
*/
	template <typename T, typename D, typename S>
	CHECKRES inline String  ToStringImpl (const GXPhysics::PhysicsValue<T,D,S> &value)
	{
		return value.ToString();
	}

}	// GXTypes
}	// GX_STL
