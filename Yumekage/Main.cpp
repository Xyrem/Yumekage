/*
*		File name:
*			Main.cpp
*
*		Use:
*			Sample driver for shadow regions.
*
*		Author:
*			Xyrem ( https://reversing.info | Xyrem@reversing.info )
*/

#include "Common.hpp"
#include "Utils/Utils.hpp"
#include "HyperDeceit.hpp"
#include "Paging/Paging.hpp"

#pragma comment(lib, "HyperDeceit.lib")

DynamicArray<void*> MemoryAllocations;
DynamicArray<PKTHREAD> WhitelistedThreads;

UNICODE_STRING YumekageDeviceName = RTL_CONSTANT_STRING( L"\\Device\\Yumekage" );
UNICODE_STRING YumekageDosSymlink = RTL_CONSTANT_STRING( L"\\DosDevices\\Yumekage" );

#define WhitelistThreadCTL CTL_CODE( FILE_DEVICE_UNKNOWN, 0xBAD, METHOD_BUFFERED, FILE_ANY_ACCESS )
#define InitializeCTL CTL_CODE( FILE_DEVICE_UNKNOWN, 0xD00D, METHOD_BUFFERED, FILE_ANY_ACCESS )

void SwapContextHook( )
{
	// Check if the current thread is whitelisted, if not then return and do nothing.
	if ( !WhitelistedThreads.Contains( KeGetCurrentThread( ) ) )
		return;

	// Copy over the pml4 table to the clone.
	memcpy( Paging::ClonePML4Virt, Paging::ClientPML4Virt, 0x1000 );

	// Setup the custom PML4 entry.
	Paging::ClonePML4Virt[ Paging::FreePML4Index ] = Paging::ShadowPML4;

	// Set the CR3
	cr3 CR3 = cr3{ .flags = __readcr3( ) };
	CR3.address_of_page_directory = Paging::CloneCR3Phys >> 12;
	HyperDeceit::AddressSpace::SetCR3( CR3 );

	// Basic logging for us.
	DBG( "Handling context swap for thread %d | process %d\n", PsGetCurrentThreadId( ), PsGetCurrentProcessId( ) );
}

NTSTATUS Dispatch( _In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp )
{
	UNREFERENCED_PARAMETER( DeviceObject );

	PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation( Irp );
	ULONG IoControlCode = Stack->Parameters.DeviceIoControl.IoControlCode;
	ULONG InputBufferLength = Stack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG OutputBufferLength = Stack->Parameters.DeviceIoControl.OutputBufferLength;
	uint32_t* SystemBuffer = (uint32_t*)Irp->AssociatedIrp.SystemBuffer;

	// Check if the major function is for device control, if not then return success.
	if ( Stack->MajorFunction != IRP_MJ_DEVICE_CONTROL )
	{
		IoCompleteRequest( Irp, IO_NO_INCREMENT );
		return STATUS_SUCCESS;
	}

	// Basic sanity checks.
	if ( InputBufferLength || OutputBufferLength != 4 || (IoControlCode != WhitelistThreadCTL && IoControlCode != InitializeCTL) )
	{
		Irp->IoStatus.Status = STATUS_INVALID_SESSION;
		IoCompleteRequest( Irp, IO_NO_INCREMENT );
		return STATUS_INVALID_SESSION;
	}

	// Using a switch case here, as I had a bunch of cases here which are now stripped for publication.
	// Feel free to use an if else here, if that's what you prefer.
	switch ( IoControlCode )
	{
		case WhitelistThreadCTL:
		{
			// Insert the current thread to the whitelist thread array.
			WhitelistedThreads.Insert( KeGetCurrentThread( ) );

			// Fix up the clone table.
			memcpy( Paging::ClonePML4Virt, Paging::ClientPML4Virt, 0x1000 );
			Paging::ClonePML4Virt[ Paging::FreePML4Index ] = Paging::ShadowPML4;

			// Set the new cr3 to the clone one.
			cr3 CR3 = cr3{ .flags = __readcr3( ) };
			CR3.address_of_page_directory = Paging::CloneCR3Phys >> 12;
			HyperDeceit::AddressSpace::SetCR3( CR3 );

			// Basic logging.
			DBG( "Whitelisted thread %d\n", PsGetCurrentThreadId( ) );

			// To inform the client that everything went well.
			*SystemBuffer = 0x1BADD00D;
			break;
		}
		case InitializeCTL:
		{
			// Clear the array of whitelisted threads, as only 1 process at a time is allowed.
			WhitelistedThreads.Clear( );

			// Read the cr3 and get the physical address of the pml4 table.
			cr3 CR3 = { .flags = __readcr3( ) };

			// Convert the physical address to a virtual address, though this is process respective.
			// Meaning that this address is only valid on the current address space.
			Paging::ClientPML4Virt = (pml4e_64*)MmGetVirtualForPhysical( PHYSICAL_ADDRESS{ .QuadPart = LONGLONG( CR3.address_of_page_directory << 12 ) } );

			// Walk the usermode range ONLY to find a free PML4 entry.
			bool Found = false;
			for ( int i = 0; i < 256; i++ )
			{
				if ( !Paging::ClientPML4Virt[ i ].flags )
				{
					Found = true;
					Paging::FreePML4Index = i;
					break;
				}
			}

			// This should never happen, so we can't do anything here, so just cause a panic crash to notify the user.
			if ( !Found )
				PANIC( 0xBAD, "NO FREE PML4 ENTRY FOUND" );
		
			// Write the free pml4 index to the system buffer, so the usermode is aware of it.
			*SystemBuffer = Paging::FreePML4Index;

			// Basic logging for us.
			DBG( "Initialized paging for process %d\n", PsGetCurrentProcessId( ) );
			break;
		}
	}

	// Everything went well, return success.
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 4;
	IoCompleteRequest( Irp, IO_NO_INCREMENT );
	return STATUS_SUCCESS;
}

void DriverUnload( _In_ PDRIVER_OBJECT DriverObject )
{
	// Delete sym link, and device object.
	IoDeleteSymbolicLink( &YumekageDosSymlink );
	IoDeleteDevice( DriverObject->DeviceObject );

	// Free all memory allocations.
	KIRQL Irql = MemoryAllocations.EnterLock( );
	for ( int32_t i = 0; i < MemoryAllocations.Size( ); i++ )
		MmFreeContiguousMemory( MemoryAllocations[ i ] );
	MemoryAllocations.ExitLock( Irql );

	// Destroy all arrays.
	WhitelistedThreads.Destroy( );
	MemoryAllocations.Destroy( );

	// Stop HyperDeceit.
	HyperDeceit::Destroy( );

	DBG( "Unloaded\n" );
}

NTSTATUS DriverEntry( _In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath )
{
	UNREFERENCED_PARAMETER( RegistryPath );

	// Basic check for windows build number, as HyperDeceit cannot run on older systems than
	// window 10 1709.
	ULONG BuildNumber = GetBuildNumber( );
	if ( BuildNumber < WIN10_BN_1709 || BuildNumber > WIN11_BN_22H2 )
	{
		DBG( "This version of windows is not supported\n" );
		return STATUS_FAILED_DRIVER_ENTRY;
	}

	// Initialize HyperDeceit.
	if ( !HyperDeceit::Initialize( ) )
	{
		DBG( "Failed to initialize HyperDeceit\n" );
		return STATUS_FAILED_DRIVER_ENTRY;
	}

	// Create the device object.
	NTSTATUS Status;
	PDEVICE_OBJECT DeviceObject;
	if ( !NT_SUCCESS( Status = IoCreateDevice( DriverObject, 0, &YumekageDeviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject ) ) )
	{
		DBG( "Failed to create device object status: %lX\n", Status );
		HyperDeceit::Destroy( );
		return STATUS_FAILED_DRIVER_ENTRY;
	}

	// Create the dos symbolic link, so our client can communicate.
	if ( !NT_SUCCESS( Status = IoCreateSymbolicLink( &YumekageDosSymlink, &YumekageDeviceName ) ) )
	{
		DBG( "Failed to create sym link status: %lX\n", Status );
		IoDeleteDevice( DeviceObject );
		HyperDeceit::Destroy( );
		return STATUS_FAILED_DRIVER_ENTRY;
	}

	// Set up the dispatch functions, and unload callback.
	DriverObject->MajorFunction[ IRP_MJ_CREATE ] = Dispatch;
	DriverObject->MajorFunction[ IRP_MJ_CLOSE ] = Dispatch;
	DriverObject->MajorFunction[ IRP_MJ_DEVICE_CONTROL ] = Dispatch;
	DriverObject->DriverUnload = DriverUnload;

	// Create the shadow regions.
	Paging::CreateShadowPages( );

	// Let HyperDeceit know that we want to intercept context swaps.
	HyperDeceit::InsertCallback( HyperDeceit::ECallbacks::ContextSwap, &SwapContextHook );

	DBG( "Loaded\n" );
	return STATUS_SUCCESS;
}