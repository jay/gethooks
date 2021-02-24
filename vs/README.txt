Currently I'm only building with Visual Studio, but there's no reason GetHooks
can't be built using mingw. I prefer Visual Studio for debugging. gdb is not as
reliable or powerful in windows. I don't have a makefile nor have I tested a
mingw build yet. If you make one send it to me.

I use Visual Studio 2010 Professional and I use two configurations frequently.
The first configuration is the debug configuration. This is the standard
boilerplate debug configuration. The second configuration is the MSVCRT
configuration. I wanted GetHooks to work without any external dependencies and
so I set this configuration to link to MSVCRT which is present on Win2k and
above. This way I have a universal binary for Windows 2k/XP/Vista/7/8 that does
not need Visual Studio dependencies; all that's needed to run is GetHooks.exe
and no DLL files. However to compile using this config you'll need WinDDK. I
used Windows Driver Kit version 7.1.0.

Currently GetHooks when compiled using the MSVCRT configuration has passed
testing on Windows XP/Vista/7/8. Although I designed GetHooks for Win2k as well
it failed for reasons I don't yet know. I probably got an offset wrong in the
handle table. I'd have to set up remote debugging in Win2k which is proving to
be a real chore with VS2010 (it might be impossible).

GetHooks 64-bit does not work in Windows 10 later than Version 1607 (OS Build
14393.1198) Someone asked me to fix for that way back when but I never got
around to it.


"Debug MSVCRT" configuration:

This configuration is an optimized build with debugging symbols that depends on
msvcrt.dll instead of a specific CRT version.

You will need the DDK to build properly. GetHooks.vcxproj uses a hardcoded
IncludePath/LibraryPath for the DDK that can be changed by editing the XML.
C:\WinDDK\7600.16385.1

To build the configuration for 32-bit you'll need msvcrt_win2000.obj from the
DDK to be placed in this directory. Verify against msvcrt_win2000.sha.


Please review the FAQ and known issues (either gh-pages dir or the website
https://jay.github.io/gethooks/). If you still have questions email me:
Jay Satiro <raysatiro$at$yahoo{}com> and put GetHooks in the subject.
