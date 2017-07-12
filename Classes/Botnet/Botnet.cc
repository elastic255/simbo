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
A classe Botnet visa definir o comportamento e decisões tomadas pelo botnet.
Seguindo o contrato de interface BotnetInterface, pode-se desenvolver diferentes variações da classe Botnet
permitindo criar várias botnets diferentes numa mesma rede, com mínima alteração no BotnetApp (Módulo Simples).
*/

Botnet::Botnet(BotnetApp *oi) {

    obj = oi;           //Cria uma referência para a classe botnet de seu BotnetApp (Módulo Simples)
    maxNumDropOut = 30; //fixme Em desenvolvimento. Tempo máximo para um bot ser considerado desconectado da botnet pelos CC ou botmaster.
}

Botnet::~Botnet() {
    topologia.apagarTudo(); //Apaga toda a estrutura armazenada em topologia.
}

/*
////////CICLO BOTNET//////////////
Ciclo que toda botnet clássica (que segue uma ordem pré-estabelacida de ações) irá executar pelo contrato BotnetInterface e BotnetApp.
*/

////CICLO DE PROSPECÇÃO
void Botnet::prospectaTopologia(){
    //Inicia um ping em todas as máquinas da rede.

    BotnetPing* ping =(BotnetPing*) obj->getParentModule()->getSubmodule("pingApp",0);
    ping->startSendingPingRequests();
    obj->EnterMethodSilentBotnetApp();
}

bool Botnet::verificaProspectaTopologia(){
    //Verifica se o ping já terminou.

    BotnetPing* ping =(BotnetPing*) obj->getParentModule()->getSubmodule("pingApp",0);
    return ping->pingFinalizado();
}

bool Botnet::terminoProspectaTopologia(){
    //Fixme criar uma condicional boleana de verdade.

    //Adiciona as maquinas que responderam ao ping (alive) na estrutura de topologia (que representa o conhecimento
    //do bot sobre a rede).

    BotnetPing* ping =(BotnetPing*) obj->getParentModule()->getSubmodule("pingApp",0);
    topologia.addIp(ping->carregaEnderecos());
    return true;
}

////CICLO DE ANÁLISE DE SUPERFÍCIE
void Botnet::prospectaSuperficies(){
    //Procura por vulnerabilidades na topologia conhecida.

    //topologia.addVulnerabilidadeAll(100); fixme: vulnerabilidades não implementadas.
}

////CICLO DE INVASÃO
void Botnet::invadeSistemas(){
    //Abre a conexão e envia pacotes para invadir uma máquina na rede.

    try{
    L3Address meuip = myip();
    //std::vector<CellTopo>::iterator it2;

    if(topologia.topo.empty()){return;}


    int teste = topologia.topo.size();
    //for(it=topologia.topo.begin();it < topologia.topo.end(); it++){


    it=topologia.topo.begin();
    for(int i=0;i<teste-1;i++){
        if(  (*it).ip.str().compare(meuip.str()) == 0 ){ it++; continue; }
        TCPSocket socket;
        socket.readDataTransferModePar(*obj);
        socket.setCallbackObject(obj);
        socket.setOutputGate(obj->gate("tcpOut"));
        //printf("%s\n",(*it).ip.str().c_str());
        socket.connect((*it).ip,10022);
        (*it).socket = socket;
        (*it).connId = socket.getConnectionId();
        it++;
    }

//fixme Descobrir porquê o codigo acima funciona e o debaixo não. Descobrir se {(*it).socket = socket;} é uma operação válida.
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
    //Envia um pacote para o botmaster ou centro de comando, avisando que esse computador está infectado e pertence a botnet.

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
//////////MÉTODOS UNITÁRIOS//////////////
Métodos que podem ser chamados pelo usuário em tempo de execução (prompt ou shell) que se referem a
um único bot.
*/
void Botnet::invadeUmSistema(L3Address ip){
    //Manda esse bot invadir um pc.

    int id;
        TCPSocket socket = setSocket(ip);
        socket.connect(ip,10022);
        id = topologia.addIp(ip);
        CellTopo* temp = topologia.getTopoById(id);
        (*temp).socket = socket;
        (*temp).connId = socket.getConnectionId();
}

void Botnet::redefinirCC(L3Address ip){
    //manda esse bot trocar de centro de comando ou botmaster.

    obj->masterSocket = setSocket(ip);
}
//////////FIM MÉTODOS UNITÁRIOS/////////////


//////////SINAIS EXTERNOS//////////////////
bool Botnet::infectaApp(int vulnerabilidade, void *ip){
    //Ao receber um pacote de invasão, determina se esse computador foi invadido com sucesso ou não.

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
    //Auxilia na configuração do socket para iniciar a conexão.

    TCPSocket socket;
    socket.readDataTransferModePar(*obj);
    socket.setCallbackObject(obj);
    socket.setOutputGate(obj->gate("tcpOut"));
    socket.connect(ip,10022);
    return socket;
}
//////////FIM AUXILIARES////////////////////////

} /* namespace inet */
