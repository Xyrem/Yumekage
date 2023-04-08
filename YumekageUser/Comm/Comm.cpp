#include "Comm.hpp"

namespace Comm
{
	const DWORD WhitelistThreadCTL = CTL_CODE( FILE_DEVICE_UNKNOWN, 0xBAD, METHOD_BUFFERED, FILE_ANY_ACCESS );
	const DWORD InitializeCTL = CTL_CODE( FILE_DEVICE_UNKNOWN, 0xD00D, METHOD_BUFFERED, FILE_ANY_ACCESS );

	HANDLE hDriver;

	bool Initialize( )
	{
		hDriver = CreateFileA( "\\\\.\\Yumekage", GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0 );
		return hDriver != INVALID_HANDLE_VALUE;
	}

	bool Destroy( )
	{
		return CloseHandle( hDriver );
	}

	bool WhitelistCurrentThread( )
	{
		uint32_t Status = 0;
		if ( !DeviceIoControl( hDriver, WhitelistThreadCTL, 0, 0, &Status, sizeof( Status ), 0, 0 ) )
			return 0;

		return Status == 0x1BADD00D;
	}

	uint64_t InitializeHiddenPages( )
	{
		int PML4Index = 0;
		if ( !DeviceIoControl( hDriver, InitializeCTL, 0, 0, &PML4Index, sizeof( PML4Index ), 0, 0 ) )
			return 0;

		return uint64_t( PML4Index ) << 39;
	}
}