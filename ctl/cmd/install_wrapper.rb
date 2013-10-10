def cmd_install_wrapper(options)
    force = options[:force]

    unless force
      `sw_vers -productVersion|grep '10\\.\\(7\\|8\\)'`
      die("this version of Asepsis is only supported under OS X 10.7 or 10.8") if $?!=0
    end

    sys("sudo rm -rf \"#{DS_LIB_RELOCATED_FOLDER}\"") if File.exists? DS_LIB_RELOCATED_FOLDER and force
    die("wrapper framework seems to be installed (#{DS_LIB_RELOCATED_FOLDER} exists), to reinstall please run: \"asepsisctl uninstall_wrapper\" first") if File.exists? DS_LIB_RELOCATED_FOLDER

    # replace DesktopServicesPriv inplace
    sys("sudo cp -r \"#{DS_LIB_FOLDER}\" \"#{DS_LIB_RELOCATED_FOLDER}\"")
    sys("sudo \"#{RESOURCES_PATH}/install_name_tool\" -id \"#{DS_LIB_RELOCATED_FOLDER}/DesktopServicesPriv\" \"#{DS_LIB_RELOCATED_FOLDER}/DesktopServicesPriv\"")

    sys("sudo cp \"#{DS_WRAPPER_SOURCE_PATH}\" \"#{DS_LIB_FOLDER}/DesktopServicesPriv\"")
    sys("sudo rm -rf \"#{DS_LIB_FOLDER}/_CodeSignature\"")
end