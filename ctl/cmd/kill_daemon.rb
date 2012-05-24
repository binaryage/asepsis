def cmd_kill_daemon(options)
    die("asepsisd is not running") if `ps aux | grep asepsis[d]` == ""
  
    sys("sudo killall -USR1 asepsisd")
    sleep 1 # give asepsisd some time to exit gratefully
end