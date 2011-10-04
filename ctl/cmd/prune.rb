def remove_if_empty(dir, dry, verbose)
    counter = 0
    removed = 0

    entries = Dir.entries(dir) - %w{ . .. }
    entries.each do |file|
        path = File.join(dir, file) 
        next unless File.directory? path
        
        c, really_removed = remove_if_empty(path, dry, verbose)
        counter += c
        removed += 1 if really_removed
    end
    
    if entries.size == removed then
        puts "remove #{dir}" if (dry or verbose)
        Dir.rmdir dir unless dry
        print "." unless (verbose or dry)
        STDOUT.flush
        return counter+1, true
    end
    
    return counter, false
end

def cmd_prune(options)
    suspend_asepsis()

    dry = options[:dry]
    verbose = options[:verbose]
    
    counter = 0
    
    entries = Dir.entries(PREFIX_PATH) - %w{ . .. }
    entries.each do |file|
        path = File.join(PREFIX_PATH, file)
        next unless File.directory? path
        
        c, removed = remove_if_empty(path, dry, verbose)
        counter += c
    end
    
    print "\n" if counter>0
    
    resume_asepsis()

    if counter>0 then
        puts "removed #{counter} empty drectories in the prefix folder"
    else 
        puts "no empty directories found, nothing to do"
    end
end