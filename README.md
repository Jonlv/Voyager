<div align="center">
    <div>
        <img src="https://githacks.org/xerox/voyager/-/raw/bc07837aae126ed06130f7524e3f8464d2b3ca13/img/unknown.png"/>
    </div>
    <img src="https://githacks.org/xerox/voyager/-/raw/master/img/amd_badge.svg"/>
    <img src="https://githacks.org/xerox/voyager/-/raw/master/img/Intel-supported-green.svg"/>
    <img src="https://githacks.org/xerox/voyager/-/raw/master/img/2004--1511-supported-green.svg"/>
    <img src="https://githacks.org/xerox/voyager/-/raw/master/bare_metal-only_2004_tested-yellow.svg"/>
</div>

# Voyager - A Hyper-V Hacking Framework For Windows 10 x64 (AMD & Intel)

Voyager is a project designed to offer module injection and vmexit hooking for both AMD & Intel versions of Hyper-V. This project works on all versions of Windows 10-x64 (2004-1511).
The project is currently split into two individual projects, one for Intel and another for AMD. 

# Compiling & Deploying

To compile Voyager for your desired Windows 10 version, please change the `WINVER` macro in both the `types.h` (found in the PayLoad solution), and `WINVER` in `Utils.h`. Then use
HXD to export the payload as a C array. Replace the array found in `PayLoad.c` with your payload. Next you are going to want to bundle the bootmgfw file with the newly compiled Voyager.efi file. Using the bundler I made: `efi-bundler.exe bootmgfw.efi voyager.efi`. 
To deploy this you must replace bootmgfw inside of your EFI partition. First you are going to mount the partition with `mountvol X: /S`, X: being the label you want to mount your EFI partition too.
Then rename bootmgfw.efi found in: EFI\Microsoft\Boot to bootmgfw.efi.backup.

### Launch.bat
```batch
NET SESSION >nul 2>&1
IF %ERRORLEVEL% EQU 0 (
    mountvol X: /S
    move X:\EFI\Microsoft\Boot\bootmgfw.efi X:\EFI\Microsoft\Boot\bootmgfw.efi.backup
    move bootmgfw.efi X:\EFI\Microsoft\Boot\
    shutdown /r
) ELSE (
    Please run as admin!
)
```

# Versions & Support

:o: -> unknown/not tested.
:heavy_check_mark: -> tested & working.
:x: -> tested & not working.

| Winver | AMD     | Intel | VM | Bare Metal |
|--------|---------|-------|----|-------|
| 2004   | :heavy_check_mark: | :heavy_check_mark:      | :heavy_check_mark:   | :heavy_check_mark:      |
| 1909   | :heavy_check_mark:        | :heavy_check_mark:      | :heavy_check_mark:   | :o:      |
| 1903   | :heavy_check_mark:        | :heavy_check_mark:      | :heavy_check_mark:   | :o:      |
| 1809   | :heavy_check_mark:        | :heavy_check_mark:      | :heavy_check_mark:   | :o:      |
| 1807   | :heavy_check_mark:        |  :heavy_check_mark:     | :heavy_check_mark:   | :o:      |
| 1803   | :heavy_check_mark:        | :heavy_check_mark:      | :heavy_check_mark:   | :o:      |
| 1709   | :heavy_check_mark:       | :heavy_check_mark:      | :heavy_check_mark:   |  :o:     |
| 1703   | :heavy_check_mark:        | :heavy_check_mark:      | :heavy_check_mark:   | :o:      |
| 1607   | :heavy_check_mark:        | :heavy_check_mark:      | :heavy_check_mark:   | :o:      |
| 1511   | :heavy_check_mark:        | :heavy_check_mark:      | :heavy_check_mark:   | :o:      |