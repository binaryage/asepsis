def cmd_create_symlink(options)
    sys("mkdir -p /usr/local/bin")
    sys("rm \"#{ASEPSISCTL_SYMLINK_PATH}\"") if File.exists? ASEPSISCTL_SYMLINK_PATH
    sys("ln -Fs \"#{ASEPSISCTL_SYMLINK_SOURCE_PATH}\" \"#{ASEPSISCTL_SYMLINK_PATH}\"")
end