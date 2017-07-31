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

//Módulo para poder enviar mensagens para vários hosts.
// Provavelmente legado com as novas atualizações do Inet.

#include "inet/applications/simbo/Modulos/AppClient/AppClient.h"

namespace inet {


#define MSGKIND_CONNECT    0
#define MSGKIND_SEND       1

Define_Module(AppClient);

AppClient::~AppClient()
{
   //cancelAndDelete(timeoutMsg); //já é feito no estágio NodeShutdownOperation
}

void AppClient::initialize(int stage)
{
    TCPAppBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        numRequestsToSend = 0;
        //earlySend = false;    // TBD make it parameter
        WATCH(numRequestsToSend);
        //WATCH(earlySend);

        startTime = par("startTime");
        stopTime = par("stopTime");
        if (stopTime >= SIMTIME_ZERO && stopTime < startTime)
            throw cRuntimeError("Invalid startTime/stopTime parameters");
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        timeoutMsg = new cMessage("timer");
        nodeStatus = dynamic_cast<NodeStatus *>(findContainingNode(this)->getSubmodule("status"));

        if (isNodeUp()) {
            timeoutMsg->setKind(MSGKIND_CONNECT);
            scheduleAt(startTime, timeoutMsg);
        }
    }
}

bool AppClient::isNodeUp()
{
    return !nodeStatus || nodeStatus->getState() == NodeStatus::UP;
}

bool AppClient::handleOperationStage(LifecycleOperation *operation, int stage, IDoneCallback *doneCallback)
{
    Enter_Method_Silent();
    if (dynamic_cast<NodeStartOperation *>(operation)) {
        if ((NodeStartOperation::Stage)stage == NodeStartOperation::STAGE_APPLICATION_LAYER) {
            simtime_t now = simTime();
            simtime_t start = std::max(startTime, now);
            if (timeoutMsg && ((stopTime < SIMTIME_ZERO) || (start < stopTime) || (start == stopTime && startTime == stopTime))) {
                timeoutMsg->setKind(MSGKIND_CONNECT);
                scheduleAt(start, timeoutMsg);
            }
        }
    }
    else if (dynamic_cast<NodeShutdownOperation *>(operation)) {
        if ((NodeShutdownOperation::Stage)stage == NodeShutdownOperation::STAGE_APPLICATION_LAYER) {
            cancelAndDelete(timeoutMsg);
            if (socket.getState() == TCPSocket::CONNECTED || socket.getState() == TCPSocket::CONNECTING || socket.getState() == TCPSocket::PEER_CLOSED)
                close();
            // TODO: wait until socket is closed
        }
    }
    else if (dynamic_cast<NodeCrashOperation *>(operation)) {
        if ((NodeCrashOperation::Stage)stage == NodeCrashOperation::STAGE_CRASH)
            cancelAndDelete(timeoutMsg);
    }
    else
        throw cRuntimeError("Unsupported lifecycle operation '%s'", operation->getClassName());
    return true;
}

void AppClient::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()){
        handleTimer(msg);
    }else{
                //TCPSocket* tmp = belongsToSocket(msg);
                //tmp->processMessage(msg);

                tempSocket = new TCPSocket(msg);
                tempSocket->setCallbackObject(this);
                tempSocket->processMessage(msg);
                delete tempSocket;
                //cancelAndDelete(msg);
        }

}

TCPSocket* AppClient::belongsToSocket(cMessage *msg)
{
    if(dynamic_cast<TCPCommand *>(msg->getControlInfo())){
        int connid = ((TCPCommand *)(msg->getControlInfo()))->getConnId();
        return getConexao(connid);
    }else{
        throw cRuntimeError("Conexao nao encontrada em belongsToSocket");
        return nullptr;
    }
}


void AppClient::handleTimer(cMessage *msg)
{
    switch (msg->getKind()) {
        case MSGKIND_CONNECT:
            conectar();
            break;


        case MSGKIND_SEND:
            int *k;
            k = (int*)msg->getContextPointer();
            sendConexao((*k));
            free((int*)msg->getContextPointer());
            numRequestsToSend--;

            //sendRequest(connId);
            // no scheduleAt(): next request will be sent when reply to this one
            // arrives (see socketDataArrived())
            break;

        default:
            throw cRuntimeError("Invalid timer msg: kind=%d", msg->getKind());
    }
    delete msg;
    //cancelAndDelete(msg);
}

void AppClient::conectar()
{
    // connect

    //Todo: transformar em funções;
    const char *connectAddress = par("connectAddress");
    int connectPort = par("connectPort");

    L3Address destination;
    //L3AddressResolver().tryResolve(connectAddress, destination);
    destination = escolheDestino();

    if (destination.isUnspecified()) {
        EV_ERROR << "Connecting to " << connectAddress << " port=" << connectPort << ": cannot resolve destination address\n";
    }
    else {
        EV_INFO << "Connecting to " << connectAddress << "(" << destination << ") port=" << connectPort << endl;
        //setStatusString("connecting");

        TCPSocket* socketQ = new TCPSocket();
        //socketQ->renewSocket();
        socketQ->readDataTransferModePar(*this);
        socketQ->setCallbackObject(this);
        socketQ->setOutputGate(this->gate("tcpOut"));

        socketQ->connect(destination, connectPort);
        filaConexoes.push_back(socketQ);

        //numSessions++;
        emit(connectSignal, 1L);
    }
}

std::vector<L3Address> AppClient::parseDestAddressesPar()
{
    std::vector<L3Address> destAddresses;
    const char *destAddrs = par("connectAddress");
    if (!strcmp(destAddrs, "*")) {
        destAddresses = getAllAddresses();
    }
    else {
        cStringTokenizer tokenizer(destAddrs);
        const char *token;

        while ((token = tokenizer.nextToken()) != nullptr) {
            L3Address addr = L3AddressResolver().resolve(token);
            destAddresses.push_back(addr);
        }
    }
    return destAddresses;
}

std::vector<L3Address> AppClient::getAllAddresses()
{
    std::vector<L3Address> result;

#if OMNETPP_VERSION < 0x500
    int lastId = getSimulation()->getLastModuleId();
#else // if OMNETPP_VERSION < 0x500
    int lastId = getSimulation()->getLastComponentId();
#endif // if OMNETPP_VERSION < 0x500

    for (int i = 0; i <= lastId; i++)
    {
        IInterfaceTable *ift = dynamic_cast<IInterfaceTable *>(getSimulation()->getModule(i));
        if (ift) {
            for (int j = 0; j < ift->getNumInterfaces(); j++) {
                InterfaceEntry *ie = ift->getInterface(j);
                if (ie && !ie->isLoopback()) {
#ifdef WITH_IPv4
                    if (ie->ipv4Data()) {
                        IPv4Address address = ie->ipv4Data()->getIPAddress();
                        if (!address.isUnspecified())
                            result.push_back(L3Address(address));
                    }
#endif // ifdef WITH_IPv4
#ifdef WITH_IPv6
                    if (ie->ipv6Data()) {
                        for (int k = 0; k < ie->ipv6Data()->getNumAddresses(); k++) {
                            IPv6Address address = ie->ipv6Data()->getAddress(k);
                            if (!address.isUnspecified() && address.isGlobal())
                                result.push_back(L3Address(address));
                        }
                    }
#endif // ifdef WITH_IPv6
                }
            }
        }
    }
    return result;
}

L3Address AppClient::escolheDestino(){

    std::vector<L3Address> vetor = parseDestAddressesPar();
    int tamanho = vetor.size();
    std::uniform_int_distribution<int> distribution(0,tamanho-1);
    int dice = distribution(generator);
    //int dice = rand() % tamanho;
    return vetor[dice];
}

TCPSocket* AppClient::getConexao(int connId){
    for(it=filaConexoes.begin(); it <= filaConexoes.end(); it++){
        if((*it)->getConnectionId() == connId){
            return (*it);
        }
    }
    throw cRuntimeError("Conexao nao encontrada em getConexao");
    return nullptr;
}

bool AppClient::deleteConexao(int connId){
    for(it=filaConexoes.begin(); it <= filaConexoes.end(); it++){
        if((*it)->getConnectionId() == connId){
            filaConexoes.erase(it);
            return true;
        }
    }
    throw cRuntimeError("Conexao nao encontrada em deleteConexaos");
    return false;
}


void AppClient::sendConexao(int connId)
{
    long requestLength = par("requestLength");
    long replyLength = par("replyLength");
    if (requestLength < 1){requestLength = 1;}
    if (replyLength < 1){replyLength = 1;}


    GenericAppMsg *msg = new GenericAppMsg("data");
    msg->setByteLength(requestLength);
    msg->setExpectedReplyLength(replyLength);
    msg->setServerClose(false);

    EV_INFO << "sending request with " << requestLength << " bytes, expected reply length " << replyLength << " bytes,"
            << "remaining " << numRequestsToSend - 1 << " request\n";



    TCPSocket* tmp = getConexao(connId);
    /////
    int numBytes = msg->getByteLength();
    emit(sentPkSignal, msg);
    packetsSent++;
    bytesSent += numBytes;
    tmp->send(msg);
    /////
}


void AppClient::socketEstablished(int connId, void *ptr)
{

    TCPAppBase::socketEstablished(connId, ptr);

    // determine number of requests in this session
    numRequestsToSend = par("numRequestsPerSession").longValue();
    if (numRequestsToSend < 1)
        numRequestsToSend = 1;

    // perform first request if not already done (next one will be sent when reply arrives)

    int *k = (int*) malloc(sizeof(int));
    *k = connId;
    reagendar(MSGKIND_SEND,k);


    //sendConexao(connId);
    //numRequestsToSend--;

}

/*
void AppClient::rescheduleOrDeleteTimer(simtime_t d, short int msgKind)
{
    cancelAndDelete(timeoutMsg);
    //cMessage *timer = new cMessage("AppClientTimer");
    if (stopTime < SIMTIME_ZERO || d < stopTime) {
        timeoutMsg->setKind(msgKind);
        scheduleAt(d, timeoutMsg);
    }
    else {
        delete timeoutMsg;
        timeoutMsg = nullptr;
    }

}
deletar essa função depois
*/

void AppClient::reagendar(short int msgKind, void *comunicado)
{
    cMessage *timer = new cMessage("AppClientTimer");
    simtime_t d = simTime() + (simtime_t)par("thinkTime");
    timer->setContextPointer(comunicado);
    timer->setKind(msgKind);
    scheduleAt(d, timer);
}

void AppClient::reagendarComTime(simtime_t k, short int msgKind, void *comunicado)
{
    cMessage *timer = new cMessage("AppClientTimer");
    simtime_t d = simTime() + k;
    timer->setContextPointer(comunicado);
    timer->setKind(msgKind);
    scheduleAt(d, timer);
}

void AppClient::socketDataArrived(int connId, void *ptr, cPacket *msg, bool urgent)
{
    TCPAppBase::socketDataArrived(connId, ptr, msg, urgent);


    if (numRequestsToSend > 0) {
        EV_INFO << "reply arrived\n";

        int *k = (int*)malloc(sizeof(int));
        *k = connId;
        reagendar(MSGKIND_SEND,k);


        //sendConexao(connId);
        //numRequestsToSend--;
    }
    else if (socket.getState() != TCPSocket::LOCALLY_CLOSED) {
        EV_INFO << "reply to last request arrived, closing session\n";

        //setStatusString("closing");
        EV_INFO << "issuing CLOSE command\n";
        TCPSocket* tmp = getConexao(connId);
        tmp->close();
        emit(connectSignal, -1L);

    }
}

void AppClient::socketClosed(int connId, void *ptr)
{
    TCPAppBase::socketClosed(connId, ptr);

    deleteConexao(connId);
    // start another session after a delay


    simtime_t g = (simtime_t)par("idleInterval");
    int *k = (int*)malloc(sizeof(int));
    *k = connId;
    reagendarComTime(g, MSGKIND_CONNECT, k);
    //conectar();


}

void AppClient::socketFailure(int connId, void *ptr, int code)
{
    TCPAppBase::socketFailure(connId, ptr, code);

    //deleteConexao(connId);

    // reconnect after a delay
    simtime_t g = (simtime_t)par("reconnectInterval");
    int *k = (int*)malloc(sizeof(int));
    *k = connId;
    reagendarComTime(g, MSGKIND_CONNECT, k);

    //conectar();
    //timeoutMsg->setKind(MSGKIND_CONNECT);
    //scheduleAt(d, timeoutMsg);
    //rescheduleOrDeleteTimer(d, MSGKIND_CONNECT);

}

} /* namespace inet */
