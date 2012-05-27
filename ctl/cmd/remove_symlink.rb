def cmd_remove_symlink(options)
    die "#{ASEPSISCTL_SYMLINK_PATH} does not exist" unless File.exists? ASEPSISCTL_SYMLINK_PATH
    sys("sudo rm \"#{ASEPSISCTL_SYMLINK_PATH}\"")
end