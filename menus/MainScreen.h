///////////////////////////////////////////////////////////////////////////////
//
// Main screen shows current dome position if dome position changes.
// Push button will activate SelectScreen
// 
///////////////////////////////////////////////////////////////////////////////

class MainScreen : public CommandScreen
{
public:
    MainScreen() :
        CommandScreen(sDisplay, kMainScreen)
    {}

    virtual void init() override
    {
        fCurrentDisplayItem = -1;
    }

    virtual void render() override
    {
        if (fCurrentItem != fCurrentDisplayItem)
        {
            sDisplay.invertDisplay(false);
            sDisplay.clearDisplay();
            sDisplay.setTextSize(2);
            sDisplay.setCursor(0, 0);
            sDisplay.println(getHostName(fCurrentItem));
            sDisplay.display();
            fCurrentDisplayItem = fCurrentItem;
        }
    }

    virtual void buttonUpPressed(bool repeat) override
    {
        fCurrentItem = getPreviousHost(fCurrentItem);
    }

    virtual void buttonDownPressed(bool repeat) override
    {
        fCurrentItem = getNextHost(fCurrentItem);
    }

    virtual void buttonRightReleased() override
    {
        buttonInReleased();
    }

    virtual void buttonInReleased() override
    {
        if (isValidHost(fCurrentItem))
        {
            printf("Selected: %s\n", fCurrentItem.c_str());
            switchToScreen(kRemoteScreen);
            if (SMQ::sendTopic("SELECT", fCurrentItem))
            {
                SMQ::sendEnd();
            }
        }
    }

    void addHost(String name, String addr)
    {
        Host* host = new Host(name, addr);
        if (fHead == nullptr)
        {
            fCurrentItem = addr;
            fHead = host;
        }
        else
        {
            fHead->fNext = host;
        }
        fTail = host;
        fCurrentDisplayItem = "dummy";
        restoreScreen();
    }

    void removeHost(String addr)
    {
        Host* host;
        Host* prev = nullptr;
        Host** hostp = &fHead;
        while ((host = *hostp) != nullptr)
        {
            if (host->fAddr == addr)
            {
                *hostp = host->fNext;
                if (host->fNext == nullptr)
                    fTail = prev;
                delete host;
            }
            else
            {
                hostp = &host->fNext;
            }
            prev = host;
        }
        if (fCurrentDisplayItem == addr)
        {
            fCurrentDisplayItem = "dummy";
            sDisplay.switchToScreen(kMainScreen);
            restoreScreen();
        }
    }

protected:
    struct Host
    {
        Host(String name, String addr) :
            fName(name),
            fAddr(addr),
            fNext(nullptr)
        {
        }
        String fName;
        String fAddr;
        Host* fNext;
    };
    Host* fHead = nullptr;
    Host* fTail = nullptr;
    String fCurrentItem = "";
    String fCurrentDisplayItem = "dummy";

    String getPreviousHost(String addr)
    {
        Host* prevHost = nullptr;
        for (Host* host = fHead; host != nullptr; host = host->fNext)
        {
            if (host->fAddr == addr)
            {
                if (prevHost != nullptr)
                    return prevHost->fAddr;
                if (fTail != nullptr)
                    return fTail->fAddr;
                break;
            }
            prevHost = host;
        }
        return "";
    }

    String getNextHost(String addr)
    {
        for (Host* host = fHead; host != nullptr; host = host->fNext)
        {
            if (host->fAddr == addr)
            {
                if (host->fNext != nullptr)
                    return host->fNext->fAddr;
                if (fHead != nullptr)
                    return fHead->fAddr;
                break;
            }
        }
        return "";
    }

    bool isValidHost(String addr)
    {
        for (Host* host = fHead; host != nullptr; host = host->fNext)
        {
            // printf("getHostName addr=%s host->fAddr=%s\n", addr.c_str(), host->fAddr.c_str());
            if (host->fAddr == addr)
                return true;
        }
        return false;
    }

    String getHostName(String addr)
    {
        for (Host* host = fHead; host != nullptr; host = host->fNext)
        {
            // printf("getHostName addr=%s host->fAddr=%s\n", addr.c_str(), host->fAddr.c_str());
            if (host->fAddr == addr)
                return host->fName;
        }
        return "No Device";
    }
};

///////////////////////////////////////////////////////////////////////////////
//
// Instantiate the screen
//
///////////////////////////////////////////////////////////////////////////////

MainScreen sMainScreen;
