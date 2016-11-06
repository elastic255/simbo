//
//Base:
//      TCPBasicClientApp
//
//

#include "inet/applications/felipe/Modulos/BotnetApp/BotnetApp.h"
#include "inet/applications/felipe/Classes/Botnet/Botnet.h"
#include "inet/applications/felipe/Classes/Botnet/BotnetInterface.h"

namespace inet {


Define_Module(BotnetApp);


void BotnetApp::initialize(int stage)
{

    botnet = new Botnet(this);


    if(par("localPort").longValue() == -1){ par("localPort").setLongValue(10022);}
    if(par("infectado").boolValue()){estado = OP_INFECTADO;}else{estado = OP_SAUDAVEL;}
    if(strcmp(getParentModule()->getName(),"inicial") == 0 && par("infectado").boolValue()){estado = OP_MASTER;}
    //TODO generalizar o nome do botmaster para qualquer outro além de inicial.
    TCPAppBase::initialize(stage);


    if (stage == INITSTAGE_LOCAL) {
        bytesRcvd = 0;
        WATCH(bytesRcvd);
        startTime = par("startTime");
        stopTime = par("stopTime");
        if (stopTime >= SIMTIME_ZERO && stopTime < startTime)
            throw cRuntimeError("Invalid startTime/stopTime parameters");
    }

    else if (stage == INITSTAGE_APPLICATION_LAYER) {

        ///////////////////////////////////////////// parametros
        const char *localAddress = par("localAddress");
        int localPort = par("localPort");
        serverSocket.readDataTransferModePar(*this);
        serverSocket.bind(*localAddress ? L3AddressResolver().resolve(localAddress) : L3Address(), localPort);
        serverSocket.setCallbackObject(this);
        serverSocket.setOutputGate(gate("tcpOut"));
        setStatusString("waiting");
        serverSocket.listen();
        /////////////////////////////////////////////

        timeoutMsg = new cMessage("timeoutMsg");
        nodeStatus = dynamic_cast<NodeStatus *>(findContainingNode(this)->getSubmodule("status"));
        if (isNodeUp()) {
            //timeoutMsg->setKind(MSG_INICIA);
            //scheduleAt(startTime, timeoutMsg);
            remarca(MSG_INICIA);
        }
    }
}

void BotnetApp::handleTimer(cMessage *msg)
{
    //Lista de mensagens internas ou timers internos.
 //TODO marca
    int timeAF;

    switch (msg->getKind()){
        case MSG_INICIA:
            resolveMyIp();
            botnet->inicia();
            inicia();
            break;

        case MSG_TOPOLOGIA:
            botnet->prospectaTopologia();
            remarca(MSG_VERIFICA_TOPOLOGIA, 3);
            break;

        case MSG_VERIFICA_TOPOLOGIA:
            if (botnet->verificaProspectaTopologia()){remarca(MSG_TERMINO_TOPOLOGIA);}else{remarca(MSG_VERIFICA_TOPOLOGIA, 3);}
            break;

        case MSG_TERMINO_TOPOLOGIA:
             botnet->terminoProspectaTopologia();
             remarca(MSG_SUPERFICIE);
            break;

        case MSG_SUPERFICIE:
            botnet->prospectaSuperficies();
            remarca(MSG_VERIFICA_SUPERFICIE, 3);
            break;

        case MSG_VERIFICA_SUPERFICIE:
            if (botnet->verificaProspectaSuperficie()){remarca(MSG_TERMINO_SUPERFICIE);}else{remarca(MSG_VERIFICA_SUPERFICIE, 3);}
            break;

        case MSG_TERMINO_SUPERFICIE:
            botnet->terminoProspectaSuperficie();
            remarca(MSG_INVASAO);
            break;

        case MSG_INVASAO:
            botnet->invadeSistemas();
            remarca(MSG_VERIFICA_INVASAO);
            break;

        case MSG_VERIFICA_INVASAO:
            if (botnet->verificaInvadeSistemas()){remarca(MSG_TERMINO_INVASAO);}else{remarca(MSG_VERIFICA_INVASAO, 3);}
            break;

        case MSG_TERMINO_INVASAO:
            botnet->terminoInvadeSistemas();
            remarca(MSG_FINALIZA);
            break;

        case MSG_FINALIZA:
            botnet->finaliza();
            break;

        case MSG_SEND_ALIVE_FEEDBACK:
            sendAliveFeedBack();
            timeAF = botnet->isSetTimerAliveFeedback();
            if(timeAF >= 0){remarca(MSG_SEND_ALIVE_FEEDBACK,timeAF);}
            break;

        case MSGKIND_CONNECT:
            //TODO retirar - função legada.
            //connect();
            break;

        case MSGKIND_SEND:
            sendRequest(msg->getContextPointer());
            free(msg->getContextPointer());
            break;

        default:
            throw cRuntimeError("Mensagem Interna Invalida do tipo(kindId)=%d", msg->getKind());
    }


    //delete msg;
    cancelAndDelete(msg);
}

void BotnetApp::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
        handleTimer(msg);
    else{
        try{
            tempSocket = new TCPSocket(msg);
            tempSocket->setCallbackObject(this);
            tempSocket->processMessage(msg);
            delete tempSocket;
            cancelAndDelete(msg);
        }catch (std::bad_alloc& ba){
            throw cRuntimeError("######Erro bad_alloc em handleMessage(BotnetAPP), Erro:%s", ba.what());
        }
    }
}

void BotnetApp::socketEstablished(int connId, void *ptr)
{
    //Se a conexão não foi iniciada por este nó, descarta.
    //Caso seja resolve de acordo com o estado da máquina.

    TCPAppBase::socketEstablished(connId, ptr);

    switch(estado){
        case OP_SAUDAVEL:
            break;
        case OP_MASTER:
        case OP_INFECTADO:
             if( botnet->topologia.hasConnId(connId) ){
                 int *temp1 = (int*)malloc(sizeof(int));
                 *temp1 = connId;
                 remarca(MSGKIND_SEND,0,temp1);
             }
            break;
        default:
             printf("DEFAULT\n");
    }

}

void BotnetApp::socketDataArrived(int connId, void *ptr, cPacket *msg, bool urgent)
{

    BotnetAppMsg *appmsg = dynamic_cast<BotnetAppMsg *>(msg);
        if(appmsg){

            if(appmsg->getAcao() == TEM_INVASION){botnet->infectaApp(appmsg->getVulnerabilidade(), msg->getContextPointer());}

            if(appmsg->getAcao() == TEM_VIVO){botnet->aliveFeedback(connId);}

            if(appmsg->getAcao() == TEM_COMANDO){botnet->recebeComando(msg->getContextPointer());}

            /*
            else if (socket.getState() != TCPSocket::LOCALLY_CLOSED) {
                EV_INFO << "reply to last request arrived, closing session\n";
                close();
            }
            */
            TCPAppBase::socketDataArrived(connId, ptr, msg, urgent);
        }
}


void BotnetApp::sendRequest(void *p)
{
    if(p == nullptr){return;}
    int connId = *((int*)p);

    long requestLength = par("requestLength");
    long replyLength = par("replyLength");
    if (requestLength < 1)
        requestLength = 1;
    if (replyLength < 1)
        replyLength = 1;

    BotnetAppMsg *msg;
    try{
        msg = new BotnetAppMsg("inv");
        msg->setAcao(TEM_INVASION);
        msg->setVulnerabilidade(100);
        msg->setByteLength(200);
        msg->setContextPointer(&myip);
    }catch (std::bad_alloc& ba){
        throw cRuntimeError("######Erro bad_alloc em 2, Erro:%s", ba.what());
    }

    EV_INFO << "sending request with " << requestLength << " bytes, expected reply length " << replyLength << " bytes,";

    sendPacket(msg,connId);
}

void BotnetApp::sendPacket(cPacket *msg, int connId){
    int numBytes = msg->getByteLength();
    emit(sentPkSignal, msg);

    TCPSocket *socket = botnet->topologia.getSocketPtr(connId);
    if(socket == nullptr){throw cRuntimeError("Porta nao definida em BotnetApp::sendPackted");}

    socket->send(msg);

    packetsSent++;
    bytesSent += numBytes;
}


///////////MINHAS FUNÇÕES////////////////////////////////////////////////////
// TODO: aqui

void BotnetApp::sendAliveFeedBack(){
    BotnetAppMsg *msg;
    int numBytes;
    try{
        msg = new BotnetAppMsg("BotAlive");
        numBytes = 10;
        msg->setAcao(TEM_VIVO);
        msg->setVulnerabilidade(2);
        msg->setByteLength(numBytes);
    }catch (std::bad_alloc& ba){
        throw cRuntimeError("######Erro bad_alloc em 3, Erro:%s", ba.what());
    }
    emit(sentPkSignal, msg);

    if( masterSocket.getRemoteAddress().isUnspecified() ){throw cRuntimeError("Conexao nao definida em sendAliveFeedBack");}
    masterSocket.send(msg);

    packetsSent++;
    bytesSent += numBytes;
}

void BotnetApp::remarca(TiposMsg f){
        cMessage *timer = new cMessage("remarca1");
        simtime_t d = simTime() + (simtime_t)par("thinkTime");
        timer->setKind(f);
        scheduleAt(d, timer);
}

void BotnetApp::remarca(TiposMsg f, simtime_t g){
        cMessage *timer = new cMessage("remarca2");
        simtime_t d = simTime() + (simtime_t)par("thinkTime");
        timer->setKind(f);
        scheduleAt(d+g, timer);
}

void BotnetApp::remarca(TiposMsg f, simtime_t g, void *p){
        cMessage *timer = new cMessage("remarca3");
        simtime_t d = simTime() + (simtime_t)par("thinkTime");
        timer->setContextPointer(p);
        timer->setKind(f);
        scheduleAt(d+g, timer);
}

void BotnetApp::inicia(){
    if(isInfected()){
        mudaIconeBotnet();
        if(estado != OP_MASTER){
            int timeAF = botnet->isSetTimerAliveFeedback();
            remarca(MSG_TOPOLOGIA);
            if(timeAF > 0){remarca(MSG_SEND_ALIVE_FEEDBACK);}
        }else{
            remarca(MSG_TOPOLOGIA);
        }
        return;}

    if(estado == OP_SAUDAVEL){return;}

}

bool BotnetApp::isInfected(){
    switch(estado){
        case OP_SAUDAVEL: return false;
        case OP_INFECTADO: return true;
        case OP_MASTER: return true;
        case OP_COMANDER: return true;
    }
    return false;
}

void BotnetApp::mudaIconeBotnet(){
    this->getDisplayString().setTagArg("i",1,"red");
    this->getParentModule()->getDisplayString().setTagArg("i",1,"red");
}

void BotnetApp::resolveMyIp(){
    InterfaceTable* interfacetab =(InterfaceTable*) getParentModule()->getSubmodule("interfaceTable");
    InterfaceEntry *interface =  interfacetab->getInterface(1);
    L3Address temp1 = L3Address(interface->ipv4Data()->getIPAddress());
    myip = temp1;
}

/////////////////////////////////////////////////////////////////////////////

BotnetApp::~BotnetApp()
{
    cancelAndDelete(timeoutMsg);
}

void BotnetApp::finish(){
    cancelAndDelete(timeoutMsg);
//delete botnet;
}

bool BotnetApp::isNodeUp()
{
    return !nodeStatus || nodeStatus->getState() == NodeStatus::UP;
}

void BotnetApp::socketClosed(int connId, void *ptr)
{
    TCPAppBase::socketClosed(connId, ptr);
}

void BotnetApp::socketFailure(int connId, void *ptr, int code)
{
    TCPAppBase::socketFailure(connId, ptr, code);
    // reconnect after a delay
    /*

    if (timeoutMsg) {
        simtime_t d = simTime() + (simtime_t)par("reconnectInterval");
        rescheduleOrDeleteTimer(d, MSGKIND_CONNECT);
    }
    */
}

void BotnetApp::rescheduleOrDeleteTimer(simtime_t d, short int msgKind)
{
    //// Função legada.
    //Mantida por causa da função handleOperationStage.
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

bool BotnetApp::handleOperationStage(LifecycleOperation *operation, int stage, IDoneCallback *doneCallback)
{
    //Função de startup e shutdown do nó. Não utilizado porquê os exemplos são de rede estática.
    // Função legada.
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

} // namespace inet
