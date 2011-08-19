//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Asepsis.kext from BinaryAge (Echelon)
//   a kernel-level folder renaming watcher for Asepsis
//   more: http://blog.binaryage.com/totalfinder-alpha
//

// following line will be patched by the build script
#define ECHELON_RELEASE_VERSION "debug"

#define ECHELON_BUNDLE_ID "com.binaryage.asepsis.kext" // also modify Info.plist!
#define ECHELON_PATH_MAX 1024 // this should match OS PATH_MAX constant
#define ECHELON_DSCAGE_PREFIX "/usr/local/.dscage"
#define ECHELON_VERSION 1

#define DSSTORE_LEN 9 // == strlen(".DS_Store")
#define SAFE_PREFIX_PATH_BUFFER_SIZE (PATH_MAX + PATH_MAX + 1)

#define ECHELON_OP_RENAME 0
#define ECHELON_OP_DELETE 1

// using binary struct passing between kernel and user-space
// do you believe that your compiler does not screw this structure?
// http://en.wikipedia.org/wiki/Data_structure_alignment
#pragma pack(push)
#pragma pack(1)

struct EchelonMessage {
    unsigned char version; // current version == ECHELON_VERSION
    unsigned char operation;
    char  path1[ECHELON_PATH_MAX];
    char  path2[ECHELON_PATH_MAX];
};

#pragma pack(pop)