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

#include "../../Modulos/DoSModule/DoSModule.h"

namespace inet {

namespace simbo {

Define_Module(DoSModule);

DoSModule::DoSModule()
{

}

DoSModule::~DoSModule()
{
    // TODO Auto-generated destructor stub
}

void DoSModule::initialize(int stage)
{
    EV_INFO << "Initializing DoS Module" << endl;
    inet::httptools::HttpBrowser::initialize(stage);
    this->setName("DoSModule");
    cancelEvent(this->eventTimer);
}

void DoSModule::finish()
{
    EV_INFO << "Finalizing DoS Module" << endl;
    inet::httptools::HttpBrowser::finish();
}

void DoSModule::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
        handleSelfMessages(msg);
    else
        inet::httptools::HttpBrowser::handleMessage(msg);
}

void DoSModule::handleSelfMessages(cMessage *msg)
{
    switch (msg->getKind()) {
    case MSGKIND_NEXT_MESSAGE:
        handleSelfNextMessage();
        break;
    default:
        inet::httptools::HttpBrowser::handleSelfMessages(msg);
    }
}

void DoSModule::handleSelfNextMessage()
{
    EV_INFO << "New browse event triggered @ T=" << simTime() << endl;
    EV_INFO << "Next message in session # " << sessionCount << " @ T=" << simTime() << ". "
            << "Current request is " << reqInCurSession << "/" << reqNoInCurSession << "\n";
    sendRequestToServer(generateRandomPageRequest(target, true, 0));
    eventTimer->setKind(MSGKIND_NEXT_MESSAGE);
    scheduleAt(simTime() + (simtime_t) 1, eventTimer);
}

void DoSModule::activateModule(std::string target)
{
    Enter_Method_Silent();
    if (eventTimer->isScheduled()) return;
    this->target = target;
    eventTimer->setKind(MSGKIND_ACTIVITY_START);
    scheduleAt(simTime() + (simtime_t) 1, eventTimer);
}

void DoSModule::desactivateModule()
{
    cancelEvent(eventTimer);
}

} /* namespace simbo */

} /* namespace inet */
