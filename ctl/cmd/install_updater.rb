def cmd_install_updater(options)
    sys("sudo cp \"#{UPDATER_PLIST_SOURCE_PATH}\" \"#{UPDATER_PLIST_PATH}\"")
end