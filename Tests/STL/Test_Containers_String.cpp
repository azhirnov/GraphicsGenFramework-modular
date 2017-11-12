// Copyright �  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "Engine/STL/Engine.STL.h"

using namespace GX_STL;
using namespace GX_STL::GXTypes;
using namespace GX_STL::GXMath;


static void String_StartsWith_EndsWith ()
{
	const StringCRef	s0 = "123456789";
	const StringCRef	s1 = "QwErTyUiOp";

	ASSERT( s0.StartsWith( "123" ) );
	ASSERT( s0.EndsWith( "789" ) );

	ASSERT( s1.StartsWithIC( "qwer" ) );
	ASSERT( s1.EndsWithIC( "UIOP" ) );

	String	ss0( s0 );	ss0.EraseFromBack( s0.Length() );	ASSERT( ss0.Empty() );
}


static void String_LessThan_SortLikeInFileSystem ()
{
	struct SortComparator {
		bool operator () (const String &left, const String &right) const {
			return right.LessThan( left );
		}
	};

	Array< String >	arr;

	arr << "aaa" << "bbb" << "aaaa";

	Sort( arr );
	Sort( arr, SortComparator() );

	ASSERT( arr[0] == "aaa" );
	ASSERT( arr[1] == "aaaa" );
	ASSERT( arr[2] == "bbb" );
}


static void String_Find1 ()
{
	String	s1 = "12345\n6789\n";
	String	s2 = "\n";
	usize	p = 0;

	ASSERT( s1.Find( s2, OUT p, 0 ) );
	ASSERT( s1.Find( s2, OUT p, p+1 ) );

	p = 0;
	ASSERT( s1.Find( s2[0], OUT p, 0 ) );
	ASSERT( s1.Find( s2[0], OUT p, p+1 ) );
}


static void String_Find2 ()
{
	String	s1 = "1234aaaa56789aaa";
	String	s2 = "aaaa";
	usize	p = 0;

	ASSERT( s1.Find( s2, OUT p, 0 ) );
	ASSERT( not s1.Find( s2, OUT p, p+1 ) );
}


static void String_Erase ()
{
	String	s0 = "01234aaaa56789";
	s0.Erase( 0, 5 );
	ASSERT( s0 == "aaaa56789" );

	String	s1 = "0123456789abcdefghijklmnop";
	s1.Erase( 0, 21 );
	ASSERT( s1 == "lmnop" );
}


extern void Test_Containers_String ()
{
	String_StartsWith_EndsWith();
	String_LessThan_SortLikeInFileSystem();
	String_Find1();
	String_Find2();
	String_Erase();
}
