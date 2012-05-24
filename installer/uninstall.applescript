property stdout : ""
on run
    set newline to ASCII character 10

    set stdout to stdout & "  asepsisctl uninstall..." & newline
    try
        do shell script "/System/Library/Extensions/Asepsis.kext/Contents/Resources/asepsisctl uninstall" with administrator privileges
    on error
        set stdout to stdout & "  something went wrong?" & newline
    end try
    
    set stdout to stdout & "Asepsis uninstallation done, reboot your computer" & newline
    
    stdout -- this is needed for platypus to display output in details window
end run