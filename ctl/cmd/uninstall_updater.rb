def cmd_uninstall_updater(options)
    die("updater has been already uninstalled, the file is missing: #{UPDATER_PLIST_PATH}") unless File.exists? UPDATER_PLIST_PATH
      
    # remove daemon registration with launchd
    sys("sudo rm \"#{UPDATER_PLIST_PATH}\"")
end