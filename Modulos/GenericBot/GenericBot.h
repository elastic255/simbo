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

#ifndef INET_APPLICATIONS_SIMBO_MODULOS_GENERICBOT_GENERICBOT_H_
#define INET_APPLICATIONS_SIMBO_MODULOS_GENERICBOT_GENERICBOT_H_

#include "inet/common/INETDefs.h"
#include "inet/common/lifecycle/ILifecycle.h"
#include "inet/common/lifecycle/LifecycleOperation.h"
#include "inet/applications/simbo/Modulos/HTTPModule/HTTPModule.h"
#include "inet/applications/simbo/Modulos/DoSModule/DoSModule.h"

namespace inet {

namespace simbo {

class INET_API GenericBot : public cSimpleModule, public ILifecycle
{
protected:
    enum Protocol { http, irc } botProtocol;
    HTTPModule *httpModule;
    DoSModule *dosModule;
    HTTPModule *downloadModule;
    bool infectedHost;
    cMessage *botMessage = nullptr;

protected:
    virtual void initialize(int stage) override;
    virtual bool handleOperationStage(LifecycleOperation *operation, int stage, IDoneCallback *doneCallback) override;

    cModule *addModule(const char *name, const char *type, bool isITCPApp = true);

public:
    GenericBot();
    virtual ~GenericBot();
};

} /* namespace simbo */
} /* namespace inet */

#endif /* INET_APPLICATIONS_SIMBO_MODULOS_GENERICBOT_GENERICBOT_H_ */
