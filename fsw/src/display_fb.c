#include "display_fb.h"
#include "display_msg.h"
#include "common_types.h"
#include "cfe_error.h"
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <string.h>
#include <unistd.h>

static uint8 *FBPtr = NULL;
static int   FBFd   = -1;
static struct fb_var_screeninfo VInfo = {0};
static struct fb_fix_screeninfo FInfo = {0};

CFE_Status_t DISPLAY_FbInit(DISPLAY_Table_t *TblPtr)
{
    CFE_Status_t status = CFE_SUCCESS;
    if (TblPtr == NULL)
    {
        status = DISPLAY_STATUS_ERROR_NULL;
    }

    // Open device file
    if (status == CFE_SUCCESS)
    {
        FBFd = open(TblPtr->DevicePath, O_RDWR);
        if (FBFd < 0)
        {
            status = DISPLAY_STATUS_ERROR_OPEN;
        }
    }
    

    // Get fixed info
    if (status == CFE_SUCCESS)
    {
        if (ioctl(FBFd, FBIOGET_FSCREENINFO, &FInfo) == -1)
        {
            status = DISPLAY_STATUS_ERROR_READ;
        }
    }

    // Get var info
    if (status == CFE_SUCCESS)
    {
        if (ioctl(FBFd, FBIOGET_VSCREENINFO, &VInfo) == -1)
        {
            status = DISPLAY_STATUS_ERROR_READ;
        }
    }

    /* Mem Map the screen pixels to local address space */
    uint32 screensize = VInfo.xres * VInfo.yres * (VInfo.bits_per_pixel / 8);
    FBPtr = (uint8 *) mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, FBFd, 0);
    if (FBPtr == MAP_FAILED)
    {
        status = DISPLAY_STATUS_ERROR_OPEN;
    }

    for (int i = 0; i < 8; i++)
    {
        memset(FBPtr, (1 << i), FInfo.smem_len);
        sleep(1);
    }

    return status;
}
