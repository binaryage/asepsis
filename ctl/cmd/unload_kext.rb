def cmd_unload_kext(options)
    stat_cmd = "sudo kextstat -lb com.binaryage.asepsis.kext"
    out(stat_cmd)
    die("asepsis.kext is not loaded") if `#{stat_cmd}`==""
    sys("sudo kextunload -v \"#{KEXT_PATH}\"")
end