def cmd_make_dscage(options)
    sys("mkdir -p \"#{PREFIX_PATH}\"")
    sys("chmod 777 \"#{PREFIX_PATH}\"")
end