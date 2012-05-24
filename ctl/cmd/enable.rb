def cmd_enable(options)
    sys("sudo rm \"#{DISABLE_FILE_PATH}\"")
    say "removed file #{DISABLE_FILE_PATH}"
    say "this takes effect for newly launched processes, reboot your machine to fully enable asepsis"
end