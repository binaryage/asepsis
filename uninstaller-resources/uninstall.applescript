property stdout : ""
on run
	set newline to ASCII character 10
	set has_error to false
	
	set stdout to stdout & "Calling a shell command: asepsisctl uninstall" & newline
	try
		do shell script "/Library/Application\\ Support/Asepsis/ctl/asepsisctl uninstall" with administrator privileges
	on error
		set stdout to stdout & "=> something went wrong!" & newline
		set has_error to true
	end try
	
	if not has_error then
		set stdout to stdout & "Asepsis uninstallation done, reboot your computer" & newline
	end if
	
	stdout -- this is needed for platypus to display output in details window
end run