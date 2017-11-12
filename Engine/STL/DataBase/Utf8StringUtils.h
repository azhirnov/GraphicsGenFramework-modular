// Copyright �  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "Engine/STL/Containers/String.h"

#ifdef GX_SQLITE_DATABASE_ENABLED

namespace GX_STL
{
namespace GXDataBase
{

	//
	// UTF-8 String Utils
	//

	struct _STL_EXPORT_ Utf8StringUtils
	{
		static void EncodeToWindows1251 (StringCRef src, OUT String &dst);

		static usize Length (const char *s)
		{
			usize len = 0;
			while (*s)
			{
				len += ( (*s++) & 0xc0 ) != 0x80;
			}
			return len;
		}
	};


}	// GXDataBase
}	// GX_STL

#endif	// GX_SQLITE_DATABASE_ENABLED
