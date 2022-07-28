///////////////////////////////////////////////////////////////////////////////
//
// Main screen shows current dome position if dome position changes.
// Push button will activate SelectScreen
// 
///////////////////////////////////////////////////////////////////////////////

class RemoteScreen : public CommandScreen
{
public:
    RemoteScreen() :
        CommandScreen(sDisplay, kRemoteScreen)
    {}

    virtual void init() override
    {
        fLastDrawSeq = ~fDrawSeq;
    }

    void drawCommand(uint8_t x, uint8_t y, bool invert, bool centered, uint8_t textSize, char* buffer)
    {
        fX = x;
        fY = y;
        fInvert = invert;
        fCentered = centered;
        fTextSize = textSize;
        strncpy(fBuffer, buffer, sizeof(fBuffer));
        fDrawSeq++;
    }

    virtual void render() override
    {
        char buffer[sizeof(fBuffer)];
        strncpy(buffer, fBuffer, sizeof(buffer));
        if (fDrawSeq != fLastDrawSeq)
        {
            sDisplay.clearDisplay();
            sDisplay.invertDisplay(fInvert);
            sDisplay.setTextSize(fTextSize);
            if (fCentered)
            {
                int16_t x1, y1;
                uint16_t w, h;
                sDisplay.getTextBounds(buffer, 0, 0, &x1, &y1, &w, &h);
                sDisplay.setCursor(SCREEN_WIDTH / 2 - w / 2, 0);
                sDisplay.print(buffer);
            }
            else
            {
                sDisplay.setCursor(fX, fY);
                char *p = buffer;
                char *str;
                while ((str = strtok_r(p, "\n", &p)) != NULL)
                    sDisplay.println(str);
            }
            sDisplay.display();
            fLastDrawSeq = fDrawSeq;
        }
    }

    virtual void buttonUpPressed(bool repeat = false)
    {
        sendSMQButtonEvent(1, true, repeat);
    }
    virtual void buttonLeftPressed(bool repeat = false)
    {
        sendSMQButtonEvent(2, true, repeat);
    }
    virtual void buttonDownPressed(bool repeat = false)
    {
        sendSMQButtonEvent(3, true, repeat);
    }
    virtual void buttonRightPressed(bool repeat = false)
    {
        sendSMQButtonEvent(4, true, repeat);
    }
    virtual void buttonInPressed(bool repeat = false)
    {
        sendSMQButtonEvent(5, true, repeat);
    }
    virtual void buttonUpReleased()
    {
        sendSMQButtonEvent(1, false);
    }
    virtual void buttonLeftReleased()
    {
        sendSMQButtonEvent(2, false);
    }
    virtual void buttonDownReleased()
    {
        sendSMQButtonEvent(3, false);
    }
    virtual void buttonRightReleased()
    {
        sendSMQButtonEvent(4, false);
    }
    virtual void buttonInReleased()
    {
        sendSMQButtonEvent(5, false);
    }
    virtual void buttonDial(long newValue, long oldValue = 0)
    {
        if (SMQ::sendTopic("DIAL"))
        {
            SMQ::send_long("new", newValue);
            SMQ::send_long("old", oldValue);
            SMQ::sendEnd();
        }
        else
        {
            printf("FAILE TO SEND DIAL\n");
        }
    }

protected:
    void sendSMQButtonEvent(uint8_t id, bool pressed, bool repeat = false)
    {
        if (SMQ::sendTopic("BUTTON"))
        {
            SMQ::send_uint8("id", id);
            SMQ::send_boolean("pressed", pressed);
            SMQ::send_boolean("repeat", repeat);
            SMQ::sendEnd();
        }
        else
        {
            printf("FAILE TO SEND BUTTON\n");
        }
    }

    uint32_t fDrawSeq = 0;
    uint32_t fLastDrawSeq = ~0;
    uint8_t fX = 0;
    uint8_t fY = 0;
    bool fInvert = false;
    bool fCentered = false;
    uint8_t fTextSize = 0;
    char fBuffer[200];
};

///////////////////////////////////////////////////////////////////////////////
//
// Instantiate the screen
//
///////////////////////////////////////////////////////////////////////////////

RemoteScreen sRemoteScreen;
