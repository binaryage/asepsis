def cmd_uninstall(options)
    ctl = "\"#{ASEPSISCTL_SYMLINK_SOURCE_PATH}\""
    
    $forced_exit_code = 0
    
    sys!("#{ctl} uninstall_wrapper")
    sys!("#{ctl} kill_daemon")
    sys!("#{ctl} uninstall_daemon")
    sys!("#{ctl} remove_symlink")
    sys!("#{ctl} enable_warnings") if prior_yosemite?
    sys!("#{ctl} uninstall_updater")

    # install launchd "runonce" task to finish the uninstallation after reboot
    sys!("sudo cp \"#{RESOURCES_PATH}/com.binaryage.asepsis.uninstall.plist\" \"/Library/LaunchDaemons\"")
    
    if $forced_exit_code==0 then
      say "Asepsis uninstallation done, reboot your computer."
    else
      say_red "Asepsis uninstallation done, but with some failures, please inspect the command output.\nTo complete uninstallation reboot your computer."
      exit 1
    end
end