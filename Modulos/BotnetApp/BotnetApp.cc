//
//Base:
//      TCPBasicClientApp
//
//

#include "inet/applications/simbo/Modulos/BotnetApp/BotnetApp.h"
#include "inet/applications/simbo/Classes/Botnet/Botnet.h"
#include "inet/applications/simbo/Classes/Botnet/BotnetInterface.h"

namespace inet {


Define_Module(BotnetApp);   //Faz o link da classe com o módulo simples.


void BotnetApp::initialize(int stage)
{
    // Método herdado do módulo simples
    // É utilizado para iniciar os módulos simples antes de começar a simulação.
    // E contém as configurações iniciais.
    try{
        //Define os parametros iniciais do módulo, geralmente definidos no arquivo .ini
        if(par("localPort").longValue() == -1){ par("localPort").setLongValue(10022);}          //Se n�o houver porta definida para o bot define a porta 10022.
        if(par("infectado").boolValue()){estado = OP_INFECTADO;}else{estado = OP_SAUDAVEL;}     //Define se este bot est� infectado.
        if(par("botmaster").boolValue()){estado = OP_MASTER;}                                   //Define se este bot � o botmaster.
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

            ////////////////PARÂMETROS/////////////////////////////
            //Define os parametros iniciais do módulo, geralmente definidos no arquivo .ini

            const char *localAddress = par("localAddress"); //Define o endereço do bot (geralmente deixado no padrão)
            int localPort = par("localPort");               //Define a porta que a rede botnet vai usar.
            serverSocket.readDataTransferModePar(*this);
            serverSocket.bind(*localAddress ? L3AddressResolver().resolve(localAddress) : L3Address(), localPort);
            serverSocket.setCallbackObject(this);
            serverSocket.setOutputGate(gate("tcpOut"));
            //setStatusString("waiting");
            serverSocket.listen();
            /////////////////////////////////////////////

            timeoutMsg = new cMessage("timeoutMsg");
            nodeStatus = dynamic_cast<NodeStatus *>(findContainingNode(this)->getSubmodule("status"));

            if(strcmp(par("typeOfBot"),"EPS")){
                botnet = new Botnet(this);
                if (isNodeUp()) {
                    remarca(MSG_INICIA);
                }
            } else {
                botnet = new Botnet(this);
            }
        }
    } catch (const std::exception& ba){throw cRuntimeError("BotnetApp::initialize ,Erro:%s",ba.what());}
}

void BotnetApp::handleTimer(cMessage *msg)
{
    //Definições de resoluções para as mensagens internas ou timers internos.
    //Define e executa o ciclo clássico de operação do bot.

 //TODO aqui
    int timeAF;
    try{
    switch (msg->getKind()){

        case MSG_INICIA:
            //Pré-ciclo
            try{
                resolveMyIp();
                botnet->inicia(); // nothing done yet
                inicia();
                break;
            } catch (const std::exception& ba){throw cRuntimeError("BotnetApp::handleTimer::MSG_INICIA,Erro:%s",ba.what());}

        case MSG_TOPOLOGIA:
            //Inicia prospecção da topologia.
            try{
                botnet->prospectaTopologia();
                remarca(MSG_VERIFICA_TOPOLOGIA, 3);
                break;
            }catch (const std::exception& ba){throw cRuntimeError("BotnetApp::handleTimer::MSG_TOPOLOGIA,Erro:%s",ba.what());}

        case MSG_VERIFICA_TOPOLOGIA:
            //Verifica se a prospecção da topologia já terminou.
            try{
                if (botnet->verificaProspectaTopologia()){remarca(MSG_TERMINO_TOPOLOGIA);}else{remarca(MSG_VERIFICA_TOPOLOGIA, 3);}
                break;
            }catch (const std::exception& ba){throw cRuntimeError("BotnetApp::handleTimer::MSG_VERIFICA_TOPOLOGIA,Erro:%s",ba.what());}

        case MSG_TERMINO_TOPOLOGIA:
            //Pós-prospecção. Fim da prospecção da topologia.
            try{
                 botnet->terminoProspectaTopologia();
                 remarca(MSG_SUPERFICIE);
                break;
            }catch (const std::exception& ba){throw cRuntimeError("BotnetApp::handleTimer::MSG_TERMINO_TOPOLOGIA,Erro:%s",ba.what());}

        case MSG_SUPERFICIE:
            //Inicia varredura da superfície de ataque.
            try{
                botnet->prospectaSuperficies();
                remarca(MSG_VERIFICA_SUPERFICIE, 3);
                break;
            }catch (const std::exception& ba){throw cRuntimeError("BotnetApp::handleTimer::MSG_SUPERFICIE,Erro:%s",ba.what());}

        case MSG_VERIFICA_SUPERFICIE:
            //Verifica se a varredura da superfície já terminou.
            try{
                if (botnet->verificaProspectaSuperficie()){remarca(MSG_TERMINO_SUPERFICIE);}else{remarca(MSG_VERIFICA_SUPERFICIE, 3);}
                break;
            }catch (const std::exception& ba){throw cRuntimeError("BotnetApp::handleTimer::MSG_VERIFICA_SUPERFICIE,Erro:%s",ba.what());}

        case MSG_TERMINO_SUPERFICIE:
            //Pós-varredura. Fim da varredura da superfície.
            try{
                botnet->terminoProspectaSuperficie();
                remarca(MSG_INVASAO);
                break;
            }catch (const std::exception& ba){throw cRuntimeError("BotnetApp::handleTimer::MSG_TERMINO_SUPERFICIE,Erro:%s",ba.what());}

        case MSG_INVASAO:
            //Inicia ataque.
            try{
                botnet->invadeSistemas();
                remarca(MSG_VERIFICA_INVASAO);
                break;
            }catch (const std::exception& ba){throw cRuntimeError("BotnetApp::handleTimer::MSG_INVASAO,\n Erro:%s",ba.what());}

        case MSG_VERIFICA_INVASAO:
            //Verifica se o ataque já terminou.
            try{
                if (botnet->verificaInvadeSistemas()){remarca(MSG_TERMINO_INVASAO);}else{remarca(MSG_VERIFICA_INVASAO, 3);}
                break;
            }catch (const std::exception& ba){throw cRuntimeError("BotnetApp::handleTimer::MSG_VERIFICA_INVASAO,Erro:%s",ba.what());}

        case MSG_TERMINO_INVASAO:
            //Pós-ataque. Fim do ataque.
            try{
                botnet->terminoInvadeSistemas();
                remarca(MSG_FINALIZA);
                break;
            }catch (const std::exception& ba){throw cRuntimeError("BotnetApp::handleTimer::MSG_TERMINO_INVASAO,Erro:%s",ba.what());}

        case MSG_FINALIZA:
            //Pós-Ciclo.
            try{
                botnet->finaliza();
                break;
            }catch (const std::exception& ba){throw cRuntimeError("BotnetApp::handleTimer::MSG_FINALIZA,Erro:%s",ba.what());}

        case MSG_SEND_ALIVE_FEEDBACK:
            //Envia uma notificação ao botmaster para sinalizar que está vivo e conectado a botnet.
            try{
                sendAliveFeedBack();
                timeAF = botnet->isSetTimerAliveFeedback();
                if(timeAF >= 0){remarca(MSG_SEND_ALIVE_FEEDBACK,timeAF);}
                break;
            }catch (const std::exception& ba){throw cRuntimeError("BotnetApp::handleTimer::MSG_SEND_ALIVE_FEEDBACK,Erro:%s",ba.what());}

        case MSG_SENDINVADE:
            //Após a conexão ser aberta, envia um pacote para realizar a invasão.
            try{
                sendInvade(msg->getContextPointer());
                free(msg->getContextPointer());
                break;
            }catch (const std::exception& ba){throw cRuntimeError("BotnetApp::handleTimer::MSG_SENDINVADE,Erro:%s",ba.what());}

        case MSG_CLOSE:
            //Fecha conexão com o outro computador.
            try{
                if(msg->getContextPointer() == nullptr){return;}
                int connId = *((int*)msg->getContextPointer());
                auxCloseConnection(connId);
                free(msg->getContextPointer());
                break;
            }catch (const std::exception& ba){throw cRuntimeError("BotnetApp::handleTimer::MSG_CLOSE,Erro:%s",ba.what());}

        default:
            throw cRuntimeError("Mensagem Interna Invalida do tipo(kindId)=%d", msg->getKind());

        }
    }catch (std::exception& ba){
                throw cRuntimeError("######BotnetApp::handleTimer::switch,\n Erro:%s", ba.what());
            }


    //delete msg;
    cancelAndDelete(msg);
}

void BotnetApp::handleMessage(cMessage *msg)
{
    //Método padrão do módulo simples, recebe todas as mensagens e timers.
    //Resolve o protocolo TCP.

    if (msg->isSelfMessage())
        handleTimer(msg);
    else{
        try{
            //fixme: n�o me lembro do porqu� usar v�rios sockets ao inv�s de apenas um.
            tempSocket = new TCPSocket(msg);
            tempSocket->setCallbackObject(this);
            tempSocket->processMessage(msg);
            delete tempSocket;
        }catch (std::bad_alloc& ba){
            throw cRuntimeError("######Erro bad_alloc em handleMessage(BotnetAPP), Erro:%s", ba.what());
        }
    }
}

void BotnetApp::socketEstablished(int connId, void *ptr)
{
    //Resolve o protocolo de conexão estabelecida do TCP.
    //Se a conexão não foi iniciada por este nó, descarta.
    //Caso seja resolve de acordo com o estado da máquina.

    TCPAppBase::socketEstablished(connId, ptr);

    switch(estado){
        case OP_SAUDAVEL:
            break;
        case OP_MASTER:
        case OP_INFECTADO:
             if(botnet->topologia.hasConnId(connId) ){
                 //fixme: utilizar o sendInvade direto aqui invés de utilizar msg.
                 //remarca(MSG_SENDINVADE,0,connId);
                 //remarca(MSG_CLOSE,10,connId);
                 this->sendInvade(&connId);
             }
            break;
        default:
             printf("DEFAULT\n");
    }

}

void BotnetApp::socketDataArrived(int connId, void *ptr, cPacket *msg, bool urgent)
{
    //Resolve o protocolo TCP quando dados chegam.
    //Verifica o tipo de dado que chegou e resolve.

    BotnetAppMsg *appmsg = dynamic_cast<BotnetAppMsg *>(msg);
        if(appmsg){

            if(appmsg->getAcao() == TEM_INVASION){
                botnet->infectaApp(appmsg->getVulnerabilidade(), msg->getContextPointer());
            }

            if(appmsg->getAcao() == TEM_COMANDO){
                botnet->recebeComando(msg->getContextPointer());
            }

            if(appmsg->getAcao() == TEM_VIVO){
                //precisa da conexão aberta para continuar recebendo keepalives
                botnet->aliveFeedback(connId);
            }else{
                auxCloseConnection(connId);
            }

            /*
            else if (socket.getState() != TCPSocket::LOCALLY_CLOSED) {
                EV_INFO << "reply to last request arrived, closing session\n";
                close();
            }
            */

            TCPAppBase::socketDataArrived(connId, ptr, msg, urgent);
        }
}


void BotnetApp::sendInvade(void *p)
{
    //M�todo que abre conex�o e envia pacotes TCP para a invas�o.

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
        throw cRuntimeError("Aviso!!! ######Erro sendRequest, Erro:%s", ba.what());
    }

    EV_INFO << "sending request with " << requestLength << " bytes, expected reply length " << replyLength << " bytes,";

    sendPacket(msg,connId);
}

void BotnetApp::sendPacket(cPacket *msg, int connId){
    //Usado em sendInvade().
    //M�todo que envia um pacote se a conex�o com aquele computador estiver aberta.


    int numBytes = msg->getByteLength();
    emit(sentPkSignal, msg);

    TCPSocket *socket = botnet->topologia.getSocketPtr(connId);
    if(socket == nullptr){throw cRuntimeError("Porta nao definida em BotnetApp::sendPackted");}

    socket->send(msg);

    packetsSent++;
    bytesSent += numBytes;
}


///////////M�TODOS AUXILIARES////////////////////////////////////////////////////
// TODO: aqui

void BotnetApp::sendAliveFeedBack(){
    //Envia um pacote TCP para o botmaster ou CC avisando que este n� entrou ou continua conectado a botnet.

    BotnetAppMsg *msg;
    int numBytes;
    try{
        msg = new BotnetAppMsg("BotAlive");
        numBytes = 10;
        msg->setAcao(TEM_VIVO);
        msg->setVulnerabilidade(2);
        msg->setByteLength(numBytes);
    }catch (std::bad_alloc& ba){
        throw cRuntimeError("######Erro sendAliveFeedBack, Erro:%s", ba.what());
    }
    emit(sentPkSignal, msg);

    if( masterSocket.getRemoteAddress().isUnspecified() ){throw cRuntimeError("Conexao com botmaster nao definida em sendAliveFeedBack");}
    masterSocket.send(msg);

    packetsSent++;
    bytesSent += numBytes;
}

void BotnetApp::remarca(TiposMsg f){
    //Define um novo timer.
    cMessage *timer = new cMessage("remarca1");
    simtime_t d = simTime() + (simtime_t)par("thinkTime");
    timer->setKind(f);
    scheduleAt(d, timer);
}

void BotnetApp::remarca(TiposMsg f, simtime_t g){
    //Define um novo timer.
    cMessage *timer = new cMessage("remarca2");
    simtime_t d = simTime() + (simtime_t)par("thinkTime");
    timer->setKind(f);
    scheduleAt(d+g, timer);
}

void BotnetApp::remarca(TiposMsg f, simtime_t g, void *p){
    //Define um novo timer.
    cMessage *timer = new cMessage("remarca3");
    simtime_t d = simTime() + (simtime_t)par("thinkTime");
    timer->setContextPointer(p);
    timer->setKind(f);
    scheduleAt(d+g, timer);
}

void BotnetApp::inicia(){
    //Primeiro método a ser chamado no ciclo clássico de operação do bot.

    if(isInfected()){
        mudaIconeBotnet();
        if (estado != OP_MASTER){
            int timeAF = botnet->isSetTimerAliveFeedback();
            remarca(MSG_TOPOLOGIA);
            if(timeAF > 0){remarca(MSG_SEND_ALIVE_FEEDBACK);}
        }
        else {
            remarca(MSG_TOPOLOGIA);
        }
        return;
    }

}

bool BotnetApp::isInfected(){
    //M�todo que retorna se o computador est� infectado ou n�o.

    switch(estado){
        case OP_SAUDAVEL: return false;
        case OP_INFECTADO: return true;
        case OP_MASTER: return true;
        case OP_COMANDER: return true;
    }
    return false;
}

void BotnetApp::mudaIconeBotnet(){
    //Muda o �cone do computador na tela gr�fica para o �cone de computador infectado.

    this->getDisplayString().setTagArg("i",1,"red");
    this->getParentModule()->getDisplayString().setTagArg("i",1,"red");
}

void BotnetApp::resolveMyIp(){
    //Retorna o ip do computador para esta classe.

    InterfaceTable* interfacetab =(InterfaceTable*) getParentModule()->getSubmodule("interfaceTable");
    InterfaceEntry *interface =  interfacetab->getInterface(1);
    L3Address temp1 = L3Address(interface->ipv4Data()->getIPAddress());
    myip = temp1;
}

bool BotnetApp::auxCloseConnection(int connId){
    //Fecha a conexão com o outro computador.
    TCPSocket* tmp = botnet->topologia.getSocketPtr(connId);
    printf("address-: %d\n", tmp);
    if(tmp){
        printf("blabla: %d\n", tmp->getConnectionId());
        if(tmp->getState() == TCPSocket::CONNECTING || tmp->getState() == TCPSocket::CONNECTED || tmp->getState() == TCPSocket::LISTENING ){
            tmp->close();
            botnet->topologia.apagarConnId(connId);
            return true;
        }
    }
    return false;
}
///////////FIM M�TODOS AUXILIARES/////////////////////////////////////////////


///////////M�TODOS EST�TICOS////////////////////////////////////////////////////
//M�todos para serem utilizados por outras fun��es que precisam executar alguma a��o em nome deste m�dulo
//ou recuperar alguma informa��o deste m�dulo.

void BotnetApp::EnterMethodSilentBotnetApp(){
    //Indica para a interface gráfica que este módulo estará fazendo o processamento.

    Enter_Method_Silent();
}

///////FUN��ES GLOBAIS/ Classe Est�ticas///////////

//Fixme mudar o nome da funcao
void BotnetApp::Global1(int vulnerabilidade, void *ip){
    Enter_Method_Silent();
    botnet->infectaApp(vulnerabilidade,ip);
}
///////////FIM M�TODOS EST�TICOS/////////////////////////////////////////////////


/////////////////////////////M�TODOS DE BAIXA RELEV�NCIA///////////////////////
BotnetApp::~BotnetApp()
{
    delete botnet;
    delete nodeStatus;
}

void BotnetApp::finish(){
    cancelAndDelete(timeoutMsg);
}

bool BotnetApp::isNodeUp()
{
    return !nodeStatus || nodeStatus->getState() == NodeStatus::UP;
}

void BotnetApp::socketClosed(int connId, void *ptr)
{
    //Resolu��o do protocolo TCP quando a conex�o � fechada.
    TCPAppBase::socketClosed(connId, ptr);
}

void BotnetApp::socketFailure(int connId, void *ptr, int code)
{
    //Resolu��o do protocolo TCP quando a conex�o falha.
    TCPAppBase::socketFailure(connId, ptr, code);
}

bool BotnetApp::handleOperationStage(LifecycleOperation *operation, int stage, IDoneCallback *doneCallback)
{
    //M�todo de startup e shutdown do n�. N�o utilizado porqu� os exemplos s�o de rede est�tica.
    // M�todo legado.
    Enter_Method_Silent();
    if (dynamic_cast<NodeStartOperation *>(operation)) {
        if ((NodeStartOperation::Stage)stage == NodeStartOperation::STAGE_APPLICATION_LAYER) {
            simtime_t now = simTime();
            simtime_t start = std::max(startTime, now);
            if (timeoutMsg && ((stopTime < SIMTIME_ZERO) || (start < stopTime) || (start == stopTime && startTime == stopTime))) {
                //timeoutMsg->setKind(MSGKIND_CONNECT);
                //scheduleAt(start, timeoutMsg);
            }
        }
    }
    else if (dynamic_cast<NodeShutdownOperation *>(operation)) {
        if ((NodeShutdownOperation::Stage)stage == NodeShutdownOperation::STAGE_APPLICATION_LAYER) {
            cancelAndDelete(timeoutMsg);
            if (socket.getState() == TCPSocket::CONNECTED || socket.getState() == TCPSocket::CONNECTING || socket.getState() == TCPSocket::PEER_CLOSED)
                close();
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

/////////////////////////////FIM M�TODOS DE BAIXA RELEV�NCIA/////////////////

} // namespace inet
