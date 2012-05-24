def cmd_uninstall_daemon(options)
    cmd_kill_daemon(options) if `ps aux | grep asepsis[d]` != ""
    
    die("daemon has been already uninstalled, the file is missing: #{DAEMON_PLIST_PATH}") unless File.exists? DAEMON_PLIST_PATH
      
    # remove daemon registration with launchd
    sys("sudo launchctl unload \"#{DAEMON_PLIST_PATH}\"")
    sys("sudo rm \"#{DAEMON_PLIST_PATH}\"")
end