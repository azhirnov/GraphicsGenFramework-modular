// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "CoreTests/STL/Common.h"

using namespace GX_STL;
using namespace GX_STL::GXTypes;
using namespace GX_STL::GXMath;

typedef Transformation< real >		Transform_t;


static void Transform_Inverse ()
{
	const Transform_t	a(	real3( 1.0f, 2.0f, 3.0f ), 
							quat().Rotation( real3( 45.0f, 0.0f, 10.0f ).To< DegreesVec<real,3> >().To< RadiansVec<real,3> >() ),
							2.0f );

	const Transform_t	b = a.Inversed().Inversed();

	TEST( a == b );
}


static void Transform_AddInverse ()
{
	const Transform_t	a(	real3( 1.0f, 2.0f, 3.0f ),
							quat().Rotation( real3( 45.0f, 0.0f, 10.0f ).To< DegreesVec<real,3> >().To< RadiansVec<real,3> >() ),
							2.0f );

	const Transform_t	b = a + a.Inversed();

	TEST( b.IsZero() );
}


static void Transform_MulVec ()
{
	const Transform_t	tr(	real3( 1.0f, 2.0f, 3.0f ),
							quat().Rotation( real3( 45.0f, 0.0f, 10.0f ).To< DegreesVec<real,3> >().To< RadiansVec<real,3> >() ),
							2.0f );

	Transform_t::Mat4_t	mat;	tr.GetMatrix( OUT mat );

	real3 const			point0( -1.0f, -1.0f, -1.0f );
	real3 const			point1( 1.0f, 1.0f, 1.0f );

	real3	mat_point0	= (mat * real4( point0, 1.0f )).xyz();
	real3	mat_point1	= (mat * real4( point1, 1.0f )).xyz();

	real3	tr_point0	= tr.Transform( point0 );
	real3	tr_point1	= tr.Transform( point1 );

	TEST( All( mat_point0 == tr_point0 ) );
	TEST( All( mat_point1 == tr_point1 ) );
}


extern void Test_Math_Transform ()
{
	Transform_Inverse();
	Transform_AddInverse();
	Transform_MulVec();
}
