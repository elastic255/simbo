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

Define_Module(AthenaBot);

//namespace athena {

AthenaBot::AthenaBot()
{
    bNewInstallation = true;
}

AthenaBot::~AthenaBot()
{
    // @todo Delete socket data structures
    sockCollection.deleteSockets();
}

void AthenaBot::initialize(int stage)
{
    EV_INFO << "Initializing Athena bot component, stage " << stage << endl;

    HttpBrowserBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        cXMLElement *rootelement = par("config").xmlValue();
        if (rootelement == nullptr)
            throw cRuntimeError("Configuration file is not defined");

        cXMLAttributeMap attributes;
        httptools::rdObjectFactory rdFactory;

        // Activity bot period length -- the waking period
        cXMLElement *element = rootelement->getFirstChildWithTag("botActivityPeriod");
        if (element == nullptr) {
            rdBotActivityLength = nullptr;    // Disabled if this parameter is not defined in the file
        }
        else {
            attributes = element->getAttributes();
            rdBotActivityLength = rdFactory.create(attributes);
            if (rdBotActivityLength == nullptr)
                throw cRuntimeError("Bot Activity period random object could not be created");
        }

        // Inter-session interval
        element = rootelement->getFirstChildWithTag("botInterSessionInterval");
        if (element == nullptr)
            throw cRuntimeError("Bot Inter-request interval parameter undefined in XML configuration");
        attributes = element->getAttributes();
        rdBotInterSessionInterval = rdFactory.create(attributes);
        if (rdBotInterSessionInterval == nullptr)
            throw cRuntimeError("Bot Inter-session interval random object could not be created");

        // Requests in session
        element = rootelement->getFirstChildWithTag("botReqInSession");
        if (element == nullptr)
            throw cRuntimeError("requests in session parameter undefined in XML configuration");
        attributes = element->getAttributes();
        rdBotReqInSession = rdFactory.create(attributes);
        if (rdBotReqInSession == nullptr)
            throw cRuntimeError("Bot Requests in session random object could not be created");

        // Infected host
        isInfected = par("infected").boolValue();
        newMessage = new cMessage("New Message");
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER && isInfected) {
        EV_INFO << "Initializing Athena bot component -- phase 1\n";

        // No scripted mode yet
        /*
        std::string scriptFile = par("scriptFile").stdstringValue();
        scriptedMode = !scriptFile.empty();
        if (scriptedMode) {
            EV_INFO << "Scripted mode. Script file: " << scriptFile << endl;
            readScriptedEvents(scriptFile.c_str());
        }
        else {
            double activationTime = par("activationTime");    // This is the activation delay. Optional
            if (rdActivityLength != nullptr)
                activationTime += (86400.0 - rdActivityLength->draw()) / 2; // First activate after half the sleep period
            EV_INFO << "Initial activation time is " << activationTime << endl;
            eventTimer->setKind(MSGKIND_ACTIVITY_START);
            scheduleAt(simTime() + (simtime_t)activationTime, eventTimer);
        }
        */
        double activationTime = par("botActivationTime"); // Activation delay for the bot. Optional
        if (rdBotActivityLength != nullptr)
            activationTime += (86400.0 - rdBotActivityLength->draw()) / 2; // First activate after half the sleep period
        EV_INFO << "Initial activation time is " << activationTime << endl;
        newMessage->setKind(MSGKIND_ACTIVITY_BOT_START);
        scheduleAt(simTime() + (simtime_t)activationTime, newMessage);
    }
}

void AthenaBot::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        handleSelfMessages(msg);
    }
    else {
        EV_INFO << "Message received: " << msg->getName() << endl;

        TCPCommand *ind = dynamic_cast<TCPCommand *>(msg->getControlInfo());
        if (!ind) {
            EV_INFO << "No control info for the message" << endl;
        }
        else {
            int connId = ind->getConnId();
            EV_INFO << "Connection ID: " << connId << endl;
        }

        // Locate the socket for the incoming message. One should definitely exist.
        TCPSocket *socket = sockCollection.findSocketFor(msg);
        if (socket == nullptr) {
            // Handle errors. @todo error instead of warning?
            EV_WARN << "No socket found for message " << msg->getName() << endl;
            delete msg;
            return;
        }
        // Submit to the socket handler. Calls the TCPSocket::CallbackInterface methods.
        // Message is deleted in the socket handler
        socket->processMessage(msg);
    }
}

void AthenaBot::handleSelfMessages(cMessage *msg)
{
    switch (msg->getKind()) {
        case MSGKIND_ACTIVITY_START:
            handleSelfActivityStart();
            break;

        case MSGKIND_START_SESSION:
            handleSelfStartSession();
            break;

        case MSGKIND_NEXT_MESSAGE:
            handleSelfNextMessage();
            break;

        case MSGKIND_SCRIPT_EVENT:
            handleSelfScriptedEvent();
            break;

        case MSGKIND_ACTIVITY_BOT_START:
            handleSelfActivityBotStart();
            break;

        case MSGKIND_BOT_START_SESSION:
            handleSelfBotStartSession();
            break;

        case HTTPT_DELAYED_REQUEST_MESSAGE:
            handleSelfDelayedRequestMessage(msg);
            break;

    }
}

void AthenaBot::handleSelfActivityBotStart()
{
    EV_INFO << "Starting new activity bot period @ T=" << simTime() << endl;
    newMessage->setKind(MSGKIND_BOT_START_SESSION);
    messagesInCurrentSession = 0;
    reqNoInCurSession = 0;
    double activityPeriodLength = rdBotActivityLength->draw();    // Get the length of the activity period
    acitivityPeriodEnd = simTime() + activityPeriodLength;    // The end of the activity period
    EV_INFO << "Activity bot period starts @ T=" << simTime() << ". Activity period is " << activityPeriodLength / 3600 << " hours." << endl;
    scheduleAt(simTime() + (simtime_t)rdBotInterSessionInterval->draw() / 2, newMessage);
}

void AthenaBot::handleSelfBotStartSession()
{
    EV_INFO << "Starting new bot session @ T=" << simTime() << endl;
    sessionCount++;
    messagesInCurrentSession = 0;
    reqInCurSession = 0;
    reqNoInCurSession = (int)rdBotReqInSession->draw();
    EV_INFO << "Starting session # " << sessionCount << " @ T=" << simTime() << ". Requests in session are " << reqNoInCurSession << "\n";
    int nextMsgType = ConnectToHttp();
    //scheduleNextBotEvent(nextMsgType);
}

int AthenaBot::ConnectToHttp()
{
    RunThroughUuidProcedure();

    /* Athena debug info
#ifdef DEBUG
    //if(usPort > nPortOffset)
    //    usPort -= nPortOffset;
    //else
    //    usPort += nPortOffset;

    SimpleDynamicXor(cBackup, usVersionLength + usServerLength + usChannelLength + usAuthHostLength);

    printf("%s\n"
            "-----------\n"
            "URL to gate.php: %s\n"
            "Backup: %s\n"
            "Port: %i\n"
            "Extra Data:\n%s %s %s %s\n%s %s %s %s %s %s %s\n"
            "%s\n", cVersion, cServer, cBackup, usPort, cServerPass, cChannel, cChannelKey, cAuthHost,
            cIrcCommandPrivmsg, cIrcCommandJoin, cIrcCommandPart, cIrcCommandUser, cIrcCommandNick, cIrcCommandPass, cIrcCommandPong, cUuid);

    //if(usPort < nPortOffset)
    //    usPort += nPortOffset;
    //else
    //    usPort -= nPortOffset;

    SimpleDynamicXor(cBackup, usVersionLength + usServerLength + usChannelLength + usAuthHostLength);

    system("pause");
#endif
     */

    // <!-------! CRC AREA START !-------!>
    if(!bConfigSetupCheckpointSeven)
        usPort = GetRandNum(100) + 1;
    // <!-------! CRC AREA STOP !-------!>

    /*HW_PROFILE_INFO HwProfInfo;
        while(!fncGetCurrentHwProfile(&HwProfInfo))
            Sleep(100);

        char cGuid[40];
        memset(cGuid, 0, sizeof(cGuid));
        strcpy(cGuid, HwProfInfo.szHwProfileGuid);
        StripDashes(cGuid);

        memset(cUuid, 0, sizeof(cUuid));
        memcpy(cUuid, cGuid + 1, 36);*/

    // bUninstallProgram -- don't needed
    //while(!bUninstallProgram)
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
        //if(Is64Bits(GetCurrentProcess()))
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
        char szModuleName[127];
        int connectPort;

        if (controller->getAnyServerInfo(cServer, szModuleName, connectPort) != 0) {
            EV_ERROR << "Unable to get a random server from controller" << endl;
            return on_exec;
        }
        EV_INFO << "Sending request to random server " << cServer << " (" << szModuleName << ") on port " << connectPort << endl;

        //strcpy(cServer, "http://192.168.56.102/ ");

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

        /**
         * Maybe not needed!
         */
        /*
        //httpreq.sin_port = fnchtons(usPort);
        httpreq.sin_port = usPort; // Little-endian!
        httpreq.sin_family = AF_INET;

        HOSTENT *paddr;

        unsigned short usHostResolves = 1;
        do
        {
            usHostResolves++;

            if(usHostResolves % 2 == 0)
                paddr = fncgethostbyname(cHttpHost);
            else if(cBackup != NULL)
            {
                SimpleDynamicXor(cBackup, usVersionLength + usServerLength + usChannelLength + usAuthHostLength);
                paddr = fncgethostbyname(cBackup);
                SimpleDynamicXor(cBackup, usVersionLength + usServerLength + usChannelLength + usAuthHostLength);
            }
            else
                continue;

            if(paddr == NULL)
            {
#ifdef DEBUG
if(usHostResolves % 2 == 0)
    printf("ERROR: Failed to resolve primary host! Trying backup host in 5 seconds...\n");
else if(cBackup != NULL)
    printf("ERROR: Failed to resolve backup host! Trying primary host in 5 seconds...\n");
#endif
//Sleep(5000);
            }
        }
        while(paddr == NULL);

        memcpy(&httpreq.sin_addr.s_addr, paddr->h_addr, paddr->h_length);
        */
        /*
#ifdef DEBUG
        printf("-----------\n"
                "Communication Information:\n"
                "DNS: %s ( %s", cHttpHost, fncinet_ntoa(httpreq.sin_addr));
#ifdef USE_ENCRYPTED_IP
        httpreq.sin_addr.s_addr = fncinet_addr(DecryptIp(fncinet_ntoa(httpreq.sin_addr), 24));
        printf("[encrypted] -> %s[decrypted]", fncinet_ntoa(httpreq.sin_addr));
#endif
        printf(" )\n"
                "Path: %s\n"
                "Port: %i\n", cHttpPath, usPort);
#endif
         */
        //while(!bUninstallProgram)
        {
            if(SendPanelRequest(httpreq, cHttpHost, cHttpPath, usPort, cDataToServer))
                //break;
                return on_exec;

            //Sleep(5000);
            // Time to schedule next event -- problems? (httpreq, cHttpHost, cHttpPath, usPort must be global
            // next event? -- maybe a global to define
            //Sleep(nCheckInInterval * 1000);
        }

        // Schedule next event

        //while(!bUninstallProgram)
        /*
        {
            memset(cDataToServer, 0, sizeof(cDataToServer));

            char cBusy[7];
            memset(cBusy, 0, sizeof(cBusy));
            if(bDdosBusy)
                strcpy(cBusy, "true");
            else
                strcpy(cBusy, "false");

            sprintf(cDataToServer, "|type:repeat|uid:%s|ram:%ld|bk_killed:%i|bk_files:%i|bk_keys:%i|busy:%s|",
                    cUuid, GetMemoryLoad(), dwKilledProcesses, dwFileChanges, dwRegistryKeyChanges, cBusy);

            if(!SendPanelRequest(httpreq, cHttpHost, cHttpPath, usPort, cDataToServer))
            {
                if(bHttpRestart)
                    break;

                //Sleep(5000);
                return repeat;
                //Sleep(nCheckInInterval * 1000);
                //continue;
            }

            if(bHttpRestart)
                break;

            //Sleep(5000);
            return repeat;
            //Sleep(nCheckInInterval * 1000);
        }
    */}
    //UninstallProgram();
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
    std::default_random_engine generator;
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
    //uuidString = (char *) malloc(44);
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
char *AthenaBot::GetVersionMicrosoftDotNetVersion()
{
    /*
    char cPath[DEFAULT];
    for(unsigned short us = 0; us < 4; us++)
    {
        sprintf(cPath, "%s\\Microsoft.NET\\Framework\\", cWindowsDirectory);

        if(us == 0)
            strcat(cPath, "v4.0.30319");
        else if(us == 1)
            strcat(cPath, "v3.5");
        else if(us == 2)
            strcat(cPath, "v3.0");
        else if(us == 3)
            strcat(cPath, "v2.0.50727");

        if(DirectoryExists(cPath))
        {
            if(us == 0)
                return (char*)"4.0";
            else if(us == 1)
                return (char*)"3.5";
            else if(us == 2)
                return (char*)"3.0";
            else if(us == 3)
                return (char*)"2.0";
        }
    }
     */
    return (char*)"N/A";
}

bool AthenaBot::IsAdmin() //Determines if the executable is running as user or admin
{
    bool bReturn = false;

    /*
    FILE * pOpenAdminFile;
    char cAdminFile[45];

    sprintf(cAdminFile, "%s\\System32\\drivers\\etc\\protocol", cWindowsDirectory);

    pOpenAdminFile = fopen(cAdminFile, "a");

    if(pOpenAdminFile != NULL)
    {
        bReturn = TRUE;
        fclose(pOpenAdminFile);
    }
    */

    // Let the luck decide for us
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(0, 2);

    bReturn = distribution(generator) % 2 ? true : false;

    return bReturn;
}

bool AthenaBot::Is64Bits()
{
    bool bReturn = true;

    /*
    BOOL bIsWow64 = FALSE;
    if(!fncIsWow64Process(hProcess, &bIsWow64))
        bReturn = FALSE;

    if(!bIsWow64)
        bReturn = FALSE;
     */
    bReturn = IsAdmin();

    return bReturn;
}

bool AthenaBot::IsLaptop()
{
    bool bReturn = false;

    /*
    SYSTEM_POWER_STATUS spsPowerInfo;
    if(GetSystemPowerStatus(&spsPowerInfo))
    {
        if(spsPowerInfo.BatteryFlag < 128)
            bReturn = TRUE;
    }
    */
    bReturn = IsAdmin();

    return bReturn;
}

unsigned int AthenaBot::GetNumCPUs() //Determines the number of processors on the host computer
{
    return 1;
}

/*
HOSTENT *AthenaBot::fncgethostbyname(char *)
{

}
*/

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
bool AthenaBot::SendPanelRequest(SOCKADDR_IN httpreq, char *cHttpHost, char *cHttpPath, unsigned short usHttpPort, char *cHttpData)
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

    cReturnNewline[0] = ' ';
    cReturnNewline[1] = '\n';
    strcpy(cOutPacket, "POST /gate.php");
    strcat(cOutPacket, cHttpPath);
    strcat(cOutPacket, " HTTP/1.1");
    strcat(cOutPacket, cReturnNewline);

    strcat(cOutPacket, "Host: ");
    strcat(cOutPacket, cHttpHost);
    strcat(cOutPacket, ":");
    char cHttpPort[7];
    usHttpPort = 80;
    memset(cHttpPort, 0, sizeof(cHttpPort));
    //itoa(usHttpPort, cHttpPort, 10);
    sprintf(cHttpPort, "%d", usHttpPort);
    strcat(cOutPacket, cHttpPort);
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

    //char cCopy[DEFAULT];
    //memset(cCopy, 0, sizeof(cCopy));
    //sprintf(cCopy, "%s:%s", cEncryptedData, cEncryptedKey);
    //CopyToClipboard(cCopy);

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
    //itoa(nPacketDataLength, cHttpContentLength, 10);
    sprintf(cHttpContentLength, "%d", nPacketDataLength);
    strcat(cOutPacket, cHttpContentLength);
    strcat(cOutPacket, cReturnNewline);
    strcat(cOutPacket, cReturnNewline);

    char cPacketData[nPacketDataLength];
    memset(cPacketData, 0, sizeof(cPacketData));
    strcpy(cPacketData, "a=");
    strcat(cPacketData, cFinalOutKey);
    //strcat(cPacketData, cEncryptedKey);
    strcat(cPacketData, "&b=");
    strcat(cPacketData, cFinalOutData);
    strcat(cPacketData, "&c=");
    strcat(cPacketData, cUrlEncodedMarker);

    //strcat(cOutPacket, cPacketData);
//CopyToClipboard(cPacketData);

    /*
#ifdef DEBUG
    printf("----------------------\nOutgoing Packet(%i bytes):\n%s\n", strlen(cOutPacket), cOutPacket);
    printf("-----------\nDecrypted Contents(%i bytes): %s\n", strlen(cHttpData), cHttpData);
#endif
     */
    EV_INFO << "----------------------\nOutgoing Packet(" << strlen(cOutPacket) << " bytes):\n" << cOutPacket << std::endl;
    EV_INFO << "-----------\nDecrypted Contents(" << strlen(cHttpData) << " bytes): " << cHttpData << std::endl;

    /*
    char cInPacket[MAX_HTTP_PACKET_LENGTH];
    memset(cInPacket, 0, sizeof(cInPacket));

    char cParsePacket[MAX_HTTP_PACKET_LENGTH];
    memset(cParsePacket, 0, sizeof(cParsePacket));

    SOCKET sSock = NULL;
    do
        sSock = fncsocket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    while(sSock == INVALID_SOCKET);
     */
    long requestLength = strlen(cOutPacket) + strlen(cPacketData);
    httptools::HttpRequestMessage *msg = new httptools::HttpRequestMessage(cOutPacket);
    msg->setTargetUrl(cHttpHost);
    msg->setProtocol(httpProtocol);
    msg->setHeading(cOutPacket);
    msg->setPayload(cPacketData);
    msg->setSerial(0);
    msg->setByteLength(requestLength);    // Add extra request size if specified
    msg->setKeepAlive(httpProtocol == 11);
    msg->setBadRequest(false);    // Simulates willingly requesting a non-existing resource.
    msg->setKind(HTTPT_REQUEST_MESSAGE);
    char szModuleName[127];
    strcpy(szModuleName, "192.168.56.102");

    const char *connectAddress = par("connectAddress");
    int connectPort = par("connectPort");

    sendRequestToServer(msg);
    EV_INFO << "Sending request to server";
    //EV_INFO << "Sending request to server " << request->targetUrl() << " (" << szModuleName << ") on port " << connectPort << endl;
    //submitToSocket(connectAddress, connectPort, msg);
    /*
    if(fncconnect(sSock, (PSOCKADDR)&httpreq, sizeof(httpreq)) != SOCKET_ERROR)
    {
        int nBytesSent = fncsend(sSock, cOutPacket, strlen(cOutPacket), NULL);
        if(nBytesSent != SOCKET_ERROR)
        {
#ifdef DEBUG
            printf("----------------------\nPacket successfully sent!(%i bytes)\n", nBytesSent);
#endif
            int nReceivedData = fncrecv(sSock, cInPacket, sizeof(cInPacket), NULL);

            strcpy(cParsePacket, cInPacket);

            if(nReceivedData < 1 && strlen(cParsePacket) < 1)
            {
#ifdef DEBUG
                printf("----------------------\nAn error occured! No server response...\n-----------\n");
#endif

                fncclosesocket(sSock);
                return FALSE;
            }
            else
            {
#ifdef DEBUG
                printf("----------------------\nIncoming Packet(%i bytes):\n%s\n-----------\n", strlen(cParsePacket), cParsePacket);
#endif
            }
        }
        else
        {
#ifdef DEBUG
            printf("-----------\nFailed to send packet to server\n");
#endif

            strcpy(cParsePacket, "ERR_FAILED_TO_SEND");
        }
    }
    else
    {
#ifdef DEBUG
        printf("-----------\nFailed to connect to server\n");
#endif

        strcpy(cParsePacket, "ERR_FAILED_TO_CONNECT");
    }
    fncclosesocket(sSock);
     */
    /*
    char *cBreakLine = strtok(cParsePacket, "\n");
    if(!IsValidHttpResponse(cBreakLine))
        return bReturn;
    else
        bReturn = TRUE;

#ifdef DEBUG
    printf("Decrypted Contents:\n");
#endif

    //if(!strstr(cInPacket, cMarkerBase64))
    if(!strstr(cInPacket, cMarker))
    {
#ifdef DEBUG
        printf("NONE![No Marker]\n");
#endif
        return FALSE;
    }

    //char *cMessageBundle = strstr(cInPacket, cMarkerBase64) + strlen(cMarkerBase64);
    char *cMessageBundle = strstr(cInPacket, cMarker) + strlen(cMarker);

    if(cMessageBundle == NULL || strstr(cInPacket, "Content-Length: 0"))
    {
#ifdef DEBUG
        printf("NONE![Content-Length: 0]\n");
#endif
        return FALSE;
    }
    else if(strstr(cMessageBundle, "Location:"))
    {
#ifdef DEBUG
        printf("NONE![LocationReturn]\n");
#endif
        bHttpRestart = TRUE;
        return FALSE;
    }

    char cDecryptedMessageBundle[MAX_HTTP_PACKET_LENGTH];
    memset(cDecryptedMessageBundle, 0, sizeof(cDecryptedMessageBundle));
    DecryptReceivedData(cMessageBundle, cKeyA, cKeyB, cDecryptedMessageBundle);

    cMessageBundle = strtok(cDecryptedMessageBundle, "\n");
    while(cMessageBundle != NULL)
    {
        ParseHttpLine(cMessageBundle);

        if(bHttpRestart)
            break;

        cMessageBundle = strtok(NULL, "\n");
    }

    return bReturn;
    */
}
void AthenaBot::EncryptSentData(char *cSource, char *cOutputData, char *cOutputEncryptedKey, char *cOutputRawKeyA, char *cOutputRawKeyB)
{
    srand(GenerateRandomSeed());

    /*
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
    */

    char cKeyA[KEY_SIZE];
    memset(cKeyA, 0, sizeof(cKeyA));
    char cKeyB[KEY_SIZE];
    memset(cKeyB, 0, sizeof(cKeyB));
    GeneratestrtrKey(cKeyA, cKeyB);

    char cKey[KEY_SIZE * 2];
    memset(cKey, 0, sizeof(cKey));
    sprintf(cKey, "%s:%s", cKeyA, cKeyB);

    char cKeyEncrypted[strlen(cKey) * 3];
    memset(cKeyEncrypted, 0, sizeof(cKeyEncrypted));
    //StringToStrongUrlEncodedString(cKey, cKeyEncrypted);

    char cEncrypted[strlen(cSource)];
    memset(cEncrypted, 0, sizeof(cEncrypted));
    strcpy(cEncrypted, cSource);
    strtr(cEncrypted, cKeyA, cKeyB);

    strcpy(cOutputData, cEncrypted);
    strcpy(cOutputEncryptedKey, cKey);
    strcpy(cOutputRawKeyA, cKeyA);
    strcpy(cOutputRawKeyB, cKeyB);

    /*
#ifdef DEBUG
    printf("----------------------\nEncryption Communication Details:\nKey: %s\nKey StrongUrlEncoded: %s\nData Encrypted(strtr): %s\n", cKey, cKeyEncrypted, cEncrypted);
#endif
*/
    EV_INFO << "----------------------\nEncryption Communication Details:\nKey: " << cKey << "\nKey StrongUrlEncoded: " << cKeyEncrypted << "\nData Encrypted(strtr): " << cEncrypted << std::endl;
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
    std::default_random_engine generator;
    std::uniform_int_distribution<unsigned long> distribution(0, std::numeric_limits<unsigned long>::max());

    return clock() + time(NULL) + distribution(generator);
    //return clock() + time(NULL) + _getpid();
}

/**
 * Utilities/Misc.cpp
 */
char *AthenaBot::GenRandLCText() //Generates random lowercase text
{
    //char cLCAlphabet[27];
    //strcpy(cLCAlphabet, "abcdefghijklmnopqrstuvwxyz");

    static char cRandomText[9];

    for(unsigned short us = 0; us < 8; us++)
        cRandomText[us] = (char)(GetRandNum(26) + 65 + 32);

    //for(unsigned short us = 0; us < 8; us++)
    //cRandomText[us] = cLCAlphabet[GetRandNum(strlen(cLCAlphabet))];

    return (char*)cRandomText;
}
void AthenaBot::RunThroughUuidProcedure()
{
    memset(cUuid, 0, sizeof(cUuid));
    UUID uuid;
    //ZeroMemory(&uuid, sizeof(UUID));
    memset(&uuid, 0, sizeof(UUID));
    fncUuidCreate(&uuid);

    char *ucUuidString = (char *) malloc(50);
    fncUuidToString(&uuid, ucUuidString);

    strcpy(cUuid, (const char*)ucUuidString);
    fncRpcStringFree(ucUuidString);
    StripDashes(cUuid);

    /* Currently, registry Key is not needed!
     *
     */
    /*
    char cKeyPath[MAX_PATH];
    strcpy(cKeyPath, "Software\\");
    //strcat(cKeyPath, cFileHash);

    if(bNewInstallation)
    {
//#ifndef DEBUG
        /FILE * pFile;
        pFile = fopen(cFile, "w");

        if(pFile != NULL)
        {
            fputs(cUuid, pFile);
            fclose(pFile);
        }/

        CreateRegistryKey(HKEY_CURRENT_USER, cKeyPath, cFileHash, cUuid);
        /if(CreateRegistryKey(HKEY_CURRENT_USER, cKeyPath, cFileHash, cUuid))
            printf("Successfully created UUID registry key value (UUID=\"%s\")[KeyPath=\"%s\", KeyName=\"%s\"]\n", cUuid, cKeyPath, cFileHash);
        else
            printf("Failed to create UUID registry key value\n");

        system("pause");/
//#endif
    }
    else
    {
        // char *cFileContents = GetContentsFromFile();
        //if(!strstr(cFileContents, "-"))
        //    strcpy(cUuid, cFileContents);

        char *cKeyValue = ReadRegistryKey(HKEY_CURRENT_USER, cKeyPath, cFileHash);

        if(!strstr(cKeyValue, "EFNF") || !strstr(cKeyValue, "NOERRSUC") || !strstr(cKeyValue, "ERROPFA") || strlen(cKeyValue) < 36)
        {
            if(strstr(cKeyValue, "\""))
                cKeyValue = FindInString(cKeyValue, (char*)"\"", (char*)"\"");

            strcpy(cUuid, cKeyValue);

            //printf("Successfully retrieved UUID registry key value (UUID=\"%s\")\n", cUuid);
        }
        //else
        //    printf("Failed to retrieve UUID registry key value\n");

        //system("pause");
    }
    */
}
unsigned long AthenaBot::GetRandNum(unsigned long range) //Returns a random number within a given range
{
    /**Avoiding CRC AREA
    // <!-------! CRC AREA START !-------!>
    char cCheckString[DEFAULT];
    sprintf(cCheckString, "%s@%s:%i", cChannel, cServer, usPort);
    char *cStr = cCheckString;
    unsigned long ulCheck = (360527*3)/(199+2);
    int nCheck;
    while((nCheck = *cStr++))
        ulCheck = ((ulCheck << 5) + ulCheck) + nCheck;
    if(ulCheck != ulChecksum7)
        return rand() % 10;
    // <!-------! CRC AREA STOP !-------!>
     */
    //Sleep(1);
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
    int i, tmplen = 3;
    size_t converted = 0;

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

//} /* namespace athena */

} /* namespace inet */