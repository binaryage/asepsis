## Asepsis == solution for .DS_Store pollution.

### WARNING: No longer supported under macOS 10.11 (El Capitan) and higher

---

If you are interested in updated version maintained by others, please check out these repos:

## Odourless 
https://github.com/xiaozhuai/odourless

Which prevents `.DS_Store` pollution on Sierra, High Sierra, Mojave, Catalina and Big Sur (On both Intel _*and*_ M1 processors!)

and is also, in turn, based on / inspired by  prior work by:
* [https://github.com/JK3Y/asepsis](https://github.com/JK3Y/asepsis)
* [https://github.com/fnesveda/asepsis](https://github.com/fnesveda/asepsis)

---
---
---

#### For end-user info please visit [http://asepsis.binaryage.com](http://asepsis.binaryage.com)

### Installation from sources

You will need Xcode5 for building it from sources:

    git clone https://github.com/binaryage/asepsis
    cd asepsis
    git submodule update --init
    rake build
    rake install
    sudo reboot

Note: for installing debug version which is more verbose in Console.app use `rake build_debug` instead of `rake build`. Also you may then run `rake debug_test` to check if asepsis works as expected.

Note: before `rake build` you might need to `open Asepsis.xcodeproj` once in Xcode. For some reason the xcodebuild hangs without that.

### Uninstallation

    asepsisctl uninstall

#### License: [MIT-Style](license.txt)
