//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "AthenaBot.h"

namespace inet {

namespace simbo {

Define_Module(AthenaBot);

AthenaBot::AthenaBot()
{
    bNewInstallation = true;

    // Bot killer variables
    dwKilledProcesses = 0;
    dwFileChanges = 0;
    dwRegistryKeyChanges = 0;
}

AthenaBot::~AthenaBot()
{

}

void AthenaBot::initialize(int stage)
{
    EV_INFO << "Initializing Athena bot component, stage " << stage << endl;
    std::string hostName = "Host " + this->getIndex();
    this->setName(hostName.c_str());
    GenericBot::initialize(stage);
    if (stage == INITSTAGE_LOCAL && infectedHost) {
        EV_INFO << "Initializing Athena bot component -- phase 1\n";
        double activationTime = par("botActivationTime");
        pingTime = par("botPingTime");
        EV_INFO << "Initial activation time is " << activationTime << endl;
        cServer = par("serverName").stringValue();
        botMessage->setKind(MSGKIND_ACTIVITY_BOT_START);
        scheduleAt(simTime() + (simtime_t)activationTime, botMessage);
    }
}

void AthenaBot::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        handleSelfMessages(msg);
    }
    else {
        EV_INFO << "Message received: " << msg->getName() << endl;
        handleDataMessage(msg);
    }
}

void AthenaBot::handleDataMessage(cMessage *msg)
{
    httptools::HttpReplyMessage *appmsg = check_and_cast<httptools::HttpReplyMessage *>(msg);
    if (appmsg == nullptr)
        throw cRuntimeError("Message (%s)%s is not a valid reply message", msg->getClassName(), msg->getName());

    EV_INFO << "Message data: " << appmsg->payload() << endl;
    delete appmsg;
    return;
}

void AthenaBot::handleSelfMessages(cMessage *msg)
{
    switch (msg->getKind()) {
        case MSGKIND_ACTIVITY_BOT_START:
            handleSelfActivityBotStart();
            break;

        case MSGKIND_BOT_START_SESSION:
            handleSelfBotStartSession();
            break;

        case MSGKIND_BOT_REPEAT_SESSION:
            handleSelfBotRepeatSession();
            break;

        case MSGKIND_BOT_RESPONSE_SESSION:
            handleSelfBotResponseSession();
            break;

    }
}

void AthenaBot::handleSelfActivityBotStart()
{
    EV_INFO << "Starting new activity bot period @ T=" << simTime() << endl;
    botMessage->setKind(MSGKIND_BOT_START_SESSION);
    scheduleAt(simTime() + (simtime_t) 1, botMessage);
}

void AthenaBot::handleSelfBotStartSession()
{
    EV_INFO << "Starting new bot session @ T=" << simTime() << endl;
    int nextMsgType = ConnectToHttp();
    scheduleNextBotEvent(nextMsgType);
}

void AthenaBot::handleSelfBotRepeatSession()
{
    if (IsAdmin()) {
        EV_INFO << "Host " << this->getIndex() << "was restarted!";
        infectedHost = false;
        scheduleNextBotEvent(on_exec);
        return;
    }
    EV_INFO << "Sending a repeat command to server @ T=" << simTime() << endl;
    int nextMsgType;

    char cDataToServer[MAX_HTTP_PACKET_LENGTH];
    memset(cDataToServer, 0, sizeof(cDataToServer));

    char cBusy[7];
    memset(cBusy, 0, sizeof(cBusy));
    bDdosBusy = IsAdmin(); // currently, IsAdmin() function act as head or tails
    if(bDdosBusy)
        strcpy(cBusy, "true");
    else
        strcpy(cBusy, "false");

    sprintf(cDataToServer, "|type:repeat|uid:%s|ram:%d|bk_killed:%i|bk_files:%i|bk_keys:%i|busy:%s|",
            cUuid, GetMemoryLoad(), dwKilledProcesses, dwFileChanges, dwRegistryKeyChanges, cBusy);

    char cHttpHost[MAX_PATH];
    memset(cHttpHost, 0, sizeof(cHttpHost));

    char cHttpPath[DEFAULT];
    memset(cHttpPath, 0, sizeof(cHttpPath));

    char cBreakUrl[strlen(cServer)];
    memset(cBreakUrl, 0, sizeof(cBreakUrl));
    strcpy(cBreakUrl, cServer);

    char *pcBreakUrl = cBreakUrl;

    if(strstr(pcBreakUrl, "http://"))
        pcBreakUrl += 7;
    else if(strstr(pcBreakUrl, "https://"))
        pcBreakUrl += 8;

    pcBreakUrl = strtok(pcBreakUrl, "/");
    if(pcBreakUrl != NULL)
        strcpy(cHttpHost, pcBreakUrl);

    pcBreakUrl = strtok(NULL, "?");
    if(pcBreakUrl != NULL)
        strcpy(cHttpPath, pcBreakUrl);

    strcpy(cHttpHostGlobal, cHttpHost);
    strcpy(cHttpPathGlobal, cHttpPath);
    if (!SendPanelRequest(cHttpHost, cHttpPath, usPort, cDataToServer))
        nextMsgType = repeat;
    else
        nextMsgType = on_exec;

    scheduleNextBotEvent(nextMsgType);
}

void AthenaBot::handleSelfBotResponseSession()
{
   // TODO: Response for a command sent from C&C
}

void AthenaBot::scheduleNextBotEvent(int msgType)
{
    EV_INFO << "Scheduling new bot message @ T=" << simTime() << endl;
    EV_INFO << "Message type: ";
    switch (msgType) {
    case on_exec: EV_INFO << "on_exec" << endl; botMessage->setKind(MSGKIND_BOT_START_SESSION); break;
    case repeat: EV_INFO << "repeat" << endl; botMessage->setKind(MSGKIND_BOT_REPEAT_SESSION); break;
    case response: EV_INFO << "response" << endl; botMessage->setKind(MSGKIND_BOT_RESPONSE_SESSION); break;
    default: EV_ERROR << "Type not defined" << endl;
    }
    scheduleAt(simTime() + (simtime_t) pingTime, botMessage);
}


int AthenaBot::ConnectToHttp()
{
    RunThroughUuidProcedure();

    // <!-------! CRC AREA START !-------!>
    if(!bConfigSetupCheckpointSeven)
        usPort = GetRandNum(100) + 1;
    // <!-------! CRC AREA STOP !-------!>

    {
        // <!-------! TRICKY UNFAIR STUFF - BUT ANTICRACK INDEED !-------!>
        time_t tTime;
        struct tm *ptmTime;
        tTime = time(NULL);
        ptmTime = localtime(&tTime);
        char cTodaysDate[20];
        memset(cTodaysDate, 0, sizeof(cTodaysDate));
        strftime(cTodaysDate, 20, "%y%m%d", ptmTime);
        if(atoi(cTodaysDate) >= nExpirationDateMedian)
            //break;
        // <!-------! TRICKY UNFAIR STUFF - BUT ANTICRACK INDEED !-------!>

        bHttpRestart = false;

        char cPrivelages[6];
        if(IsAdmin())
            strcpy(cPrivelages, "admin");
        else
            strcpy(cPrivelages, "user");

        char cBits[3];
        if (Is64Bits())
            strcpy(cBits, "64");
        else
            strcpy(cBits, "86");

        char cComputerGender[8];
        if(IsLaptop())
            strcpy(cComputerGender, "laptop");
        else
            strcpy(cComputerGender, "desktop");

        char cDataToServer[MAX_HTTP_PACKET_LENGTH];

        memset(cDataToServer, 0, sizeof(cDataToServer));
        cVersion[0] = '0';
        cVersion[1] = '\0';

        sprintf(cDataToServer, "|type:on_exec|uid:%s|priv:%s|arch:x%s|gend:%s|cores:%i|os:%s|ver:%s|net:%s|new:",
                cUuid, cPrivelages, cBits, cComputerGender, GetNumCPUs(), GetOs(), cVersion, GetVersionMicrosoftDotNetVersion());

        if(bNewInstallation)
            strcat(cDataToServer, "1");
        else
            strcat(cDataToServer, "0");

        strcat(cDataToServer, "|");

        char cHttpHost[MAX_PATH];
        memset(cHttpHost, 0, sizeof(cHttpHost));

        char cHttpPath[DEFAULT];
        memset(cHttpPath, 0, sizeof(cHttpPath));

        char cBreakUrl[strlen(cServer)];
        memset(cBreakUrl, 0, sizeof(cBreakUrl));
        strcpy(cBreakUrl, cServer);

        char *pcBreakUrl = cBreakUrl;

        if(strstr(pcBreakUrl, "http://"))
            pcBreakUrl += 7;
        else if(strstr(pcBreakUrl, "https://"))
            pcBreakUrl += 8;

        pcBreakUrl = strtok(pcBreakUrl, "/");
        if(pcBreakUrl != NULL)
            strcpy(cHttpHost, pcBreakUrl);

        pcBreakUrl = strtok(NULL, "?");
        if(pcBreakUrl != NULL)
            strcpy(cHttpPath, pcBreakUrl);

        strcpy(cHttpHostGlobal, cHttpHost);
        strcpy(cHttpPathGlobal, cHttpPath);

        {
            if(SendPanelRequest(cHttpHost, cHttpPath, usPort, cDataToServer))
                return on_exec;
        }
    }
    return repeat;
}

/**
 * WinAPI functions
 */
void AthenaBot::fncUuidCreate(UUID *uuid)
{
    unsigned long f8 = 4294967295;
    unsigned short f4 = 65535;
    unsigned short f2 = 255;
    std::default_random_engine generator(simTime().dbl());
    std::uniform_int_distribution<unsigned long> distribution_long(0, f8);
    std::uniform_int_distribution<unsigned short> distribution_short(0, f4);
    std::uniform_int_distribution<unsigned short> distribution_char(0, f2);

    uuid->Data1 = distribution_long(generator);
    uuid->Data2 = distribution_short(generator);
    uuid->Data3 = distribution_short(generator);
    for (int i = 0; i < 8; ++i)
        uuid->Data4[i] = distribution_char(generator);
}

void AthenaBot::fncUuidToString(UUID *uuid, char *uuidString)
{
    sprintf(uuidString, "%lX-%hX-%hX-%hhX%hhX-%hhX%hhX%hhX%hhX%hhX%hhX", uuid->Data1, uuid->Data2, uuid->Data3,
            uuid->Data4[0], uuid->Data4[1], uuid->Data4[2], uuid->Data4[3], uuid->Data4[4],
            uuid->Data4[5], uuid->Data4[6], uuid->Data4[7]);
}

void AthenaBot::fncRpcStringFree(char *str)
{
    free(str);
}

void AthenaBot::StripDashes(char *pcString)
{
    DWORD dwOffset = 0;

    for(unsigned short us = 0; us < strlen(pcString); us++)
    {
        if(pcString[us] == '-')
        {
            dwOffset++;
            pcString[us] = pcString[us + dwOffset];
        }
    }
}

/**
 * Utilities/ComputerInfo.cpp
 */
unsigned int AthenaBot::GetMemoryLoad()
{
    std::default_random_engine generator(simTime().dbl());
    std::uniform_int_distribution<DWORD> distribution;
    unsigned int dwLoad = distribution(generator);
    return dwLoad;
}

char *AthenaBot::GetVersionMicrosoftDotNetVersion()
{
    return (char*)"N/A";
}

bool AthenaBot::IsAdmin() //Determines if the executable is running as user or admin
{
    bool bReturn = false;

    // Let the luck decide for us
    std::default_random_engine generator (simTime().dbl());
    std::uniform_int_distribution<int> distribution;

    bReturn = distribution(generator) % 2 ? true : false;

    return bReturn;
}

bool AthenaBot::Is64Bits()
{
    bool bReturn = true;

    bReturn = IsAdmin();

    return bReturn;
}

bool AthenaBot::IsLaptop()
{
    bool bReturn = false;

    bReturn = IsAdmin();

    return bReturn;
}

unsigned int AthenaBot::GetNumCPUs() //Determines the number of processors on the host computer
{
    return 1;
}

char *AthenaBot::SimpleDynamicXor(char *pcString, DWORD dwKey)
{
    for(unsigned short us = 0; us < strlen(pcString); us++)
        pcString[us] = (pcString[us] ^ dwKey);

    // <!-------! CRC AREA START !-------!>
    bConfigSetupCheckpointFive = true;
    // <!-------! CRC AREA STOP !-------!>

    return pcString;
}

/**
 * Hub/HttpUtilities.cpp
 */
bool AthenaBot::SendPanelRequest(char *cHttpHost, char *cHttpPath, unsigned short usHttpPort, char *cHttpData)
{
    bool bReturn = false;

    char cEncryptedData[MAX_HTTP_PACKET_LENGTH];
    memset(cEncryptedData, 0, sizeof(cEncryptedData));
    char cEncryptedKey[MAX_HTTP_PACKET_LENGTH];
    memset(cEncryptedKey, 0, sizeof(cEncryptedKey));
    char cKeyA[KEY_SIZE];
    memset(cKeyA, 0, sizeof(cKeyA));
    char cKeyB[KEY_SIZE];
    memset(cKeyB, 0, sizeof(cKeyB));
    EncryptSentData(cHttpData, cEncryptedData, cEncryptedKey, cKeyA, cKeyB);

    char cOutPacket[MAX_HTTP_PACKET_LENGTH];
    memset(cOutPacket, 0, sizeof(cOutPacket));

    cReturnNewline[0] = '\r';
    cReturnNewline[1] = '\n';
    cReturnNewline[2] = '\0';
    strcpy(cOutPacket, "POST /gate.php");
    strcat(cOutPacket, cHttpPath);
    strcat(cOutPacket, " HTTP/1.1");
    strcat(cOutPacket, cReturnNewline);

    strcat(cOutPacket, "Host: ");
    strcat(cOutPacket, cHttpHost);
    strcat(cOutPacket, ":");
    char cHttpPort[7];
    memset(cHttpPort, 0, sizeof(cHttpPort));
    //itoa(usHttpPort, cHttpPort, 10);
    sprintf(cHttpPort, "%d", usHttpPort);
    strcat(cOutPacket, cHttpPort);
    //strcat(cOutPacket, "192.168.56.102");
    strcat(cOutPacket, cReturnNewline);

    strcat(cOutPacket, "Connection: close");
    strcat(cOutPacket, cReturnNewline);

    strcat(cOutPacket, "Content-Type: application/x-www-form-urlencoded");
    strcat(cOutPacket, cReturnNewline);

    strcat(cOutPacket, "Cache-Control: no-cache");
    strcat(cOutPacket, cReturnNewline);

    /* Currently, no 'User-Agent' string on the request
    if(dwOperatingSystem > WINDOWS_XP)
    {
        char cObtainedUserAgentString[MAX_HTTP_PACKET_LENGTH];
        memset(cObtainedUserAgentString, 0, sizeof(cObtainedUserAgentString));
        DWORD dwUserAgentLength;
        if(fncObtainUserAgentString(0, cObtainedUserAgentString, &dwUserAgentLength) == NOERROR)
        {
            strcat(cOutPacket, "User-Agent: ");
            strcat(cOutPacket, cObtainedUserAgentString);
            strcat(cOutPacket, cReturnNewline);
        }
    }
    */

    char cFinalOutData[strlen(cEncryptedData) * 3];
    memset(cFinalOutData, 0, sizeof(cFinalOutData));
    StringToStrongUrlEncodedString(cEncryptedData, cFinalOutData);

    char cFinalOutKey[strlen(cEncryptedKey) * 3];
    memset(cFinalOutKey, 0, sizeof(cFinalOutKey));
    StringToStrongUrlEncodedString(cEncryptedKey, cFinalOutKey);

    char cMarker[26];
    memset(cMarker, 0, sizeof(cMarker));
    char cMarkerBase64[sizeof(cMarker) * 3];
    memset(cMarkerBase64, 0, sizeof(cMarkerBase64));
    GenerateMarker(cMarker, cMarkerBase64);

    char cUrlEncodedMarker[strlen(cMarker) * 3];
    memset(cUrlEncodedMarker, 0, sizeof(cUrlEncodedMarker));
    StringToStrongUrlEncodedString(cMarker, cUrlEncodedMarker);

    strcat(cOutPacket, "Content-Length: ");
    char cHttpContentLength[25];
    memset(cHttpContentLength, 0, sizeof(cHttpContentLength));
    int nPacketDataLength = 2 + strlen(cFinalOutKey) + 3 + strlen(cFinalOutData) + 3 + strlen(cUrlEncodedMarker);
    sprintf(cHttpContentLength, "%d", nPacketDataLength);
    strcat(cOutPacket, cHttpContentLength);
    strcat(cOutPacket, cReturnNewline);
    strcat(cOutPacket, cReturnNewline);

    char cPacketData[nPacketDataLength];
    memset(cPacketData, 0, sizeof(cPacketData));
    strcpy(cPacketData, "a=");
    strcat(cPacketData, cFinalOutKey);
    strcat(cPacketData, "&b=");
    strcat(cPacketData, cFinalOutData);
    strcat(cPacketData, "&c=");
    strcat(cPacketData, cUrlEncodedMarker);
    strcat(cPacketData, "\n");

    EV_INFO << "----------------------\nOutgoing Packet(" << strlen(cOutPacket) << " bytes):\n" << cOutPacket << std::endl;
    EV_INFO << "-----------\nDecrypted Contents(" << strlen(cHttpData) << " bytes): " << cHttpData << std::endl;

    httpModule = this->getParentModule()->getSubmodule("HTTPModule", 1);
    HTTPModule *http = check_and_cast<HTTPModule *>(httpModule);
    http->sendRequestToServer(this, cHttpHost, cOutPacket, cPacketData);

    return bReturn;
}
void AthenaBot::EncryptSentData(char *cSource, char *cOutputData, char *cOutputEncryptedKey, char *cOutputRawKeyA, char *cOutputRawKeyB)
{
    srand(GenerateRandomSeed());

    char cKeyA[KEY_SIZE];
    memset(cKeyA, 0, sizeof(cKeyA));
    char cKeyB[KEY_SIZE];
    memset(cKeyB, 0, sizeof(cKeyB));
    GeneratestrtrKey(cKeyA, cKeyB);
    char cKey[KEY_SIZE * 2];
    memset(cKey, 0, sizeof(cKey));
    sprintf(cKey, "%s:%s", cKeyA, cKeyB);
    char cKeyEncryptedA[strlen(cKey) * 3];
    memset(cKeyEncryptedA, 0, sizeof(cKeyEncryptedA));
    base64_encode((unsigned char*)cKey, strlen(cKey), cKeyEncryptedA, sizeof(cKeyEncryptedA));
    char cKeyEncryptedB[strlen(cKeyEncryptedA) * 3];
    memset(cKeyEncryptedB, 0, sizeof(cKeyEncryptedB));
    StringToStrongUrlEncodedString(cKeyEncryptedA, cKeyEncryptedB);
    char cEncryptedA[strlen(cSource) * 3];
    memset(cEncryptedA, 0, sizeof(cEncryptedA));
    base64_encode((unsigned char*)cSource, strlen(cSource), cEncryptedA, sizeof(cEncryptedA));
    char cEncryptedB[strlen(cEncryptedA)];
    memset(cEncryptedB, 0, sizeof(cEncryptedB));
    strcpy(cEncryptedB, cEncryptedA);
    strtr(cEncryptedB, cKeyA, cKeyB);
    strcpy(cOutputData, cEncryptedB);
    strcpy(cOutputEncryptedKey, cKeyEncryptedB);
    strcpy(cOutputRawKeyA, cKeyA);
    strcpy(cOutputRawKeyB, cKeyB);

    EV_INFO << "----------------------\nEncryption Communication Details:\nKey: " << cKey << "\nKey StrongUrlEncoded: " << cKeyEncryptedB << "\nData Encrypted(strtr): " << cEncryptedB << std::endl;
}
int AthenaBot::GeneratestrtrKey(char *cOutputA, char *cOutputB)
{
    if(KEY_SIZE % 2 == 0)
        return 0;

    bool bSwitched = false;

    char cKeyA[KEY_SIZE];
    memset(cKeyA, 0, sizeof(cKeyA));

    char cKeyB[KEY_SIZE];
    memset(cKeyB, 0, sizeof(cKeyB));

    while(true)
    {
        unsigned short us = GetRandNum(26) + 97;

        char cChar[2];
        memset(cChar, 0, sizeof(cChar));
        cChar[0] = (char)us;

        if(!bSwitched)
        {
            if(strstr(cKeyA, cChar))
                continue;
            else
                strcat(cKeyA, cChar);
        }
        else
        {
            if(!strstr(cKeyA, cChar) || strstr(cKeyB, cChar))
                continue;
            else
                strcat(cKeyB, cChar);
        }

        if((strlen(cKeyA) == (KEY_SIZE - 1) / 2) && !bSwitched)
            bSwitched = true;
        else if(strlen(cKeyB) == (KEY_SIZE - 1) / 2)
            break;
    }

    int nCombinedLength = strlen(cKeyA) + strlen(cKeyB);

    strcpy(cOutputA, cKeyA);
    strcpy(cOutputB, cKeyB);

    return nCombinedLength;
}
void AthenaBot::StringToStrongUrlEncodedString(char *cSource, char *cOutput)
{
    char cUrlEncoded[strlen(cSource) * 3];
    memset(cUrlEncoded, 0, sizeof(cUrlEncoded));

    for(unsigned int ui = 0; ui < strlen(cSource); ui++)
    {
        char cHex[4];
        memset(cHex, 0, sizeof(cHex));
        sprintf(cHex, "%%%X", (unsigned int)cSource[ui]);

        strcat(cUrlEncoded, cHex);
    }

    strcpy(cOutput, cUrlEncoded);
}
void AthenaBot::GenerateMarker(char *cOutput, char *cOutputBase64)
{
    char cMarker[26];
    memset(cMarker, 0, sizeof(cMarker));

    for(unsigned short us = 0; us < 3; us++)
        strcat(cMarker, GenRandLCText());

    char cBase64[strlen(cMarker) * 3];
    memset(cBase64, 0, sizeof(cBase64));
    base64_encode((unsigned char*)cMarker, strlen(cMarker), cBase64, sizeof(cBase64));

    strcpy(cOutput, cMarker);
    strcpy(cOutputBase64, cBase64);
}

/*
 * Utilities/srandRelated.cpp
 */
unsigned long AthenaBot::GenerateRandomSeed()
{
    std::default_random_engine generator(simTime().dbl());
    std::uniform_int_distribution<unsigned long> distribution;

    return clock() + time(NULL) + distribution(generator);
}

/**
 * Utilities/Misc.cpp
 */
char *AthenaBot::GenRandLCText() //Generates random lowercase text
{
    static char cRandomText[9];

    for(unsigned short us = 0; us < 8; us++)
        cRandomText[us] = (char)(GetRandNum(26) + 65 + 32);

    return (char*)cRandomText;
}

void AthenaBot::RunThroughUuidProcedure()
{
    memset(cUuid, 0, sizeof(cUuid));
    UUID uuid;
    memset(&uuid, 0, sizeof(UUID));
    fncUuidCreate(&uuid);

    char *ucUuidString = (char *) malloc(50);
    fncUuidToString(&uuid, ucUuidString);

    strcpy(cUuid, (const char*)ucUuidString);
    fncRpcStringFree(ucUuidString);
    StripDashes(cUuid);
}
unsigned long AthenaBot::GetRandNum(unsigned long range) //Returns a random number within a given range
{
    srand(GenerateRandomSeed());

    return rand() % range;
}

/**
 * Encryption/Base64_.cpp
 */
/**
 * encode three bytes using base64 (RFC 3548)
 *
 * @param triple three bytes that should be encoded
 * @param result buffer of four characters where the result is stored
 */
void AthenaBot::_base64_encode_triple(unsigned char triple[3], char result[4])
{
    int tripleValue, i;

    tripleValue = triple[0];
    tripleValue *= 256;
    tripleValue += triple[1];
    tripleValue *= 256;
    tripleValue += triple[2];

    for (i=0; i<4; i++)
    {
        result[3-i] = BASE64_CHARS[tripleValue%64];
        tripleValue /= 64;
    }
}

/**
 * encode an array of bytes using Base64 (RFC 3548)
 *
 * @param source the source buffer
 * @param sourcelen the length of the source buffer
 * @param target the target buffer
 * @param targetlen the length of the target buffer
 * @return 1 on success, 0 otherwise
 */
int AthenaBot::base64_encode(unsigned char *source, size_t sourcelen, char *target, size_t targetlen)
 {
    /* check if the result will fit in the target buffer */
    if ((sourcelen+2)/3*4 > targetlen-1)
        return 0;

    /* encode all full triples */
    while (sourcelen >= 3)
    {
        _base64_encode_triple(source, target);
        sourcelen -= 3;
        source += 3;
        target += 4;
    }

    /* encode the last one or two characters */
    if (sourcelen > 0)
    {
        unsigned char temp[3];
        memset(temp, 0, sizeof(temp));
        memcpy(temp, source, sourcelen);
        _base64_encode_triple(temp, target);
        target[3] = '=';
        if (sourcelen == 1)
            target[2] = '=';

        target += 4;
    }

    /* terminate the string */
    target[0] = 0;

    return 1;
}

/**
 * determine the value of a base64 encoding character
 *
 * @param base64char the character of which the value is searched
 * @return the value in case of success (0-63), -1 on failure
 */
int AthenaBot::_base64_char_value(char base64char)
{
    if (base64char >= 'A' && base64char <= 'Z')
        return base64char-'A';
    if (base64char >= 'a' && base64char <= 'z')
        return base64char-'a'+26;
    if (base64char >= '0' && base64char <= '9')
        return base64char-'0'+2*26;
    if (base64char == '+')
        return 2*26+10;
    if (base64char == '/')
        return 2*26+11;
    return -1;
}

/**
 * decode a 4 char base64 encoded byte triple
 *
 * @param quadruple the 4 characters that should be decoded
 * @param result the decoded data
 * @return lenth of the result (1, 2 or 3), 0 on failure
 */
int AthenaBot::_base64_decode_triple(char quadruple[4], unsigned char *result)
{
    int i, triple_value, bytes_to_decode = 3, only_equals_yet = 1;
    int char_value[4];

    for (i=0; i<4; i++)
        char_value[i] = _base64_char_value(quadruple[i]);

    /* check if the characters are valid */
    for (i=3; i>=0; i--)
    {
        if (char_value[i]<0)
        {
            if (only_equals_yet && quadruple[i]=='=')
            {
                /* we will ignore this character anyway, make it something
                 * that does not break our calculations */
                char_value[i]=0;
                bytes_to_decode--;
                continue;
            }
            return 0;
        }
        /* after we got a real character, no other '=' are allowed anymore */
        only_equals_yet = 0;
    }

    /* if we got "====" as input, bytes_to_decode is -1 */
    if (bytes_to_decode < 0)
        bytes_to_decode = 0;

    /* make one big value out of the partial values */
    triple_value = char_value[0];
    triple_value *= 64;
    triple_value += char_value[1];
    triple_value *= 64;
    triple_value += char_value[2];
    triple_value *= 64;
    triple_value += char_value[3];

    /* break the big value into bytes */
    for (i=bytes_to_decode; i<3; i++)
        triple_value /= 256;
    for (i=bytes_to_decode-1; i>=0; i--)
    {
        result[i] = triple_value%256;
        triple_value /= 256;
    }

    return bytes_to_decode;
}

/**
 * decode base64 encoded data
 *
 * @param source the encoded data (zero terminated)
 * @param target pointer to the target buffer
 * @param targetlen length of the target buffer
 * @return length of converted data on success, -1 otherwise
 */
size_t AthenaBot::base64_decode(const char *source, char *target, size_t targetlen)
{
    char *src, *tmpptr;
    char quadruple[4], tmpresult[3];
    int i;
    size_t tmplen = 3, converted = 0;

    /* concatenate '===' to the source to handle unpadded base64 data */
    //src = (char *)HeapAlloc(GetProcessHeap(), 0, strlen(source)+5);
    src = (char *) malloc(strlen(source)+5);
    if (src == NULL)
        return -1;
    strcpy(src, source);
    strcat(src, "====");
    tmpptr = src;

    /* convert as long as we get a full result */
    while (tmplen == 3)
    {
        /* get 4 characters to convert */
        for (i=0; i<4; i++)
        {
            /* skip invalid characters - we won't reach the end */
            while (*tmpptr != '=' && _base64_char_value(*tmpptr)<0)
                tmpptr++;

            quadruple[i] = *(tmpptr++);
        }

        /* convert the characters */
        tmplen = _base64_decode_triple(quadruple, (unsigned char*)tmpresult);

        /* check if the fit in the result buffer */
        if (targetlen < tmplen)
        {
            //HeapFree(GetProcessHeap(), 0, src);
            free(src);
            return -1;
        }

        /* put the partial result in the result buffer */
        memcpy(target, tmpresult, tmplen);
        target += tmplen;
        targetlen -= tmplen;
        converted += tmplen;
    }

    //HeapFree(GetProcessHeap(), 0, src);
    free(src);
    return converted;
}

/**
 * Hub/IrcUtilities.cpp
 */
char *AthenaBot::GetOs()
{
    srand(GenerateRandomSeed());
    dwOperatingSystem = rand() % 7 + 1;
    if(dwOperatingSystem == WINDOWS_UNKNOWN)
        return (char*)"UNKW";
    else if(dwOperatingSystem == WINDOWS_2000)
        return (char*)"W_2K";
    else if(dwOperatingSystem == WINDOWS_XP)
        return (char*)"W_XP";
    else if(dwOperatingSystem == WINDOWS_2003)
        return (char*)"W2K3";
    else if(dwOperatingSystem == WINDOWS_VISTA)
        return (char*)"WVIS";
    else if(dwOperatingSystem == WINDOWS_7)
        return (char*)"WIN7";
    else if(dwOperatingSystem == WINDOWS_8)
        return (char*)"WIN8";

    return (char*)"ERRO";
}

void AthenaBot::strtr(char *cSource, char *cCharArrayA, char *cCharArrayB)
{
    int nSourceLength = strlen(cSource);

    for(int i = 0; i < nSourceLength; i++)
    {
        if(cSource[i] == '\0')
            break;

        for(unsigned short us = 0; us < strlen(cCharArrayA); us++)
        {
            if(cSource[i] == cCharArrayA[us])
            {
                cSource[i] = cCharArrayB[us];
                break;
            }
            else
            {
                if(us == strlen(cCharArrayA)-1)
                    cSource[i] = cSource[i];
            }
        }
    }
}

} /* namespace simbo */

} /* namespace inet */
