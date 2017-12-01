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

#include "inet/applications/simbo/Classes/Botnet/Botnet.h"


namespace inet {

/*
A classe Botnet visa definir o comportamento e decis�es tomadas pelo botnet.
Seguindo o contrato de interface BotnetInterface, pode-se desenvolver diferentes varia��es da classe Botnet
permitindo criar v�rias botnets diferentes numa mesma rede, com m�nima altera��o no BotnetApp (M�dulo Simples).
*/

Botnet::Botnet(BotnetApp *oi) {

    obj = oi;           //Cria uma refer�ncia para a classe botnet de seu BotnetApp (M�dulo Simples)
    maxNumDropOut = 30; //fixme Em desenvolvimento. Tempo m�ximo para um bot ser considerado desconectado da botnet pelos CC ou botmaster.
}

Botnet::~Botnet() {
    topologia.apagarTudo(); //Apaga toda a estrutura armazenada em topologia.
}

/*
////////CICLO BOTNET//////////////
Ciclo que toda botnet clássica (que segue uma ordem pré-estabelecida de ações) irá executar pelo contrato BotnetInterface e BotnetApp.
*/

////CICLO DE PROSPECÇÂO
void Botnet::prospectaTopologia(){
    //Inicia um ping em todas as máquinas da rede.

    BotnetPing* ping =(BotnetPing*) obj->getParentModule()->getSubmodule("pingApp",0);
    ping->startSendingPingRequests();
    obj->EnterMethodSilentBotnetApp();
}

bool Botnet::verificaProspectaTopologia(){
    //Verifica se o ping j� terminou.

    BotnetPing* ping =(BotnetPing*) obj->getParentModule()->getSubmodule("pingApp",0);
    return ping->pingFinalizado();
}

bool Botnet::terminoProspectaTopologia(){
    // Adiciona as maquinas que responderam ao ping (alive) na estrutura de topologia (que representa o conhecimento
    // do bot sobre a rede).

    BotnetPing* ping =(BotnetPing*) obj->getParentModule()->getSubmodule("pingApp",0);
    std::vector<L3Address> enderecos = ping->carregaEnderecos();
    if (enderecos.empty())
        return false;
    topologia.addIp(enderecos);
    return true;
}

////CICLO DE ANÁLISE DE SUPERFÍCIE
void Botnet::prospectaSuperficies(){
    //Procura por vulnerabilidades na topologia conhecida.

    //topologia.addVulnerabilidadeAll(100); fixme: vulnerabilidades n�o implementadas.
}

////CICLO DE INVASÃO
void Botnet::invadeSistemas(){
    // Abre a conexão e envia pacotes para invadir uma máquina na rede.

    try {
        L3Address meuip = myip();

        if(topologia.topo.empty()){return;}

        for (SubBotIterator it2 = topologia.topo.begin(); it2 != topologia.topo.end(); ++it2) {
            if (!it2->ip.str().compare(meuip.str()))
                continue;
            TCPSocket socket;
            socket.readDataTransferModePar(*obj);
            socket.setCallbackObject(obj);
            socket.setOutputGate(obj->gate("tcpOut"));
            printf("%s\n", it2->ip.str().c_str());
            socket.connect(it2->ip, 10022);
            it2->socket = socket;
            it2->connId = socket.getConnectionId();
        }

 /*       it=topologia.topo.begin();
        int teste = topologia.topo.size();
        for(int i = 0; i < teste-1; i++) {
            if((*it).ip.str().compare(meuip.str()) == 0 ){ it++; continue; }
            TCPSocket socket;
            socket.readDataTransferModePar(*obj);
            socket.setCallbackObject(obj);
            socket.setOutputGate(obj->gate("tcpOut"));
            printf("%s\n",it->ip.str().c_str());
            socket.connect((*it).ip,10022);
            (*it).socket = socket;
            (*it).connId = socket.getConnectionId();
            it++;
        }*/

        //fixme Descobrir porquê o codigo acima funciona e o debaixo não. Descobrir se {(*it).socket = socket;} É uma operação válida.
        /*
    char out[100];
    int tamanho = topologia.topo.size();
    for(it=topologia.topo.begin(); it != topologia.topo.end(); it++){
        if(  (*it).ip.str().compare(meuip.str()) == 0 ){ continue; }
        printf("%s\n",(*it).ip.str().c_str());
        sprintf(out,"%s\n",(*it).ip.str().c_str());
        TCPSocket socket;
        socket.readDataTransferModePar(*obj);
        socket.setCallbackObject(obj);
        socket.setOutputGate(obj->gate("tcpOut"));
        socket.connect((*it).ip,10022);

        (*it).socket = socket;
        (*it).connId = socket.getConnectionId();

    }
         */


    }catch (const std::exception& ba){throw cRuntimeError("Botnet::invadeSistemas,Erro:%s",ba.what());}

}

bool Botnet::aliveFeedback(int connId){
    //Envia um pacote para o botmaster ou centro de comando, avisando que esse computador est� infectado e pertence a botnet.

    try{
        CellTopo oi; //tomar cuidado aqui.
        if(subBotnet.empty()){
            if(topologia.getEstruturaConnId(connId,&oi)){
                subBotnet.push_back(oi);
                return true;
            }
        }

        try{
            /*
            for(it=subBotnet.begin();it <= subBotnet.end(); it++){
                (*it).NumDropOut++;
                if(connId == (*it).connId){
                    (*it).NumDropOut = 0;
                    return true;
                }
            }

            for(it=subBotnet.begin();it <= subBotnet.end(); it++){
                if((*it).NumDropOut > maxNumDropOut){
                    subBotnet.erase(it);
                    break;
                    //Fixme melhorar a gambiarra acima.
                }
            }
             */
        }catch (const std::exception& ba){throw cRuntimeError("Botnet::aliveFeedback::Voce errou o tamanho da fila,Erro:%s",ba.what());}

        if(topologia.getEstruturaConnId(connId,&oi)){
            //subBotnet.push_back(oi);
        }
        return true;
    }catch (const std::exception& ba){throw cRuntimeError("Botnet::aliveFeedback,Erro:%s",ba.what());}
}


bool Botnet::recebeComando(void *){return false;}
//////////FIM CICLO BOTNET///////////////

/*
//////////M�TODOS UNIT�RIOS//////////////
M�todos que podem ser chamados pelo usu�rio em tempo de execu��o (prompt ou shell) que se referem a
um �nico bot.
*/
void Botnet::invadeUmSistema(L3Address ip){
    //Manda esse bot invadir um pc.

    int id;
    TCPSocket socket = setSocket(ip);
    socket.connect(ip,10022);
    printf("add2: %s", ip.str().c_str());
    id = topologia.addIp(ip);
    CellTopo* temp = topologia.getTopoById(id);
    (*temp).socket = socket;
    (*temp).connId = socket.getConnectionId();
}

void Botnet::redefinirCC(L3Address ip){
    //manda esse bot trocar de centro de comando ou botmaster.

    obj->masterSocket = setSocket(ip);
}
//////////FIM M�TODOS UNIT�RIOS/////////////


//////////SINAIS EXTERNOS//////////////////
bool Botnet::infectaApp(int vulnerabilidade, void *ip){
    //Ao receber um pacote de invas�o, determina se esse computador foi invadido com sucesso ou n�o.

    if(obj->estado == BotnetApp::OP_SAUDAVEL){
        obj->getDisplayString().setTagArg("i",0,"block/dispatch");
        obj->getDisplayString().setTagArg("i",1,"red");
        obj->setEstado(BotnetApp::OP_INFECTADO);
        obj->masterSocket = setSocket( *((L3Address *)ip) );
        obj->inicia();
        return true;
    }else{
        return false;
    }
}
//////////FIM SINAIS EXTERNOS///////////////


//////////AUXILIARES////////////////////////
L3Address Botnet::myip(){
    //Retorna o IP do computador ou bot.

    InterfaceTable* interfacetab =(InterfaceTable*) obj->getParentModule()->getSubmodule("interfaceTable");
    InterfaceEntry *interface =  interfacetab->getInterface(1);
    L3Address temp1 = L3Address(interface->ipv4Data()->getIPAddress());
    return temp1;
}

TCPSocket Botnet::setSocket(L3Address ip){
    //Auxilia na configura��o do socket para iniciar a conex�o.

    TCPSocket socket;
    socket.readDataTransferModePar(*obj);
    socket.setCallbackObject(obj);
    socket.setOutputGate(obj->gate("tcpOut"));
    socket.connect(ip,10022);
    return socket;
}
//////////FIM AUXILIARES////////////////////////

} /* namespace inet */
