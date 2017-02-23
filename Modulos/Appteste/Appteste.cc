//
//Base:
//      TCPBasicClientApp
//
//

#include "Appteste.h"

#include "inet/common/lifecycle/NodeOperations.h"
#include "inet/common//RawPacket.h"
#include "inet/common/ModuleAccess.h"
#include "inet/applications/tcpapp/GenericAppMsg_m.h"



namespace inet {

#define MSGKIND_CONNECT    0
#define MSGKIND_SEND       1

Define_Module(Appteste);

Appteste::~Appteste()
{
    cancelAndDelete(timeoutMsg);
}

void Appteste::initialize(int stage)
{
    TCPAppBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        numRequestsToSend = 0;
        earlySend = false;    // TBD make it parameter
        WATCH(numRequestsToSend);
        WATCH(earlySend);

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

bool Appteste::isNodeUp()
{
    return !nodeStatus || nodeStatus->getState() == NodeStatus::UP;
}

bool Appteste::handleOperationStage(LifecycleOperation *operation, int stage, IDoneCallback *doneCallback)
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
            cancelEvent(timeoutMsg);
            if (socket.getState() == TCPSocket::CONNECTED || socket.getState() == TCPSocket::CONNECTING || socket.getState() == TCPSocket::PEER_CLOSED)
                close();
            // TODO: wait until socket is closed
        }
    }
    else if (dynamic_cast<NodeCrashOperation *>(operation)) {
        if ((NodeCrashOperation::Stage)stage == NodeCrashOperation::STAGE_CRASH)
            cancelEvent(timeoutMsg);
    }
    else
        throw cRuntimeError("Unsupported lifecycle operation '%s'", operation->getClassName());
    return true;
}

void Appteste::sendRequest()
{
    long requestLength = par("requestLength");
    long replyLength = par("replyLength");
    if (requestLength < 1)
        requestLength = 1;
    if (replyLength < 1)
        replyLength = 1;

    // TODO: aqui


/*
    GenericAppMsg *msg = new GenericAppMsg("data");
    //msg->setByteLength(requestLength);
    msg->setByteLength(999);
    //msg->setExpectedReplyLength(replyLength);
    msg->setExpectedReplyLength(700);
    msg->setServerClose(false);
   // TCPSegment_Base *ff = new TCPSegment_Base("oi",0);
    cMessage *mm4 = new cMessage("oi2");
   // cPacket* mm2 = new cPacket("oi");
    //RawPacket *mm = check_and_cast<RawPacket *)(mm2);
    //RawPacket *mm = check_and_cast<RawPacket *>(mm2);
*/
    //cPacket* pac = new cPacket("oii");


// Deu certo !!!!
    RawPacket *mm = new RawPacket("oi3");
    char a[11] = "0123456789";
    mm->setDataFromBuffer(a,11);
    mm->setByteLength(11);

    cPacket* pac = new cPacket("oi");
    mm->encapsulate (pac);

   // cPacket* pac = check_and_cast<cPacket *>(mm);
   // pac->setByteLength(11);
////////////////////////////////////////////////////////

    EV_INFO << "sending request with " << requestLength << " bytes, expected reply length " << replyLength << " bytes,"
            << "remaining " << numRequestsToSend - 1 << " request\n";

    sendPacket(mm);
}

void Appteste::handleTimer(cMessage *msg)
{
    switch (msg->getKind()) {
        // Recomeça ou perda de conexão
        case MSGKIND_CONNECT:
            connect();    // active OPEN

            // significance of earlySend: if true, data will be sent already
            // in the ACK of SYN, otherwise only in a separate packet (but still
            // immediately)
            if (earlySend)
                sendRequest();

            break;

        // Respostas repetidas do tipo  numRequestsToSend são enviadas aqui.
            // no scheduleAt(): next request will be sent when reply to this one
            // arrives (see socketDataArrived())
        case MSGKIND_SEND:
            sendRequest();
            numRequestsToSend--;

            break;

        default:
            throw cRuntimeError("Invalid timer msg: kind=%d", msg->getKind());
    }
}

void Appteste::socketEstablished(int connId, void *ptr)
{
    TCPAppBase::socketEstablished(connId, ptr);

    // determine number of requests in this session
    numRequestsToSend = par("numRequestsPerSession").longValue();
    if (numRequestsToSend < 1)
        numRequestsToSend = 1;

    // perform first request if not already done (next one will be sent when reply arrives)
    // AQUI !!!!! é enviada da primeira requisição
    if (!earlySend){
        sendRequest();
    }
    numRequestsToSend--;



    ////////////////////////////////
    if (numRequestsToSend < 1 && socket.getState() != TCPSocket::LOCALLY_CLOSED){
        close();}
//////////////////////////////////////


}

void Appteste::rescheduleOrDeleteTimer(simtime_t d, short int msgKind)
{
    cancelEvent(timeoutMsg);

    if (stopTime < SIMTIME_ZERO || d < stopTime) {
        timeoutMsg->setKind(msgKind);
        scheduleAt(d, timeoutMsg);
    }
    else {
        delete timeoutMsg;
        timeoutMsg = nullptr;
    }
}

void Appteste::socketDataArrived(int connId, void *ptr, cPacket *msg, bool urgent)
{
    TCPAppBase::socketDataArrived(connId, ptr, msg, urgent);

    if (numRequestsToSend > 0) {
        EV_INFO << "reply arrived\n";

        if (timeoutMsg) {
            simtime_t d = simTime() + (simtime_t)par("thinkTime");
            rescheduleOrDeleteTimer(d, MSGKIND_SEND);
        }
    }
    else if (socket.getState() != TCPSocket::LOCALLY_CLOSED) {
        EV_INFO << "reply to last request arrived, closing session\n";
        close();
    }
}

void Appteste::socketClosed(int connId, void *ptr)
{
    TCPAppBase::socketClosed(connId, ptr);

    // start another session after a delay
    if (timeoutMsg) {
        simtime_t d = simTime() + (simtime_t)par("idleInterval");
        // ###########################   rescheduleOrDeleteTimer(d, MSGKIND_CONNECT);
    }
}

void Appteste::socketFailure(int connId, void *ptr, int code)
{
    TCPAppBase::socketFailure(connId, ptr, code);

    // reconnect after a delay
    if (timeoutMsg) {
        simtime_t d = simTime() + (simtime_t)par("reconnectInterval");
        rescheduleOrDeleteTimer(d, MSGKIND_CONNECT);
    }
}

} // namespace inet

