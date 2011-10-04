def cmd_enable(options)
    sys("sudo rm \"#{DISABLE_FILE_PATH}\"")
    puts "removed file #{DISABLE_FILE_PATH}"
    puts "this takes effect for newly launched processes, reboot your machine to fully enable asepsis"
end