#include <stddef.h>

#include "xfs_hls_test.h"
#include "xparameters.h"


XFs_hls_test_Config XFs_hls_test_ConfigTable[] = {
    {
        "fs_hls_test_0",
        XPAR_FS_HLS_TEST_0_BASEADDR,
    },
    {
        NULL,
        0U,
    },
};
