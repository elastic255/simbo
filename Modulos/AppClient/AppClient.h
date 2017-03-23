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

#ifndef APPCLIENT_H_
#define APPCLIENT_H_

#include "inet/common/INETDefs.h"

#include "inet/applications/tcpapp/TCPAppBase.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include "inet/common/lifecycle/ILifecycle.h"

#include "inet/common/lifecycle/NodeOperations.h"
#include "inet/common/ModuleAccess.h"
#include "inet/applications/tcpapp/GenericAppMsg_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/ipv4/IPv4InterfaceData.h"
#include "inet/networklayer/ipv6/IPv6InterfaceData.h"
#include <vector>
#include <random>

namespace inet {

class INET_API AppClient : public TCPAppBase, public ILifecycle{

public:
    AppClient(){};
    virtual ~AppClient();

protected:
    cMessage *timeoutMsg = nullptr;
    NodeStatus *nodeStatus = nullptr;
    bool earlySend = false;    // if true, don't wait with sendRequest() until established()
    int numRequestsToSend = 0;    // requests to send in this session
    simtime_t startTime;
    simtime_t stopTime;

    std::vector<TCPSocket*> filaConexoes;
    std::vector<TCPSocket*>::iterator it;

    TCPSocket* tempSocket;

    std::default_random_engine generator;

    //virtual void rescheduleOrDeleteTimer(simtime_t d, short int msgKind);

    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleTimer(cMessage *msg) override;
    virtual void socketEstablished(int connId, void *yourPtr) override;
    virtual void socketDataArrived(int connId, void *yourPtr, cPacket *msg, bool urgent) override;
    virtual void socketClosed(int connId, void *yourPtr) override;
    virtual void socketFailure(int connId, void *yourPtr, int code) override;
    virtual bool isNodeUp();
    virtual bool handleOperationStage(LifecycleOperation *operation, int stage, IDoneCallback *doneCallback) override;
    virtual void conectar();
    virtual TCPSocket* getConexao(int connId);
    virtual bool deleteConexao(int connId);
    virtual void sendConexao(int connId);
    virtual void handleMessage(cMessage *msg) override;
    virtual TCPSocket* belongsToSocket(cMessage *msg);
    virtual L3Address escolheDestino();
    virtual std::vector<L3Address> getAllAddresses();
    virtual std::vector<L3Address> parseDestAddressesPar();
    virtual void reagendar(short int msgKind, void *comunicado);
    virtual void reagendarComTime(simtime_t d, short int msgKind, void *comunicado);

};

} /* namespace inet */

#endif /* APPCLIENT_H_ */
