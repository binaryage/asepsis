## Asepsis == solution for .DS_Store pollution.

#### For end-user info please visit [http://asepsis.binaryage.com](http://asepsis.binaryage.com)

### Installation from sources

You will need Xcode5 for building it from sources:

    git clone https://github.com/binaryage/asepsis
    cd asepsis
    git submodule update --init
    rake build
    rake install
    sudo reboot

Note: for installing debug version which is more verbose in Console.app use `rake build_debug` instead of `rake build`. Also you may then run `rake debug_test` to check if `libAsepsis.dylib` works as expected.

### Uninstallation

    asepsisctl uninstall

---

#### License: [MIT-Style](asepsis/raw/master/license.txt)