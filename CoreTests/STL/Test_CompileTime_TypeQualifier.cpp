// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#include "CoreTests/STL/Common.h"

using namespace GX_STL;
using namespace GX_STL::GXTypes;
using namespace GX_STL::CompileTime;

struct Test
{
	int memberVar = 0;
	
	const int memberConst = 1;

	int MemberFunc (int, float)					{ return 0; }
	int MemberFuncConst (int, float) const		{ return 0; }

	virtual void VirtMemberFunc (double d) = 0;
};

typedef int Test::*Test_memberVar;
typedef int Test::*Test_memberConst;
typedef int (Test::*Test_MemberFunc) (int, float);
typedef int (Test::*Test_MemberFuncConst) (int, float) const;
typedef int (Test::*Test_VirtMemberFunc) (double);
typedef int (*Test_StaticFunc) (float, double);

enum ETestEnum {};

struct PodTest : PODType
{};

struct FastCopyableTest : FastCopyable
{};

struct ComplexTest : TypeQualifier< ETypeQualifier::Def_Complex >
{};

struct ComplexTest2
{
	String	s;
};

class ComplexTest3
{
private:
	int i;

public:
	ComplexTest3 () : i(1) {}
};

struct NoncopyableTest : TypeQualifier< ETypeQualifier::Def_Noncopyable >
{};

struct UnknownQualTest : TypeQualifier< ETypeQualifier::Def_Complex >, NoncopyableTest, FastCopyableTest
{
	static const ETypeQualifier::type	__typeQualifierValue__ = ETypeQualifier::Def_SimplePOD;
};

struct UnknownQualTest2 : TypeQualifier< ETypeQualifier::Def_Complex >, NoncopyableTest, FastCopyableTest
{
	using FastCopyableTest::__typeQualifierValue__;
};

struct UnknownQualTest3 : InheritWithTypeQualifier< ETypeQualifier::Def_Complex, ComplexTest, NoncopyableTest, FastCopyableTest >
{};

struct CopyQualTest : CopyQualifiers< PODStruct, FastCopyable, PODType >
{
};

struct PodTypeNoQual
{
	int i;
	float f;
};

union UnionPodTypeNoQual
{
	int i;
	float f;
};


extern void Test_CompileTime_TypeQualifier ()
{
	STATIC_ASSERT( IsPOD<int> );
	STATIC_ASSERT( IsPOD< ETestEnum > );
	STATIC_ASSERT( IsPOD< Test_memberVar > );
	STATIC_ASSERT( IsPOD< Test_memberConst > );
	STATIC_ASSERT( IsPOD< Test_MemberFunc > );
	STATIC_ASSERT( IsPOD< Test_MemberFuncConst > );
	STATIC_ASSERT( IsPOD< Test_VirtMemberFunc > );
	STATIC_ASSERT( IsPOD< Test_StaticFunc > );
	STATIC_ASSERT( IsPOD< PodTypeNoQual > );
	STATIC_ASSERT( IsPOD< UnionPodTypeNoQual > );
	STATIC_ASSERT( not IsPOD< Test > );
	STATIC_ASSERT( IsPOD< const char[3] > );

	const ETypeQualifier::type val0 = _ctime_hidden_::_SwitchStrongerQualifier< ETypeQualifier::Def_SimplePOD, ETypeQualifier::Def_SimplePOD >::value;
	STATIC_ASSERT( val0 == ETypeQualifier::Def_SimplePOD );
	
	const ETypeQualifier::type val1 = _ctime_hidden_::_SwitchStrongerQualifier< ETypeQualifier::Def_SimplePOD, ETypeQualifier::Def_ComplexPOD >::value;
	STATIC_ASSERT( val1 == ETypeQualifier::Def_ComplexPOD );

	STATIC_ASSERT( _ctime_hidden_::_GetTypeQualifier<char>::value  == ETypeQualifier::Def_SimplePOD );
	STATIC_ASSERT( _ctime_hidden_::_GetTypeQualifier<int>::value   == ETypeQualifier::Def_SimplePOD );
	STATIC_ASSERT( _ctime_hidden_::_GetTypeQualifier<ulong>::value == ETypeQualifier::Def_SimplePOD );
	STATIC_ASSERT( _ctime_hidden_::_GetTypeQualifier<float>::value == ETypeQualifier::Def_SimplePOD );
	STATIC_ASSERT( _ctime_hidden_::_GetTypeQualifier<const char [4]>::value == ETypeQualifier::Def_SimplePOD );
	STATIC_ASSERT( _ctime_hidden_::_GetTypeQualifier<volatile const char [4]>::value == ETypeQualifier::Def_SimplePOD );
	
	STATIC_ASSERT( _ctime_hidden_::_GetTypeQualifier< MemoryContainer<char> >::value	== ETypeQualifier::Def_FastCopyable );
	STATIC_ASSERT( _ctime_hidden_::_GetTypeQualifier< String >::value					== ETypeQualifier::Def_FastCopyable );
	STATIC_ASSERT( _ctime_hidden_::_GetTypeQualifier< Array<int> >::value				== ETypeQualifier::Def_FastCopyable );
	STATIC_ASSERT( _ctime_hidden_::_GetTypeQualifier< ETestEnum >::value				== ETypeQualifier::Def_SimplePOD );

	STATIC_ASSERT(( _ctime_hidden_::_GetTypeQualifier< StaticMemoryContainer<int, 2> >::value	== ETypeQualifier::Def_FastCopyable ));
	STATIC_ASSERT(( _ctime_hidden_::_GetTypeQualifier< FixedSizeArray<int, 2> >::value			== ETypeQualifier::Def_FastCopyable ));
	STATIC_ASSERT(( _ctime_hidden_::_GetTypeQualifier< StaticString<8> >::value					== ETypeQualifier::Def_FastCopyable ));
	STATIC_ASSERT(( _ctime_hidden_::_GetTypeQualifier< FixedSizeSet<String, 4> >::value			== ETypeQualifier::Def_FastCopyable ));

	STATIC_ASSERT(( _ctime_hidden_::_GetTypeQualifier< GXTypes::Pair<StaticString<8>, int> >::value			== ETypeQualifier::Def_FastCopyable ));
	STATIC_ASSERT(( _ctime_hidden_::_GetTypeQualifier< Hash<int> >::value									== ETypeQualifier::Def_FastCopyable ));
	STATIC_ASSERT(( _ctime_hidden_::_GetTypeQualifier< FixedSizeHashMap<StaticString<8>, int, 5> >::value	== ETypeQualifier::Def_FastCopyable ));

	STATIC_ASSERT( _ctime_hidden_::_GetTypeQualifier< PodTest >::value			== ETypeQualifier::Def_SimplePOD );
	STATIC_ASSERT( _ctime_hidden_::_GetTypeQualifier< FastCopyableTest >::value	== ETypeQualifier::Def_FastCopyable );
	STATIC_ASSERT( _ctime_hidden_::_GetTypeQualifier< ComplexTest >::value		== ETypeQualifier::Def_Complex );
	STATIC_ASSERT( _ctime_hidden_::_GetTypeQualifier< ComplexTest2 >::value		== ETypeQualifier::Def_Complex );
	STATIC_ASSERT( _ctime_hidden_::_GetTypeQualifier< ComplexTest3 >::value		== ETypeQualifier::Def_Complex );
	STATIC_ASSERT( _ctime_hidden_::_GetTypeQualifier< NoncopyableTest >::value	== ETypeQualifier::Def_Noncopyable );

	STATIC_ASSERT( _ctime_hidden_::_GetTypeQualifier< UnknownQualTest >::value	== ETypeQualifier::Def_SimplePOD );
	STATIC_ASSERT( _ctime_hidden_::_GetTypeQualifier< UnknownQualTest2 >::value	== ETypeQualifier::Def_FastCopyable );
	STATIC_ASSERT( _ctime_hidden_::_GetTypeQualifier< UnknownQualTest3 >::value	== ETypeQualifier::Def_Complex );
	
	STATIC_ASSERT( IsCtorAvailable< FastCopyable > );
	STATIC_ASSERT( IsDtorAvailable< FastCopyable > );
	
	STATIC_ASSERT( IsCtorAvailable< CopyQualTest > );
	STATIC_ASSERT( IsDtorAvailable< CopyQualTest > );

	using PODUnion_t = GXTypes::Union< GXMath::float4, GXMath::uint4 >;
	STATIC_ASSERT( _ctime_hidden_::_GetTypeQualifier<PODUnion_t>::value  == ETypeQualifier::Def_ComplexPOD );

	//const auto ststr_type = _ctime_hidden_::_GetTypeQualifier< StaticMemoryContainer<char, 16> >::value;
	//STATIC_ASSERT( ststr_type	== ETypeQualifier::Def_FastCopyable );
}
