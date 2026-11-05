#ifndef _usb_keyboard_kernel_h
#define _usb_keyboard_kernel_h

#include <circle/actled.h>
#include <circle/devicenameservice.h>
#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/koptions.h>
#include <circle/logger.h>
#include <circle/screen.h>
#include <circle/serial.h>
#include <circle/timer.h>
#include <circle/types.h>
#include <circle/usb/usbkeyboard.h>
#include <circle/usb/usbhcidevice.h>

enum TShutdownMode
{
    ShutdownHalt,
    ShutdownReboot
};

class CKernel
{
public:
    CKernel(void);
    ~CKernel(void);

    boolean Initialize(void);
    TShutdownMode Run(void);

private:
    static void KeyPressedHandler(const char *text);
    static void KeyboardRemovedHandler(CDevice *device, void *context);
    void HandleInput(const char *text);
    void RunCommand(void);
    void WriteText(const char *text);

    /* Keep Circle's required device initialization order. */
    CActLED m_ActLED;
    CKernelOptions m_Options;
    CDeviceNameService m_DeviceNameService;
    CScreenDevice m_Screen;
    CSerialDevice m_Serial;
    CExceptionHandler m_ExceptionHandler;
    CInterruptSystem m_Interrupt;
    CTimer m_Timer;
    CLogger m_Logger;
    CUSBHCIDevice m_USBHCI;
    CUSBKeyboardDevice * volatile m_pKeyboard;
    char m_InputLine[80];
    unsigned m_InputLength;
    volatile TShutdownMode m_ShutdownMode;

    static CKernel *s_pThis;
};

#endif
