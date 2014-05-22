def cmd_uninstall_wrapper(options)
    sudo = "sudo"
    sudo = "" if options[:nosudo]

    if options[:panic] then
      # find the latest panic backup
      filenames = Dir.glob(DS_LIB_PANIC_BACKUP_FOLDER+"*")
      die("no panic backup folders found at #{DS_LIB_PANIC_BACKUP_FOLDER}") if filenames.size==0
      
      sorted_filenames = filenames.sort_by { |filename| File.mtime(filename) }
      latest_backup = sorted_filenames[-1]
      sys("#{sudo} cp -a \"#{latest_backup}\"/* \"#{DS_LIB_FOLDER}\"")
      exit
    end

    # sanity check
    if not File.exists? DS_LIB_RELOCATED_FOLDER
      die("wrapper framework seems not to be installed (#{DS_LIB_RELOCATED_FOLDER} does not exist)")
    end

    # this is an extra inteligence for this scenario:
    #   1. user has OS X 10.9.2
    #   2. user installs Asepsis, backup of v10.9.2 DesktopServicesPriv is saved into DS_LIB_BACKUP_FOLDER
    #   3. user updates system to OS X 10.9.3, this will overwrite DesktopServicesPriv with v10.9.3
    #   4. Asepsis diagnosis correctly reports broken Asepsis
    #   5. user correctly runs> asepsisctl uninstall_wrapper, this restores DesktopServicesPriv files to v10.9.2
    #   6. OS X 10.9.3 has additional checks and fails to run with v10.9.2 version
    #
    # solution: we check if our DesktopServicesPriv wrapper is present, if not, we skip backup restoration
    #
    ds_lib = File.join(DS_LIB_FOLDER, "DesktopServicesPriv")
    skip_restore = false
    if not desktopservicespriv_wrapper?(ds_lib) then
      puts "DesktopServicesPriv wrapper was replaced by system version since last Asepsis installation.\nUsually this is the case when you install a system update which updates DesktopServicesPriv related files.\n => continuing in wrapper uninstallation, but skipping backup restoration."
      skip_restore = true
    end

    # replace DesktopServicesPriv with original version
    if File.exists? DS_LIB_ASEPSIS_REV then
      # asepsis 1.4 path
      die("the backup folder is missing! (#{DS_LIB_BACKUP_FOLDER} does not exist)") unless File.exists? DS_LIB_BACKUP_FOLDER

      sys("#{sudo} cp -a \"#{DS_LIB_BACKUP_FOLDER}\"/* \"#{DS_LIB_FOLDER}\"") unless skip_restore
      sys("#{sudo} rm -rf \"#{DS_LIB_BACKUP_FOLDER}\"")
      sys("#{sudo} rm -rf \"#{DS_LIB_RELOCATED_FOLDER}\"")
      sys("#{sudo} rm \"#{DS_LIB_ASEPSIS_REV}\"")
    else
      # asepsis 1.3.x and lower
      sys("#{sudo} cp -r \"#{DS_LIB_RELOCATED_FOLDER}/_CodeSignature\" \"#{DS_LIB_FOLDER}\"") unless skip_restore
      sys("#{sudo} cp \"#{DS_LIB_RELOCATED_FOLDER}/DesktopServicesPriv\" \"#{DS_LIB_FOLDER}/DesktopServicesPriv\"") unless skip_restore
      sys("#{sudo} \"#{RESOURCES_PATH}/install_name_tool\" -id \"#{DS_LIB_FOLDER}/DesktopServicesPriv\" \"#{DS_LIB_FOLDER}/DesktopServicesPriv\"") unless skip_restore
      sys("#{sudo} rm -rf \"#{DS_LIB_RELOCATED_FOLDER}\"")
    end
end