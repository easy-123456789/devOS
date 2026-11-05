#include "kernel.h"

#include <circle/startup.h>

int main(void)
{
    CKernel kernel;
    if (!kernel.Initialize())
    {
        halt();
        return EXIT_HALT;
    }

    switch (kernel.Run())
    {
    case ShutdownReboot:
        reboot();
        return EXIT_REBOOT;
    case ShutdownHalt:
    default:
        halt();
        return EXIT_HALT;
    }
}
