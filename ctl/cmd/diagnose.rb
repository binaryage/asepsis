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
    
    if not File.exists? KEXT_PATH or not File.directory? KEXT_PATH then
        cry "KEXT is not installed in #{KEXT_PATH} ?"
    end
    
    stat = `stat "#{KEXT_PATH}"`
    stat_parts = stat.split(" ")
    unless stat_parts[2]=="drwxr-xr-x" and stat_parts[4]=="root" and stat_parts[5]=="wheel" then
        cry "KEXT has unexpected attributes: #{stat.strip}"
    end
    
    if not File.exists? ASEPSISD_PATH then
        cry "asepsisd is not installed in #{ASEPSISD_PATH} ?"
    end

    stat = `stat "#{ASEPSISD_PATH}"`
    stat_parts = stat.split(" ")
    unless stat_parts[2]=="-rwxr-xr-x" and stat_parts[4]=="root" and stat_parts[5]=="wheel" then
        cry "asepsisd has unexpected attributes: #{stat.strip}"
    end

    if `ps -ax | grep asepsis[d]`=="" then
        cry "asepsisd is not running! it should be launched by launchd during system launch or right after aspesis installation"
    end
    
    if not File.exists? DS_LIB_RELOCATED_FOLDER then
        cry "Relocated DesktopServicesPriv '#{DS_LIB_RELOCATED_FOLDER}' does not exist! The wrapper is non-functional!"
    end
    
    ds_lib = File.join(DS_LIB_FOLDER, "DesktopServicesPriv")
    stat = `stat "#{ds_lib}"`
    stat_parts = stat.split(" ")
    unless stat_parts[2]=="-rwxr-xr-x" and stat_parts[4]=="root" and stat_parts[5]=="wheel" then
        cry "DesktopServicesPriv has unexpected attributes: #{stat.strip}"
    end
    
    if `otool -L "#{ds_lib}" | grep "A_/DesktopServicesPriv"`==""
        cry "DesktopServicesPriv (#{ds_lib}) does not link against /System/Library/PrivateFrameworks/DesktopServicesPriv.framework/Versions/A_/DesktopServicesPriv"
    end
    
    # test_store_file = File.join(PREFIX_PATH, "tmp", "_DS_Store")
    # `rm "#{test_store_file}"` if File.exists? test_store_file
    # `rm /tmp/.DS_Store` if File.exists? "/tmp/.DS_Store"
    # `touch /tmp/.DS_Store`
    # unless File.exists? test_store_file then
    #     puts "tried> touch /tmp/.DS_Store but #{test_store_file} wasn't created - something is wrong"
    #     is_ok = false
    # end
    
    say "Your Asepsis setup seems to be OK" if $is_ok
end