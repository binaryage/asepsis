def cmd_uninstall_dylib(options)
    sys("sudo launchctl unsetenv DYLD_INSERT_LIBRARIES")
  
    die("/etc/launchd.conf has been already removed, nothing to do") unless File.exists? "/etc/launchd.conf"
  
    backup = "/etc/launchd.conf.backup"
    postfix = ""
    i = 0
    while File.exists? "#{backup}#{postfix}" do
        i = i + 1
        postfix = i.to_s
    end
  
    backup = "#{backup}#{postfix}"
  
    sys("sudo mv /etc/launchd.conf #{backup}")
    say "launchd.conf successfully renamed, you may want to review and delete #{backup}"
end