def cmd_enable_warnings(options)
    sys("sudo sysctl -w vm.shared_region_unnest_logging=1")
end