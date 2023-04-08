#include "Common.hpp"
#include "Comm/Comm.hpp"

int main( )
{
	printf( "[*] Yumekage Usermode Demo\n\n" );

	// Initialize the communication with the driver.
	if ( !Comm::Initialize( ) )
	{
		printf( "[-] Failed to initialize comms.\n" );
		Sleep( 5000 );
		return 0;
	}

	printf( "[+] Initialized comms.\n" );

	// Get the hidden page's address.
	uint64_t Address = Comm::InitializeHiddenPages( );
	if ( !Address )
	{
		printf( "[-] Failed to initialize hidden pages.\n" );
		Sleep( 5000 );
		return 0;
	}

	printf( "[+] Hidden pages created at 0x%llX\n", Address );

	// Use IsBadReadPtr to validate the hidden page.
	printf( "[*] Trying to access page before whitelisting: %s\n", IsBadReadPtr( (void*)Address, 1 ) ? "Failed" : "Success" );

	// Whitelist the current thread.
	if ( !Comm::WhitelistCurrentThread( ) )
	{
		printf( "[-] Failed to whitelist thread.\n" );
		Sleep( 5000 );
		return 0;
	}

	// Use IsBadReadPtr to validate the hidden page.
	printf( "[*] Trying to access page after whitelisting: %s\n", IsBadReadPtr( (void*)Address, 1 ) ? "Failed" : "Success" );

	// Show that the address is able to be read and written to.
	for(int i = 0; i <= 5; i++ )
	{
		Sleep( 50 );

		// Force the compiler to not optimize this out and reference 'i' for our print by using "volatile".
		*(volatile int*)Address = i;
		printf( "[*] Read and written index %d\n", *(volatile int*)Address );
	}

	printf( "[*] Done exitting...\n" );

	// Disconnect from the driver.
	Comm::Destroy( );
	Sleep( -1 );
	return 0;
}