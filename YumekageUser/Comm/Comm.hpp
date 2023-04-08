#pragma once
#include "..\Common.hpp"

namespace Comm
{
	bool Initialize( );
	bool Destroy( );

	bool WhitelistCurrentThread( );
	uint64_t InitializeHiddenPages( );
}