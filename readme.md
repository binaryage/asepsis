# Asepsis (ALPHA)

### The final solution for .DS_Store pollution. 

Available for folks running Lion.

## What is .DS_Store?

It is pretty famous file which got its page on wikipedia. Please read [the article](http://en.wikipedia.org/wiki/.DS_Store).

## How does .DS_Store redirection work?

There is a private system framework DesktopServicesPriv which is responsible for creating and manipulating .DS_Store files. This framework is used mainly by Finder, but there are also other apps which link against it and use it.

DesktopServicesPriv was modified in Lion and now uses standard libc calls to manipulate .DS_Store files, which enabled this solution.

Asepsis provides a dynamic library libAsepsis.dylib which is loaded into every process thanks to DYLD_INSERT_LIBRARIES. It interposes some libc calls used by DesktopServicesPriv. Interposed functions detect paths talking about .DS_Store files and redirect them into a prefix directory. This should be transparent to DesktopServicesPriv.

Asepsis also implements kernel extension and user-space system-wide daemon whose purpose is to monitor system-wide folder renames and deletes and mirror those operations in prefixed folder. This way you don't lose your settings when renaming folders, because rename is also executed on folders in prefixed directory.

## The prefix direcotry is **/usr/local/.dscage**

This project implements:

  1. libAsepsis.dylib - a shared library for interposing file manipulation calls
  2. Asepsis.kext - a kernel extension for watching folder operations
  3. asepsisd - a system-wide daemon for mirroring folder renames and deleted in prefixed folder

## Installation

You will need XCode4 for building from sources

    git clone https://github.com/binaryage/asepsis
    cd asepsis
    rake build
    rake install
    sudo bash -c "echo \"setenv DYLD_INSERT_LIBRARIES /System/Library/Extensions/Asepsis.kext/Contents/Resources/libAsepsis.dylib\" >> /etc/launchd.conf"
    sudo reboot
    
Note: you may want to edit launchd.conf manually    
    
TODO: provide a nice installer in the future.

## Known Issues

  * when copying folders, DS_Store settings are not copied over (daemon is unable to distinguish this class of file operations)
 
Please report any issues. Similar approach was used in TotalFinder for more than 2 years without troubles. It is not a perfect solution but it improves situation for people who don't care about .DS_Store files that much and can afford losing them from time to time.

## Could this be ported to Snow Leopard?

Probably yes. Pre-Lion DesktopServicesPriv calls File Manager APIs from CoreServices. Technically same approach can be done there. This is what TotalFinder does under Snow Leopard using mach_override.

## History

Historically this functionality was implemented as part of [TotalFinder](http://totalfinder.binaryage.com) which is a Finder plugin.

Here is what I did in TotalFinder:

  * I've redirected low-level filesystem calls which Finder.app does: 
    * Anytime Finder.app is asking to open `/some/folder/.DS_Store` file, I open it as `/usr/local/.dscache/some/folder/_DS_Store`
      * since TotalFinder 1.1.8, I'm checking if .DS_Store is present in `/some/folder/` - I'm not doing redirection when present.
    * This way Finder thinks files are at original places but they are being physically created in prefix folder, effectively sandboxing them
    * The only exception is the prefix folder itself, when you go and see it in the Finder, no redirection is applied
  * I've implemented kernel extension Echelon, which monitors folder renames (and deletes) and sends them to TotalFinder
    * You see why. This is important to keep DS_Store folder structure in prefix directory mirroring actual structure on the disk

Yeah, kernel extension sounds scary. But I didn't find a better solution in user-space. FSEvents are not precious enough (it just reports "something was changed"). BSD kqueues must be registered on per-file basis, so it is not usable in this scenario. In the end of the day that kernel extension turned out to be really light-weight solution. I use KAUTH API to monitor kernel filesystem events. I do it only if TotalFinder is connected and only for renames and deletes. Testing is simple C-string comparison and sending notification via socket.

---

#### License: [MIT-Style](asepsis/raw/master/license.txt)