def cmd_clean(options)
    suspend_asepsis()

    dry = options[:dry]
    verbose = options[:verbose]
    
    counter = 0

    entries = Dir.entries(PREFIX_PATH) - %w{ . .. }
    entries.each do |file|
        path = File.join(PREFIX_PATH, file)
        next unless File.directory? path
        
        puts "rm -rf \"#{path}\"" if (dry or verbose)
        `rm -rf \"#{path}\"` unless dry

        counter += 1
    end
    
    print "\n" if counter>0
    
    resume_asepsis()

    if counter>0 then
        puts "removed #{counter} drectories in the prefix folder"
    else 
        puts "no directories found in the prefix folder, nothing to do"
    end
end