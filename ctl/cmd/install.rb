def cmd_install(options)
    # prevent installing on future OS versions
    os_version_check()

    ctl = "\"#{ASEPSISCTL_SYMLINK_SOURCE_PATH}\""
    sys!("#{ctl} create_symlink")
    sys!("#{ctl} make_dscage")
    sys!("#{ctl} install_daemon")
    sys!("#{ctl} launch_daemon")
    sys!("#{ctl} disable_warnings")
    sys!("#{ctl} install_wrapper")
    sys!("#{ctl} install_updater")

    # remove (possibly) scheduled uninstall (pathological case when someone uninstalls and installs without restart)
    sys("sudo rm \"/Library/LaunchDaemons/com.binaryage.asepsis.uninstall.plist\"") if File.exists? "/Library/LaunchDaemons/com.binaryage.asepsis.uninstall.plist"
    
    say "Asepsis installation done, it is effective for newly launched processes, you should reboot your computer"
end