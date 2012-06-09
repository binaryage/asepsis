#define DSCAGE_PREFIX "/usr/local/.dscage"

#define DSSTORE_LEN 9 // == strlen(".DS_Store")
#define SAFE_PREFIX_PATH_BUFFER_SIZE (PATH_MAX + PATH_MAX + 1)

// if you really need to touch this, don't forget to also modify paths ctl/asepsisctl!
#define DISABLED_TWEAK_PATH "/var/db/.asepsis.disabled"