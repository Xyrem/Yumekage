/*
*		File name:
*			Paging.cpp
*
*		Use:
*			Utilities for paging tables, and shadow pages.
*
*		Author:
*			Xyrem ( https://reversing.info | Xyrem@reversing.info )
*/


#include "Paging.hpp"

namespace Paging
{
	pml4e_64 ShadowPML4;
	pml4e_64* ClonePML4Virt;
	pml4e_64* ClientPML4Virt;
	uint64_t CloneCR3Phys;
	int FreePML4Index;

	/*
	*	Create and setup our own paging tables for the shadow ranges.
	*/
	void CreateShadowPages( )
	{
		// Allocate memory for all the tables and pages.
		void* ShadowPage = MmAllocateContiguousMemory( 0x200000, PHYSICAL_ADDRESS{ .QuadPart = -1 } );
		pde_64* PD = (pde_64*)MmAllocateContiguousMemory( 0x1000, PHYSICAL_ADDRESS{ .QuadPart = -1 } );
		pdpte_64* PDPT = (pdpte_64*)MmAllocateContiguousMemory( 0x1000, PHYSICAL_ADDRESS{ .QuadPart = -1 } );
		Paging::ClonePML4Virt = (pml4e_64*)MmAllocateContiguousMemory( 0x1000, PHYSICAL_ADDRESS{ .QuadPart = -1 } );

		// Erase the memory for all the allocations.
		memset( ShadowPage, 0, 0x200000 );
		memset( PD, 0, 0x1000 );
		memset( PDPT, 0, 0x1000 );
		memset( Paging::ClonePML4Virt, 0, 0x1000 );

		// Set the paging table entries.
		Paging::ConstructPD( PD, MmGetPhysicalAddress( ShadowPage ).QuadPart, true, true );
		Paging::ConstructPDPT( PDPT, MmGetPhysicalAddress( PD ).QuadPart, true );
		Paging::ConstructPML4( &Paging::ShadowPML4, MmGetPhysicalAddress( PDPT ).QuadPart, true );

		// Store the physical address.
		Paging::CloneCR3Phys = MmGetPhysicalAddress( Paging::ClonePML4Virt ).QuadPart;

		// Remember all the allocations created.
		MemoryAllocations.Insert( ShadowPage );
		MemoryAllocations.Insert( PD );
		MemoryAllocations.Insert( PDPT );
		MemoryAllocations.Insert( Paging::ClonePML4Virt );
	}

	/*
	*	Flush the TLB 
	*/
	void FlushTLB( )
	{
		__writecr3( __readcr3( ) );

		uint64_t CR4 = __readcr4( );
		__writecr4( CR4 ^ 0x80 );
		__writecr4( CR4 );
	}

	/*
	*	Construct a PML4 entry.
	*/
	void ConstructPML4( _In_ pml4e_64* _PML4E, _In_ uint64_t Phys, _In_ bool Usermode )
	{
		_PML4E->flags = 0;

		_PML4E->supervisor = Usermode;
		_PML4E->write = true;
		_PML4E->page_frame_number = Phys >> 12;
		_PML4E->present = true;
	}

	/*
	*	Construct a PDPT entry.
	*/
	void ConstructPDPT( _In_ pdpte_64* PDPTE, _In_ uint64_t Phys, _In_ bool Usermode, _In_ bool LargePage )
	{
		PDPTE->flags = 0;

		PDPTE->supervisor = Usermode;
		PDPTE->large_page = LargePage;
		PDPTE->write = true;
		LargePage ? ((pdpte_1gb_64*)PDPTE)->page_frame_number = Phys >> 30 : PDPTE->page_frame_number = Phys >> 12;
		PDPTE->present = true;
	}

	/*
	*	Construct a PD entry.
	*/
	void ConstructPD( _In_ pde_64* PDE, _In_ uint64_t Phys, _In_ bool Usermode, _In_ bool LargePage )
	{
		PDE->flags = 0;

		PDE->large_page = LargePage;
		PDE->write = true;
		PDE->supervisor = Usermode;
		LargePage ? ((pde_2mb_64*)PDE)->page_frame_number = Phys >> 21 : PDE->page_frame_number = Phys >> 12;
		PDE->present = true;
	}
}