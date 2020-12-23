#pragma oncce
#include "pch.h"
#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iterator>
#include <windows.h>
#include <tlhelp32.h>

#include "config.h"
#include "capsicain.h"
#include "constants.h"
#include "utils.h"
#include "scancodes.h"
#include "modifiers.h"

using namespace std;

//cut comments, tab to space, trim, single blanks, lowercase
void normalizeLine(string &line)
{
    auto idxComment = line.find_first_of('#');
    if (string::npos != idxComment)
        line.erase(idxComment);

    replace(line.begin(), line.end(), '\t', ' ');

    line.erase(0, line.find_first_not_of(' '));
    line.erase(line.find_last_not_of(' ') + 1);

    std::string::size_type pos;
    while ((pos = line.find("  ")) != std::string::npos)
        line.replace(pos, 2, " ");

    std::transform(line.begin(), line.end(), line.begin(), ::tolower);
}

int checkSyntax(std::vector<string> iniLines)
{
    int errors = 0;
    bool inAlpha = false;
    for (std::vector<std::string>::iterator t = iniLines.begin(); t != iniLines.end(); t++) 
    {
        if (stringStartsWith(*t, "alpha_from"))
            inAlpha = true;
        else if (stringStartsWith(*t, "alpha_end"))
        {
            inAlpha = false;
            continue;
        }
        if (inAlpha)
            continue;

        if (
            stringStartsWith(*t, "[") ||
            stringStartsWith(*t, "global") ||
            stringStartsWith(*t, "include") ||
            stringStartsWith(*t, "rewire") ||
            stringStartsWith(*t, "combo") ||
            stringStartsWith(*t, "option")
            )
            continue;

        std::cout << "Syntax error: Unknown keyword: " << *t << std::endl;
        errors++;
    }
    return errors;
}

// Read .ini file, normalize lines, drop empty lines, drop [Reference* sections
bool readSanitizeIniFile(std::vector<string> &iniLines)
{
    iniLines.clear();
    string line;
    bool inReferenceSection = false;
    ifstream f("capsicain.ini");
    if (!f.is_open())
        return false;

    bool detectBom = true;  //that nasty UTF BOM that MS loves so much...
    while (getline(f, line)) {
        if (detectBom)
        {
            detectBom = false;
            if (line[0] == (char)0xEF)
                continue;
        }
        normalizeLine(line);
        if (line == "")
            continue;
        if (stringStartsWith(line, "[reference"))
            inReferenceSection = true;
        else if (stringStartsWith(line, "[") && ! stringStartsWith(line, "[ "))
            inReferenceSection = false;
        if(!inReferenceSection)
            iniLines.push_back(line);
    }

    if (f.bad())
    {
        cout << "Error while reading .ini file";
        return false;
    }
    int numErrors = checkSyntax(iniLines);
    if (numErrors>0)
        cout << "**** There are " << numErrors << " errors in your ini file ****";
    return true;
}


//Returns empty vector if section does not exist, or is empty
std::vector<std::string> getSectionFromIni(std::string sectionName, std::vector<std::string> iniContent)
{
    std::vector<std::string> sectionContent;
    string sectName = stringToLower(sectionName);
    string line;
    bool inSection = false;

    for (string line : iniContent)
    {

        if (stringStartsWith(line, "[" + sectName + "]"))
            inSection = true;
        else if (inSection)
        {
            if (stringStartsWith(line, "["))
                break;
            sectionContent.push_back(line);
        }
    }
    if (inSection && sectionContent.size() == 0)
        sectionContent.push_back("option layername empty_layer_do_nothing");
    return sectionContent;
}
//Returns all lines starting with tag, with the tag removed, or empty vector if none
std::vector<std::string> getTaggedLinesFromIni(std::string tag, std::vector<std::string> iniContent)
{
    std::vector<std::string> taggedContent;
    tag = stringToLower(tag);
    string line;
    for (string line : iniContent)
    {
        if (stringGetFirstToken(line) == tag)
            taggedContent.push_back(stringGetRestBehindFirstToken(line));
    }
    return taggedContent;
}

bool configHasKey(string key, vector<string> sectionLines)
{
    key = stringToLower(key);
    for (string line : sectionLines)
    {
        if (stringGetFirstToken(line) == key)
            return true;
    }
    return false;
}

bool configHasTaggedKey(std::string tag, std::string key, std::vector<std::string> sectionLines)
{
    tag = stringToLower(tag);
    key = stringToLower(key);
    for (string line : sectionLines)
    {
        if (stringGetFirstToken(line) == tag
            && stringGetFirstToken(stringGetRestBehindFirstToken(line)) == key)
            return true;
    }
    return false;
}

bool getStringValueForTaggedKey(string tag, string key, std::string &value, vector<string> sectionLines)
{
    tag = stringToLower(tag);
    key = stringToLower(key);
    vector<string> splitline;
    value = "";
    for (string line : sectionLines)
    {
        splitline = stringSplit(line, ' ');
        if (splitline.size() < 3)
            continue;
        if (splitline.at(0) == tag && splitline.at(1) == key)
        {
            for (int i = 2; i < splitline.size(); i++)
                value += splitline.at(i) + " ";
            normalizeLine(value);
            return true;
        }
    }
    return false;
}

bool getStringValueForKey(std::string key, std::string &value, vector<string> sectionLines)
{
    key = stringToLower(key);
    for (string line : sectionLines)
    {
        if (stringStartsWith(line, key))
        {
            value = stringGetRestBehindFirstToken(line);
            return true;
        }
    }
    return false;
}

bool getIntValueForTaggedKey(string tag, string key, int &value, vector<string> sectionLines)
{
    string val;
    if (!getStringValueForTaggedKey(tag, key, val, sectionLines))
        return false;
    try
    {
        value = stoi(val);
    }
    catch (...)
    {
        cout << endl << "Error: not a number: " << val;
        return false;
    }
    return true;
}

bool getIntValueForKey(std::string key, int &value, vector<std::string> sectionLines)
{
    key = stringToLower(key);
    string val;
    if (!getStringValueForKey(key, val, sectionLines))
        return false;
    try
    {
        value = stoi(val);
    }
    catch (...)
    {
        cout << endl << "Error: not a number: " << val;
        return false;
    }
    return true;
}

std::string stringGetFirstToken(std::string line)
{
    size_t idx = line.find_first_of(" ");
    if (idx == string::npos)
        idx = line.length();
    return line.substr(0, idx);
}
std::string stringGetLastToken(std::string line)
{
    return line.substr(line.find_last_of(' ') + 1);
}
std::string stringGetRestBehindFirstToken(std::string line)
{
    size_t idx = line.find_first_of(" ");
    if (idx == string::npos)
        return("");
    line = line.substr(idx);
    line.erase(0, line.find_first_not_of(' '));
    return line;
}

// lex "a b c ALPHA_TO x y z"
bool lexAlphaFromTo(std::string alpha_to, int (&alphamap)[MAX_VCODES], std::string scLabels[])
{
    size_t idx1 = alpha_to.find(stringToLower(INI_TAG_ALPHA_TO));
    if (idx1 == string::npos)
        return false;
    string tmpfrom = (alpha_to.substr(0, idx1));
    string tmpto = (alpha_to.substr(idx1 + INI_TAG_ALPHA_TO.length()));
    normalizeLine(tmpfrom);
    normalizeLine(tmpto);
    vector<string> sfrom = stringSplit(tmpfrom, ' ');
    vector<string> sto = stringSplit(tmpto, ' ');
    if (sfrom.size() == 0 || sfrom.size() != sto.size())
    {
        cout << endl << "ERROR: " << INI_TAG_ALPHA_FROM << " and " << INI_TAG_ALPHA_TO << " lists are different size";
        return false;
    }

    for (int i = 0; i < sfrom.size(); i++)
    {
        int ifrom = getVcode(sfrom[i], scLabels);
        int ito = getVcode(sto[i], scLabels);
        if (ifrom < 0 || ito < 0)
        {
            cout << endl << "Unknown scancode labels: " << sfrom[i] << " and " << sto[i];
            return false;
        }
        if (alphamap[ifrom] != ifrom)
        {
            cout << endl << "WARNING: Ignoring redefinition of alpha key: " << sfrom[i] << " to " << sto[i];
        }
        alphamap[(unsigned char)ifrom] = (unsigned char)ito;
    }
    return true;
}

// parse scancodes "A B"  or  "A B C". Does not touch optional keys that are not defined in the line.
bool lexRewireRule(std::string line, unsigned char &keyA, int &keyB, int &keyC, std::string scLabels[])
{
    vector<string> labels = stringSplit(line, ' ');
    if (labels.size() != 2 && labels.size() != 3)
        return false;

    int ikeyA = getVcode(labels[0], scLabels);
    int ikeyB = getVcode(labels[1], scLabels);
    int ikeyC;
    bool hasTapConfig = labels.size() == 3;
    if (hasTapConfig)
        ikeyC = getVcode(labels[2], scLabels);

    if (ikeyA < 0 || ikeyB < 0 || (hasTapConfig && ikeyC < 0))
        return false; //invalid key label
    if (ikeyA > 255)
    {
        cout << endl << "ERROR: the rewired key cannot be a virtual key: " << labels[0];
        return false;
    }

    keyA = ikeyA;
    keyB = ikeyB;
    if(hasTapConfig)
        keyC = ikeyC;

    return true;
}

//convert ("xyz_&.", '&') to 000010
unsigned short lexModString(string modString, char filter)
{
    string binString = "";
    for (int i = 0; i < modString.length(); i++)
    {
        if (modString[i] == filter)
            binString += '1';
        else
            binString += '0';
    }
    return std::stoi(binString, nullptr, 2);
}

bool lexFunctionCombo(std::string &funcParams, std::string * scLabels, std::vector<VKeyEvent> &strokeSeq)
{
    vector<string> labels = stringSplit(funcParams, '+');
    int isc;
    for (string label : labels)
    {
        isc = getVcode(label, scLabels);
        if (isc < 0)
            return false;
        strokeSeq.push_back({ (unsigned char)isc, true });
    }
    size_t len = strokeSeq.size();
    for (size_t i = len; i > 0; i--)	//copy upstrokes in reverse order
        strokeSeq.push_back({ strokeSeq.at(i - 1).vcode,false });
    return true;
}

//parse keyLabel  [!!!& .--. ....] > function(param)
//returns false if the rule is not valid.
bool lexRule(std::string line, int &key, unsigned short(&mods)[3], std::vector<VKeyEvent> &strokeSequence, std::string scLabels[])
{
    string strkey = stringGetFirstToken(line);
    if (strkey.length() < 1)
        return false;
    line = line.substr(strkey.length());

    int itmpKey = getVcode(strkey, scLabels);
    if (itmpKey < 0)
        return false;
    key = itmpKey;

    line.erase(std::remove(line.begin(), line.end(), ' '), line.end());

    size_t modIdx1 = line.find_first_of('[') + 1;
    size_t modIdx2 = line.find_first_of(']');
    if (modIdx1 < 1 || modIdx2 < 2 || modIdx1 > modIdx2 || modIdx1 == string::npos || modIdx2 == string::npos)
        return false;
    string mod = line.substr(modIdx1, modIdx2 - modIdx1);

    mods[0] = lexModString(mod, '&'); //and 
    mods[1] = lexModString(mod, '^'); //not 
    mods[2] = lexModString(mod, 't'); //tap

    //extract function name + param
    size_t funcIdx1 = line.find_first_of('>') + 1;
    if (funcIdx1 == string::npos || funcIdx1 < 2)
    {
        cout << endl << "Error in ini: missing '>'";
        return false;
    }
    size_t funcIdx2 = line.find_first_of('(');
    if (funcIdx2 == string::npos || funcIdx2 < funcIdx1 + 2)
    {
        cout << endl << "Error in ini: missing '('";
        return false;
    }
    size_t funcIdx3 = line.find_first_of(')');
    if (funcIdx3 == string::npos || funcIdx3 < funcIdx2 + 2)
    {
        cout << endl << "Error in ini: missing ')'";
        return false;
    }
    string funcName = line.substr(funcIdx1, funcIdx2 - funcIdx1);
    funcIdx2++;
    string funcParams = line.substr(funcIdx2, funcIdx3 - funcIdx2);

    //translate 'function' into a char sequence
    vector<VKeyEvent> strokeSeq;
    if (funcName == "key")
    {
        int isc = getVcode(funcParams, scLabels);
        if (isc < 0)
            return false;
        strokeSeq.push_back({ isc, true });
        strokeSeq.push_back({ isc, false });
    }
    else if (funcName == "combo")
    {
        if (!lexFunctionCombo(funcParams, scLabels, strokeSeq))
            return false;
    }
    else if (funcName == "combontimes")
    {
        vector<string> comboTimes = stringSplit(funcParams, ',');
        if (comboTimes.size() != 2)
            return false;
        if (!lexFunctionCombo(comboTimes.at(0), scLabels, strokeSeq))
            return false;
        int times = stoi(comboTimes.at(1));
        auto len = strokeSeq.size();
        for (int j = 1; j < times; j++)
            for (int i = 0; i < len; i++)
                strokeSeq.push_back(strokeSeq.at(i));
    }
    else if (funcName == "altchar")
    {
        strokeSeq.push_back({ SC_CPS_ESC, true }); //temp release LSHIFT if it is currently down
        strokeSeq.push_back({ CPS_ESC_SEQUENCE_TYPE_TEMPALTERMODIFIERS, true });
        strokeSeq.push_back({ BITMASK_LSHIFT | BITMASK_RSHIFT | BITMASK_LCTRL | BITMASK_RCTRL , false });
        strokeSeq.push_back({ BITMASK_RALT , true });
        strokeSeq.push_back({ SC_CPS_ESC, false });
        for (int i = 0; i < funcParams.length(); i++)
        {
            char c = funcParams[i];
            if (c < '0' || c > '9')
                return false;
            string temp = "NP";
            temp += c;
            int isc = getVcode(temp, scLabels);
            if (isc < 0)
                return false;
            strokeSeq.push_back({ (unsigned char)isc, true });
            strokeSeq.push_back({ (unsigned char)isc, false });
        }
        strokeSeq.push_back({ SC_CPS_ESC, false });
    }
    else if (funcName == "moddedkey")
    {
        vector<string> modKeyParams = stringSplit(funcParams, '+');
        if (modKeyParams.size() != 2)
            return false;
        int isc = getVcode(modKeyParams[0], scLabels);
        if (isc < 0)
            return false;

        unsigned short modsPress = lexModString(modKeyParams[1], '&'); //and (press if up)
        unsigned short modsRelease = lexModString(modKeyParams[1], '^'); //not (release if down)

        strokeSeq.push_back({ SC_CPS_ESC, true });
        strokeSeq.push_back({ CPS_ESC_SEQUENCE_TYPE_TEMPALTERMODIFIERS, true });
        if (modsPress > 0)
            strokeSeq.push_back({ modsPress, true });
        if (modsRelease > 0)
            strokeSeq.push_back({ modsRelease, false });
        strokeSeq.push_back({ SC_CPS_ESC, false });
        strokeSeq.push_back({ (unsigned char)isc, true });
        strokeSeq.push_back({ (unsigned char)isc, false });
        strokeSeq.push_back({ SC_CPS_ESC, false }); //second UP does UNDO the temp mod changes
    }
    else if (funcName == "sequence")
    {
        const string PAUSE_TAG = "pause:";
        vector<string> params = stringSplit(funcParams, '_');
        bool downkeys[256] = { 0 };

        for (string param : params)
        {
            //handle the "pause:10" items
            if (stringStartsWith(param, PAUSE_TAG))
            {
                string sleeptime = param.substr(PAUSE_TAG.length());
                int stime = stoi(sleeptime);
                if (stime > 255)
                {
                    cout << endl << "Sequence() defines pause > 255. Reducing to 255 (25.5 seconds)";
                    stime = 255;
                }
                if (stime == 0)
                {
                    cout << endl << "Sequence() defines pause:0. Ignoring the pause.";
                    continue;
                }
                strokeSeq.push_back({ SC_CPS_ESC, true });
                strokeSeq.push_back({ CPS_ESC_SEQUENCE_TYPE_SLEEP, true });
                strokeSeq.push_back({ (unsigned short)stime, true });
                continue;
            }

            // &key is down, ^key is up, key is both.
            bool downstroke = true;
            bool upstroke = true;
            if (param.at(0) == '&')
                upstroke = false;
            else if (param.at(0) == '^')
                downstroke = false;
            if (!downstroke || !upstroke)
                param = param.substr(1);

            int isc = getVcode(param, scLabels);
            if (isc < 0)
            {
                cout << endl << "Unknown key label in sequence(): " << param;
                return false;
            }

            if (downstroke)
            {
                strokeSeq.push_back({ (unsigned char)isc, true });
                downkeys[(unsigned char)isc] = true;
            }
            if (upstroke)
            {
                strokeSeq.push_back({ (unsigned char)isc, false });
                downkeys[(unsigned char)isc] = false;
            }
        }
        //check if all keys were released
        for (int i = 0; i < 256; i++)
        {
            if (downkeys[i])
            {
                cout << endl << "Sequence() does not release key: " << scLabels[i] << " (discarding this rule)";
                return false;
            }
        }
    }
    else
        return false;

    strokeSequence = strokeSeq;
    return true;
}