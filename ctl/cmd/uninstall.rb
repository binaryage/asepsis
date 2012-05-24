def cmd_uninstall(options)
    ctl = "\"#{ASEPSISCTL_SYMLINK_SOURCE_PATH}\""
    sys!("#{ctl} uninstall_wrapper")
    sys!("#{ctl} kill_daemon")
    sys!("#{ctl} uninstall_daemon")
    sys!("#{ctl} unload_kext")
    sys!("#{ctl} remove_symlink")
    sys!("#{ctl} enable_warnings")

    # install launchd "runonce" task to finish the uninstallation after reboot
    sys("sudo cp \"/System/Library/Extensions/Asepsis.kext/Contents/Resources/com.binaryage.asepsis.uninstall.plist\" \"/Library/LaunchDaemons\"")
    
    say "Asepsis uninstallation done, reboot your computer"
end