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

#include "inet/applications/felipe/Classes/Botnet/Botnet.h"

namespace inet {

Botnet::Botnet(BotnetApp *oi) {
    obj = oi;
}

Botnet::~Botnet() {
    topologia.apagarTudo();
}


////Prospecta
void Botnet::prospectaTopologia(){
    BotnetPing* ping =(BotnetPing*) obj->getParentModule()->getSubmodule("pingApp",0);
    ping->startSendingPingRequests();
}

bool Botnet::verificaProspectaTopologia(){
    BotnetPing* ping =(BotnetPing*) obj->getParentModule()->getSubmodule("pingApp",0);
    return ping->pingFinalizado();
}

bool Botnet::terminoProspectaTopologia(){
    BotnetPing* ping =(BotnetPing*) obj->getParentModule()->getSubmodule("pingApp",0);

    topologia.addIp(ping->carregaEnderecos());
    return true;
}

////Superfície

void Botnet::prospectaSuperficies(){
    topologia.addVulnerabilidadeAll(100);
}

////Invade
void Botnet::invadeSistemas(){

    L3Address meuip = myip();
    std::vector<CellTopo>::iterator it;

    if(topologia.topo.empty()){return;}
    int teste = topologia.topo.size();
    //for(it=topologia.topo.begin();it < topologia.topo.end(); it++){
    it=topologia.topo.begin();
    for(int i=0;i<teste-1;i++){
        TCPSocket socket;
        socket.readDataTransferModePar(*obj);
        socket.setCallbackObject(obj);
        socket.setOutputGate(obj->gate("tcpOut"));

        if(  (*it).ip.str().compare(meuip.str()) == 0 ){ it++; continue; }

        //printf("%s\n",(*it).ip.str().c_str());

        socket.connect((*it).ip,10022);
        (*it).socket = socket;
        (*it).connId = socket.getConnectionId();
        it++;
    }
}


////

//Sinais externos
bool Botnet::infectaApp(int vulnerabilidade, void *ip){
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

//Auxiliares


L3Address Botnet::myip(){
    InterfaceTable* interfacetab =(InterfaceTable*) obj->getParentModule()->getSubmodule("interfaceTable");
    InterfaceEntry *interface =  interfacetab->getInterface(1);
    L3Address temp1 = L3Address(interface->ipv4Data()->getIPAddress());
    return temp1;
}

TCPSocket Botnet::setSocket(L3Address ip){
TCPSocket socket;
socket.readDataTransferModePar(*obj);
socket.setCallbackObject(obj);
socket.setOutputGate(obj->gate("tcpOut"));
socket.connect(ip,10022);
return socket;
}

} /* namespace inet */
