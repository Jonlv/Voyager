#include "BootMgfw.h"

SHITHOOK BootMgfwShitHook;
EFI_STATUS EFIAPI RestoreBootMgfw(VOID)
{
	UINTN HandleCount = NULL;
	EFI_STATUS Result;
	EFI_HANDLE* Handles = NULL;
	EFI_DEVICE_PATH* DevicePath = NULL;
	EFI_FILE_HANDLE VolumeHandle;
	EFI_FILE_HANDLE BootMgfwHandle;
	EFI_FILE_IO_INTERFACE* FileSystem = NULL;

	// get all the handles to file systems...
	if (EFI_ERROR((Result = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &HandleCount, &Handles))))
	{
		DBG_PRINT("error getting file system handles -> 0x%p\n", Result);
		return Result;
	}

	// for each handle to the file system, open a protocol with it...
	for (UINT32 Idx = 0u; Idx < HandleCount && !FileSystem; ++Idx)
	{
		if (EFI_ERROR((Result = gBS->OpenProtocol(Handles[Idx], &gEfiSimpleFileSystemProtocolGuid, (VOID**)&FileSystem, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL))))
		{
			DBG_PRINT("error opening protocol -> 0x%p\n", Result);
			return Result;
		}

		if (EFI_ERROR((Result = FileSystem->OpenVolume(FileSystem, &VolumeHandle))))
		{
			DBG_PRINT("error opening file system -> 0x%p\n", Result);
			return Result;
		}

		if (!EFI_ERROR(VolumeHandle->Open(VolumeHandle, &BootMgfwHandle, WINDOWS_BOOTMGFW_PATH, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY)))
		{
			EFI_FILE_PROTOCOL* BootMgfwFile = NULL;
			EFI_DEVICE_PATH* BootMgfwPathProtocol = FileDevicePath(Handles[Idx], WINDOWS_BOOTMGFW_PATH);

			if (EFI_ERROR((Result = EfiOpenFileByDevicePath(&BootMgfwPathProtocol, &BootMgfwFile, EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ, NULL))))
			{
				DBG_PRINT("error opening bootmgfw... reason -> %r\n", Result);
				return Result;
			}

			if (EFI_ERROR((Result = BootMgfwFile->Delete(BootMgfwFile))))
			{
				DBG_PRINT("error deleting bootmgfw... reason -> %r\n", Result);
				return Result;
			}

			BootMgfwPathProtocol = FileDevicePath(Handles[Idx], WINDOWS_BOOTMGFW_BACKUP_PATH);
			if (EFI_ERROR((Result = EfiOpenFileByDevicePath(&BootMgfwPathProtocol, &BootMgfwFile, EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ, NULL))))
			{
				DBG_PRINT("failed to open backup file... reason -> %r\n", Result);
				return Result;
			}

			EFI_FILE_INFO* FileInfoPtr = NULL;
			UINTN FileInfoSize = NULL;
			if (EFI_ERROR((Result = BootMgfwFile->GetInfo(BootMgfwFile, &gEfiFileInfoGuid, &FileInfoSize, NULL))))
			{
				if (Result == EFI_BUFFER_TOO_SMALL)
				{
					gBS->AllocatePool(EfiBootServicesData, FileInfoSize, &FileInfoPtr);
					if (EFI_ERROR(Result = BootMgfwFile->GetInfo(BootMgfwFile, &gEfiFileInfoGuid, &FileInfoSize, FileInfoPtr)))
					{
						DBG_PRINT("get backup file information failed... reason -> %r\n", Result);
						return Result;
					}
				}
				else
				{
					DBG_PRINT("Failed to get file information... reason -> %r\n", Result);
					return Result;
				}
			}
			VOID* BootMgfwBuffer = NULL;
			gBS->AllocatePool(EfiBootServicesData, FileInfoPtr->FileSize, &BootMgfwBuffer);

			UINTN BootMgfwSize = FileInfoPtr->FileSize;
			if (EFI_ERROR((Result = BootMgfwFile->Read(BootMgfwFile, &BootMgfwSize, BootMgfwBuffer))))
			{
				DBG_PRINT("Failed to read backup file into buffer... reason -> %r\n", Result);
				return Result;
			}

			if (EFI_ERROR((Result = BootMgfwFile->Delete(BootMgfwFile))))
			{
				DBG_PRINT("unable to delete backup file... reason -> %r\n", Result);
				return Result;
			}

			BootMgfwPathProtocol = FileDevicePath(Handles[Idx], WINDOWS_BOOTMGFW_PATH);
			if (EFI_ERROR((Result = EfiOpenFileByDevicePath(&BootMgfwPathProtocol, &BootMgfwFile, EFI_FILE_MODE_CREATE | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ, EFI_FILE_SYSTEM))))
			{
				DBG_PRINT("unable to create new bootmgfw on disk... reason -> %r\n", Result);
				return Result;
			}

			BootMgfwSize = FileInfoPtr->FileSize;
			if (EFI_ERROR((Result = BootMgfwFile->Write(BootMgfwFile, &BootMgfwSize, BootMgfwBuffer))))
			{
				DBG_PRINT("unable to write to newly created bootmgfw.efi... reason -> %r\n", Result);
				return Result;
			}

			VolumeHandle->Close(VolumeHandle);
			BootMgfwFile->Close(BootMgfwFile);
			gBS->FreePool(FileInfoPtr);
			gBS->FreePool(BootMgfwBuffer);
			return EFI_SUCCESS;
		}

		if (EFI_ERROR((Result = gBS->CloseProtocol(Handles[Idx], &gEfiSimpleFileSystemProtocolGuid, gImageHandle, NULL))))
		{
			DBG_PRINT("error closing protocol -> 0x%p\n", Result);
			return Result;
		}
	}
	return EFI_ABORTED;
}

EFI_STATUS EFIAPI InstallBootMgfwHooks(EFI_HANDLE ImageHandle)
{
	EFI_STATUS Result = EFI_SUCCESS;
	EFI_LOADED_IMAGE* BootMgfw = NULL;

	if (EFI_ERROR(Result = gBS->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID**)&BootMgfw)))
		return Result;

	DBG_PRINT("Module base -> 0x%p\n", BootMgfw->ImageBase);
	DBG_PRINT("Module size -> 0x%x\n", BootMgfw->ImageSize);

	VOID* ArchStartBootApplication = 
		FindPattern(
			BootMgfw->ImageBase,
			BootMgfw->ImageSize,
			START_BOOT_APPLICATION_SIG, 
			START_BOOT_APPLICATION_MASK
		);

	if (!ArchStartBootApplication)
		return EFI_ABORTED;

	DBG_PRINT("ArchStartBootApplication -> 0x%p\n", RESOLVE_RVA(ArchStartBootApplication, 5, 1));
	MakeShitHook(&BootMgfwShitHook, RESOLVE_RVA(ArchStartBootApplication, 5, 1), &ArchStartBootApplicationHook, TRUE);
	return EFI_SUCCESS;
}

EFI_STATUS EFIAPI ArchStartBootApplicationHook(VOID* AppEntry, VOID* ImageBase, UINT32 ImageSize, UINT8 BootOption, VOID* ReturnArgs)
{
	DisableShitHook(&BootMgfwShitHook);
	VOID* ImgLoadPEImageEx =
		FindPattern(
			ImageBase,
			ImageSize,
			LOAD_PE_IMG_SIG,
			LOAD_PE_IMG_MASK
		);

	Print(L"PE PayLoad Size -> 0x%x\n", PayLoadSize());
	Print(L"winload base -> 0x%p\n", ImageBase);
	Print(L"winload size -> 0x%x\n", ImageSize);
	Print(L"winload.BlImgLoadPEImageEx -> 0x%p\n", RESOLVE_RVA(ImgLoadPEImageEx, 5, 1));

	MakeShitHook(&WinLoadImageShitHook, RESOLVE_RVA(ImgLoadPEImageEx, 5, 1), &BlImgLoadPEImageEx, TRUE);
	return ((IMG_ARCH_START_BOOT_APPLICATION)BootMgfwShitHook.Address)(AppEntry, ImageBase, ImageSize, BootOption, ReturnArgs);
}