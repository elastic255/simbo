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

#include "HTTPModule.h"

namespace inet {

namespace simbo {

Define_Module(HTTPModule);

HTTPModule::HTTPModule() {
    lastSender = nullptr;

}

HTTPModule::~HTTPModule() {
    // TODO Auto-generated destructor stub
}

void HTTPModule::initialize(int stage) {
    EV_INFO << "Initializing HTTPModule component, stage " << stage << endl;
    inet::httptools::HttpBrowser::initialize(stage);
    this->setName("HTTPModule");
    this->cancelEvent(eventTimer);
}

/**
 * This module must not send any self message
 */
void HTTPModule::handleSelfMessages(cMessage *msg) {
    switch (msg->getKind()) {
    case MSGKIND_ACTIVITY_START:
    case MSGKIND_START_SESSION:
    case MSGKIND_NEXT_MESSAGE:
    case MSGKIND_SCRIPT_EVENT:
    default: ;
    }
}

void HTTPModule::handleMessage(cMessage *msg)
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

void HTTPModule::handleDataMessage(cMessage *msg)
{
    httptools::HttpReplyMessage *appmsg = check_and_cast<httptools::HttpReplyMessage *>(msg);
    if (appmsg == nullptr)
        throw cRuntimeError("Message (%s)%s is not a valid reply message", msg->getClassName(), msg->getName());

    cGate *senderInput;
    try {
        senderInput = lastSender->gate("httpgate");
    }
    catch (cRuntimeError e)
    {
        senderInput = lastSender->addGate("httpgate", cGate::INPUT, false);
    }
    this->sendDirect(msg, senderInput);
    lastSender = nullptr;
    //logResponse(appmsg);
    //EV_INFO << "Message data: " << appmsg->payload() << endl;
    //delete appmsg;
    return;
}


void HTTPModule::socketDataArrived(int connId, void *yourPtr, cPacket *msg, bool urgent)
{
    EV_INFO << "Socket data arrived on connection " << connId << ": " << msg->getName() << endl;
    if (yourPtr == nullptr) {
        EV_ERROR << "socketDataArrivedfailure. Null pointer" << endl;
        return;
    }

    SockData *sockdata = (SockData *)yourPtr;
    TCPSocket *socket = sockdata->socket;
    handleDataMessage(msg);

    if (--sockdata->pending == 0) {
        EV_INFO << "Received last expected reply on this socket. Issuing a close" << endl;
        socket->close();
    }
    // Message deleted in handler - do not delete here!
}


bool HTTPModule::sendRequestToServer(cModule *sender, std::string url, std::string header, std::string body) {
    Enter_Method_Silent();

    if (this->lastSender != nullptr) {
        EV_ERROR << "Unable to send request, HTTPModule is currently busy." << endl;
        return false;
    }

    int connectPort;
    char szWWW[127];
    char szModuleName[127];
    this->lastSender = sender;

    if (controller->getServerInfo(url.c_str(), szModuleName, connectPort) != 0) {
        EV_ERROR << "Unable to get server info for URL " << url << endl;
        return false;
    }

    // Request message construction
    int requestLength = header.size() + body.size();
    inet::httptools::HttpRequestMessage *msg = new inet::httptools::HttpRequestMessage(header.c_str());
    msg->setTargetUrl(url.c_str());
    msg->setProtocol(httpProtocol);
    msg->setHeading(header.c_str());
    msg->setPayload(body.c_str());
    msg->setSerial(0);
    msg->setByteLength(requestLength);    // Add extra request size if specified
    msg->setKeepAlive(httpProtocol == 11);
    msg->setBadRequest(false);    // Simulates willingly requesting a non-existing resource.
    msg->setKind(HTTPT_REQUEST_MESSAGE);

    this->submitToSocket(szModuleName, connectPort, msg);
    return true;
}

} /* namespace simbo */
} /* namespace inet */
