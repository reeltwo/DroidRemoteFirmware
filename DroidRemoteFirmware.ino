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
#include "core/StringUtils.h"
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

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "menus/CommandScreenDisplay.h"
Adafruit_SSD1306 sLCD(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
CommandScreenDisplay<Adafruit_SSD1306> sDisplay(sLCD, sPinManager, []() {
    sDisplay.setEnabled(sLCD.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS));
    return sDisplay.isEnabled();
});

///////////////////////////////////////////////////////////////////////////////

#include "menus/RemoteScreen.h"
#include "menus/MainScreen.h"
#include "menus/SplashScreen.h"

////////////////////////////////

#include <Preferences.h>
Preferences preferences;
#define PREFERENCE_REMOTE_HOSTNAME          "rhost"
#define PREFERENCE_REMOTE_SECRET            "rsecret"

////////////////////////////////

void setup()
{
    REELTWO_READY();

    PrintReelTwoInfo(Serial, "Droid Remote");

    if (!preferences.begin("roamadome", false))
    {
        DEBUG_PRINTLN("Failed to init prefs");
    }

    SetupEvent::ready();

    //Puts ESP in STATION MODE
    WiFi.mode(WIFI_STA);
    if (SMQ::init(preferences.getString(PREFERENCE_REMOTE_HOSTNAME, SMQ_HOSTNAME),
                    preferences.getString(PREFERENCE_REMOTE_SECRET, SMQ_SECRET)))
    {
        printf("Droid Remote %s:%s\n",
            preferences.getString(PREFERENCE_REMOTE_HOSTNAME, SMQ_HOSTNAME).c_str(),
                preferences.getString(PREFERENCE_REMOTE_SECRET, SMQ_SECRET).c_str());
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
    }
    Wire.begin();

    Wire.beginTransmission(SCREEN_ADDRESS);
    Wire.endTransmission();
    if (sDisplay.begin())
    {
        printf("ENABLED\n");
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

#define CONSOLE_BUFFER_SIZE                 300
#define COMMAND_BUFFER_SIZE                 256

static bool sNextCommand;
static bool sProcessing;
static unsigned sPos;
static uint32_t sWaitNextSerialCommand;
static char sBuffer[CONSOLE_BUFFER_SIZE];
static bool sCmdNextCommand;
static char sCmdBuffer[COMMAND_BUFFER_SIZE];

static void runSerialCommand()
{
    sWaitNextSerialCommand = 0;
    sProcessing = true;
}

static void resetSerialCommand()
{
    sWaitNextSerialCommand = 0;
    sNextCommand = false;
    sProcessing = (sCmdBuffer[0] == ':');
    sPos = 0;
}

static void abortSerialCommand()
{
    sBuffer[0] = '\0';
    sCmdBuffer[0] = '\0';
    sCmdNextCommand = false;
    resetSerialCommand();
}

void reboot()
{
    Serial.println(F("Restarting..."));
    preferences.end();
    ESP.restart();
}

void processConfigureCommand(const char* cmd)
{
    if (startswith_P(cmd, F("#DRRESTART")))
    {
        reboot();
    }
    else if (startswith_P(cmd, F("#DRNAME")))
    {
        String newName = String(cmd);
        if (preferences.getString(PREFERENCE_REMOTE_HOSTNAME, SMQ_HOSTNAME) != newName)
        {
            preferences.putString(PREFERENCE_REMOTE_HOSTNAME, cmd);
            printf("Changed.\n");
            reboot();
        }
    }
    else if (startswith_P(cmd, F("#DRSECRET")))
    {
        String newSecret = String(cmd);
        if (preferences.getString(PREFERENCE_REMOTE_SECRET, SMQ_HOSTNAME) != newSecret)
        {
            preferences.putString(PREFERENCE_REMOTE_SECRET, newSecret);
            printf("Changed.\n");
            reboot();
        }
    }
    else if (startswith_P(cmd, F("#DRCONFIG")))
    {
        printf("Name=%s\n", preferences.getString(PREFERENCE_REMOTE_HOSTNAME, SMQ_HOSTNAME).c_str());
        printf("Secret=%s\n", preferences.getString(PREFERENCE_REMOTE_HOSTNAME, SMQ_SECRET).c_str());
    }
    else
    {
        Serial.println(F("Invalid"));
    }
}

bool processDroidRemoteCommand(const char* cmd)
{
    switch (*cmd++)
    {
        default:
            // INVALID
            return false;
    }
    return true;
}

bool processCommand(const char* cmd, bool firstCommand)
{
    sWaitNextSerialCommand = 0;
    if (*cmd == '\0')
        return true;
    if (!firstCommand)
    {
        if (cmd[0] != ':')
        {
            Serial.println(F("Invalid"));
            return false;
        }
        return processDroidRemoteCommand(cmd+1);
    }
    switch (cmd[0])
    {
        case ':':
            if (cmd[1] == 'D' && cmd[2] == 'R')
                return processDroidRemoteCommand(cmd+3);
            break;
        case '#':
            processConfigureCommand(cmd);
            return true;
        default:
            Serial.println(F("Invalid"));
            break;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////

void loop()
{
    AnimatedEvent::process();
    sDisplay.process();
    SMQ::process();

    if (Serial.available())
    {
        int ch = Serial.read();
        if (ch == 0x0A || ch == 0x0D)
        {
            runSerialCommand();
        }
        else if (sPos < SizeOfArray(sBuffer)-1)
        {
            sBuffer[sPos++] = ch;
            sBuffer[sPos] = '\0';
        }
    }
    if (sProcessing && millis() > sWaitNextSerialCommand)
    {
        if (sCmdBuffer[0] == ':')
        {
            char* end = strchr(sCmdBuffer+1, ':');
            if (end != nullptr)
                *end = '\0';
            if (!processCommand(sCmdBuffer, !sCmdNextCommand))
            {
                // command invalid abort buffer
                DEBUG_PRINT(F("Unrecognized: ")); DEBUG_PRINTLN(sCmdBuffer);
                abortSerialCommand();
                end = nullptr;
            }
            if (end != nullptr)
            {
                *end = ':';
                strcpy(sCmdBuffer, end);
                DEBUG_PRINT(F("REMAINS: "));
                DEBUG_PRINTLN(sCmdBuffer);
                sCmdNextCommand = true;
            }
            else
            {
                sCmdBuffer[0] = '\0';
                sCmdNextCommand = false;
            }
        }
        else if (sBuffer[0] == ':')
        {
            char* end = strchr(sBuffer+1, ':');
            if (end != nullptr)
                *end = '\0';
            if (!processCommand(sBuffer, !sNextCommand))
            {
                // command invalid abort buffer
                DEBUG_PRINT(F("Unrecognized: ")); DEBUG_PRINTLN(sBuffer);
                abortSerialCommand();
                end = nullptr;
            }
            if (end != nullptr)
            {
                *end = ':';
                strcpy(sBuffer, end);
                sPos = strlen(sBuffer);
                DEBUG_PRINT(F("REMAINS: "));
                DEBUG_PRINTLN(sBuffer);
                sNextCommand = true;
            }
            else
            {
                resetSerialCommand();
                sBuffer[0] = '\0';
            }
        }
        else
        {
            processCommand(sBuffer, true);
            resetSerialCommand();
        }
    }}
#pragma GCC diagnostic pop
