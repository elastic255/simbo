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

#include "GenericBot.h"

namespace inet {

namespace simbo {

GenericBot::GenericBot()
{
    downloadModule = nullptr;
    dosModule = nullptr;
    httpModule = nullptr;
}

GenericBot::~GenericBot()
{
    // TODO Auto-generated destructor stub
}

void GenericBot::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        infectedHost = par("infected").boolValue();
        botProtocol = strcmp(par("botProtocol").stringValue(), "http") ? irc : http;
        botMessage = new cMessage("Bot Message");
    }
}

bool GenericBot::handleOperationStage(LifecycleOperation *operation, int stage, IDoneCallback *doneCallback)
{
    Enter_Method_Silent();
    throw cRuntimeError("Unsupported lifecycle operation '%s'", operation->getClassName());
    return true;
}

cModule *GenericBot::addModule(const char *name, const char *type, bool isITCPApp)
{
    cModuleType *moduleType = cModuleType::get(type);
    cModule *module = moduleType->createScheduleInit(name, this->getParentModule());

    if (isITCPApp) {
        cModule *tcp = this->getParentModule()->getSubmodule("tcp");

        if (tcp != nullptr) {
            int gateSize = tcp->gateSize("appIn");

            tcp->setGateSize("appIn", gateSize+1);
            tcp->setGateSize("appOut", gateSize+1);
            cGate *tcpIn = tcp->gate("appIn", gateSize);
            cGate *tcpOut = tcp->gate("appOut", gateSize);
            cGate *gateIn = module->gate("tcpIn");
            cGate *gateOut = module->gate("tcpOut");
            gateOut->connectTo(tcpIn);
            tcpOut->connectTo(gateIn);
        }
    }
    return module;
}

} /* namespace simbo */
} /* namespace inet */
