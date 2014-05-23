def cmd_install_wrapper(options)
    force = options[:force]

    codesign_check()

    # prevent installing on future OS versions
    os_version_check() unless force

    # sanity check
    ds_lib = File.join(DS_LIB_FOLDER, "DesktopServicesPriv")
    if desktopservicespriv_wrapper?(ds_lib) then
        die("wrapper framework seems to be installed (#{ds_lib} was created by Asepsis), to reinstall please run: \"asepsisctl uninstall_wrapper\" first")
    end

    # forced installation should remove potentially existing backup
    if force and File.exists? DS_LIB_RELOCATED_FOLDER
      sys("sudo rm -rf \"#{DS_LIB_RELOCATED_FOLDER}\"")
    end

    # sanity check
    if File.exists? DS_LIB_RELOCATED_FOLDER
        die("wrapper framework seems to be installed (#{DS_LIB_RELOCATED_FOLDER} exists), to reinstall please run: \"asepsisctl uninstall_wrapper\" first")
    end
    
    # make panic backup (only once, the first time asepsis was installed)
    unless File.exists? DS_LIB_PANIC_BACKUP_FOLDER then
        sys("sudo cp -a \"#{DS_LIB_FOLDER}\" \"#{DS_LIB_PANIC_BACKUP_FOLDER}\"")
    end
    
    # make timestamped panic backup
    timestamp = Time.now.strftime("%Y%m%d%H%M%S")
    panic_backup_folder_with_timestamp = "#{DS_LIB_PANIC_BACKUP_FOLDER}_#{timestamp}"
    unless File.exists? panic_backup_folder_with_timestamp then
        sys("sudo cp -a \"#{DS_LIB_FOLDER}\" \"#{panic_backup_folder_with_timestamp}\"")
    end

    # make normal backup
    sys("sudo cp -a \"#{DS_LIB_FOLDER}\" \"#{DS_LIB_BACKUP_FOLDER}\"")
    sys("sudo touch \"#{DS_LIB_ASEPSIS_REV}\"")

    # replace DesktopServicesPriv inplace
    sys("sudo cp -a \"#{DS_LIB_FOLDER}\" \"#{DS_LIB_RELOCATED_FOLDER}\"")
    sys("sudo \"#{RESOURCES_PATH}/install_name_tool\" -id \"#{DS_LIB_RELOCATED_FOLDER}/DesktopServicesPriv\" \"#{DS_LIB_RELOCATED_FOLDER}/DesktopServicesPriv\"")
    # apply code signatures only under Mavericks and higher
    sys("sudo /usr/bin/codesign --timestamp=none --force --sign - \"#{DS_LIB_RELOCATED_FOLDER}/DesktopServicesPriv\"") unless lions?
    sys("sudo cp \"#{DS_WRAPPER_SOURCE_PATH}\" \"#{DS_LIB_FOLDER}/DesktopServicesPriv\"")
    # since 10.9.3 it is important to codesing in-place after copying the file, see https://github.com/binaryage/asepsis/issues/13
    sys("sudo /usr/bin/codesign --timestamp=none --force --sign - \"#{DS_LIB_FOLDER}\"") unless lions?
    sys("sudo rm -rf \"#{DS_LIB_FOLDER}/_CodeSignature\"")
end