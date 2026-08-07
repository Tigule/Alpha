#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#include <Base/Base.h>
#include "Input.h"
#include "Os/OsTime.h"

#include <windows.h>
#include <imm.h>
#include <string.h>

#ifndef WM_XBUTTONDOWN
#define WM_XBUTTONDOWN 0x020B
#endif

#ifndef WM_XBUTTONUP
#define WM_XBUTTONUP 0x020C
#endif

// The bundled VC6 SDK predates the standard VK_OEM_* names used here.
#ifndef VK_OEM_1
#define VK_OEM_1      0xBA
#define VK_OEM_PLUS   0xBB
#define VK_OEM_COMMA  0xBC
#define VK_OEM_MINUS  0xBD
#define VK_OEM_PERIOD 0xBE
#define VK_OEM_2      0xBF
#define VK_OEM_3      0xC0
#define VK_OEM_4      0xDB
#define VK_OEM_5      0xDC
#define VK_OEM_6      0xDD
#define VK_OEM_7      0xDE
#endif

typedef long (*OSWINDOWPROC)(LPVOID window, UINT message, UINT wparam, long lparam);

struct OSEVENT {
  OSINPUT id;
  int     param[4];
};

static OSWINDOWPROC  s_windowProc;
static long          s_savedResize;
static RECT          s_defaultwindowrect;
static int           s_screenIsWindow = 1;
static int           s_AppIsActive = 1;
static UINT          s_buttonState;
static int           s_releasing;
static OSEVENT       s_queue[0x10];
static int           s_queueHead;
static int           s_queueTail;
static POINT         s_mousePos;
static HWND          s_mouseWnd;
static POINT         s_mouseCenter;
static OS_MOUSE_MODE s_mouseMode;
static short         s_numlockState;

static UINT latin1lookup[0x20] = {0xFFFEu, 0xFFFEu, 0x201Au, 0x0192u, 0x201Eu, 0x2026u, 0x2020u, 0x2021u, 0x02C6u, 0x2030u, 0x0160u,
                                  0x2039u, 0x0152u, 0xFFFEu, 0xFFFEu, 0xFFFEu, 0xFFFEu, 0x2018u, 0x2019u, 0x201Cu, 0x201Du, 0x2022u,
                                  0x2013u, 0x2014u, 0x02DCu, 0x2122u, 0x0161u, 0x203Au, 0x0153u, 0xFFFEu, 0xFFFEu, 0x0178u};

static UINT latin2lookup[0x100] = {
    0x0000u, 0x0001u, 0x0002u, 0x0003u, 0x0004u, 0x0005u, 0x0006u, 0x0007u, 0x0008u, 0x0009u, 0x000Au, 0x000Bu, 0x000Cu, 0x000Du, 0x000Eu, 0x000Fu,
    0x0010u, 0x0011u, 0x0012u, 0x0013u, 0x0014u, 0x0015u, 0x0016u, 0x0017u, 0x0018u, 0x0019u, 0x001Au, 0x001Bu, 0x001Cu, 0x001Du, 0x001Eu, 0x001Fu,
    0x0020u, 0x0021u, 0x0022u, 0x0023u, 0x0024u, 0x0025u, 0x0026u, 0x0027u, 0x0028u, 0x0029u, 0x002Au, 0x002Bu, 0x002Cu, 0x002Du, 0x002Eu, 0x002Fu,
    0x0030u, 0x0031u, 0x0032u, 0x0033u, 0x0034u, 0x0035u, 0x0036u, 0x0037u, 0x0038u, 0x0039u, 0x003Au, 0x003Bu, 0x003Cu, 0x003Du, 0x003Eu, 0x003Fu,
    0x0040u, 0x0041u, 0x0042u, 0x0043u, 0x0044u, 0x0045u, 0x0046u, 0x0047u, 0x0048u, 0x0049u, 0x004Au, 0x004Bu, 0x004Cu, 0x004Du, 0x004Eu, 0x004Fu,
    0x0050u, 0x0051u, 0x0052u, 0x0053u, 0x0054u, 0x0055u, 0x0056u, 0x0057u, 0x0058u, 0x0059u, 0x005Au, 0x005Bu, 0x005Cu, 0x005Du, 0x005Eu, 0x005Fu,
    0x0060u, 0x0061u, 0x0062u, 0x0063u, 0x0064u, 0x0065u, 0x0066u, 0x0067u, 0x0068u, 0x0069u, 0x006Au, 0x006Bu, 0x006Cu, 0x006Du, 0x006Eu, 0x006Fu,
    0x0070u, 0x0071u, 0x0072u, 0x0073u, 0x0074u, 0x0075u, 0x0076u, 0x0077u, 0x0078u, 0x0079u, 0x007Au, 0x007Bu, 0x007Cu, 0x007Du, 0x007Eu, 0x007Fu,
    0x20ACu, 0xFFFEu, 0x201Au, 0xFFFEu, 0x201Eu, 0x2026u, 0x2020u, 0x2021u, 0xFFFEu, 0x2030u, 0x0160u, 0x2039u, 0x015Au, 0x0164u, 0x017Du, 0x0179u,
    0xFFFEu, 0x2018u, 0x2019u, 0x201Cu, 0x201Du, 0x2022u, 0x2013u, 0x2014u, 0xFFFEu, 0x2122u, 0x0161u, 0x203Au, 0x015Bu, 0x0165u, 0x017Eu, 0x017Au,
    0x00A0u, 0x02C7u, 0x02D8u, 0x0141u, 0x00A4u, 0x0104u, 0x00A6u, 0x00A7u, 0x00A8u, 0x00A9u, 0x015Eu, 0x00ABu, 0x00ACu, 0x00ADu, 0x00AEu, 0x017Bu,
    0x00B0u, 0x00B1u, 0x02DBu, 0x0142u, 0x00B4u, 0x00B5u, 0x00B6u, 0x00B7u, 0x00B8u, 0x0105u, 0x015Fu, 0x00BBu, 0x013Du, 0x02DDu, 0x013Eu, 0x017Cu,
    0x0154u, 0x00C1u, 0x00C2u, 0x0102u, 0x00C4u, 0x0139u, 0x0106u, 0x00C7u, 0x010Cu, 0x00C9u, 0x0118u, 0x00CBu, 0x011Au, 0x00CDu, 0x00CEu, 0x010Eu,
    0x0110u, 0x0143u, 0x0147u, 0x00D3u, 0x00D4u, 0x0150u, 0x00D6u, 0x00D7u, 0x0158u, 0x016Eu, 0x00DAu, 0x0170u, 0x00DCu, 0x00DDu, 0x0162u, 0x00DFu,
    0x0155u, 0x00E1u, 0x00E2u, 0x0103u, 0x00E4u, 0x013Au, 0x0107u, 0x00E7u, 0x010Du, 0x00E9u, 0x0119u, 0x00EBu, 0x011Bu, 0x00EDu, 0x00EEu, 0x010Fu,
    0x0111u, 0x0144u, 0x0148u, 0x00F3u, 0x00F4u, 0x0151u, 0x00F6u, 0x00F7u, 0x0159u, 0x016Fu, 0x00FAu, 0x0171u, 0x00FCu, 0x00FDu, 0x0163u, 0x02D9u
};

static UINT cyrilliclookup[0x100] = {
    0x0000u, 0x0001u, 0x0002u, 0x0003u, 0x0004u, 0x0005u, 0x0006u, 0x0007u, 0x0008u, 0x0009u, 0x000Au, 0x000Bu, 0x000Cu, 0x000Du, 0x000Eu, 0x000Fu,
    0x0010u, 0x0011u, 0x0012u, 0x0013u, 0x0014u, 0x0015u, 0x0016u, 0x0017u, 0x0018u, 0x0019u, 0x001Au, 0x001Bu, 0x001Cu, 0x001Du, 0x001Eu, 0x001Fu,
    0x0020u, 0x0021u, 0x0022u, 0x0023u, 0x0024u, 0x0025u, 0x0026u, 0x0027u, 0x0028u, 0x0029u, 0x002Au, 0x002Bu, 0x002Cu, 0x002Du, 0x002Eu, 0x002Fu,
    0x0030u, 0x0031u, 0x0032u, 0x0033u, 0x0034u, 0x0035u, 0x0036u, 0x0037u, 0x0038u, 0x0039u, 0x003Au, 0x003Bu, 0x003Cu, 0x003Du, 0x003Eu, 0x003Fu,
    0x0040u, 0x0041u, 0x0042u, 0x0043u, 0x0044u, 0x0045u, 0x0046u, 0x0047u, 0x0048u, 0x0049u, 0x004Au, 0x004Bu, 0x004Cu, 0x004Du, 0x004Eu, 0x004Fu,
    0x0050u, 0x0051u, 0x0052u, 0x0053u, 0x0054u, 0x0055u, 0x0056u, 0x0057u, 0x0058u, 0x0059u, 0x005Au, 0x005Bu, 0x005Cu, 0x005Du, 0x005Eu, 0x005Fu,
    0x0060u, 0x0061u, 0x0062u, 0x0063u, 0x0064u, 0x0065u, 0x0066u, 0x0067u, 0x0068u, 0x0069u, 0x006Au, 0x006Bu, 0x006Cu, 0x006Du, 0x006Eu, 0x006Fu,
    0x0070u, 0x0071u, 0x0072u, 0x0073u, 0x0074u, 0x0075u, 0x0076u, 0x0077u, 0x0078u, 0x0079u, 0x007Au, 0x007Bu, 0x007Cu, 0x007Du, 0x007Eu, 0x007Fu,
    0x0402u, 0x0403u, 0x201Au, 0x0453u, 0x201Eu, 0x2026u, 0x2020u, 0x2021u, 0x20ACu, 0x2030u, 0x0409u, 0x2039u, 0x040Au, 0x040Cu, 0x040Bu, 0x040Fu,
    0x0452u, 0x2018u, 0x2019u, 0x201Cu, 0x201Du, 0x2022u, 0x2013u, 0x2014u, 0x0020u, 0x2122u, 0x0459u, 0x203Au, 0x045Au, 0x045Cu, 0x045Bu, 0x045Fu,
    0x00A0u, 0x040Eu, 0x045Eu, 0x0408u, 0x00A4u, 0x0490u, 0x00A6u, 0x00A7u, 0x0401u, 0x00A9u, 0x0404u, 0x00ABu, 0x00ACu, 0x00ADu, 0x00AEu, 0x0407u,
    0x00B0u, 0x00B1u, 0x0406u, 0x0456u, 0x0491u, 0x00B5u, 0x00B6u, 0x00B7u, 0x0451u, 0x2116u, 0x0454u, 0x00BBu, 0x0458u, 0x0405u, 0x0455u, 0x0457u,
    0x0410u, 0x0411u, 0x0412u, 0x0413u, 0x0414u, 0x0415u, 0x0416u, 0x0417u, 0x0418u, 0x0419u, 0x041Au, 0x041Bu, 0x041Cu, 0x041Du, 0x041Eu, 0x041Fu,
    0x0420u, 0x0421u, 0x0422u, 0x0423u, 0x0424u, 0x0425u, 0x0426u, 0x0427u, 0x0428u, 0x0429u, 0x042Au, 0x042Bu, 0x042Cu, 0x042Du, 0x042Eu, 0x042Fu,
    0x0430u, 0x0431u, 0x0432u, 0x0433u, 0x0434u, 0x0435u, 0x0436u, 0x0437u, 0x0438u, 0x0439u, 0x043Au, 0x043Bu, 0x043Cu, 0x043Du, 0x043Eu, 0x043Fu,
    0x0440u, 0x0441u, 0x0442u, 0x0443u, 0x0444u, 0x0445u, 0x0446u, 0x0447u, 0x0448u, 0x0449u, 0x044Au, 0x044Bu, 0x044Cu, 0x044Du, 0x044Eu, 0x044Fu
};

static UINT latin5lookup[0x100] = {
    0x0000u, 0x0001u, 0x0002u, 0x0003u, 0x0004u, 0x0005u, 0x0006u, 0x0007u, 0x0008u, 0x0009u, 0x000Au, 0x000Bu, 0x000Cu, 0x000Du, 0x000Eu, 0x000Fu,
    0x0010u, 0x0011u, 0x0012u, 0x0013u, 0x0014u, 0x0015u, 0x0016u, 0x0017u, 0x0018u, 0x0019u, 0x001Au, 0x001Bu, 0x001Cu, 0x001Du, 0x001Eu, 0x001Fu,
    0x0020u, 0x0021u, 0x0022u, 0x0023u, 0x0024u, 0x0025u, 0x0026u, 0x0027u, 0x0028u, 0x0029u, 0x002Au, 0x002Bu, 0x002Cu, 0x002Du, 0x002Eu, 0x002Fu,
    0x0030u, 0x0031u, 0x0032u, 0x0033u, 0x0034u, 0x0035u, 0x0036u, 0x0037u, 0x0038u, 0x0039u, 0x003Au, 0x003Bu, 0x003Cu, 0x003Du, 0x003Eu, 0x003Fu,
    0x0040u, 0x0041u, 0x0042u, 0x0043u, 0x0044u, 0x0045u, 0x0046u, 0x0047u, 0x0048u, 0x0049u, 0x004Au, 0x004Bu, 0x004Cu, 0x004Du, 0x004Eu, 0x004Fu,
    0x0050u, 0x0051u, 0x0052u, 0x0053u, 0x0054u, 0x0055u, 0x0056u, 0x0057u, 0x0058u, 0x0059u, 0x005Au, 0x005Bu, 0x005Cu, 0x005Du, 0x005Eu, 0x005Fu,
    0x0060u, 0x0061u, 0x0062u, 0x0063u, 0x0064u, 0x0065u, 0x0066u, 0x0067u, 0x0068u, 0x0069u, 0x006Au, 0x006Bu, 0x006Cu, 0x006Du, 0x006Eu, 0x006Fu,
    0x0070u, 0x0071u, 0x0072u, 0x0073u, 0x0074u, 0x0075u, 0x0076u, 0x0077u, 0x0078u, 0x0079u, 0x007Au, 0x007Bu, 0x007Cu, 0x007Du, 0x007Eu, 0x007Fu,
    0x20ACu, 0x0020u, 0x201Au, 0x0192u, 0x201Eu, 0x2026u, 0x2020u, 0x2021u, 0x02C6u, 0x2030u, 0x0160u, 0x2039u, 0x0152u, 0x0020u, 0x0020u, 0x0020u,
    0x0020u, 0x2018u, 0x2019u, 0x201Cu, 0x201Du, 0x2022u, 0x2013u, 0x2014u, 0x02DCu, 0x2122u, 0x0161u, 0x203Au, 0x0153u, 0x0020u, 0x0020u, 0x0178u,
    0x00A0u, 0x00A1u, 0x00A2u, 0x00A3u, 0x00A4u, 0x00A5u, 0x00A6u, 0x00A7u, 0x00A8u, 0x00A9u, 0x00AAu, 0x00ABu, 0x00ACu, 0x00ADu, 0x00AEu, 0x00AFu,
    0x00B0u, 0x00B1u, 0x00B2u, 0x00B3u, 0x00B4u, 0x00B5u, 0x00B6u, 0x00B7u, 0x00B8u, 0x00B9u, 0x00BAu, 0x00BBu, 0x00BCu, 0x00BDu, 0x00BEu, 0x00BFu,
    0x00C0u, 0x00C1u, 0x00C2u, 0x00C3u, 0x00C4u, 0x00C5u, 0x00C6u, 0x00C7u, 0x00C8u, 0x00C9u, 0x00CAu, 0x00CBu, 0x00CCu, 0x00CDu, 0x00CEu, 0x00CFu,
    0x011Eu, 0x00D1u, 0x00D2u, 0x00D3u, 0x00D4u, 0x00D5u, 0x00D6u, 0x00D7u, 0x00D8u, 0x00D9u, 0x00DAu, 0x00DBu, 0x00DCu, 0x0130u, 0x015Eu, 0x00DFu,
    0x00E0u, 0x00E1u, 0x00E2u, 0x00E3u, 0x00E4u, 0x00E5u, 0x00E6u, 0x00E7u, 0x00E8u, 0x00E9u, 0x00EAu, 0x00EBu, 0x00ECu, 0x00EDu, 0x00EEu, 0x00EFu,
    0x011Fu, 0x00F1u, 0x00F2u, 0x00F3u, 0x00F4u, 0x00F5u, 0x00F6u, 0x00F7u, 0x00F8u, 0x00F9u, 0x00FAu, 0x00FBu, 0x00FCu, 0x0131u, 0x015Fu, 0x00FFu
};

static UINT thailookup[0x100] = {
    0x0000u, 0x0001u, 0x0002u, 0x0003u, 0x0004u, 0x0005u, 0x0006u, 0x0007u, 0x0008u, 0x0009u, 0x000Au, 0x000Bu, 0x000Cu, 0x000Du, 0x000Eu, 0x000Fu,
    0x0010u, 0x0011u, 0x0012u, 0x0013u, 0x0014u, 0x0015u, 0x0016u, 0x0017u, 0x0018u, 0x0019u, 0x001Au, 0x001Bu, 0x001Cu, 0x001Du, 0x001Eu, 0x001Fu,
    0x0020u, 0x0021u, 0x0022u, 0x0023u, 0x0024u, 0x0025u, 0x0026u, 0x0027u, 0x0028u, 0x0029u, 0x002Au, 0x002Bu, 0x002Cu, 0x002Du, 0x002Eu, 0x002Fu,
    0x0030u, 0x0031u, 0x0032u, 0x0033u, 0x0034u, 0x0035u, 0x0036u, 0x0037u, 0x0038u, 0x0039u, 0x003Au, 0x003Bu, 0x003Cu, 0x003Du, 0x003Eu, 0x003Fu,
    0x0040u, 0x0041u, 0x0042u, 0x0043u, 0x0044u, 0x0045u, 0x0046u, 0x0047u, 0x0048u, 0x0049u, 0x004Au, 0x004Bu, 0x004Cu, 0x004Du, 0x004Eu, 0x004Fu,
    0x0050u, 0x0051u, 0x0052u, 0x0053u, 0x0054u, 0x0055u, 0x0056u, 0x0057u, 0x0058u, 0x0059u, 0x005Au, 0x005Bu, 0x005Cu, 0x005Du, 0x005Eu, 0x005Fu,
    0x0060u, 0x0061u, 0x0062u, 0x0063u, 0x0064u, 0x0065u, 0x0066u, 0x0067u, 0x0068u, 0x0069u, 0x006Au, 0x006Bu, 0x006Cu, 0x006Du, 0x006Eu, 0x006Fu,
    0x0070u, 0x0071u, 0x0072u, 0x0073u, 0x0074u, 0x0075u, 0x0076u, 0x0077u, 0x0078u, 0x0079u, 0x007Au, 0x007Bu, 0x007Cu, 0x007Du, 0x007Eu, 0x007Fu,
    0x20ACu, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x2026u, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u,
    0x0020u, 0x2018u, 0x2019u, 0x201Cu, 0x201Du, 0x2022u, 0x2013u, 0x2014u, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0020u,
    0x00A0u, 0x0E01u, 0x0E02u, 0x0E03u, 0x0E04u, 0x0E05u, 0x0E06u, 0x0E07u, 0x0E08u, 0x0E09u, 0x0E0Au, 0x0E0Bu, 0x0E0Cu, 0x0E0Du, 0x0E0Eu, 0x0E0Fu,
    0x0E10u, 0x0E11u, 0x0E12u, 0x0E13u, 0x0E14u, 0x0E15u, 0x0E16u, 0x0E17u, 0x0E18u, 0x0E19u, 0x0E1Au, 0x0E1Bu, 0x0E1Cu, 0x0E1Du, 0x0E1Eu, 0x0E1Fu,
    0x0E20u, 0x0E21u, 0x0E22u, 0x0E23u, 0x0E24u, 0x0E25u, 0x0E26u, 0x0E27u, 0x0E28u, 0x0E29u, 0x0E2Au, 0x0E2Bu, 0x0E2Cu, 0x0E2Du, 0x0E2Eu, 0x0E2Fu,
    0x0E30u, 0x0E31u, 0x0E32u, 0x0E33u, 0x0E34u, 0x0E35u, 0x0E36u, 0x0E37u, 0x0E38u, 0x0E39u, 0x0E3Au, 0x0020u, 0x0020u, 0x0020u, 0x0020u, 0x0E3Fu,
    0x0E40u, 0x0E41u, 0x0E42u, 0x0E43u, 0x0E44u, 0x0E45u, 0x0E46u, 0x0E47u, 0x0E48u, 0x0E49u, 0x0E4Au, 0x0E4Bu, 0x0E4Cu, 0x0E4Du, 0x0E4Eu, 0x0E4Fu,
    0x0E50u, 0x0E51u, 0x0E52u, 0x0E53u, 0x0E54u, 0x0E55u, 0x0E56u, 0x0E57u, 0x0E58u, 0x0E59u, 0x0E5Au, 0x0E5Bu, 0x0020u, 0x0020u, 0x0020u, 0x0020u
};

LPVOID OsGuiGetWindow(int inWindowType);
BOOL   OsGuiProcessMessage(LPVOID inMsgData);
BOOL   OsGuiIsModifierKeyDown(int inKey);
int    OsSleepInBackground();
DWORD  OsGetBackgroundSleepMs();

static BOOL OsQueueGet(OSINPUT *id, int *param0, int *param1, int *param2, int *param3);
static void OsQueueSetParam(int index, int param);
static void CenterMouse();
static void RestoreMouse();
static void SaveMouse(HWND window, const POINT &pt);
static void OsQueuePut(OSINPUT id, int param0, int param1, int param2, int param3);
static BOOL ConvertKeyCode(int vkey, KEY *key);
static BOOL ConvertButton(UINT message, UINT wparam, MOUSEBUTTON *button);

static void OsQueuePut(OSINPUT id, int param0, int param1, int param2, int param3) {
  int nextHead;

  nextHead = s_queueHead == 0xF ? 0 : s_queueHead + 1;
  if (nextHead == s_queueTail) {
    s_queueTail = s_queueTail == 0xF ? 0 : s_queueTail + 1;
  }

  s_queue[s_queueHead].id = id;
  s_queue[s_queueHead].param[0] = param0;
  s_queue[s_queueHead].param[1] = param1;
  s_queue[s_queueHead].param[2] = param2;
  s_queue[s_queueHead].param[3] = param3;
  s_queueHead = nextHead;
}

static BOOL OsQueueGet(OSINPUT *id, int *param0, int *param1, int *param2, int *param3) {
  if (s_queueTail == s_queueHead) {
    return 0;
  }

  OSEVENT *event = &s_queue[s_queueTail];
  *id = event->id;
  *param0 = event->param[0];
  *param1 = event->param[1];
  *param2 = event->param[2];
  *param3 = event->param[3];

  if (s_queueTail == 0xF) {
    s_queueTail = 0;
  } else {
    ++s_queueTail;
  }

  return 1;
}

static void OsQueueSetParam(int index, int param) {
  int queueIndex = s_queueTail;

  while (queueIndex != s_queueHead) {
    s_queue[queueIndex].param[index] = param;
    if (queueIndex == 0xF) {
      queueIndex = 0;
    } else {
      ++queueIndex;
    }
  }
}

static BOOL ConvertKeyCode(int vkey, KEY *key) {
  if (vkey >= '0' && vkey <= '9') {
    *key = static_cast<KEY>(vkey);
    return 1;
  }

  if (vkey >= 'A' && vkey <= 'Z') {
    *key = static_cast<KEY>(vkey);
    return 1;
  }

  if (vkey >= VK_F1 && vkey <= VK_F12) {
    *key = static_cast<KEY>(KEY_F1 + vkey - VK_F1);
    return 1;
  }

  switch (vkey) {
    case VK_SHIFT:
      *key = KEY_SHIFT;
      break;
    case VK_CONTROL:
      *key = KEY_CONTROL;
      break;
    case VK_MENU:
      *key = KEY_ALT;
      break;
    case VK_ESCAPE:
      *key = KEY_ESCAPE;
      break;
    case VK_RETURN:
      *key = KEY_ENTER;
      break;
    case VK_BACK:
      *key = KEY_BACKSPACE;
      break;
    case VK_TAB:
      *key = KEY_TAB;
      break;
    case VK_LEFT:
      *key = KEY_LEFT;
      break;
    case VK_UP:
      *key = KEY_UP;
      break;
    case VK_RIGHT:
      *key = KEY_RIGHT;
      break;
    case VK_DOWN:
      *key = KEY_DOWN;
      break;
    case VK_INSERT:
      *key = KEY_INSERT;
      break;
    case VK_DELETE:
      *key = KEY_DELETE;
      break;
    case VK_HOME:
      *key = KEY_HOME;
      break;
    case VK_END:
      *key = KEY_END;
      break;
    case VK_PRIOR:
      *key = KEY_PAGEUP;
      break;
    case VK_NEXT:
      *key = KEY_PAGEDOWN;
      break;
    case VK_CAPITAL:
      *key = KEY_CAPSLOCK;
      break;
    case VK_NUMLOCK:
      *key = KEY_NUMLOCK;
      break;
    case VK_SCROLL:
      *key = KEY_SCROLLLOCK;
      break;
    case VK_PAUSE:
      *key = KEY_PAUSE;
      break;
    case VK_SNAPSHOT:
      *key = KEY_PRINTSCREEN;
      break;
    case VK_SPACE:
      *key = KEY_SPACE;
      break;

    case VK_NUMPAD0:
      *key = KEY_NUMPAD0;
      break;
    case VK_NUMPAD1:
      *key = KEY_NUMPAD1;
      break;
    case VK_NUMPAD2:
      *key = KEY_NUMPAD2;
      break;
    case VK_NUMPAD3:
      *key = KEY_NUMPAD3;
      break;
    case VK_NUMPAD4:
      *key = KEY_NUMPAD4;
      break;
    case VK_NUMPAD5:
      *key = KEY_NUMPAD5;
      break;
    case VK_NUMPAD6:
      *key = KEY_NUMPAD6;
      break;
    case VK_NUMPAD7:
      *key = KEY_NUMPAD7;
      break;
    case VK_NUMPAD8:
      *key = KEY_NUMPAD8;
      break;
    case VK_NUMPAD9:
      *key = KEY_NUMPAD9;
      break;
    case VK_ADD:
      *key = KEY_NUMPAD_PLUS;
      break;
    case VK_SUBTRACT:
      *key = KEY_NUMPAD_MINUS;
      break;
    case VK_MULTIPLY:
      *key = KEY_NUMPAD_MULTIPLY;
      break;
    case VK_DIVIDE:
      *key = KEY_NUMPAD_DIVIDE;
      break;
    case VK_DECIMAL:
      *key = KEY_NUMPAD_DECIMAL;
      break;

    case VK_OEM_PLUS:
      *key = KEY_PLUS;
      break;
    case VK_OEM_MINUS:
      *key = KEY_MINUS;
      break;
    case VK_OEM_4:
      *key = KEY_BRACKET_OPEN;
      break;
    case VK_OEM_6:
      *key = KEY_BRACKET_CLOSE;
      break;
    case VK_OEM_2:
      *key = KEY_SLASH;
      break;
    case VK_OEM_5:
      *key = KEY_BACKSLASH;
      break;
    case VK_OEM_1:
      *key = KEY_SEMICOLON;
      break;
    case VK_OEM_7:
      *key = KEY_APOSTROPHE;
      break;
    case VK_OEM_COMMA:
      *key = KEY_COMMA;
      break;
    case VK_OEM_PERIOD:
      *key = KEY_PERIOD;
      break;
    case VK_OEM_3:
      *key = KEY_TILDE;
      break;

    default:
      *key = KEY_SHIFT;
      return 0;
  }

  return 1;
}

static void CenterMouse() {
  RECT r;

  GetWindowRect(static_cast<HWND>(OsGuiGetWindow(0)), &r);
  s_mouseCenter.x = r.right / 2;
  s_mouseCenter.y = r.bottom / 2;
  SetCursorPos(s_mouseCenter.x, s_mouseCenter.y);
}

static BOOL ConvertButton(UINT message, UINT wparam, MOUSEBUTTON *button) {
  switch (message) {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
      *button = MOUSE_BUTTON_LEFT;
      return 1;

    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
      *button = MOUSE_BUTTON_RIGHT;
      return 1;

    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
      *button = MOUSE_BUTTON_MIDDLE;
      return 1;

    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
      if (HIWORD(wparam) == 1) {
        *button = MOUSE_BUTTON_XBUTTON1;
        return 1;
      }
      if (HIWORD(wparam) == 2) {
        *button = MOUSE_BUTTON_XBUTTON2;
        return 1;
      }
      break;
  }

  *button = MOUSE_BUTTON_NONE;
  return 0;
}

static void RestoreMouse() {
  POINT pt = s_mousePos;

  ClientToScreen(s_mouseWnd, &pt);
  SetCursorPos(pt.x, pt.y);
}

static void SaveMouse(HWND window, const POINT &pt) {
  s_mouseWnd = window;
  s_mousePos = pt;
}

BOOL OsInputGet(OSINPUT *id, int *param0, int *param1, int *param2, int *param3) {
  MSG message;
  int messageAvailable;

  *id = static_cast<OSINPUT>(-1);

  if (s_savedResize) {
    int w = static_cast<short>(LOWORD(s_savedResize));
    int h = static_cast<short>(HIWORD(s_savedResize));

    s_defaultwindowrect.right = static_cast<long>(static_cast<float>(w));
    s_defaultwindowrect.left = 0;
    s_defaultwindowrect.bottom = static_cast<long>(static_cast<float>(h));
    s_defaultwindowrect.top = 0;

    *id = OS_INPUT_SIZE;
    *param0 = w;
    *param1 = h;
    *param2 = 0;
    *param3 = 0;
    s_savedResize = 0;
    return 1;
  }

  if (s_queueTail != s_queueHead) {
    OsQueueGet(id, param0, param1, param2, param3);
    return 1;
  }

  messageAvailable = PeekMessageA(&message, 0, 0, 0, PM_NOREMOVE);
  if (s_queueTail != s_queueHead) {
    goto deliverMessage;
  }

  while (messageAvailable) {
    if (!GetMessageA(&message, 0, 0, 0)) {
      *id = OS_INPUT_SHUTDOWN;
      return 1;
    }

    if (OsGuiProcessMessage(&message)) {
      goto deliverMessage;
    }

    if (s_queueTail != s_queueHead) {
      goto deliverMessage;
    }

    TranslateMessage(&message);
    DispatchMessageA(&message);

    if (s_queueTail != s_queueHead) {
      goto deliverMessage;
    }

    messageAvailable = PeekMessageA(&message, 0, 0, 0, PM_NOREMOVE);
    if (s_queueTail != s_queueHead) {
      goto deliverMessage;
    }
  }

  if (!s_AppIsActive && OsSleepInBackground()) {
    OsSleep(OsGetBackgroundSleepMs());
  }

  return 0;

deliverMessage:
  OsQueueSetParam(3, static_cast<int>(message.time));
  OsQueueGet(id, param0, param1, param2, param3);
  return 1;
}

void OsInputNotifyScreenResize(int x, int y) {
  s_savedResize = x | (y << 16);
}

void OsInputSetScreenIsWindow(int inVal) {
  s_screenIsWindow = inVal;
}

void OsInputSetMouseMode(OS_MOUSE_MODE mode) {
  FATALASSERT(mode < OS_MOUSE_MODES);

  if (mode == s_mouseMode) {
    return;
  }

  if (mode == OS_MOUSE_MODE_RELATIVE) {
    s_mouseMode = OS_MOUSE_MODE_RELATIVE;
    CenterMouse();
  } else if (s_mouseMode == OS_MOUSE_MODE_RELATIVE) {
    s_mouseMode = OS_MOUSE_MODE_NORMAL;
    RestoreMouse();
  }
}

void OsInputGetMousePosition(int *x, int *y) {
  HWND  window = static_cast<HWND>(OsGuiGetWindow(0));
  POINT pt;

  GetCursorPos(&pt);
  ScreenToClient(window, &pt);
  SaveMouse(window, pt);

  if (x) {
    *x = pt.x;
  }
  if (y) {
    *y = pt.y;
  }
}

void OsInputSetMousePosition(int x, int y) {
  HWND  window = static_cast<HWND>(OsGuiGetWindow(0));
  POINT pt;

  pt.x = x;
  pt.y = y;
  SaveMouse(window, pt);
  ClientToScreen(window, &pt);
  SetCursorPos(pt.x, pt.y);
}

BOOL OsGetDefaultWindowRect(RECT *rect) {
  FATALASSERT(rect);

  if ((!s_defaultwindowrect.right || !s_defaultwindowrect.bottom) && !GetClientRect(static_cast<HWND>(OsGuiGetWindow(0)), &s_defaultwindowrect)) {
    return 0;
  }

  *rect = s_defaultwindowrect;
  return 1;
}

UINT OsInputGetCodePage() {
  return GetACP();
}

void OsInputInitialize() {
  s_numlockState = GetAsyncKeyState(VK_NUMLOCK);
}

void OsInputDestroy() {
  INPUT event;

  if (GetAsyncKeyState(VK_NUMLOCK) != s_numlockState) {
    memset(&event, 0, sizeof(event));
    event.type = INPUT_KEYBOARD;
    event.ki.wVk = VK_NUMLOCK;
    SendInput(1, &event, sizeof(event));
  }
}

void OsSetWindowProc(OSWINDOWPROC windowproc) {
  ASSERT(!(windowproc && s_windowProc));

  s_windowProc = windowproc;
}

long OsWindowProc(LPVOID _window, UINT message, UINT wparam, long lparam) {
  HWND        hWnd = static_cast<HWND>(_window);
  POINT       pt;
  KEY         key;
  MOUSEBUTTON button;
  int         buttonDown;
  UINT        character;
  UINT        byte;
  UINT        codepage;
  UINT        imeFlags;

  switch (message) {
    case WM_SIZE:
      s_savedResize = lparam;
      break;

    case WM_ACTIVATE:
      s_buttonState = 0;
      OsQueuePut(OS_INPUT_FOCUS, static_cast<WORD>(LOWORD(wparam)) != WA_INACTIVE, 0, 0, 0);
      break;

    case WM_CLOSE:
      OsQueuePut(OS_INPUT_CLOSE, 0, 0, 0, 0);
      return 0;

    case WM_ACTIVATEAPP:
      s_AppIsActive = wparam;
      break;

    case WM_INPUTLANGCHANGE:
      OsQueuePut(OS_INPUT_IME, 0, 0, 0, 0);
      return 1;

    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
      if (!ConvertKeyCode(wparam, &key)) {
        break;
      }

      OsQueuePut(
          message == WM_KEYDOWN || message == WM_SYSKEYDOWN ? OS_INPUT_KEY_DOWN : OS_INPUT_KEY_UP, key, static_cast<WORD>(LOWORD(lparam)), 0, 0
      );

      if (key == KEY_F4 && OsGuiIsModifierKeyDown(KEY_ALT)) {
        break;
      }
      return 0;

    case WM_CHAR:
      if (wparam < 0x20) {
        break;
      }

      character = wparam;
      byte = wparam & 0xFF;
      if (byte >= 0x80) {
        codepage = OsInputGetCodePage();
        if (codepage == 1250) {
          character = latin2lookup[byte];
        } else if (codepage == 874) {
          character = thailookup[byte];
        } else if (codepage == 1251) {
          character = cyrilliclookup[byte];
        } else if (codepage == 1252 && byte < 0xA0) {
          character = latin1lookup[wparam & 0x7F];
        } else if (codepage == 1254) {
          character = latin5lookup[byte];
        }
      }

      OsQueuePut(OS_INPUT_CHAR, character, static_cast<short>(LOWORD(lparam)), 0, 0);
      return 0;

    case WM_IME_STARTCOMPOSITION:
      OsQueuePut(OS_INPUT_IME, 1, 0, 0, 0);
      return 1;

    case WM_IME_ENDCOMPOSITION:
      OsQueuePut(OS_INPUT_IME, 6, 0, 0, 0);
      return 1;

    case WM_IME_COMPOSITION:
      imeFlags = 0;
      if (lparam & 0x80) {
        imeFlags |= 1;
      }
      if (lparam & 0x08) {
        imeFlags |= 2;
      }
      if (lparam & 0x800) {
        imeFlags |= 4;
      }
      OsQueuePut(OS_INPUT_IME, 2, 0, imeFlags, 0);
      return 1;

    case WM_SYSCOMMAND:
      if (wparam == SC_CLOSE) {
        OsQueuePut(OS_INPUT_CLOSE, 0, 0, 0, 0);
        return 0;
      }
      if (s_windowProc) {
        return s_windowProc(_window, message, wparam, lparam);
      }
      if (wparam == SC_KEYMENU) {
        return 0;
      }
      return DefWindowProcA(hWnd, message, wparam, lparam);

    case WM_MOUSEMOVE:
      GetCursorPos(&pt);
      if (s_mouseMode == OS_MOUSE_MODE_NORMAL) {
        ScreenToClient(hWnd, &pt);
        OsQueuePut(OS_INPUT_MOUSE_MOVE, 0, pt.x, pt.y, 0);
        SaveMouse(hWnd, pt);
      } else {
        if (s_mouseMode != OS_MOUSE_MODE_RELATIVE) {
          FATALERROR(("Invalid case: %s=%u", "s_mouseMode", s_mouseMode));
        }

        if (pt.x != s_mouseCenter.x || pt.y != s_mouseCenter.y) {
          OsQueuePut(OS_INPUT_MOUSE_MOVE_RELATIVE, 0, pt.x - s_mouseCenter.x, pt.y - s_mouseCenter.y, 0);
          CenterMouse();
        }
      }
      break;

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
      if (!ConvertButton(message, wparam, &button)) {
        break;
      }

      buttonDown = message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN || message == WM_MBUTTONDOWN || message == WM_XBUTTONDOWN;
      if (buttonDown) {
        if (!s_buttonState) {
          SetCapture(hWnd);
        }
        s_buttonState |= button;
      } else {
        s_buttonState &= ~button;
        if (!s_buttonState) {
          s_releasing = 1;
          ReleaseCapture();
          s_releasing = 0;
        }
      }

      OsQueuePut(
          buttonDown ? OS_INPUT_MOUSE_DOWN : OS_INPUT_MOUSE_UP, button, static_cast<short>(LOWORD(lparam)), static_cast<short>(HIWORD(lparam)), 0
      );
      return message == WM_XBUTTONDOWN || message == WM_XBUTTONUP;

    case WM_MOUSEWHEEL:
      OsQueuePut(OS_INPUT_MOUSE_WHEEL, static_cast<short>(HIWORD(wparam)), static_cast<short>(LOWORD(lparam)), static_cast<short>(HIWORD(lparam)), 0);
      return 0;

    case WM_CAPTURECHANGED:
      if (s_releasing) {
        break;
      }

      s_buttonState = 0;
      GetCursorPos(&pt);
      ScreenToClient(hWnd, &pt);
      OsQueuePut(OS_INPUT_CAPTURE_CHANGED, 0, pt.x, pt.y, 0);
      return 0;

    case WM_DISPLAYCHANGE:
      if (s_screenIsWindow) {
        s_savedResize = lparam;
      }
      break;

    case WM_IME_SETCONTEXT:
      OsQueuePut(OS_INPUT_IME, 0, 0, 0, 0);
      lparam = 0;
      break;

    case WM_IME_NOTIFY:
      switch (wparam) {
        case IMN_CHANGECANDIDATE:
          OsQueuePut(OS_INPUT_IME, 4, 0, lparam, 0);
          break;
        case IMN_CLOSECANDIDATE:
          OsQueuePut(OS_INPUT_IME, 5, 0, 0, 0);
          break;
        case IMN_OPENCANDIDATE:
          OsQueuePut(OS_INPUT_IME, 3, 0, lparam, 0);
          break;
        case IMN_SETCONVERSIONMODE:
        case IMN_SETOPENSTATUS:
          OsQueuePut(OS_INPUT_IME, 0, 0, 0, 0);
          break;
      }
      break;

    case WM_IME_CONTROL:
    case WM_IME_COMPOSITIONFULL:
    case WM_IME_SELECT:
      return 0;
  }

  if (s_windowProc) {
    return s_windowProc(_window, message, wparam, lparam);
  }
  return DefWindowProcA(hWnd, message, wparam, lparam);
}
