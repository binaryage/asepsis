def cmd_load_kext(options)
    stat_cmd = "sudo kextstat -lb com.binaryage.asepsis.kext"
    out(stat_cmd)
    die("asepsis.kext is already loaded") if `#{stat_cmd}`!=""
    sys("sudo kextload -v \"#{KEXT_PATH}\"")
end