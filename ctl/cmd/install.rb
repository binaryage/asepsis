def cmd_install(options)
    ctl = "\"#{ASEPSISCTL_SYMLINK_SOURCE_PATH}\""
    sys!("#{ctl} disable_warnings")
    sys!("#{ctl} make_dscage")
    sys!("#{ctl} create_symlink")
    sys!("#{ctl} install_daemon")
    sys!("#{ctl} launch_daemon")
    sys!("#{ctl} install_wrapper")
    
    say "Asepsis installation done, it is effective for newly launched processes, you should reboot your computer"
end