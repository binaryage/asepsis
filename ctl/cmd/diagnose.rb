def cry(what)
    puts "\033[31m#{what}\033[0m"
    $is_ok = false
end

def cmd_diagnose(options)
    $is_ok = true

    if not File.exists? ASEPSISCTL_SYMLINK_PATH then
        cry "asepsisctl symlink is not installed: #{ASEPSISCTL_SYMLINK_PATH}"
    end

    if not File.exists? PREFIX_PATH then
        cry "Prefix directory '#{PREFIX_PATH}' does not exist!"
    end

    if not File.exists? RESOURCES_PATH or not File.directory? RESOURCES_PATH then
        cry "Asepsis is not installed in #{RESOURCES_PATH} ?"
    end

    stat = `/usr/bin/stat "#{RESOURCES_PATH}"`
    stat_parts = stat.split(" ")
    unless stat_parts[2]=="drwxr-xr-x" and stat_parts[4]=="root" and stat_parts[5]=="admin" then
        cry "Asepsis has unexpected attributes: #{stat.strip}"
    end

    logging = `/usr/sbin/sysctl vm.shared_region_unnest_logging`.strip
    if logging != "vm.shared_region_unnest_logging: 0" then
        cry "Asepsis should have set sysctl vm.shared_region_unnest_logging to 0"
    end

    if not File.exists? ASEPSISD_PATH then
        cry "asepsisd is not installed in #{ASEPSISD_PATH} ?"
    end

    stat = `/usr/bin/stat "#{ASEPSISD_PATH}"`
    stat_parts = stat.split(" ")
    unless stat_parts[2]=="-rwxr-xr-x" and stat_parts[4]=="root" and stat_parts[5]=="admin" then
        cry "asepsisd has unexpected attributes: #{stat.strip}"
    end

    if `/bin/ps -ax | grep asepsis[d]`=="" then
        cry "asepsisd is not running! it should be launched by launchd during system launch or right after Asepsis installation"
    end

    if not File.exists? DS_LIB_RELOCATED_FOLDER then
        cry "Relocated DesktopServicesPriv '#{DS_LIB_RELOCATED_FOLDER}' does not exist! The wrapper is non-functional!"
    end

    ds_lib = File.join(DS_LIB_FOLDER, "DesktopServicesPriv")
    stat = `/usr/bin/stat "#{ds_lib}"`
    stat_parts = stat.split(" ")
    unless stat_parts[2]=="-rwxr-xr-x" and stat_parts[4]=="root" and stat_parts[5]=="wheel" then
        cry "DesktopServicesPriv has unexpected attributes: #{stat.strip}"
    end

    if not desktopservicespriv_wrapper?(ds_lib) then
        cry "DesktopServicesPriv (#{ds_lib}) is not properly installed.\n  => Have you installed a system update recently? It might revert it back to the original version."
    end

    say "Your Asepsis setup seems to be OK" if $is_ok
    exit 1 unless $is_ok
end