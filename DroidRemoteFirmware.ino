#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

///////////////////////////////////

#if __has_include("build_version.h")
#include "build_version.h"
#endif

#if __has_include("reeltwo_build_version.h")
#include "reeltwo_build_version.h"
#endif

///////////////////////////////////
// CONFIGURABLE OPTIONS
///////////////////////////////////

#define USE_DEBUG                       // Define to enable debug diagnostic
//#define USE_SMQDEBUG
#define SMQ_HOSTNAME                    "Remote"
#define SMQ_SECRET                      "Astromech"

///////////////////////////////////

#include "pin-map.h"

///////////////////////////////////

#include "ReelTwoSMQ32.h"
#include "core/AnimatedEvent.h"
#include "core/PinManager.h"
#ifdef USE_SERVOS
#include "ServoDispatchDirect.h"
#include "ServoEasing.h"
#endif
#include <Wire.h>

///////////////////////////////////

PinManager sPinManager;

///////////////////////////////////

#define SCREEN_WIDTH            128     // OLED display width, in pixels
#define SCREEN_HEIGHT           32      // OLED display height, in pixels

#define SCREEN_ADDRESS          0x3C
#define KEY_REPEAT_RATE_MS      500     // Key repeat rate in milliseconds

enum ScreenID
{
    kInvalid = -1,
    kSplashScreen = 0,
    kMainScreen = 1,
    kRemoteScreen = 2
};

#include "menus/CommandScreen.h"
#include "menus/CommandScreenHandlerSSD1306.h"

CommandScreenHandlerSSD1306 sDisplay(sPinManager);

///////////////////////////////////////////////////////////////////////////////

#include "menus/RemoteScreen.h"
#include "menus/MainScreen.h"
#include "menus/SplashScreen.h"

////////////////////////////////

void setup()
{
    REELTWO_READY();

    PrintReelTwoInfo(Serial, "Droid Remote");

    SetupEvent::ready();

    //Puts ESP in STATION MODE
    WiFi.mode(WIFI_STA);
    SMQ::init(SMQ_HOSTNAME, SMQ_SECRET);

    SMQ::setHostDiscoveryCallback([](SMQHost* host) {
        if (host->hasTopic("BUTTON"))
        {
            printf("Discovered: %s\n", host->getHostName().c_str());
            sMainScreen.addHost(host->getHostName(), host->getHostAddress());
        }
    });

    SMQ::setHostLostCallback([](SMQHost* host) {
        printf("Lost: %s\n", host->getHostName().c_str());
        sMainScreen.removeHost(host->getHostAddress());
    });

    Wire.begin();

    Wire.beginTransmission(SCREEN_ADDRESS);
    Wire.endTransmission();
    sDisplay.setEnabled(sDisplay.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS));
    if (sDisplay.isEnabled())
    {
        sDisplay.invertDisplay(false);
        sDisplay.clearDisplay();
        sDisplay.setRotation(2);
    }
    Serial.println("READY");
}

///////////////////////////////////////////////////////////////////////////////

SMQMESSAGE(LCD, {
    char buffer[200];
    uint8_t x = msg.get_uint8("x");
    uint8_t y = msg.get_uint8("y");
    bool invert = msg.get_boolean("invert");
    bool centered = msg.get_boolean("centered");
    uint8_t siz = msg.get_uint8("size");
    if (msg.get_string("text", buffer, sizeof(buffer)) != nullptr)
    {
        sRemoteScreen.drawCommand(x, y, invert, centered, siz, buffer);
    }
    else
    {
        printf("RECEIVED LCD (%d,%d) [i:%d,c:%d,s:%d] <null>\n", x, y, invert, centered, siz);
    }
})

///////////////////////////////////////////////////////////////////////////////

SMQMESSAGE(EXIT, {
    String addr = msg.getString("addr");
    sDisplay.switchToScreen(kMainScreen);
})

///////////////////////////////////////////////////////////////////////////////

void loop()
{
    AnimatedEvent::process();
    sDisplay.process();
    SMQ::process();
}
#pragma GCC diagnostic pop
