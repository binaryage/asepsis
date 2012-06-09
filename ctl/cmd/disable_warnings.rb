def cmd_disable_warnings(options)
    if Process.uid == 0 then
        sys("sysctl -w vm.shared_region_unnest_logging=0")
        set_permanent_sysctl("vm.shared_region_unnest_logging", "0")
    else
        ctl = "\"#{ASEPSISCTL_SYMLINK_SOURCE_PATH}\""
        system("sudo #{ctl} disable_warnings")
    end
end