def cmd_disable(options)
    sys("sudo touch \"#{DISABLE_FILE_PATH}\"")
    puts "created file #{DISABLE_FILE_PATH}"
    puts "this takes effect for newly launched processes, reboot your machine to fully disable asepsis"
end