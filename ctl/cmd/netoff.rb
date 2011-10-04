def cmd_netoff(options)
    sys("defaults write com.apple.desktopservices DSDontWriteNetworkStores true")
end