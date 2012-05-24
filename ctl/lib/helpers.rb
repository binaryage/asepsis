def out(cmd)
    puts "\033[33m> #{cmd}\033[0m"
end

def say(what)
    puts "\033[34m#{what}\033[0m"
end

def sys(cmd)
    out(cmd)
    $stdout.flush
    if not system(cmd) then
        puts "\033[31mfailed with code #{$?}\033[0m"
        exit $?.exitstatus
    end
end

def sys!(cmd)
    out(cmd)
    $stdout.flush
    system(cmd)
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