#pragma once;

#include "pch.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <string>
#include <Windows.h>  //for Sleep()

#include "capsicain.h"
#include "constants.h"
#include "modifiers.h"
#include "scancodes.h"
#include "resource.h"

using namespace std;

const string VERSION = "63beta";


string PRETTY_VK_LABELS[MAX_VKEYS]; // contains e.g. [01]="ESC" instead of SC_ESCAPE, and all VKs > 0xFF

//defaults and constants
const int LAYER_DISABLED = 0; // layer 0 does nothing
const string LAYER_DISABLED_LAYER_NAME = "Capsicain disabled. No processing, forward everything. Only ESC-X and ESC-0..9 work.";
const int DEFAULT_ACTIVE_LAYER = 1;
const string DEFAULT_ACTIVE_LAYER_NAME = "Layer not initialized. Forwarding all keys.";
const int DEFAULT_DELAY_ON_STARTUP_MS = 0; //time to release all keys (e.g. if capsicain is started via shortcut)
const int DEFAULT_DELAY_FOR_KEY_SEQUENCE_MS = 5;  //System may drop keys when they are sent too fast. Local host needs 0-1ms, Linux VM 5+ms for 100% reliable keystroke detection

const bool DEFAULT_START_AHK_ON_STARTUP = true;
const int DEFAULT_DELAY_FOR_AHK_MS = 50;	    //autohotkey is slow
const unsigned short AHK_HOTKEY1 = SC_F14;  //this key triggers supporting AHK script
const unsigned short AHK_HOTKEY2 = SC_F15;

vector<string> sanitizedIniContent;  //loaded on startup and reset

struct Options
{
    string iniVersion = "unnamed version - add 'iniVersion xyz' to capsicain.ini";
    bool debug = true;
    int delayForKeySequenceMS = DEFAULT_DELAY_FOR_KEY_SEQUENCE_MS;
    bool flipZy = false;
    bool altAltToAlt = false;
    bool shiftShiftToCapsLock = false;
    bool flipAltWinOnAppleKeyboards = false;
    bool LControlLWinBlocksAlphaMapping = false;
    bool processOnlyFirstKeyboard = false;
    int delayOnStartupMS = DEFAULT_DELAY_ON_STARTUP_MS;
    bool startMinimized = false;
    bool startInTraybar = false;
} option;

struct ModifierCombo
{
    int key = SC_NOP;
    unsigned short modAnd = 0;
    unsigned short modNot = 0;
    unsigned short modTap = 0;
    vector<VKeyEvent> keyEventSequence;
};

struct RewireIftappedMapping
{
    int inkey = SC_NOP;
    int outkey = SC_NOP;
    int ifTapped = SC_NOP;
};

struct AllMaps
{
    vector<RewireIftappedMapping> rewireIftappedMapping;
    vector<ModifierCombo> modCombos;
    int alphamap[MAX_VKEYS] = { SC_NOP };
} allMaps;

struct GlobalState
{
    int  activeLayer = 0;
    string activeLayerName = DEFAULT_ACTIVE_LAYER_NAME;

    InterceptionContext interceptionContext = NULL;
    InterceptionDevice interceptionDevice = NULL;
    InterceptionDevice previousInterceptionDevice = NULL;

    bool realEscapeIsDown = false;

    string deviceIdKeyboard = "";
    bool deviceIsAppleKeyboard = false;

    VKeyEvent previousKeyEventIn = { SC_NOP, 0 };
    VKeyEvent previous2KeyEventIn = { SC_NOP, 0 }; //2 keys ago
    VKeyEvent previousKeyEventInStore = { SC_NOP, 0 }; //mostly useless but simplifies the handling
    bool keysDownSent[256] = { false };  //sent keys must be 8 bit

    bool recordingMacro = false;
    vector<VKeyEvent> recordedMacro;

    vector<VKeyEvent> username;
    vector<VKeyEvent> password;
    bool readingUsername = false;
    bool readingPassword = false;
} globalState;

struct ModifierState
{
    unsigned short modifierDown = 0;
    unsigned short modifierTapped = 0;
    bool lastModBrokeTapping = false;

    vector<VKeyEvent> modsTempAltered;

} modifierState;

struct LoopState
{
    unsigned short scancode = SC_NOP; //hardware code sent by Interception
    int vcode = SC_NOP; //key code used internally; equals scancode or a Virtual code > FF
    bool isDownstroke = false;
    bool isModifier = false;
    bool wasTapped = false;

    InterceptionKeyStroke originalIKstroke = { SC_NOP, 0 };

    bool blockKey = false;  //true: do not send the current key
    vector<VKeyEvent> resultingVKeyEventSequence;

    chrono::steady_clock::time_point loopStartTimepoint;
} loopState;

string errorLog = "";
void error(string txt)
{
    cout << endl << "ERROR: " << txt << endl;
    errorLog += "\r\n" + txt;
}


void readGlobalsOnStartup()
{
    option.debug = configHasTaggedKey(INI_TAG_GLOBAL, "debugOnStartup", sanitizedIniContent);

    getStringValueForTaggedKey(INI_TAG_GLOBAL, "iniVersion", option.iniVersion, sanitizedIniContent);
    if (option.iniVersion == "")
        option.iniVersion = "iniVersion is undefined";

    if (!getIntValueForTaggedKey(INI_TAG_GLOBAL, "delayOnStartupMS", option.delayOnStartupMS, sanitizedIniContent))
        IFDEBUG cout << endl << "No ini setting for 'GLOBAL delayOnStartup'. Using default " << DEFAULT_DELAY_ON_STARTUP_MS;

    option.startMinimized = configHasTaggedKey(INI_TAG_GLOBAL, "startMinimized", sanitizedIniContent);
    option.startInTraybar = configHasTaggedKey(INI_TAG_GLOBAL, "startInTraybar", sanitizedIniContent);

    int initialLayer = DEFAULT_ACTIVE_LAYER;
    if (!getIntValueForTaggedKey(INI_TAG_GLOBAL, "ActiveLayerOnStartup", globalState.activeLayer, sanitizedIniContent))
    {
        globalState.activeLayer = DEFAULT_ACTIVE_LAYER;
        globalState.activeLayerName = DEFAULT_ACTIVE_LAYER_NAME;
        cout << endl << "No ini setting for 'GLOBAL activeLayerOnStartup'. Setting default layer " << DEFAULT_ACTIVE_LAYER;
    }
}

int main()
{
    initConsoleWindow();
    printHelloHeader();

    defineAllPrettyVKLabels(PRETTY_VK_LABELS);
    resetAllStatesToDefault();

    if (!readSanitizeIniFile(sanitizedIniContent))
    {
        cout << endl << "No capsicain.ini - exiting..." << endl;
        Sleep(5000);
        return 0;
    }

    readGlobalsOnStartup();
    for (int i = 0; i < MAX_VKEYS; i++)  //initialize to "map to same char"
        allMaps.alphamap[i] = i;
    switchLayer(globalState.activeLayer);

    cout << endl << "Release all keys now... (waiting DelayOnStartupMS = " << option.delayOnStartupMS << " ms)" << endl;
    Sleep(option.delayOnStartupMS); //time to release shortcut keys that started capsicain

    globalState.interceptionContext = interception_create_context();
    interception_set_filter(globalState.interceptionContext, interception_is_keyboard, INTERCEPTION_FILTER_KEY_ALL);

    printHelloFeatures();

    if (DEFAULT_START_AHK_ON_STARTUP)
    {
        string msg = startProgramSameFolder(PROGRAM_NAME_AHK);
        cout << endl << endl << "starting AHK... ";
        cout << (msg == "" ? "OK" : "Not. '" + msg + "'");
    }
    cout << endl << endl << "[ESC] + [X] to stop." << endl << "[ESC] + [H] for Help";
    cout << endl << endl << "capsicain running.... ";

    if (option.startMinimized)
        ShowInTaskbarMinimized();
    if (option.startInTraybar)
        ShowInTraybar();

    raise_process_priority(); //careful: if we spam key events, other processes get no timeslots to process them. Sleep a bit...

    while (interception_receive( globalState.interceptionContext,
                                 globalState.interceptionDevice = interception_wait(globalState.interceptionContext),
                                 (InterceptionStroke *)&loopState.originalIKstroke, 1)   > 0)
    {
         IFDEBUG cout << ". ";
        //ignore secondary keyboard?
        if (option.processOnlyFirstKeyboard 
            && (globalState.previousInterceptionDevice != NULL)
            && (globalState.previousInterceptionDevice != globalState.interceptionDevice))
        {
            IFDEBUG cout << endl << "Ignore 2nd board (" << globalState.interceptionDevice << ") scancode: " << loopState.originalIKstroke.code;
            interception_send(globalState.interceptionContext, globalState.interceptionDevice, (InterceptionStroke *)&loopState.originalIKstroke, 1);
            continue;
        }

        //check for Apple Keyboard / device ID
        if (globalState.previousInterceptionDevice == NULL    //startup
            || globalState.previousInterceptionDevice != globalState.interceptionDevice)  //keyboard changed
        {
            getHardwareId();
            cout << endl << "new keyboard: " << (globalState.deviceIsAppleKeyboard ? "Apple keyboard" : "IBM keyboard");
            resetCapsNumScrollLock();
            globalState.previousInterceptionDevice = globalState.interceptionDevice;
        }

        loopState.loopStartTimepoint = timepointNow();
        resetLoopState();

        //copy InterceptionKeyStroke (unpleasant to use) to plain KeyEvent
        VKeyEvent originalVKeyEvent = ikstroke2VKeyEvent(loopState.originalIKstroke);
        loopState.scancode = originalVKeyEvent.vcode;
        loopState.isDownstroke = originalVKeyEvent.isDownstroke;

        //TODO this is messy
        //store last key and the one before
        globalState.previous2KeyEventIn = globalState.previousKeyEventIn;
        globalState.previousKeyEventIn = globalState.previousKeyEventInStore;
        globalState.previousKeyEventInStore = originalVKeyEvent;
        
        //Tapped key?
        loopState.wasTapped = !loopState.isDownstroke 
            && (loopState.scancode == globalState.previousKeyEventIn.vcode)
            && ((loopState.scancode != globalState.previous2KeyEventIn.vcode)
                || (!globalState.previous2KeyEventIn.isDownstroke));

        //sanity check
        if (loopState.originalIKstroke.code >= 0x80)
        {
            error("Received unexpected extended Interception Key Stroke code > 0x79: " + to_string(loopState.originalIKstroke.code));
            continue;
        }
        if (loopState.originalIKstroke.code == 0)
        {
            error("Received unexpected SC_NOP Key Stroke code 0. Ignoring this.");
            continue;
        }

        //remember the hardware ESC key state
        //do not send v until ^ comes in
        //ESC+X exits, unconditionally
        //ESC+[0]..[9]  layer switch
        //I process those early, in case layer 0 continues without further checks
        if (loopState.scancode == SC_ESCAPE)
        {
            globalState.realEscapeIsDown = loopState.isDownstroke;
        }
        else if (globalState.realEscapeIsDown && loopState.isDownstroke)
        {
            if (loopState.scancode == SC_X) //extra check to make sure Exit is bug free
            {
                ShowInTaskbar();
                break;
            }
            else if (loopState.scancode == SC_0)
            {
                cout << endl << "LAYER CHANGE: " << LAYER_DISABLED;
                switchLayer(LAYER_DISABLED);
                resetLoopState();
                resetModifierState();
                releaseAllRealModifiers();
                resetAlphaMap();
                continue;
            }
            else if (loopState.scancode >= SC_1 && loopState.scancode <= SC_9)
            {
                int layer = loopState.scancode - 1;
                cout << endl << "LAYER CHANGE: " << layer;
                switchLayer(layer);
                continue;
            }
        }

        //Layer 0: standard keyboard, no further processing, just forward everything
        if (globalState.activeLayer == LAYER_DISABLED)
        {
            interception_send(globalState.interceptionContext, globalState.interceptionDevice, (InterceptionStroke *)&loopState.originalIKstroke, 1);
            continue;
        }

        //hard rewire all REWIREd keys
        loopState.vcode = loopState.scancode;
        processRewireScancodeToVirtualcode();
        if (loopState.vcode == SC_NOP)   //rewired to NOP to disable keys
        {
            IFDEBUG cout << " (NOP)";
            continue;
        }

        //Hardware ESC is lazy key; if it is rewired, that key must not trigger on ESC+command
        if (loopState.scancode == SC_ESCAPE)
        {
            IFDEBUG cout << endl << "(Hard ESC" << (loopState.isDownstroke ? "v " : "^ ") << ")";
            if (loopState.isDownstroke)
                continue;
            //hardware ESC tapped -> reset state incl. sticky modifiers
            if(globalState.previousKeyEventIn.vcode == SC_ESCAPE)
            {
                releaseAllRealModifiers(); //release sticky mods before sending ESC
                //handle rewired ESC^ (send ESC or whatever it is rewired to)
                IFDEBUG cout << endl << "(Send " << PRETTY_VK_LABELS[loopState.vcode] << "v^ )";
                sendVKeyEvent( { loopState.vcode, true } );
                sendVKeyEvent( { loopState.vcode, false } );
                //reset state on hardware ESC
                resetLoopState();
                resetModifierState();
            }
            continue;
        }

        // All other ESC + key commands
        // NOTE: some major key shadowing here...
        // - cherry is good
        // - apple keyboard cannot do RCTRL+CAPS+ESC and ESC+Caps shadows the entire row a-s-d-f-g-....
        // - Dell cant do ctrl-caps-x
        // - Cypher has no RControl... :(
        // - HP shadows the 2-w-s-x and 3-e-d-c lines
        if (globalState.realEscapeIsDown)
        {
            if (loopState.isDownstroke)
            {
                if (processCommand())
                    continue;
                else
                {
                    ShowInTaskbar();
                    break;
                }
            }
            else if(isModifier(loopState.vcode)) //sticky mods with ESC+mod
            {
                continue;
            }
        }

        //Username
        if (globalState.readingUsername)
        {
            if (loopState.vcode == SC_RETURN)
            {
                globalState.readingUsername = false;
                cout << " done";
            }
            else if (loopState.isDownstroke || globalState.username.size() > 0)  //filter upstroke from the command key
                globalState.username.push_back({ loopState.vcode,loopState.isDownstroke });
            continue;
        }
        //Password
        if (globalState.readingPassword)
        {
            if (loopState.scancode == SC_RETURN)
            {
                globalState.readingPassword = false;
                cout << " done";
            }
            else if (loopState.isDownstroke || globalState.password.size() > 0)  //filter upstroke from the command key
                globalState.password.push_back({ loopState.vcode, loopState.isDownstroke });
            continue;
        }

        /////CONFIGURED RULES
        IFDEBUG cout << endl << " [" << \
            (loopState.vcode == loopState.scancode ? "" : PRETTY_VK_LABELS[loopState.scancode] + " => ") \
            << PRETTY_VK_LABELS[loopState.vcode] << getSymbolForIKStrokeState(loopState.originalIKstroke.state)
            << " =" << hex << loopState.originalIKstroke.code << " " << loopState.originalIKstroke.state << "]";

        processModifierState();
    
        IFDEBUG cout << " [M:" << hex << modifierState.modifierDown;
        IFDEBUG if (modifierState.modifierTapped)  cout << " TAP:" << hex << modifierState.modifierTapped;
        IFDEBUG cout << "] ";

        //evaluate modified keys
        if (!loopState.isModifier && (modifierState.modifierDown > 0 || modifierState.modifierTapped > 0))
        {
            processModifiedKeys();
        }

        //basic character key layout. Don't remap the Ctrl combos?
        if (!loopState.isModifier && 
            !(option.LControlLWinBlocksAlphaMapping && (IS_LCTRL_DOWN || IS_LWIN_DOWN) ))
        {
            processMapAlphaKeys(loopState.scancode);
            if (option.flipZy)
            {
                switch (loopState.scancode)
                {
                case SC_Y:		loopState.scancode = SC_Z;		break;
                case SC_Z:		loopState.scancode = SC_Y;		break;
                }
            }
        }

        IFDEBUG cout << "\t (" << dec << millisecondsSinceTimepoint(loopState.loopStartTimepoint) << " ms)";
        IFDEBUG loopState.loopStartTimepoint = timepointNow();
        sendResultingKeyOrSequence();
        IFDEBUG cout << "\t (" << dec << millisecondsSinceTimepoint(loopState.loopStartTimepoint) << " ms)";
    }
    interception_destroy_context(globalState.interceptionContext);

    cout << endl << "bye" << endl;
    return 0;
}
////////////////////////////////////END MAIN//////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////


void processModifiedKeys()
{
    if (loopState.isDownstroke)
    {
        for (ModifierCombo modcombo : allMaps.modCombos)
        {
            if (modcombo.key == loopState.vcode)
            {
                if (
                    (modifierState.modifierDown & modcombo.modAnd) == modcombo.modAnd &&
                    (modifierState.modifierDown & modcombo.modNot) == 0 &&
                    ((modifierState.modifierTapped & modcombo.modTap) == modcombo.modTap)
                    )
                {
                    loopState.resultingVKeyEventSequence = modcombo.keyEventSequence;
                    break;
                }
            }
        }
    }
    modifierState.modifierTapped = 0;
}

void processMapAlphaKeys(unsigned short &scancode)
{
    if (scancode > 0xFF)
    {
        error("Unexpected scancode > 255 while mapping alphachars: " + std::to_string(scancode));
    }
    else
    {
        scancode = allMaps.alphamap[scancode];
    }
}

// [ESC]+x combos
// returns false if exit was requested
// uses the unwired keys for regular keys, and wired modifiers
bool processCommand()
{
    bool continueLooping = true;
    bool popupConsole = false;
    cout << endl << endl << "::";
    
    switch (loopState.scancode)
    {
    case SC_T:
    {
        if (IsCapsicainInTray())
        {
            cout << "Show in taskbar";
            ShowInTaskbar();
        }
        else
        {
            cout << "Show traybar";
            ShowInTraybar();
        }
        break;
    }
    case SC_Q:   // quit only if a debug build
#ifdef NDEBUG
        sendVKeyEvent({ SC_ESCAPE, true });
        sendVKeyEvent({ SC_Q, true });
        sendVKeyEvent({ SC_Q, false });
        sendVKeyEvent({ SC_ESCAPE, false });
#else
        continueLooping = false;
#endif
        break;
    case SC_W:
        option.flipAltWinOnAppleKeyboards = !option.flipAltWinOnAppleKeyboards;
        cout << "Flip ALT<>WIN for Apple boards: " << (option.flipAltWinOnAppleKeyboards ? "ON" : "OFF") << endl;
        break;
    case SC_E:
        cout << "ERROR LOG: " << endl << errorLog << endl;
        popupConsole = true;
        break;
    case SC_R:
        cout << "RESET";
        reset();
        getHardwareId();
        cout << endl << (globalState.deviceIsAppleKeyboard ? "APPLE keyboard (flipping Win<>Alt)" : "PC keyboard");
        break;
    case SC_Y:
        cout << "Stop AHK";
        closeOrKillProgram("autohotkey.exe");
        break;
    case SC_U:
    {
        if (globalState.username.size() == 0)
        {
            cout << "Type Username (Enter)";
            globalState.readingUsername = true;
        }
        else
        {
            cout << "Play user";
            playKeyEventSequence(globalState.username);
        }
        break;
    }
    case SC_I:
    {
        cout << "INI filtered for layer " << globalState.activeLayerName;
        vector<string> assembledLayer = assembleLayerConfig(globalState.activeLayer);
        for (string line : assembledLayer)
            cout << endl << line;
        break;
    }
    case SC_P:
    {
        if (globalState.password.size() == 0)
        {
            cout << "Type Password (Enter)";
            globalState.readingPassword = true;
        }
        else
        {
            cout << "Play password";
            playKeyEventSequence(globalState.password);
        }
        break;
    }
    case SC_A:
    {
        cout << "Start AHK";
        string msg = startProgramSameFolder("autohotkey.exe");
        if (msg != "")
            cout << endl << "Cannot start: " << msg;
        break;
    }
    case SC_S:
        printStatus();
        popupConsole = true;
        break;
    case SC_D:
        option.debug = !option.debug;
        cout << "DEBUG mode: " << (option.debug ? "ON" : "OFF");
        popupConsole = true;
        break;
    case SC_H:
        printHelp();
        popupConsole = true;
        break;
    case SC_J:
        cout << "MACRO START RECORDING";
        globalState.recordingMacro = true;
        globalState.recordedMacro.clear();
        break;
    case SC_K:
        if (globalState.recordingMacro)
            cout << "MACRO STOP RECORDING (" << globalState.recordedMacro.size() << ")";
        else
            cout << "MACRO RECORDING ALREADY STOPPED";
        globalState.recordingMacro = false;
        break;
    case SC_L:
        cout << "MACRO PLAYBACK";
        playKeyEventSequence(globalState.recordedMacro);
        break;
    case SC_SEMI:
    {
        cout << "COPY MACRO TO CLIPBOARD";
        string macro = "";
        for (VKeyEvent key : globalState.recordedMacro)
        {
            if (macro.size() > 0)
                macro += "_";
            if (key.isDownstroke)
                macro += "&";
            else
                macro += "^";
            macro += PRETTY_VK_LABELS[key.vcode];
        }
        copyToClipBoard(macro);
        break;
    }
    case SC_Z:
        option.flipZy = !option.flipZy;
        cout << "Flip Z<>Y mode: " << (option.flipZy ? "ON" : "OFF");
        break;
    case SC_C:
        cout << "List of all Key Labels for scancodes" << endl
             << "------------------------------------" << endl;
        printKeylabels();
        popupConsole = true;
        break;
    case SC_COMMA:
        if (option.delayForKeySequenceMS >= 1)
            option.delayForKeySequenceMS -= 1;
        cout << "delay between characters in key sequences (ms): " << dec << option.delayForKeySequenceMS;
        break;
    case SC_DOT:
        if (option.delayForKeySequenceMS <= 100)
            option.delayForKeySequenceMS += 1;
        cout << "delay between characters in key sequences (ms): " << dec << option.delayForKeySequenceMS;
        break;
    default: 
    {
        //Sticky modifiers with soft (potentially rewired) ESC+modifier. Tap hard ESC to release.
        if (isModifier(loopState.vcode))
        {
            if (loopState.isDownstroke)
            {
                modifierState.modifierDown |= getBitmaskForModifier(loopState.vcode);
                modifierState.modifierTapped = 0;
                if (isRealModifier(loopState.vcode))
                    sendResultingKeyOrSequence();
                IFDEBUG cout << endl << "Locking modifier: " << PRETTY_VK_LABELS[loopState.vcode];
            }
        }
        else
            cout << "Unknown command";
        break;
    }
    }

    if(popupConsole)
    {
        ShowInTaskbar();
    }
    return continueLooping;
}


void sendResultingKeyOrSequence()
{
    if (loopState.resultingVKeyEventSequence.size() > 0)
    {
        playKeyEventSequence(loopState.resultingVKeyEventSequence);
    }
    else
    {
        IFDEBUG
        {
            if (loopState.blockKey)
                cout << "\t--> BLOCKED ";
            else if (loopState.scancode != loopState.vcode)
                cout << "\t--> " << PRETTY_VK_LABELS[loopState.vcode] << " " << getSymbolForIKStrokeState(loopState.originalIKstroke.state);
            else
                cout << "\t--";
        }
        if (!loopState.blockKey && loopState.vcode != SC_NOP)
        {
            sendVKeyEvent({ loopState.vcode, loopState.isDownstroke });
        }
    }
}

void processModifierState()
{
    if (!loopState.isModifier)
        return; 

    unsigned short bitmask = getBitmaskForModifier(loopState.vcode);
    if ((bitmask & 0xFF00) > 0)
        loopState.blockKey = true;

    //a configured tapped mod?
    bool tappedModConfigFound = false;
    if (loopState.wasTapped)
    {
        for (RewireIftappedMapping map : allMaps.rewireIftappedMapping)
        {
            if (loopState.vcode == map.inkey)
                {
                if (map.ifTapped != SC_NOP)
                {
                    if (!loopState.blockKey)
                        loopState.resultingVKeyEventSequence.push_back({ loopState.vcode, false });
                    loopState.resultingVKeyEventSequence.push_back({ map.ifTapped, true });
                    loopState.resultingVKeyEventSequence.push_back({ map.ifTapped, false });
                    loopState.blockKey = false;
                    tappedModConfigFound = true;
                    modifierState.modifierTapped = 0;
                }
                break;
            }
        }
    }

    //set internal modifier state
    if (loopState.isDownstroke)
    {
        modifierState.lastModBrokeTapping = modifierState.modifierTapped & bitmask;
        modifierState.modifierDown |= bitmask;
    }
    else
        modifierState.modifierDown &= ~bitmask;

    //Set tapped bitmask. You can combine mod-taps (like tap-Ctrl then tap-Alt).
    //Double-tap clears all taps.
    //Long presses, release will not register as tapped.
    if (!tappedModConfigFound && !modifierState.lastModBrokeTapping)
    {
        bool sameKey = loopState.scancode == globalState.previousKeyEventIn.vcode;
        if (loopState.wasTapped && modifierState.lastModBrokeTapping)
            modifierState.modifierTapped = 0;
        else if (loopState.wasTapped)
            modifierState.modifierTapped |= bitmask;
        else if (sameKey && loopState.isDownstroke)
        {
            if (modifierState.modifierTapped & bitmask)
            {
                modifierState.lastModBrokeTapping = true;
                modifierState.modifierTapped &= ~bitmask;
            }
            else
                modifierState.lastModBrokeTapping = false;
        }
        else
            modifierState.lastModBrokeTapping = false;
    }
}

//handle all REWIRE configs
void processRewireScancodeToVirtualcode()
{
    if (option.flipAltWinOnAppleKeyboards && globalState.deviceIsAppleKeyboard)
    {
        switch (loopState.scancode)
        {
        case SC_LALT: loopState.vcode = SC_LWIN; break;
        case SC_LWIN: loopState.vcode = SC_LALT; break;
        case SC_RALT: loopState.vcode = SC_RWIN; break;
        case SC_RWIN: loopState.vcode = SC_RALT; break;
        }
    }

    bool finalVcode = false;

    //TODO too complicated
    if (option.altAltToAlt)
    {
        if (loopState.vcode == SC_LALT || loopState.vcode == SC_RALT)
        {
            if (loopState.isDownstroke)
            {
                if (globalState.previousKeyEventIn.vcode != loopState.scancode) //not tapped
                {
                    if (IS_MOD12_DOWN)
                    {
                        modifierState.modifierDown &= ~BITMASK_MOD12; //clear MOD12
                        finalVcode = true;
                    }
                }
                else //tapped
                {
                    if (modifierState.modifierDown & (BITMASK_LALT | BITMASK_RALT))
                        finalVcode = true;
                    else if (modifierState.modifierTapped & BITMASK_MOD12)
                    {
                        modifierState.modifierTapped &= ~BITMASK_MOD12;
                        finalVcode = true;
                    }
                }
            }
            else
            {
                if(    loopState.vcode == SC_LALT && IS_LALT_DOWN
                    || loopState.vcode == SC_RALT && IS_RALT_DOWN)
                    finalVcode = true;
            }
        }
    }

    //TODO how does this recognize tappings???
    if (!finalVcode)
    {
        for (RewireIftappedMapping map : allMaps.rewireIftappedMapping)
        {
            if (loopState.vcode == map.inkey)
            {
                loopState.vcode = map.outkey;
                break;
            }
        }
    }

    if (!finalVcode && option.shiftShiftToCapsLock)
    {
        switch (loopState.vcode)
        {
        case SC_LSHIFT:  //handle LShift+RShift -> CapsLock
            if (loopState.isDownstroke
                && (modifierState.modifierDown == (BITMASK_RSHIFT))
                && (GetKeyState(VK_CAPITAL) & 0x0001)) //ask Win for Capslock state
            {
                keySequenceAppendMakeBreakKey(SC_CAPS, loopState.resultingVKeyEventSequence);
                finalVcode = true;
            }
            break;
        case SC_RSHIFT:
            if (loopState.isDownstroke
                && (modifierState.modifierDown == (BITMASK_LSHIFT))
                && !(GetKeyState(VK_CAPITAL) & 0x0001))
            {
                keySequenceAppendMakeBreakKey(SC_CAPS, loopState.resultingVKeyEventSequence);
                finalVcode = true;
            }
            break;
        }
    }

    loopState.isModifier = isModifier(loopState.vcode) ? true : false;
}

//May contain Capsicain Escape sequences, those are a bit hacky. 
//They are used to temporarily make/break modifiers (for example, ALT+Q -> Shift+1... but don't mess with shift state if it is actually pressed)
//Sequence starts with SC_CPS_ESC DOWN
//Scancodes inside are modifier bitmasks. State DOWN means "set these modifiers if they are up", UP means "clear those if they are down".
//Sequence ends with SC_CPS_ESC UP -> the modifier sequence is played.
//Second SC_CPS_ESC UP -> the previous changes to the modifiers are reverted.
void playKeyEventSequence(vector<VKeyEvent> keyEventSequence)
{
    VKeyEvent newKeyEvent;
    unsigned int delayBetweenKeyEventsMS = option.delayForKeySequenceMS;
    bool inCpsEscape = false;  //inside an escape sequence, read next keyEvent
    int escapeSequenceType = 0; // 1: escape sequence to temp release modifiers; 2: pause; 3... undefined

    IFDEBUG cout << "\t--> SEQUENCE (" << dec << keyEventSequence.size() << ")";
    for (VKeyEvent keyEvent : keyEventSequence)
    {
        if (keyEvent.vcode == SC_CPS_ESC)
        {
            if (keyEvent.isDownstroke)
            {
                if(inCpsEscape)
                    error("Internal error: Received double SC_CPS_ESC down.");
                else
                {
                    if (modifierState.modsTempAltered.size() > 0)
                    {
                        error("Internal error: previous escape sequence for 'temp alter mods' was not undone.");
                        modifierState.modsTempAltered.clear();
                    }
                }
            }
            else if (escapeSequenceType == CPS_ESC_SEQUENCE_TYPE_TEMPALTERMODIFIERS)
            {
                if (inCpsEscape) //play the escape sequence
                {
                    for (VKeyEvent strk : modifierState.modsTempAltered)
                    {
                        sendVKeyEvent(strk);
                        Sleep(delayBetweenKeyEventsMS);
                    }
                }
                else //undo the previous temp modifier changes, in reverse order
                {
                    IFDEBUG cout << endl << "undo alter modifiers" << keyEvent.vcode;
                    VKeyEvent strk;
                    for (size_t i = modifierState.modsTempAltered.size(); i>0; i--)
                    {
                        strk = modifierState.modsTempAltered[i - 1];
                        strk.isDownstroke = !strk.isDownstroke;
                        sendVKeyEvent(strk);
                        Sleep(delayBetweenKeyEventsMS);
                    }
                    modifierState.modsTempAltered.clear();
                    escapeSequenceType = 0;
                }
            }
            else
            {
                error("Internal error: received a SC_CPS_ESC UP but we're not in any escape sequence.");
                escapeSequenceType = 0;
            }
            inCpsEscape = keyEvent.isDownstroke;
            continue;
        }
        if (inCpsEscape)  //first key after SC_CAP_ESC = escape function: 1=conditional break/make of modifiers, 2=Pause
        {
            if (escapeSequenceType == 0)
            {
                switch (keyEvent.vcode)
                {
                case 1:
                case 2:
                    escapeSequenceType = keyEvent.vcode;
                    IFDEBUG cout << endl << "CPS_ESC_SEQUENCE_TYPE: " << keyEvent.vcode;
                    break;
                default:
                    error("internal error: escapeSequenceType not defined: " + to_string(keyEvent.vcode));
                    return;
                }
            }
            else if (escapeSequenceType == CPS_ESC_SEQUENCE_TYPE_TEMPALTERMODIFIERS)
            {
                IFDEBUG cout << endl << "temp alter modifiers" << keyEvent.vcode;

                //the scancode is a modifier bitmask. Press/Release keys as necessary.
                //make modifier IF the system knows it is up
                unsigned short tempModChange = keyEvent.vcode;       //escaped scancode carries the modifier bitmask
                VKeyEvent newKeyEvent;

                if (keyEvent.isDownstroke)  //make modifiers when they are up
                {
                    unsigned short exor = modifierState.modifierDown ^ tempModChange; //figure out which ones we must press
                    tempModChange &= exor;
                }
                else	//break mods if down
                    tempModChange &= modifierState.modifierDown;

                newKeyEvent.isDownstroke = keyEvent.isDownstroke;

                const int NUMBER_OF_MODIFIERS_ALTERED_IN_SEQUENCES = 8; //high modifiers are skipped because they are never sent anyways.
                for (int i = 0; i < NUMBER_OF_MODIFIERS_ALTERED_IN_SEQUENCES; i++)  //push keycodes for all mods to be altered
                {
                    unsigned short modBitmask = tempModChange & (1 << i);
                    if (!modBitmask)
                        continue;
                    unsigned short sc = getModifierForBitmask(modBitmask);
                    newKeyEvent.vcode = sc;
                    modifierState.modsTempAltered.push_back(newKeyEvent);
                }
                continue;
            }
            else if (escapeSequenceType == CPS_ESC_SEQUENCE_TYPE_SLEEP)   //PAUSE
            {
                IFDEBUG cout << endl << "sleep: " << keyEvent.vcode;
                Sleep(keyEvent.vcode * 100);
                inCpsEscape = false;  //type 2 does not need an "escape up" key to finish the sequence
                escapeSequenceType = 0;
            }
            else
            {
                error("Internal error: unknown CPS_ESC_SEQUENCE_TYPE: " + to_string(escapeSequenceType));
                return;
            }
        }
        else //regular non-escaped keyEvent
        {
            sendVKeyEvent(keyEvent);
            if (keyEvent.vcode == AHK_HOTKEY1 || keyEvent.vcode == AHK_HOTKEY2)
                delayBetweenKeyEventsMS = DEFAULT_DELAY_FOR_AHK_MS;
            else
                Sleep(delayBetweenKeyEventsMS);
        }
    }
    if (inCpsEscape)
        error("SC_CPS escape sequence was not finished properly. Check your config.");
}

void getHardwareId()
{
    {
        wchar_t  hardware_id[500] = { 0 };
        string id;
        size_t length = interception_get_hardware_id(globalState.interceptionContext, globalState.interceptionDevice, hardware_id, sizeof(hardware_id));
        if (length > 0 && length < sizeof(hardware_id))
        {
            wstring wid(hardware_id);
            string sid(wid.begin(), wid.end());
            id = sid;
        } 
        else
            id = "UNKNOWN_ID";

        globalState.deviceIdKeyboard = id;
        globalState.deviceIsAppleKeyboard = (id.find("VID_05AC") != string::npos) || (id.find("VID&000205ac") != string::npos);

        IFDEBUG cout << endl << "getHardwareId:" << id << " / Apple keyboard: " << globalState.deviceIsAppleKeyboard;
    }
}


void initConsoleWindow()
{
    //disable quick edit; blocking the console window means the keyboard is dead
    HANDLE Handle = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(Handle, &mode);
    mode &= ~ENABLE_QUICK_EDIT_MODE;
    mode &= ~ENABLE_MOUSE_INPUT;
    SetConsoleMode(Handle, mode);

    system("color 8E");  //byte1=background, byte2=text
    string title = ("Capsicain v" + VERSION);
    SetConsoleTitle(title.c_str());
}

// Parses the OPTIONS in the given section.
// Returns false if section does not exist.
bool parseIniOptions(std::vector<std::string> assembledIni)
{
    vector<string> sectLines = getTaggedLinesFromIni(INI_TAG_OPTIONS, assembledIni);
    if (sectLines.size() == 0)
        return false;

    if (!getIntValueForKey("delayForKeySequenceMS", option.delayForKeySequenceMS, sectLines))
        IFDEBUG cout << endl << "No ini setting for 'option delayForKeySequenceMS'. Using default " << DEFAULT_DELAY_FOR_KEY_SEQUENCE_MS;

    option.debug = configHasKey("debug", sectLines);
    option.flipZy = configHasKey("flipZy", sectLines);
    option.shiftShiftToCapsLock = configHasKey("shiftShiftToCapsLock", sectLines);
    option.altAltToAlt = configHasKey("altAltToAlt", sectLines);
    option.flipAltWinOnAppleKeyboards = configHasKey("flipAltWinOnAppleKeyboards", sectLines);
    option.LControlLWinBlocksAlphaMapping = configHasKey("LControlLWinBlocksAlphaMapping", sectLines);
    option.processOnlyFirstKeyboard = configHasKey("processOnlyFirstKeyboard", sectLines);
    return true;
}

bool parseIniRewire(std::vector<std::string> assembledIni)
{
    vector<string> sectLines = getTaggedLinesFromIni(INI_TAG_REWIRE, assembledIni);
    allMaps.rewireIftappedMapping.clear();
    if (sectLines.size() == 0)
        return false;

    int keyIn, keyOut, keyIfTapped;
    for (string line : sectLines)
    {
        if (lexScancodeMapping(line, keyIn, keyOut, keyIfTapped, PRETTY_VK_LABELS))
        {
            bool isDuplicate = false;
            for (RewireIftappedMapping test : allMaps.rewireIftappedMapping)
            {
                if (test.inkey == keyIn) 
                {
                    if( test.outkey != keyOut || test.ifTapped != keyIfTapped)
                        IFDEBUG cout << endl << "WARNING: ignoring redefinition of " << INI_TAG_REWIRE << " "
                            << PRETTY_VK_LABELS[keyIn] << " " << PRETTY_VK_LABELS[keyOut] << " " << PRETTY_VK_LABELS[keyIfTapped];
                    isDuplicate = true;
                    break;
                }
            }
            if(!isDuplicate)
                allMaps.rewireIftappedMapping.push_back({ keyIn, keyOut, keyIfTapped });

            if (!isModifier(keyOut) && keyIfTapped != SC_NOP)
                IFDEBUG cout << endl << "WARNING: 'If-Tapped' definition is ignored for non-modifier: " << INI_TAG_REWIRE << " " << line;
        }
        else
            error("Bad Rewire / key mapping: " + line);
    }
    return true;
}

bool parseIniCombos(std::vector<std::string> assembledIni)
{
    allMaps.modCombos.clear();
    vector<string> sectLines = getTaggedLinesFromIni(INI_TAG_COMBOS, assembledIni);
    if (sectLines.size() == 0)
        return false;

    unsigned short mods[3] = { 0 }; //and, not, tap (nop, for)
    vector<VKeyEvent> keyEventSequence;

    for (string line : sectLines)
    {
        int key;
        if (lexRule(line, key, mods, keyEventSequence, PRETTY_VK_LABELS))
        {
            bool isDuplicate = false;
            for (ModifierCombo testcombo : allMaps.modCombos)
            {
                if (key == testcombo.key && mods[0] == testcombo.modAnd
                    && mods[1] == testcombo.modNot && mods[2] == testcombo.modTap)
                {
                    //warn only if the combos are different
                    bool redefined = false;
                    if (testcombo.keyEventSequence.size() == keyEventSequence.size())
                    {
                        for (int i = 0; i < keyEventSequence.size(); i++)
                        {
                            if (keyEventSequence[i].vcode != testcombo.keyEventSequence[i].vcode
                                || keyEventSequence[i].isDownstroke != testcombo.keyEventSequence[i].isDownstroke)
                            {
                                redefined = true;
                                break;
                            }
                        }
                    }
                    else
                        redefined = true;

                    if(redefined)
                        IFDEBUG cout << endl << "WARNING: Ignoring redefinition of Combo: " <<
                        PRETTY_VK_LABELS[key] << " [ " << hex << mods[0] << ", " << mods[1] << ", " << mods[2] << "] > ...";

                    isDuplicate = true;
                    break;
                }
            }
            if(!isDuplicate)
                allMaps.modCombos.push_back({ key, mods[0], mods[1], mods[2], keyEventSequence });
        }
        else
            error("Cannot parse combo: " + line);
    }
    return true;
}

bool parseIniAlphaLayout(std::vector<std::string> assembledIni)
{
    string tagFrom = stringToLower(INI_TAG_ALPHA_FROM);
    string tagEnd = stringToLower(INI_TAG_ALPHA_END);

    resetAlphaMap();
    string mapFromTo = "";
    bool inMapFromTo = false;
    for (string line : assembledIni)
    {
        string firstToken = stringGetFirstToken(line);
        if (firstToken == tagFrom)
        {
            if (inMapFromTo)
            {
                error("Bad " + INI_TAG_ALPHA_FROM + ".." + INI_TAG_ALPHA_TO + "definition - received second "+ INI_TAG_ALPHA_FROM +". Forgot the "+INI_TAG_ALPHA_END+"?");
                return false;
            }
            inMapFromTo = true;
            mapFromTo = stringGetRestBehindFirstToken(line) + " ";
        }
        else if (firstToken == tagEnd)
        {
            inMapFromTo = false;
            if (!lexAlphaFromTo(mapFromTo, allMaps.alphamap, PRETTY_VK_LABELS))
                error("Cannot parse the " + INI_TAG_ALPHA_FROM + ".." + INI_TAG_ALPHA_TO + " alpha definition");
        }
        else if (inMapFromTo)
        {
            mapFromTo += line + " ";
        }
    }
    return true;
}

//insert all the INCLUDEd sub-sections into the base layer section
std::vector<std::string> assembleLayerConfig(int layer)
{
    string sectionName = "layer_" + to_string(layer);
    vector<string> assembledIni = getSectionFromIni(sectionName, sanitizedIniContent);

    while (true)
    {
        bool foundInclude = false;
        for (int i = 0; i < assembledIni.size(); i++)
        {
            string line = assembledIni.at(i);
            if (stringStartsWith(line, "include "))
            {
                assembledIni.erase(assembledIni.begin() + i);
                string subSectionName = stringGetRestBehindFirstToken(line);
                vector<string> subsection = getSectionFromIni(subSectionName, sanitizedIniContent);
                if (subsection.size() == 0)
                {
                    error("Subsection [" + subSectionName + "] does not exist or is empty)");
                }
                else
                {
                    IFDEBUG cout << endl << "inserting sub-section: " + subSectionName << " (" << subsection.size() << " lines)";
                    assembledIni.insert(assembledIni.begin() + i, subsection.begin(), subsection.end());
                }
                foundInclude = true;
                break;
            }
        }
        if (!foundInclude)
            break;
    }

    return assembledIni;
}

bool parseIni(int layer, std::string &newLayerName)
{
    if (sanitizedIniContent.size() == 0)
    {
        cout << endl << "Capsicain.ini is missing or empty.";
        return false;
    }

    option.debug = false;
    vector<string> assembledLayer = assembleLayerConfig(layer);
    if (assembledLayer.size() == 0)
    {
        cout << endl << "No valid configuration for Layer " << layer;
        return false;
    }
    if (configHasTaggedKey(INI_TAG_OPTIONS, "debug", assembledLayer))
    {
        option.debug = true;
        assembledLayer = assembleLayerConfig(layer);  //repeat just for the debug output
    }
    IFDEBUG cout << endl << "Assembled config for Layer " << layer << " : " << dec << assembledLayer.size() << " lines";

    parseIniOptions(assembledLayer);
    parseIniRewire(assembledLayer);
    IFDEBUG cout << endl << "Rewire Definitions: " << dec << allMaps.rewireIftappedMapping.size();
    parseIniCombos(assembledLayer);
    IFDEBUG cout << endl << "Combo  Definitions: " << dec << allMaps.modCombos.size();
    parseIniAlphaLayout(assembledLayer);
    IFDEBUG
    {
        int remapped = 0;
        for (int i = 0; i < 256; i++)
            if (i != allMaps.alphamap[i])
                remapped++;
        cout << endl << "Alpha  Definitions: " << dec << remapped;
    }

    if (!getStringValueForTaggedKey(INI_TAG_OPTIONS, "layerName", newLayerName, assembledLayer))
        newLayerName = "OPTION_layerName_undefined";

    return true;
}

void switchLayer(int layer)
{
    string newLayerName;

    if (layer == LAYER_DISABLED)
    {
        globalState.activeLayer = LAYER_DISABLED;
        globalState.activeLayerName = LAYER_DISABLED_LAYER_NAME;
    }
    else if (parseIni(layer, newLayerName))
    {
        globalState.activeLayer = layer;
        globalState.activeLayerName = newLayerName;
    }

    cout << endl << endl << "ACTIVE LAYER: " << globalState.activeLayer << " = " << globalState.activeLayerName;
}

void resetCapsNumScrollLock()
{
    //set NumLock, release CapsLock+Scrolllock
    vector<VKeyEvent> sequence;
    if (!(GetKeyState(VK_NUMLOCK) & 0x0001))
        keySequenceAppendMakeBreakKey(SC_NUMLOCK, sequence);
    if (GetKeyState(VK_CAPITAL) & 0x0001)
        keySequenceAppendMakeBreakKey(SC_CAPS, sequence);
    if (GetKeyState(VK_SCROLL) & 0x0001)
        keySequenceAppendMakeBreakKey(SC_SCRLOCK, sequence);
    if (sequence.size() != 0)
        playKeyEventSequence(sequence);
}

void resetAlphaMap()
{
    for (int i = 0; i < 256; i++)
        allMaps.alphamap[i] = i;
}

void resetLoopState()
{
    loopState.blockKey = false;
    loopState.isDownstroke = false;
    loopState.scancode = 0;
    loopState.isModifier = false;
    loopState.wasTapped = false;
    loopState.resultingVKeyEventSequence.clear();
}

void resetModifierState()
{
    modifierState.modifierDown = 0;
    modifierState.modifierTapped = 0;
    modifierState.lastModBrokeTapping = false;
    modifierState.modsTempAltered.clear();

    resetCapsNumScrollLock();
}
void resetAllStatesToDefault()
{
    //	globalState.interceptionDevice = NULL;
    globalState.deviceIdKeyboard = "";
    globalState.deviceIsAppleKeyboard = false;
    option.delayForKeySequenceMS = DEFAULT_DELAY_FOR_KEY_SEQUENCE_MS;

    resetModifierState();
    resetLoopState();
}

void printHelloHeader()
{
    string line1 = "Capsicain v" + VERSION;
#ifdef NDEBUG
    line1 += " (Release build)";
#else
    line1 += " (DEBUG build)";
#endif
    size_t linelen = line1.length();

    cout << endl;
    for (int i = 0; i < linelen; i++)
        cout << "-";
    cout << endl << line1 << endl;
    for (int i = 0; i < linelen; i++)
        cout << "-";
    cout << endl;
}

void printHelloFeatures()
{
    cout
        << endl << "ini version: " << option.iniVersion
        << endl << endl << "FEATURES"
        << endl << (option.flipZy ? "ON:" : "OFF:") << " Z <-> Y"
        << endl << (option.shiftShiftToCapsLock ? "ON:" : "OFF:") << " LShift + RShift -> ShiftLock"
        << endl << (option.altAltToAlt ? "ON:" : "OFF:") << " LAlt + RAlt -> Alt"
        << endl << (option.flipAltWinOnAppleKeyboards ? "ON:" : "OFF:") << " Alt <-> Win for Apple keyboards"
        << endl << (option.LControlLWinBlocksAlphaMapping ? "ON:" : "OFF:") << " Left Control and Win block alpha key mapping ('Ctrl + C is never changed')"
        << endl << (option.processOnlyFirstKeyboard ? "ON:" : "OFF:") << " Process only the keyboard that sent the first key"
        ;
}

void printStatus()
{
    int numMakeSent = 0;
    for (int i = 0; i < 255; i++)
    {
        if (globalState.keysDownSent[i])
            numMakeSent++;
    }
    cout << "STATUS" << endl << endl
        << "Capsicain version: " << VERSION << endl
        << "ini version: " << option.iniVersion << endl
        << "keyboard hardware id: " << globalState.deviceIdKeyboard << endl
        << "Apple keyboard: " << globalState.deviceIsAppleKeyboard << endl
        << "active layer: " << globalState.activeLayer << " = " << globalState.activeLayerName << endl
        << "delay between keys in sequences (ms): " << option.delayForKeySequenceMS << endl
        << "number of keys-down sent: " << dec <<   numMakeSent << endl
        << (errorLog.length() > 1 ? "ERROR LOG contains entries" : "clean error log") << " (" << dec << errorLog.length() << " chars)" << endl
        ;
}

void printKeylabels()
{
    for (int i = 0; i <= 255; i++)
        cout << "sc " << uppercase << hex << i << " = " << PRETTY_VK_LABELS[i] << endl;
}

void printHelp()
{
    cout << "HELP" << endl << endl
        << "Press [ESC] + [key] for core commands" << endl << endl
        << "[H] Help" << endl
        << "[X] Exit" << endl
        << "[0]..[9] switch layers. [0] is the 'do nothing but listen for commands' layer" << endl
        << "[W] flip ALT <-> WIN on Apple keyboards" << endl
        << "[Z] (labeled [Y] on GER keyboard): flip Y <-> Z keys" << endl
        << "[S] Status" << endl
        << "[D] Debug mode output" << endl
        << "[E] Error log" << endl
        << "[C] Print list of key labels for all scancodes" << endl
        << "[R] Reset and reload the .ini" << endl
        << "[T] Move Taskbar icon to Tray and back" << endl
        << "[U] Username Enter/Playback" << endl
        << "[P] Password Enter/Playback. DO NOT USE if strangers can access your local machine." << endl
        << "[I] Show processed Ini for the active layer" << endl
        << "[A] Autohotkey start" << endl
        << "[Y] autohotkeY stop" << endl
        << "[J][K][L][;] Macro Recording: Start,Stop,Playback,Copy macro definition to clipboard." << endl
        << "[,] and [.]: delay between keys in sequences -/+ 1ms " << endl
        << "[any modifier]: 'sticky modifier' : keeps it pressed until you hit ESC"
        << "[Q] (dev feature) Stop the debug build if both release and debug are running" << endl
        << endl << "These commands work anywhere, Capsicain does not have to be the active window."
        ;
}


void normalizeIKStroke(InterceptionKeyStroke &ikstroke) {
    if (ikstroke.code > 0x7F) {
        ikstroke.code &= 0x7F;
        ikstroke.state |= 2;
    }
}

InterceptionKeyStroke vkeyEvent2ikstroke(VKeyEvent vkstroke)
{
    InterceptionKeyStroke iks = { (unsigned short) vkstroke.vcode, 0 };

    if (vkstroke.vcode >= 0xFF)
    {
        error("bug: trying to send an interception keystroke > xFF");
        iks.code = SC_NOP;
    }

    if (vkstroke.vcode >= 0x80)
    {
        iks.code = static_cast<unsigned short>(vkstroke.vcode & 0x7F);
        iks.state |= 2;
    }
    if (!vkstroke.isDownstroke)
        iks.state |= 1;

    return iks;
}

VKeyEvent ikstroke2VKeyEvent(InterceptionKeyStroke ikStroke)
{
    VKeyEvent strk;
    strk.vcode = ikStroke.code;
    if ((ikStroke.state & 2) == 2)
        strk.vcode |= 0x80;
    strk.isDownstroke = ikStroke.state & 1 ? false : true;
    return strk;
}

void handleSendVkey(VKeyEvent keyEvent)
{
    cout << endl << "TODO: handleSendVkey handle 'sending' high keys";
}

void sendVKeyEvent(VKeyEvent keyEvent)
{
    if (keyEvent.vcode > 0xFF)
    {
        handleSendVkey(keyEvent);
        return;
    }

    if (keyEvent.vcode == 0xE4)  //what was that for?
        IFDEBUG cout << " {sending E4} ";

    if (!keyEvent.isDownstroke &&  !globalState.keysDownSent[(unsigned char)keyEvent.vcode])  //ignore up when key is already up
    {
        IFDEBUG cout << " >(blocked " << PRETTY_VK_LABELS[keyEvent.vcode] << " UP: was not down)>";
        return;
    }
    globalState.keysDownSent[(unsigned char)keyEvent.vcode] = keyEvent.isDownstroke;

    if (globalState.recordingMacro)
    {
        if (globalState.recordedMacro.size() < MAX_MACRO_LENGTH)
            globalState.recordedMacro.push_back(keyEvent);
        else
        {
            globalState.recordingMacro = false;
            cout << endl << "Macro Length > " << MAX_MACRO_LENGTH << ". Forgotten Macro? Stopping recording.";
        }
    }
    
    InterceptionKeyStroke iks = vkeyEvent2ikstroke(keyEvent);
    IFDEBUG cout << " {" << PRETTY_VK_LABELS[keyEvent.vcode] << (keyEvent.isDownstroke ? "v" : "^") << "}";
    interception_send(globalState.interceptionContext, globalState.interceptionDevice, (InterceptionStroke *)&iks, 1);
//    interception_send(globalState.interceptionContext, globalState.interceptionDevice, (InterceptionStroke *)&loopState.originalIKstroke, 1);
}

void reset()
{
    resetAllStatesToDefault();

    releaseAllRealModifiers();

    readSanitizeIniFile(sanitizedIniContent);
    readGlobalsOnStartup();
    switchLayer(globalState.activeLayer);
}

void releaseAllRealModifiers()
{
    IFDEBUG cout << endl << "Resetting all modifiers to UP" << endl;
    for (int i = 0; i < 255; i++)	//Send() suppresses key UP if it thinks it is already up.
        globalState.keysDownSent[i] = true;

    vector<VKeyEvent> keyEventSequence;
    keyEventSequence.push_back({ SC_LSHIFT, false });
    keyEventSequence.push_back({ SC_RSHIFT, false });
    keyEventSequence.push_back({ SC_LCTRL, false });
    keyEventSequence.push_back({ SC_RCTRL, false });
    keyEventSequence.push_back({ SC_LWIN, false });
    keyEventSequence.push_back({ SC_RWIN, false });
    keyEventSequence.push_back({ SC_LALT, false });
    keyEventSequence.push_back({ SC_RALT, false });
    keyEventSequence.push_back({ SC_CAPS, false });
    keyEventSequence.push_back({ AHK_HOTKEY1, false });
    keyEventSequence.push_back({ AHK_HOTKEY2, false });
    playKeyEventSequence(keyEventSequence);

    for (int i = 0; i < 255; i++)
        globalState.keysDownSent[i] = false;
}


void keySequenceAppendMakeKey(unsigned short scancode, vector<VKeyEvent> &sequence)
{
    sequence.push_back({ scancode, true });
}
void keySequenceAppendBreakKey(unsigned short scancode, vector<VKeyEvent> &sequence)
{
    sequence.push_back({ scancode, false });
}
void keySequenceAppendMakeBreakKey(unsigned short scancode, vector<VKeyEvent> &sequence)
{
    sequence.push_back({ scancode, true });
    sequence.push_back({ scancode, false });
}

string getSymbolForIKStrokeState(unsigned short state)
{
    switch (state)
    {
    case 0: return "v";
    case 1: return "^";
    case 2: return "*v";
    case 3: return "*^";
    }
    return "???" + state;
}
