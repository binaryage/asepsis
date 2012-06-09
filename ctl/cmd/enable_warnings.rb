def cmd_enable_warnings(options)
    if Process.uid == 0 then
        sys("sudo sysctl -w vm.shared_region_unnest_logging=1")
        remove_permanent_sysctl("vm.shared_region_unnest_logging")
    else
        ctl = "\"#{ASEPSISCTL_SYMLINK_SOURCE_PATH}\""
        system("sudo #{ctl} enable_warnings")
    end
end