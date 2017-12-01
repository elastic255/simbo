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

Define_Module(AthenaCC);

//namespace athena {

AthenaCC::AthenaCC() {
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
        EV_DEBUG << "Initializing server component (sockets version)" << endl;

        int port = par("port");

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
}

void AthenaCC::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        // Self messages not used at the moment
    }
    else {
        EV_DEBUG << "Handle inbound message " << msg->getName() << " of kind " << msg->getKind() << endl;
        TCPSocket *socket = sockCollection.findSocketFor(msg);
        if (!socket) {
            EV_DEBUG << "No socket found for the message. Create a new one" << endl;
            // new connection -- create new socket object and server process
            socket = new TCPSocket(msg);
            socket->setOutputGate(gate("tcpOut"));
            socket->setDataTransferMode(TCP_TRANSFER_OBJECT);
            socket->setCallbackObject(this, socket);
            sockCollection.addSocket(socket);
        }
        EV_DEBUG << "Process the message " << msg->getName() << endl;
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
    EV_DEBUG << "Socket data arrived on connection " << connId << ". Message=" << msg->getName() << ", kind=" << msg->getKind() << endl;

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

    EV_DEBUG << "Handling received message " << msg->getName() << ". Target URL: " << request->targetUrl() << endl;

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
    /* For debug purposes */
    /*
    httptools::HttpReplyMessage *replymsg;

    EV_DEBUG << "Heading: " << request->heading() << endl;
    cStringTokenizer tokenizer2 = cStringTokenizer(request->payload(), "&");
    EV_DEBUG << "Payload:" << endl;
    std::vector<std::string> payload = tokenizer2.asVector();
    std::vector<std::string> contents;
    for (std::vector<std::string>::iterator it = payload.begin(); it != payload.end(); ++it) {
        EV_DEBUG << *it << endl;
        cStringTokenizer tokenizer3 = cStringTokenizer(it->c_str(), "=");
        std::vector<std::string> content = tokenizer3.asVector();
        std::cout << content[0] << " : " << content[1];
        std::string ret;
        char ch;
        int j = 0;
        for (unsigned int i = 0; i < content[1].length(); i++) {
            if (content[1][i] == '%') {
                std::string str (content[1].substr(i+1,2));
                std::istringstream in(str);
                in >> std::hex >> j;
                ch = static_cast<char>(j);
                ret += ch;
                i = i + 2;
            } else {
                ret += content[1][i];
            }
        }
        contents.push_back(ret);
    }
    char key_coded[2000];
    char key_decoded[2000];

    for (int i = 0; i < contents[0].length(); ++i) {
        key_coded[i] = contents[0][i];
        key_coded[i+1] = '\0';
    }

    //size_t result = base64_decode(key_coded, key_decoded, 2000);
    cStringTokenizer keyTokenizer = cStringTokenizer(key_coded, ":");
    std::vector<std::string> keys = keyTokenizer.asVector();
    char data_coded[2000];
    char key0[2000];
    char key1[2000];

    for (int i = 0; i < contents[1].length(); ++i) {
        data_coded[i] = contents[1][i];
        data_coded[i+1] = '\0';
    }

    for (int i = 0; i < keys[0].length(); ++i) {
        key0[i] = keys[0][i];
        key0[i+1] = '\0';
    }

    for (int i = 0; i < keys[1].length(); ++i) {
        key1[i] = keys[1][i];
        key1[i+1] = '\0';
    }
    EV_DEBUG << "Key A: " << key0 << endl << "Key B: " << key1 << endl;
    strtr(data_coded, key1, key0);
    char data[2000];
    EV_DEBUG << "Data:" << endl << data_coded << endl;

    replymsg = generateErrorReply(request, 404);

    return replymsg;
    */
}

/**
 * For Athena bot only
 */
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

void AthenaCC::strtr(char *cSource, char *cCharArrayA, char *cCharArrayB)
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
