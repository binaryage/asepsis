def cmd_launch_daemon(options)
    die("daemon plist is not in place, install the daemon first (#{DAEMON_PLIST_PATH} is missing)") unless File.exists? DAEMON_PLIST_PATH
  
    sys("sudo launchctl start \"#{DAEMON_LABEL}\"")
end