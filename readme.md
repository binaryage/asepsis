## Asepsis == solution for .DS_Store pollution.

#### For user info visit [http://asepsis.binaryage.com](http://asepsis.binaryage.com)

### Installation from sources

#### DO NOT INSTALL IT ON MAVERICKS (10.9)

You will need Xcode5 for building it from sources:

    git clone https://github.com/binaryage/asepsis
    cd asepsis
    git submodule update --init
    rake build
    rake install
    sudo reboot

Note: for installing debug version which is more verbose in Console.app use `rake build_debug` instead of `rake build`. Also you may then run `rake debug_test` to check if libAsepsis.dylib works as expected.

### Uninstallation

    cd asepsis
    rake uninstall

---

#### License: [MIT-Style](asepsis/raw/master/license.txt)