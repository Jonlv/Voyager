#pragma once
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <IndustryStandard/PeImage.h>
#include <Guid/GlobalVariable.h>
#include <Library/ShellLib.h>
#include "WinLoad.h"

#if WINVER > 1709
#define START_BOOT_APPLICATION_SIG "\x48\x8B\xC4\x48\x89\x58\x20\x44\x89\x40\x18\x48\x89\x50\x10\x48\x89\x48\x08\x55\x56\x57\x41\x54"
#define START_BOOT_APPLICATION_MASK "xxxxxxxxxxxxxxxxxxxxxxxx"
#elif WINVER == 1709
#define START_BOOT_APPLICATION_SIG "\x48\x8B\xC4\x48\x89\x58\x00\x44\x89\x40\x00\x48\x89\x50\x00\x48\x89\x48\x00\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\x68"
#define START_BOOT_APPLICATION_MASK "xxxxxx?xxx?xxx?xxx?xxxxxxxxxxxxxx"
#elif WINVER == 1703
#define START_BOOT_APPLICATION_SIG "\x48\x8B\xC4\x48\x89\x58\x00\x44\x89\x48\x00\x44\x89\x40\x00\x48\x89\x48\x00\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\x68\xA9"
#define START_BOOT_APPLICATION_MASK "xxxxxx?xxx?xxx?xxx?xxxxxxxxxxxxxxx"
#endif

static_assert(sizeof(START_BOOT_APPLICATION_SIG) == sizeof(START_BOOT_APPLICATION_MASK), "signature and mask size's dont match...");
#define WINDOWS_BOOTMGFW_PATH L"\\efi\\microsoft\\boot\\bootmgfw.efi"
#define WINDOWS_BOOTMGFW_BACKUP_PATH L"\\efi\\microsoft\\boot\\bootmgfw.efi.backup"

extern SHITHOOK BootMgfwShitHook;
typedef EFI_STATUS(EFIAPI* IMG_ARCH_START_BOOT_APPLICATION)(VOID*, VOID*, UINT32, UINT8, VOID*);
EFI_STATUS EFIAPI RestoreBootMgfw(VOID);
EFI_STATUS EFIAPI InstallBootMgfwHooks(EFI_HANDLE BootMgfwPath);
EFI_STATUS EFIAPI ArchStartBootApplicationHook(VOID* AppEntry, VOID* ImageBase, UINT32 ImageSize, UINT8 BootOption, VOID* ReturnArgs);