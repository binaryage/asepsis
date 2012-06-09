def cmd_uninstall_kext(options)
    return unless File.exists? "/System/Library/Extensions/Asepsis.kext"
  
    # install launchd "runonce" task to finish the uninstallation after reboot
    sys("sudo cp \"#{RESOURCES_PATH}/com.binaryage.asepsis.kext-uninstall.plist\" \"/Library/LaunchDaemons\"")
    
    say "Old Asepsis KEXT uninstallation done, it will be finished after you reboot the computer"
end