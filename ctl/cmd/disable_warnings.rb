def cmd_disable_warnings(options)
    sys("sudo sysctl -w vm.shared_region_unnest_logging=0")
end