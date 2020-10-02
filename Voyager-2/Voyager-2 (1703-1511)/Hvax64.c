#include "Hvax64.h"

VOID* MapModule(PVOYAGER_T VoyagerData, UINT8* ImageBase)
{
	EFI_IMAGE_DOS_HEADER* dosHeaders = (EFI_IMAGE_DOS_HEADER*)ImageBase;
	if (dosHeaders->e_magic != EFI_IMAGE_DOS_SIGNATURE)
		return NULL;

	EFI_IMAGE_NT_HEADERS64* ntHeaders = (EFI_IMAGE_NT_HEADERS64*)(ImageBase + dosHeaders->e_lfanew);

	// Map headers
	MemCopy(VoyagerData->ModuleBase, ImageBase, ntHeaders->OptionalHeader.SizeOfHeaders);

	// Map sections
	EFI_IMAGE_SECTION_HEADER* sections = (EFI_IMAGE_SECTION_HEADER*)((UINT8*)&ntHeaders->OptionalHeader + ntHeaders->FileHeader.SizeOfOptionalHeader);
	for (UINT32 i = 0; i < ntHeaders->FileHeader.NumberOfSections; ++i) 
	{
		EFI_IMAGE_SECTION_HEADER* section = &sections[i];
		if (section->SizeOfRawData) 
			MemCopy(VoyagerData->ModuleBase + section->VirtualAddress, ImageBase + section->PointerToRawData, section->SizeOfRawData);
	}

	// set exported pointer to voyager context...
	EFI_IMAGE_EXPORT_DIRECTORY* ExportDir = (EFI_IMAGE_EXPORT_DIRECTORY*)(
		VoyagerData->ModuleBase + ntHeaders->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

	UINT32* Address = (UINT32*)(VoyagerData->ModuleBase + ExportDir->AddressOfFunctions);
	UINT32* Name = (UINT32*)(VoyagerData->ModuleBase + ExportDir->AddressOfNames);
	UINT16* Ordinal = (UINT16*)(VoyagerData->ModuleBase + ExportDir->AddressOfNameOrdinals);

	for (UINT16 i = 0; i < ExportDir->AddressOfFunctions; i++)
	{
		if (AsciiStrStr(VoyagerData->ModuleBase + Name[i], "voyager_context"))
		{
			*(VOYAGER_T*)(VoyagerData->ModuleBase + Address[Ordinal[i]]) = *VoyagerData;
			break;
		}
	}

	// Resolve relocations
	EFI_IMAGE_DATA_DIRECTORY* baseRelocDir = &ntHeaders->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC];
	if (baseRelocDir->VirtualAddress) 
	{
		EFI_IMAGE_BASE_RELOCATION* reloc = (EFI_IMAGE_BASE_RELOCATION*)(VoyagerData->ModuleBase + baseRelocDir->VirtualAddress);
		for (UINT32 currentSize = 0; currentSize < baseRelocDir->Size; ) 
		{
			UINT32 relocCount = (reloc->SizeOfBlock - sizeof(EFI_IMAGE_BASE_RELOCATION)) / sizeof(UINT16);
			UINT16* relocData = (UINT16*)((UINT8*)reloc + sizeof(EFI_IMAGE_BASE_RELOCATION));
			UINT8* relocBase = VoyagerData->ModuleBase + reloc->VirtualAddress;

			for (UINT32 i = 0; i < relocCount; ++i, ++relocData) 
			{
				UINT16 data = *relocData;
				UINT16 type = data >> 12;
				UINT16 offset = data & 0xFFF;

				switch (type) 
				{
				case EFI_IMAGE_REL_BASED_ABSOLUTE:
					break;
				case EFI_IMAGE_REL_BASED_DIR64: 
				{
					UINT64* rva = (UINT64*)(relocBase + offset);
					*rva = (UINT64)(VoyagerData->ModuleBase + (*rva - ntHeaders->OptionalHeader.ImageBase));
					break;
				}
				default:
					return NULL;
				}
			}

			currentSize += reloc->SizeOfBlock;
			reloc = (EFI_IMAGE_BASE_RELOCATION*)relocData;
		}
	}

	return VoyagerData->ModuleBase + ntHeaders->OptionalHeader.AddressOfEntryPoint;
}

VOID MakeVoyagerData
(
	PVOYAGER_T VoyagerData,
	VOID* HypervAlloc,
	UINT64 HypervAllocSize,
	VOID* PayLoadBase,
	UINT64 PayLoadSize
)
{
	VoyagerData->HypervModuleBase = HypervAlloc;
	VoyagerData->HypervModuleSize = HypervAllocSize;
	VoyagerData->ModuleBase = PayLoadBase;
	VoyagerData->ModuleSize = PayLoadSize;

	VOID* VmExitHandlerCall =
		FindPattern(
			HypervAlloc,
			HypervAllocSize,
			VCPU_RUN_HANDLER_SIG,
			VCPU_RUN_HANDLER_MASK
		);

	UINT64 VmExitHandlerCallRip = (UINT64)VmExitHandlerCall + 5; // + 5 bytes because "call vmexit_c_handler" is 5 bytes
	UINT64 VmExitHandlerFunc = VmExitHandlerCallRip + *(INT32*)((UINT64)VmExitHandlerCall + 1); // + 1 to skip E8 (call) and read 4 bytes (RVA)
	VoyagerData->VmExitHandlerRVA = ((UINT64)PayLoadEntry(PayLoadBase)) - VmExitHandlerFunc;
}

VOID* HookVmExit(VOID* HypervBase, VOID* HypervSize, VOID* VmExitHook)
{
	VOID* VmExitHandlerCall =
		FindPattern(
			HypervBase,
			HypervSize,
			VCPU_RUN_HANDLER_SIG,
			VCPU_RUN_HANDLER_MASK
		);

	UINT64 VmExitHandlerCallRip = ((UINT64)VmExitHandlerCall) + 5; // + 5 bytes to next instructions address...
	UINT64 VmExitHandlerFunction = VmExitHandlerCallRip + *(INT32*)(((UINT64)VmExitHandlerCall) + 1); // + 1 to skip E8 (call) and read 4 bytes (RVA)
	INT32 NewVmExitHandlerRVA = ((INT64)VmExitHook) - VmExitHandlerCallRip;
	*(INT32*)((UINT64)VmExitHandlerCall + 1) = NewVmExitHandlerRVA;
	return VmExitHandlerFunction;
}