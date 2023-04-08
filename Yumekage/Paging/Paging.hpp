/*
*		File name:
*			Paging.hpp
*
*		Use:
*			Utilities for paging tables, and shadow pages.
*
*		Author:
*			Xyrem ( https://reversing.info | Xyrem@reversing.info )
*/


#pragma once
#include "..\Common.hpp"
#include "..\Utils\Utils.hpp"

namespace Paging
{
	extern pml4e_64 ShadowPML4;
	extern pml4e_64* ClonePML4Virt;
	extern pml4e_64* ClientPML4Virt;
	extern uint64_t CloneCR3Phys;
	extern int FreePML4Index;

	void CreateShadowPages( );
	void FlushTLB( );

	void ConstructPML4( _In_ pml4e_64* _PML4E, _In_ uint64_t Phys, _In_ bool Usermode );
	void ConstructPDPT( _In_ pdpte_64* PDPTE, _In_ uint64_t Phys, _In_ bool Usermode, _In_ bool LargePage = false );
	void ConstructPD( _In_ pde_64* PDE, _In_ uint64_t Phys, _In_ bool Usermode, _In_ bool LargePage = false );
}