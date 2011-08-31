# Asepsis (ALPHA)

### smart solution for .DS_Store pollution. 

### For more info visit [http://asepsis.binaryage.com](http://asepsis.binaryage.com)

---

# Developer info

## Installation from sources

You will need XCode4 for building from sources

    git clone https://github.com/binaryage/asepsis
    cd asepsis
    rake build
    rake install
    sudo bash -c "echo \"setenv DYLD_INSERT_LIBRARIES /System/Library/Extensions/Asepsis.kext/Contents/Resources/libAsepsis.dylib\" >> /etc/launchd.conf"
    sudo reboot
    
Note: you may want to edit `launchd.conf` manually

Note: for installing debug version which is more verbose in Console.app use `rake build_debug` instead of `rake build`. Also you may then run `rake debug_test` to check if libAsepsis.dylib works as expected.
    
## Uninstallation

    cd asepsis
    rake uninstall

## Development History

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