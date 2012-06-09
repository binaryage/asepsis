require 'fileutils'

def cmd_migratein(options)
    root = options[:root]
    dry = options[:dry]
    verbose = options[:verbose]
    
    counter = 0
    
    say "migrating all .DS_Store files from #{root} into #{PREFIX_PATH}"
    
    Dir.glob(File.join(root, "**", ".DS_Store"), File::FNM_DOTMATCH) do |source|
        dest = File.join(PREFIX_PATH, source)
        dest[-9] = "_" # /some/path/.DS_Store -> /some/path/_DS_Store
        
        dest_dir = File.dirname(dest)
        FileUtils.mkdir_p(dest_dir) unless dry
        FileUtils.mv(source, dest, {:verbose => verbose, :force => true}) unless dry
        puts "#{source} -> #{dest}" if dry
        counter = counter + 1
        print "." unless (verbose or dry)
        STDOUT.flush
    end
    
    print "\n" if counter>0

    if counter>0 then
        say "moved #{counter} .DS_Store file(s) into the prefix folder"
    else 
        say "no .DS_Store files found, nothing to do"
    end
end