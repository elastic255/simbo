//
// Copyright (C) 2004 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//



#ifndef __INET_BotnetApp_H
#define __INET_BotnetApp_H

#include "inet/common/INETDefs.h"

#include "inet/applications/tcpapp/TCPAppBase.h"
#include "inet/common/lifecycle/NodeStatus.h"
#include "inet/common/lifecycle/ILifecycle.h"
#include "inet/transportlayer/tcp_common/TCPSegment.h"

#include "inet/common/lifecycle/NodeOperations.h"
#include "inet/common//RawPacket.h"
#include "inet/common/ModuleAccess.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "inet/applications/simbo/Modulos/BotnetApp/BotnetAppMsg_m.h"

#include <cstring>


namespace inet {


//class Botnet; //Não utilizada neste arquivo, substituido por BotnetInterface.
class BotnetInterface;

/**
 * An example request-reply based client application.
 */
class INET_API BotnetApp : public TCPAppBase, public ILifecycle
{

  public:
    enum TiposMsg: int {MSG_INICIA,
                        MSGKIND_CONNECT,
                        MSGKIND_SEND,
                        MSG_FINALIZA,

                        MSG_TOPOLOGIA,
                        MSG_VERIFICA_TOPOLOGIA,
                        MSG_TERMINO_TOPOLOGIA,
                        MSG_SUPERFICIE,
                        MSG_VERIFICA_SUPERFICIE,
                        MSG_TERMINO_SUPERFICIE,
                        MSG_INVASAO,
                        MSG_VERIFICA_INVASAO,
                        MSG_TERMINO_INVASAO,

                        MSG_SEND_ALIVE_FEEDBACK};

    enum Operanti: int {OP_SAUDAVEL,
                        OP_INFECTADO,
                        OP_MASTER,
                        OP_COMANDER};

    enum TipoExtMsg: int  {TEM_INVASION,
                           TEM_VIVO,
                           TEM_COMANDO};


  protected:
    cMessage *timeoutMsg = nullptr;
    NodeStatus *nodeStatus = nullptr;
    simtime_t startTime;
    simtime_t stopTime;


    virtual void sendRequest(void *);
    virtual void rescheduleOrDeleteTimer(simtime_t d, short int msgKind);

    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleTimer(cMessage *msg) override;
    virtual void socketEstablished(int connId, void *yourPtr) override;
    virtual void socketDataArrived(int connId, void *yourPtr, cPacket *msg, bool urgent) override;
    virtual void socketClosed(int connId, void *yourPtr) override;
    virtual void socketFailure(int connId, void *yourPtr, int code) override;
    virtual bool isNodeUp();
    virtual bool handleOperationStage(LifecycleOperation *operation, int stage, IDoneCallback *doneCallback) override;
    virtual void handleMessage(cMessage *) override;

    ////MINHAS FUNÇÕES
  protected:
    virtual void sendPacket(cPacket *,int);
    TCPSocket *tempSocket;  //Porta para resolver recebimento de pacotes.



  public:
    BotnetInterface *botnet; //Motor de comportamento da botnet.

    Operanti estado = OP_SAUDAVEL;
    TCPSocket serverSocket; //Porta para receber pacotes (server).
    TCPSocket masterSocket; //Porta para se comunicar com o botmaster ou CC.
    L3Address myip; //IP do nó

    virtual void inicia();
    void setEstado(Operanti a){estado = a;}
    Operanti getEstado(){return estado;}
    virtual void remarca(TiposMsg);
    virtual void remarca(TiposMsg, simtime_t);
    virtual void remarca(TiposMsg , simtime_t , void*);
    virtual bool isInfected();
    virtual void mudaIconeBotnet();
    virtual void resolveMyIp();

    virtual void sendAliveFeedBack();

    virtual void Global1(int vulnerabilidade, void *ip);

  public:
    BotnetApp(){};
    virtual void finish() override;
    virtual ~BotnetApp();

};

} // namespace inet

#endif // ifndef __INET_BotnetApp_H

