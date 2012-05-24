def cmd_install_daemon(options)
    sys("sudo cp \"#{DAEMON_PLIST_SOURCE_PATH}\" \"#{DAEMON_PLIST_PATH}\"")
    sys("sudo launchctl load \"#{DAEMON_PLIST_PATH}\"")
end