#include "kernel.h"

#include <circle/string.h>

CKernel *CKernel::s_pThis = 0;

static boolean command_is(const char *line, const char *word)
{
    unsigned i = 0;
    while (line[i] != '\0' && word[i] != '\0' && line[i] == word[i]) ++i;
    return line[i] == '\0' && word[i] == '\0';
}

CKernel::CKernel(void)
: m_Screen(m_Options.GetWidth(), m_Options.GetHeight()),
  m_Timer(&m_Interrupt),
  m_Logger(m_Options.GetLogLevel(), &m_Timer),
  m_USBHCI(&m_Interrupt, &m_Timer, TRUE),
  m_pKeyboard(0),
  m_InputLength(0),
  m_ShutdownMode(ShutdownHalt)
{
    s_pThis = this;
    m_InputLine[0] = '\0';
    m_ActLED.Blink(3);
}

CKernel::~CKernel(void)
{
    s_pThis = 0;
}

void CKernel::WriteText(const char *text)
{
    m_Screen.Write(text, strlen(text));
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

    m_Screen.Write("USB keyboard support enabled. Plug in a keyboard.\n", 51);
    return TRUE;
}

TShutdownMode CKernel::Run(void)
{
    while (m_ShutdownMode != ShutdownReboot)
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
                WriteText("Keyboard detected. Type 'help' for commands.\n> ");
            }
        }

        if (m_pKeyboard != 0) m_pKeyboard->UpdateLEDs();
    }

    return ShutdownReboot;
}

void CKernel::KeyPressedHandler(const char *text)
{
    if (s_pThis != 0) s_pThis->HandleInput(text);
}

void CKernel::HandleInput(const char *text)
{
    if (text == 0) return;

    while (*text != '\0')
    {
        char ch = *text++;
        if (ch == '\r' || ch == '\n')
        {
            WriteText("\n");
            m_InputLine[m_InputLength] = '\0';
            RunCommand();
            m_InputLength = 0;
            m_InputLine[0] = '\0';
            if (m_ShutdownMode != ShutdownReboot) WriteText("> ");
        }
        else if (ch == '\b' || ch == 0x7f)
        {
            if (m_InputLength != 0)
            {
                --m_InputLength;
                m_InputLine[m_InputLength] = '\0';
                WriteText("\b \b");
            }
        }
        else if ((unsigned char) ch >= 0x20 && (unsigned char) ch <= 0x7e)
        {
            if (m_InputLength + 1 < sizeof m_InputLine)
            {
                m_InputLine[m_InputLength++] = ch;
                m_InputLine[m_InputLength] = '\0';
                m_Screen.Write(&ch, 1);
            }
        }
    }
}

void CKernel::RunCommand(void)
{
    if (command_is(m_InputLine, "help"))
    {
        WriteText("Commands: help, status, systems, reboot\n");
    }
    else if (command_is(m_InputLine, "status"))
    {
        WriteText("USB HID keyboard: active\nThis Circle firmware is a keyboard shell; the original C kernel's SD loader is not included.\n");
    }
    else if (command_is(m_InputLine, "systems"))
    {
        WriteText("Game Boy, Game Boy Color, and Game Boy Advance: planned\nNintendo DS / DSi: prototype plans\n");
    }
    else if (command_is(m_InputLine, "reboot"))
    {
        WriteText("Rebooting...\n");
        m_ShutdownMode = ShutdownReboot;
    }
    else if (m_InputLine[0] != '\0')
    {
        WriteText("Unknown command. Type 'help'.\n");
    }
}

void CKernel::KeyboardRemovedHandler(CDevice *, void *)
{
    if (s_pThis == 0) return;
    s_pThis->m_pKeyboard = 0;
    s_pThis->WriteText("Keyboard disconnected.\n");
}
