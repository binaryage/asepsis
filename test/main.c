#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "echelon.h"
#include "logging.h"

#include "../daemon/shared.c"

static int g_failed_count = 0;

static int test_prefix(const char* path, const char* expected_output, int expected_res) {
    char prefixPath[SAFE_PREFIX_PATH_BUFFER_SIZE];
    int res;
    
    res = makePrefixPath(path, prefixPath);
    if (!(res==expected_res && (res==0 || strcmp(prefixPath, expected_output)==0))) {
        printf("makePrefixPath failed: '%s' => '%s' (%d) expected: '%s'\n", path, prefixPath, res, expected_output);
        g_failed_count++;
        return 1;
    }
        
    return 0;
}

#define LONG_PATH_980 \
"/123456789/123456789/123456789/123456789/123456789/123456789/123456789/123456789/123456789/123456789"\
"/123456789/123456789/123456789/123456789/123456789/123456789/123456789/123456789/123456789/123456789"\
"/123456789/123456789/123456789/123456789/123456789/123456789/123456789/123456789/123456789/123456789"\
"/123456789/123456789/123456789/123456789/123456789/123456789/123456789/123456789/123456789/123456789"\
"/123456789/123456789/123456789/123456789/123456789/123456789/123456789/123456789/123456789/123456789"\
"/123456789/123456789/123456789/123456789/123456789/123456789/123456789/123456789/123456789/123456789"\
"/123456789/123456789/123456789/123456789/123456789/123456789/123456789/123456789/123456789/123456789"\
"/123456789/123456789/123456789/123456789/123456789/123456789/123456789/123456789/123456789/123456789"\
"/123456789/123456789/123456789/123456789/123456789/123456789/123456789/123456789/123456789/123456789"\
"/123456789/123456789/123456789/123456789/123456789/123456789/123456789/123456789"

int main (int argc, const char * argv[]) {
    printf("testing asepsis...\n");
 
    test_prefix("", ECHELON_DSCAGE_PREFIX, 1);
    test_prefix("a", ECHELON_DSCAGE_PREFIX "a", 1);
    test_prefix("/x", ECHELON_DSCAGE_PREFIX "/x", 1);
    test_prefix("/xxx/yyy/zzz", ECHELON_DSCAGE_PREFIX "/xxx/yyy/zzz", 1);
    test_prefix("/xxx/.DS_Store", ECHELON_DSCAGE_PREFIX "/xxx/.DS_Store", 1);
    test_prefix("/xxx/12345/some_file.txt", ECHELON_DSCAGE_PREFIX "/xxx/12345/some_file.txt", 1);

    // PATH_MAX should be 1024
    if (PATH_MAX!=1024) {
        printf("PATH_MAX is %d\n", PATH_MAX);
    }

    test_prefix(LONG_PATH_980 "/abcdefghi.txt", ECHELON_DSCAGE_PREFIX LONG_PATH_980 "/abcdefghi.txt", 1); // still fits
    test_prefix(LONG_PATH_980 "/xxxxx/yyyyy/zzzzz/abcdefghi.txt", "", 0); // does not fit
    test_prefix(LONG_PATH_980 "/xxxxx/yyyyy/zzzzz/abcdefghi/qqq/wwwwwww/eeee/", "", 0); // does not fit

    if (!g_failed_count) {
        printf("all ok :-)\n");
    } else {
        printf("%d failed :-(\n", g_failed_count);
    }
    
    return 0;
}