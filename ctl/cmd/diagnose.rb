def cmd_diagnose(options)
    is_ok = true
    if not File.exists? PREFIX_PATH then
        puts "Prefix directory '#{PREFIX_PATH}' does not exits!"
        is_ok = false
    end
    
    if not File.exists? KEXT_PATH or not File.directory? KEXT_PATH then
        puts "KEXT is not installed in #{KEXT_PATH} ?"
        is_ok = false
    end
    
    stat = `stat "#{KEXT_PATH}"`
    stat_parts = stat.split(" ")
    unless stat_parts[2]=="drwxr-xr-x" and stat_parts[4]=="root" and stat_parts[5]=="wheel" then
        puts "KEXT has unexpected attributes: #{stat} (#{KEXT_PATH})"
        is_ok = false
    end
    
    if not File.exists? ASEPSISD_PATH then
        puts "asepsisd is not installed in #{ASEPSISD_PATH} ?"
        is_ok = false
    end

    stat = `stat "#{ASEPSISD_PATH}"`
    stat_parts = stat.split(" ")
    unless stat_parts[2]=="-rwxr-xr-x" and stat_parts[4]=="root" and stat_parts[5]=="wheel" then
        puts "asepsisd has unexpected attributes: #{stat} (#{ASEPSISD_PATH})"
        is_ok = false
    end

    if not File.exists? LIBASEPSIS_PATH then
        puts "libAsepsis.dylib is not installed in #{LIBASEPSIS_PATH} ?"
        is_ok = false
    end
    
    stat = `stat "#{LIBASEPSIS_PATH}"`
    stat_parts = stat.split(" ")
    unless stat_parts[2]=="-rwxr-xr-x" and stat_parts[4]=="root" and stat_parts[5]=="wheel" then
        puts "libAsepsis.dylib has unexpected attributes: #{stat} (#{LIBASEPSIS_PATH})"
        is_ok = false
    end
    
    unless (`ps -ax | grep asepsisd`.split("\n").select { |line| not line =~ /grep asepsisd/ }).size==1 then
        puts "asepsisd is not running! it should be launched by launchd during system launch or right after aspesis installation"
        is_ok = false
    end
    
    env = `launchctl getenv DYLD_INSERT_LIBRARIES`
    if not env=~/\/System\/Library\/Extensions\/Asepsis\.kext\/Contents\/Resources\/libAsepsis\.dylib/ then
        puts "user's launchctl getenv DYLD_INSERT_LIBRARIES does not contain libAsepsis.dylib!"
        puts "=> returned: #{env}"
        puts "note: some applications like Glims may override DYLD_INSERT_LIBRARIES environmental settings, please consult http://asepsis.binaryage.com/#known-issues"
        is_ok = false
    end

    env = `sudo launchctl getenv DYLD_INSERT_LIBRARIES`
    if not env=~/\/System\/Library\/Extensions\/Asepsis\.kext\/Contents\/Resources\/libAsepsis\.dylib/ then
        puts "root launchctl getenv DYLD_INSERT_LIBRARIES does not contain libAsepsis.dylib!"
        puts "=> returned: #{env}"
        puts "note: some applications like Glims may override DYLD_INSERT_LIBRARIES environmental settings, please consult http://asepsis.binaryage.com/#known-issues"
        is_ok = false
    end
    
    test_store_file = File.join(PREFIX_PATH, "tmp", "_DS_Store")
    `rm "#{test_store_file}"` if File.exists? test_store_file
    `rm /tmp/.DS_Store` if File.exists? "/tmp/.DS_Store"
    `touch /tmp/.DS_Store`
    unless File.exists? test_store_file then
        puts "tried> touch /tmp/.DS_Store but #{test_store_file} wasn't created - something is wrong"
        `rm /tmp/.DS_Store` if File.exists? "/tmp/.DS_Store"
        `DYLD_INSERT_LIBRARIES="#{LIBASEPSIS_PATH}" touch /tmp/.DS_Store`
        unless File.exists? test_store_file then
            puts "tried> DYLD_INSERT_LIBRARIES=\"#{LIBASEPSIS_PATH}\" touch /tmp/.DS_Store but #{test_store_file} still wasn't created"
        end
        is_ok = false
    end
    
    puts "your Asepsis setup seems to be OK" if is_ok
end