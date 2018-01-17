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

#ifndef INET_APPLICATIONS_SIMBO_MODULOS_HTTPMODULE_HTTPMODULE_H_
#define INET_APPLICATIONS_SIMBO_MODULOS_HTTPMODULE_HTTPMODULE_H_

#include "inet/common/INETDefs.h"

#include "inet/applications/httptools/browser/HttpBrowser.h"

namespace inet {

namespace simbo {

/**
 * A HTTP Module only used for passing HTTP messages.
 * It is just a workaround to call protected methods from httptools::HttpBrowser
 */
// TODO: maybe, I will use this module to send random requests (just a way to simulate)
class INET_API HTTPModule : public httptools::HttpBrowser
{
private:
    cModule *lastSender;

protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    void handleSelfMessages(cMessage *msg);
    void handleDataMessage(cMessage *msg);


    /*
     * Handler for socket data arrival.
     * Called by the socket->processMessage(msg) handler call in handleMessage.
     * virtual method of the parent class. The counter for pending replies is decremented for each one handled.
     * Close is called on the socket once the counter reaches zero.
     */
    virtual void socketDataArrived(int connId, void *yourPtr, cPacket *msg, bool urgent) override;

public:
    bool sendRequestToServer(cModule *sender, std::string url, std::string header, std::string body);
    void set_name(std::string name);

public:
    HTTPModule();
    virtual ~HTTPModule();
};

} /* namespace simbo */
} /* namespace inet */

#endif /* INET_APPLICATIONS_SIMBO_MODULOS_HTTPMODULE_HTTPMODULE_H_ */
