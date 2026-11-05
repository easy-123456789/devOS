#include "kernel.h"

#include <circle/string.h>

static const char FromKernel[] = "usb-keyboard";
CKernel *CKernel::s_pThis = 0;

CKernel::CKernel(void)
: m_Screen(m_Options.GetWidth(), m_Options.GetHeight()),
  m_Timer(&m_Interrupt),
  m_Logger(m_Options.GetLogLevel(), &m_Timer),
  m_USBHCI(&m_Interrupt, &m_Timer, TRUE),
  m_pKeyboard(0)
{
    s_pThis = this;
    m_ActLED.Blink(3);
}

CKernel::~CKernel(void)
{
    s_pThis = 0;
}

boolean CKernel::Initialize(void)
{
    if (!m_Screen.Initialize()) return FALSE;
    if (!m_Serial.Initialize(115200)) return FALSE;

    CDevice *logTarget = m_DeviceNameService.GetDevice(m_Options.GetLogDevice(), FALSE);
    if (logTarget == 0) logTarget = &m_Screen;
    if (!m_Logger.Initialize(logTarget)) return FALSE;
    if (!m_Interrupt.Initialize()) return FALSE;
    if (!m_Timer.Initialize()) return FALSE;
    if (!m_USBHCI.Initialize()) return FALSE;

    m_Screen.Write("USB keyboard ready. Plug in a USB keyboard and type:\n", 53);
    return TRUE;
}

TShutdownMode CKernel::Run(void)
{
    for (unsigned count = 0; ; ++count)
    {
        /* Circle services DWC2 and the Pi's built-in USB hub here. */
        boolean changed = m_USBHCI.UpdatePlugAndPlay();
        if (changed && m_pKeyboard == 0)
        {
            m_pKeyboard = (CUSBKeyboardDevice *)
                m_DeviceNameService.GetDevice("ukbd1", FALSE);
            if (m_pKeyboard != 0)
            {
                m_pKeyboard->RegisterRemovedHandler(KeyboardRemovedHandler, 0);
                m_pKeyboard->RegisterKeyPressedHandler(KeyPressedHandler);
                m_Screen.Write("Keyboard detected.\n", 19);
            }
        }

        if (m_pKeyboard != 0) m_pKeyboard->UpdateLEDs();
        m_Screen.Rotor(0, count);
    }
}

void CKernel::KeyPressedHandler(const char *text)
{
    if (s_pThis == 0 || text == 0) return;
    s_pThis->m_Screen.Write(text, strlen(text));
}

void CKernel::KeyboardRemovedHandler(CDevice *, void *)
{
    if (s_pThis == 0) return;
    s_pThis->m_pKeyboard = 0;
    s_pThis->m_Screen.Write("Keyboard disconnected.\n", 23);
}
