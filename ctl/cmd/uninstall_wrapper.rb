def cmd_uninstall_wrapper(options)
    die("wrapper framework seems not to be installed (#{DS_LIB_RELOCATED_FOLDER} does not exist)") unless File.exists? DS_LIB_RELOCATED_FOLDER

    # replace DesktopServicesPriv with original version inplace
    sys("sudo cp -r \"#{DS_LIB_RELOCATED_FOLDER}/Resources\" \"#{DS_LIB_FOLDER}\"")
    sys("sudo cp -r \"#{DS_LIB_RELOCATED_FOLDER}/_CodeSignature\" \"#{DS_LIB_FOLDER}\"")
    sys("sudo cp \"#{DS_LIB_RELOCATED_FOLDER}/DesktopServicesPriv\" \"#{DS_LIB_FOLDER}/DesktopServicesPriv\"")
    sys("sudo install_name_tool -id \"#{DS_LIB_FOLDER}/DesktopServicesPriv\" \"#{DS_LIB_FOLDER}/DesktopServicesPriv\"")
    sys("sudo rm -rf \"#{DS_LIB_RELOCATED_FOLDER}\"")
end