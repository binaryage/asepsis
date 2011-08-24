property stdout : ""
on run
    set newline to ASCII character 10

    set stdout to stdout & "  stop asepsisd ..." & newline
    try
        do shell script "sudo killall -USR1 asepsisd" with administrator privileges
        delay 1 -- give asepsisd some time to exit gratefully
    on error
        set stdout to stdout & "    asepsisd not running?" & newline
    end try
    
    set stdout to stdout & "  unload daemon ..." & newline
    try
        do shell script "sudo launchctl unload \"/Library/LaunchDaemons/com.binaryage.asepsis.daemon.plist\"" with administrator privileges
    on error
        set stdout to stdout & "    unable to unload /Library/LaunchDaemons/com.binaryage.asepsis.daemon.plist" & newline
    end try
    
    set stdout to stdout & "  remove daemon registration ..." & newline
    try
        do shell script "sudo rm \"/Library/LaunchDaemons/com.binaryage.asepsis.daemon.plist\"" with administrator privileges
    on error
        set stdout to stdout & "    unable to remove /Library/LaunchDaemons/com.binaryage.asepsis.daemon.plist" & newline
    end try

    set stdout to stdout & "  remove asepsisctl ..." & newline
    try
        do shell script "sudo rm \"/usr/local/bin/asepsisctl\"" with administrator privileges
    on error
        set stdout to stdout & "    unable to remove /usr/local/bin/asepsisctl" & newline
    end try
    
    set stdout to stdout & "  unset DYLD_INSERT_LIBRARIES in launchd environment ..." & newline
    try
        do shell script "sudo launchctl unsetenv DYLD_INSERT_LIBRARIES" with administrator privileges
    on error
        set stdout to stdout & "    unable to unset DYLD_INSERT_LIBRARIES in launchd environment" & newline
    end try

    set stdout to stdout & "  remove lauchd.conf ..." & newline
    try
        do shell script "sudo mv /etc/launchd.conf /etc/launchd.conf.backup" with administrator privileges
    on error
        set stdout to stdout & "    unable to remove /etc/launchd.conf" & newline
    end try
    
    set stdout to stdout & "  unload Asepsis.kext ..." & newline
    try
        do shell script "sudo kextunload \"/System/Library/Extensions/Asepsis.kext\"" with administrator privileges
    on error
        set stdout to stdout & "    Asepsis.kext not loaded" & newline
    end try
    
    -- we cannot remove /System/Library/Extensions/Asepsis.kext/Contntents/Resources/libAsepsis.dylib at this point because DYLD_INSERT_LIBRARIES is still active until reboot
    -- instead we install launchd "runonce" task to finish the uninstallation after reboot
    set stdout to stdout & "  remove Asepsis.kext ..." & newline
    try
        do shell script "sudo cp \"/System/Library/Extensions/Asepsis.kext/Contents/Resources/com.binaryage.asepsis.uninstall.plist\" \"/Library/LaunchDaemons\"" with administrator privileges
    on error
        set stdout to stdout & "    unable to copy com.binaryage.asepsis.uninstall.plist to /Library/LaunchDaemons" & newline
    end try

    set stdout to stdout & "Asepsis uninstallation done, reboot your computer" & newline
    
    stdout -- this is needed for platypus to display output in details window
end run