The binaries of EdkShellBinPkg are generated with EFI shell project (svn r41) and build with Edk Compatibility & BaseTools Package (r10348). The binaries are built with no debug information by building with "RELEASE" target.

The following steps can help to re-generate these binaries for customization:
1. Check out EdkCompatibilityPkg (r10348) to $(WORKSPACE)\EdkCompatibilityPkg (svn http://edk2.svn.sourceforge.net/svnroot/edk2/trunk/edk2/EdkCompatibilityPkg).
2. Check out EdkShellPkg(r10348) to $(WORKSPACE)\EdkShellPkg (svn http://edk2.svn.sourceforge.net/svnroot/edk2/trunk/edk2/EdkShellPkg).
3. Check out Edk Shell project source (r41) to $(WORKSPACE) (svn http://efi-shell.svn.sourceforge.net/svnroot/efi-shell/trunk/Shell). 
4. Update to the newest BaseTools package (r10348) to $(WORKSPACE) (svn http://edk2.svn.sourceforge.net/svnroot/edk2/trunk/edk2/BaseTools).
5. Under workspace directory (i.e. c:\EdkII), 
   To generate Minimum Shell, execute: "build -a IA32 -a X64 -a IPF -p EdkShellPkg\EdkShellPkg.dsc -m EdkShellPkg\Shell\Shell.inf -b RELEASE" 
   To generate Full Shell, execute: "build -a IA32 -a X64 -a IPF -p EdkShellPkg\EdkShellPkg.dsc -m EdkShellPkg\Shell\ShellFull.inf -b RELEASE" 
6. In EdkShellBinPkg\Bin, we only provides the binary files of those applications which haven't been contained in Minmum Shell but in Full Shell.
   To generate them, execute: "build -a IA32 -a X64 -a IPF -p EdkShellPkg\EdkShellPkg.dsc -m EdkShellPkg\Xxx\Xxx.inf -b RELEASE". Xxx means the corresponding module name.