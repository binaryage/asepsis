def cmd_disable(options)
    sys("sudo touch \"#{DISABLE_FILE_PATH}\"")
    say "created file #{DISABLE_FILE_PATH}"
    say "this takes effect for newly launched processes, reboot your machine to fully disable asepsis"
end