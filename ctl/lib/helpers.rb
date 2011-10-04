def sys(cmd)
    puts ">#{cmd}"
    if not system(cmd) then
        puts "failded with code #{$?}"
        exit $?
    end
end

def die(message, code=1)
    puts message
    exit code
end

def suspend_asepsis()
    puts "trying to lock #{SUSPEND_LOCK_PATH}"
    file = File.new(SUSPEND_LOCK_PATH, File::CREAT|File::TRUNC|File::RDWR, 0644)
    file.flock(File::LOCK_EX)
    Signal.trap("INT") do
        file.flock(File::LOCK_UN)
        File.unlink(SUSPEND_LOCK_PATH)
        puts "asepsis operation resumed"
        exit 0
    end
end

def resume_asepsis()
    file = File.new(SUSPEND_LOCK_PATH, File::CREAT|File::TRUNC|File::RDWR, 0644)
    file.flock(File::LOCK_UN)
    File.unlink(SUSPEND_LOCK_PATH)
    puts "removed lock #{SUSPEND_LOCK_PATH}"
end