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

#include "../AthenaCC/AthenaCC.h"

namespace inet {

namespace simbo {

Define_Module(AthenaCC);

//namespace athena {

AthenaCC::AthenaCC() {
    on_exec_requests = 0;
    repeat_requests = 0;
}

AthenaCC::~AthenaCC() {

}

void AthenaCC::initialize(int stage)
{
    HttpServerBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        numBroken = 0;
        socketsOpened = 0;

        WATCH(numBroken);
        WATCH(socketsOpened);
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        EV_INFO << "Initializing server component (sockets version)" << endl;

        usPort = par("port");

        TCPSocket listensocket;
        listensocket.setOutputGate(gate("tcpOut"));
        listensocket.setDataTransferMode(TCP_TRANSFER_OBJECT);
        listensocket.bind(port);
        listensocket.setCallbackObject(this);
        listensocket.listen();

    }
}

void AthenaCC::finish()
{
    HttpServerBase::finish();

    EV_INFO << "on_exec requests received: " << on_exec_requests << endl;
    EV_INFO << "repeat requests received: " << repeat_requests << endl;
    EV_INFO << "Bots information: " << endl;
    EV_INFO << "|botid|newbot|country|country_code|ip|os|cpu|type|cores|version|net|botskilled|files|regkey|admin|ram|busy|lastseen|" << endl;
    for (std::vector<BOT>::iterator it = botlist.begin(); it != botlist.end(); ++it) {
        EV_INFO << "|" << it->botid << "|" << it->newbot << "|" << it->country << "|";
        EV_INFO << it->country_code << "|" << it->ip << "|" << it->os << "|" << it->cpu << "|";
        EV_INFO << it->type << "|" << it->cores << "|" << it->version << "|" << it->net << "|";
        EV_INFO << it->botskilled << "|" << it->files << "|" << it->regkey << "|" << it->admin << "|";
        EV_INFO << it->ram << "|" << it->busy << "|" << it->lastseen << "|" << endl;
    }
}

void AthenaCC::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        // Self messages not used at the moment
    }
    else {
        EV_INFO << "Handle inbound message " << msg->getName() << " of kind " << msg->getKind() << endl;
        TCPSocket *socket = sockCollection.findSocketFor(msg);
        if (!socket) {
            EV_INFO << "No socket found for the message. Create a new one" << endl;
            // new connection -- create new socket object and server process
            socket = new TCPSocket(msg);
            socket->setOutputGate(gate("tcpOut"));
            socket->setDataTransferMode(TCP_TRANSFER_OBJECT);
            socket->setCallbackObject(this, socket);
            sockCollection.addSocket(socket);
        }
        EV_INFO << "Process the message " << msg->getName() << endl;
        socket->processMessage(msg);
    }
}

void AthenaCC::socketEstablished(int connId, void *yourPtr)
{
    EV_INFO << "connected socket with id=" << connId << endl;
    socketsOpened++;
}

void AthenaCC::socketDataArrived(int connId, void *yourPtr, cPacket *msg, bool urgent)
{
    if (yourPtr == nullptr) {
        EV_ERROR << "Socket establish failure. Null pointer" << endl;
        return;
    }
    TCPSocket *socket = (TCPSocket *)yourPtr;

    // Should be a HttpReplyMessage
    EV_INFO << "Socket data arrived on connection " << connId << ". Message=" << msg->getName() << ", kind=" << msg->getKind() << endl;

    // call the message handler to process the message.
    cMessage *reply = handleReceivedMessage(msg);
    if (reply != nullptr) {
        socket->send(reply);    // Send to socket if the reply is non-zero.
    }
    delete msg;    // Delete the received message here. Must not be deleted in the handler!
}

void AthenaCC::socketPeerClosed(int connId, void *yourPtr)
{
    if (yourPtr == nullptr) {
        EV_ERROR << "Socket establish failure. Null pointer" << endl;
        return;
    }
    TCPSocket *socket = (TCPSocket *)yourPtr;

    // close the connection (if not already closed)
    if (socket->getState() == TCPSocket::PEER_CLOSED) {
        EV_INFO << "remote TCP closed, closing here as well. Connection id is " << connId << endl;
        socket->close();    // Call the close method to properly dispose of the socket.
    }
}

void AthenaCC::socketClosed(int connId, void *yourPtr)
{
    EV_INFO << "connection closed. Connection id " << connId << endl;

    if (yourPtr == nullptr) {
        EV_ERROR << "Socket establish failure. Null pointer" << endl;
        return;
    }
    // Cleanup
    TCPSocket *socket = (TCPSocket *)yourPtr;
    sockCollection.removeSocket(socket);
    delete socket;
}

void AthenaCC::socketFailure(int connId, void *yourPtr, int code)
{
    EV_WARN << "connection broken. Connection id " << connId << endl;
    numBroken++;

    EV_INFO << "connection closed. Connection id " << connId << endl;

    if (yourPtr == nullptr) {
        EV_ERROR << "Socket establish failure. Null pointer" << endl;
        return;
    }
    TCPSocket *socket = (TCPSocket *)yourPtr;

    if (code == TCP_I_CONNECTION_RESET)
        EV_WARN << "Connection reset!\n";
    else if (code == TCP_I_CONNECTION_REFUSED)
        EV_WARN << "Connection refused!\n";

    // Cleanup
    sockCollection.removeSocket(socket);
    delete socket;
}

cPacket *AthenaCC::handleReceivedMessage(cMessage *msg)
{
    httptools::HttpRequestMessage *request = check_and_cast<httptools::HttpRequestMessage *>(msg);
    if (request == nullptr)
        throw cRuntimeError("Message (%s)%s is not a valid request", msg->getClassName(), msg->getName());

    EV_INFO << "Handling received message " << msg->getName() << ". Target URL: " << request->targetUrl() << endl;

    logRequest(request);

    if (httptools::extractServerName(request->targetUrl()) != hostName) {
        // This should never happen but lets check
        throw cRuntimeError("Received message intended for '%s'", request->targetUrl());    // TODO: DEBUG HERE
        return nullptr;
    }

    httptools::HttpReplyMessage *replymsg;

    // Parse the request string on spaces
    cStringTokenizer tokenizer = cStringTokenizer(request->heading(), " ");
    std::vector<std::string> res = tokenizer.asVector();
    /*if (res.size() != 3) {
        EV_ERROR << "Invalid request string: " << request->heading() << endl;
        replymsg = generateErrorReply(request, 400);
        logResponse(replymsg);
        return replymsg;
    }*/

    if (request->badRequest()) {
        // Bad requests get a 404 reply.
        EV_ERROR << "Bad request - bad flag set. Message: " << request->getName() << endl;
        replymsg = generateErrorReply(request, 404);
    }
    else if (res[0] == "GET") {
        replymsg = handleGetRequest(request, res[1]);    // Pass in the resource string part
    }
    else if (res[0] == "POST") {
        replymsg = handlePostRequest(request); // Pass the entire request
    }
    else {
        EV_ERROR << "Unsupported request type " << res[0] << " for " << request->heading() << endl;
        replymsg = generateErrorReply(request, 400);
    }

    if (replymsg != nullptr)
        logResponse(replymsg);

    return replymsg;
}

httptools::HttpReplyMessage *AthenaCC::handlePostRequest(httptools::HttpRequestMessage *request)
{
    /* Using the server as a proxy */
    /*
    SOCKET sSock;
    struct sockaddr_in httpreq;
    char cInPacket[MAX_HTTP_PACKET_LENGTH];
    memset(cInPacket, 0, sizeof(cInPacket));

    char cParsePacket[MAX_HTTP_PACKET_LENGTH];
    memset(cParsePacket, 0, sizeof(cParsePacket));

    do
        sSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    while (sSock == INVALID_SOCKET);

    httpreq.sin_port = htons(usPort);
    httpreq.sin_family = AF_INET;
    httpreq.sin_addr.s_addr = inet_addr(srvAddr);

    char cOutPacket[MAX_HTTP_PACKET_LENGTH];
    strcpy(cOutPacket, request->heading());
    strcat(cOutPacket, request->payload());

    EV_INFO << "Packet sent:\n" << cOutPacket;
    if (connect(sSock, (struct sockaddr *) &httpreq, sizeof(httpreq)) != SOCKET_ERROR) {
        int nBytesSent = sendto(sSock, cOutPacket, strlen(cOutPacket), NULL, (struct sockaddr *) &httpreq, sizeof(httpreq));
        if(nBytesSent != SOCKET_ERROR)
        {
            int nReceivedData = recv(sSock, cInPacket, sizeof(cInPacket), NULL);
            strcpy(cParsePacket, cInPacket);
            if(nReceivedData < 1 && strlen(cParsePacket) < 1)
            {
                close(sSock);
            }
            else
            {
                EV_INFO << "----------------------\nIncoming Packet(" << strlen(cParsePacket) << " bytes):\n" << cParsePacket << "\n-----------\n";
            }
        }
        else
        {
            EV_INFO << "-----------\nFailed to send packet to server\n";
            strcpy(cParsePacket, "ERR_FAILED_TO_SEND");
        }
    }
    else
    {
        EV_INFO << "-----------\nFailed to connect to server\n";
        strcpy(cParsePacket, "ERR_FAILED_TO_CONNECT");
    }
    close(sSock);
    httptools::HttpReplyMessage reply;
    reply.setPayload(cParsePacket);
    reply.setResult(200);
     */

    httptools::HttpReplyMessage *replymsg;

    EV_INFO << "Heading: " << request->heading() << endl;
    cStringTokenizer tokenizer = cStringTokenizer(request->payload(), "&");
    EV_INFO << "Payload:" << endl;
    std::vector<std::string> payload = tokenizer.asVector();
    std::vector<std::string> contents;
    for (std::vector<std::string>::iterator it = payload.begin(); it != payload.end(); ++it) {
        EV_INFO << *it << endl;
        tokenizer = cStringTokenizer(it->c_str(), "=");
        std::vector<std::string> content = tokenizer.asVector();
        contents.push_back(urldecode(content[1]));
    }

    // Decoding the keys (base64 decode)
    int keys_size = contents[0].size();
    char coded_keys[keys_size+1];
    char decoded_keys[keys_size+1];
    memset(decoded_keys, 0, sizeof(decoded_keys));
    for (int i = 0; i < keys_size; ++i) {
        coded_keys[i] = contents[0][i];
        coded_keys[i+1] = '\0';
    }
    size_t result = base64_decode(coded_keys, decoded_keys, keys_size+1);
    tokenizer = cStringTokenizer(decoded_keys, ":");
    std::vector<std::string> keys = tokenizer.asVector();

    // Decoding data (base64 decode)
    int n = contents[1].size();
    char coded_data[n+1];
    char decoded_data[n+1];
    memset(decoded_data, 0, sizeof(decoded_data));
    for (int i = 0; i < n; ++i) {
        coded_data[i] = contents[1][i];
        coded_data[i+1] = '\0';
    }
    strtr(coded_data, keys[1].c_str(), keys[0].c_str());

    EV_INFO << "Key A: " << keys[0] << endl << "Key B: " << keys[1] << endl;
    result = base64_decode(coded_data, decoded_data, n+1);
    EV_INFO << "Data:" << endl << decoded_data << endl;
    processData(decoded_data);
    replymsg = generateReply(request, contents[2]);

    return replymsg;
}

httptools::HttpReplyMessage *AthenaCC::generateReply(httptools::HttpRequestMessage *request, std::string& outDataMarker)
{
    char szReply[512];
    sprintf(szReply, "HTTP/1.1 200 OK");
    httptools::HttpReplyMessage *replymsg = new httptools::HttpReplyMessage(szReply);
    replymsg->setHeading("HTTP/1.1 200 OK");
    replymsg->setOriginatorUrl(hostName.c_str());
    replymsg->setTargetUrl(request->originatorUrl());
    replymsg->setProtocol(request->protocol());
    replymsg->setSerial(request->serial());
    replymsg->setResult(200);
    replymsg->setContentType(1);    // Emulates the content-type header field
    replymsg->setKind(HTTPT_RESPONSE_MESSAGE);
    char outDataMarker_encoded[2000];
    memset(outDataMarker_encoded, 0, sizeof(outDataMarker_encoded));
    char *outDataMarker_decoded = new char[outDataMarker.size() + 1];
    std::copy(outDataMarker.begin(), outDataMarker.end(), outDataMarker_decoded);
    outDataMarker_decoded[outDataMarker.size()+1] = '\0';
    base64_encode((unsigned char *) outDataMarker_decoded, strlen(outDataMarker_decoded), outDataMarker_encoded, sizeof(outDataMarker_encoded));
    replymsg->setPayload(outDataMarker_encoded);
    int size = strlen(outDataMarker_encoded);
    replymsg->setByteLength(size);
    delete[] outDataMarker_decoded;
    return replymsg;
}

void AthenaCC::processData(char *data)
{
    cStringTokenizer tokenizer = cStringTokenizer(data, "|");
    std::vector<std::string> tokens = tokenizer.asVector();
    std::string type, uid, priv, arch, gend, cores, os, ver, net, new_, ram;
    std::string killed, files, regkey, busy, taskid, return_;
    for (std::vector<std::string>::iterator it = tokens.begin(); it != tokens.end(); ++it) {
        tokenizer = cStringTokenizer(it->c_str(), ":");
        std::vector<std::string> tok = tokenizer.asVector();
        std::string key = tok[0];
        std::string value = tok[1];
        if (key.compare("type") == 0) {
            type = value;
            if (value.compare("on_exec") == 0)
                on_exec_requests++;
            else if (value.compare("repeat") == 0)
                repeat_requests++;
        }
        else if (key.compare("uid") == 0)
            uid = value;
        else if (key.compare("priv") == 0)
            priv = value;
        else if (key.compare("arch") == 0)
            arch = value;
        else if (key.compare("gend") == 0)
            gend = value;
        else if (key.compare("cores") == 0)
            cores = value;
        else if (key.compare("os") == 0)
            os = value;
        else if (key.compare("ver") == 0)
            ver = value;
        else if (key.compare("net") == 0)
            net = value;
        else if (key.compare("new") == 0)
            new_ = value;
        else if (key.compare("ram") == 0)
            ram = value;
        else if (key.compare("bk_killed") == 0)
            killed = value;
        else if (key.compare("bk_files") == 0)
            files = value;
        else if (key.compare("bk_keys") == 0)
            regkey = value;
        else if (key.compare("busy") == 0)
            busy = value;
        else if (key.compare("taskid") == 0)
            taskid = value;
        else if (key.compare("return") == 0)
            return_ = value;
    }

    if (!uid.size())
        EV_INFO << "TASKS_OUTPUT";

    if (type.compare("on_exec") == 0) {
        BOT newBot;
        newBot.botid = uid;
        newBot.newbot = std::stoi(new_);
        newBot.os = os;
        newBot.cpu = arch.compare("x64") ? 0 : 1;
        newBot.type = gend.compare("laptop") ? 0 : 1;
        newBot.cores = std::stoi(cores);
        newBot.version = ver;
        newBot.net = net;
        newBot.admin = priv.compare("admin") ? 1 : 0;
        newBot.busy = busy.compare("true") ? 1 : 0;
        botlist.push_back(newBot);
    }
}

std::string AthenaCC::urldecode(std::string str)
{
    char ch;
    int j = 0, counter;
    std::string ret;
    do {
        ret.clear();
        counter = 0;
        for (unsigned int i = 0; i < str.length(); i++) {
            if (str[i] == '%') {
                std::string aux (str.substr(i+1,2));
                std::istringstream in(aux);
                in >> std::hex >> j;
                ch = static_cast<char>(j);
                ret += ch;
                i = i + 2;
                counter++;
            } else {
                ret += str[i];
            }
        }
        str = ret;
    } while (counter != 0);

    return ret;
}

/**
 * encode three bytes using base64 (RFC 3548)
 *
 * @param triple three bytes that should be encoded
 * @param result buffer of four characters where the result is stored
 */
void AthenaCC::_base64_encode_triple(unsigned char triple[3], char result[4])
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
int AthenaCC::base64_encode(unsigned char *source, size_t sourcelen, char *target, size_t targetlen)
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
int AthenaCC::_base64_char_value(char base64char)
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

int AthenaCC::_base64_decode_triple(char quadruple[4], unsigned char *result)
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
size_t AthenaCC::base64_decode(const char *source, char *target, size_t targetlen)
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

void AthenaCC::strtr(char *cSource, const char *cCharArrayA, const char *cCharArrayB)
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
