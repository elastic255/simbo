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

#ifndef INET_APPLICATIONS_SIMBO_MODULOS_DOSMODULE_DOSMODULE_H_
#define INET_APPLICATIONS_SIMBO_MODULOS_DOSMODULE_DOSMODULE_H_

#include "inet/applications/httptools/browser/HttpBrowser.h"

namespace inet {

namespace simbo {

/**
 * Only http flood implemented
 */
class INET_API DoSModule : public inet::httptools::HttpBrowser {
private:
    std::string target;
protected:
    virtual void initialize(int stage) override;
    virtual void finish() override;
    virtual void handleMessage(cMessage *msg) override;
    void handleSelfMessages(cMessage *msg);
    void handleSelfNextMessage();
public:
    DoSModule();
    virtual ~DoSModule();
    void activateModule(std::string target);
    void desactivateModule();
};

} /* namespace simbo */

} /* namespace inet */

#endif /* INET_APPLICATIONS_SIMBO_MODULOS_DOSMODULE_DOSMODULE_H_ */
