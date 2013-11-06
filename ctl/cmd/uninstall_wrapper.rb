def cmd_uninstall_wrapper(options)
    if options[:panic] then
      sys("#{sudo} cp -r \"#{DS_LIB_PANIC_BACKUP_FOLDER}\"/* \"#{DS_LIB_FOLDER}\"")
      exit
    end

    die("wrapper framework seems not to be installed (#{DS_LIB_RELOCATED_FOLDER} does not exist)") unless File.exists? DS_LIB_RELOCATED_FOLDER

    sudo = "sudo"
    sudo = "" if options[:nosudo]

    # replace DesktopServicesPriv with original version
    if File.exists? DS_LIB_ASEPSIS_REV then
      # asepsis 1.4 path
      die("the backup folder is missing! (#{DS_LIB_BACKUP_FOLDER} does not exist)") unless File.exists? DS_LIB_BACKUP_FOLDER

      sys("#{sudo} cp -r \"#{DS_LIB_BACKUP_FOLDER}\"/* \"#{DS_LIB_FOLDER}\"")
      sys("#{sudo} rm -rf \"#{DS_LIB_BACKUP_FOLDER}\"")
      sys("#{sudo} rm -rf \"#{DS_LIB_RELOCATED_FOLDER}\"")
      sys("#{sudo} rm \"#{DS_LIB_ASEPSIS_REV}\"")
    else
      # asepsis 1.3.x and lower
      sys("#{sudo} cp -r \"#{DS_LIB_RELOCATED_FOLDER}/_CodeSignature\" \"#{DS_LIB_FOLDER}\"")
      sys("#{sudo} cp \"#{DS_LIB_RELOCATED_FOLDER}/DesktopServicesPriv\" \"#{DS_LIB_FOLDER}/DesktopServicesPriv\"")
      sys("#{sudo} \"#{RESOURCES_PATH}/install_name_tool\" -id \"#{DS_LIB_FOLDER}/DesktopServicesPriv\" \"#{DS_LIB_FOLDER}/DesktopServicesPriv\"")
      sys("#{sudo} rm -rf \"#{DS_LIB_RELOCATED_FOLDER}\"")
    end
end