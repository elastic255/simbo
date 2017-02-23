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

#include "inet/applications/simbo/Modulos/ControleSuper/ControleComandos.h"

#include "inet/applications/simbo/Modulos/ControleSuper/ControleSuper.h"
#include "inet/applications/simbo/Modulos/BotnetApp/BotnetApp.h"
#include "inet/applications/simbo/Classes/Botnet/BotnetInterface.h"

#include "inet/applications/simbo/Classes/Botnet/Botnet.h"


//#include <omnetpp.h>

namespace inet {

cModule * ControleComandos::ModSim=NULL;

ControleComandos::ControleComandos(std::string name2, std::string path2, std::ofstream &dout) {
    name = name2; // somente necess�rio para proximoComando.
    path = path2; //somente necess�rio para proximoComando.
    dataoutput = &dout;

}

void ControleComandos::setModuloSim(cModule *mod){
    ModSim = mod;
}

void ControleComandos::exec(char *s){
    char * pch;
    pch = strtok (s," ,-");
    if(pch == nullptr){return;}
    if(strcmp(pch,"ipbotmaster")==0){ipbotmaster();}
    if(strcmp(pch,"ipcomputador")==0){ipcomputador();}
    if(strcmp(pch,"sniffcomp")==0){sniffcomp();}
    if(strcmp(pch,"sniffcompn")==0){sniffcompn();}
    if(strcmp(pch,"Tinfecta")==0){Tinfecta();}
    if(strcmp(pch,"BotInfecta")==0){BotInfecta();}

    if(strcmp(pch,"LoadScript")==0){autoScript();}
    if(strcmp(pch,"##")==0){proximoComando();}
    if(strcmp(pch,"Finish")==0){Finish();}
    //if(strcmp(pch,"Start")==0){Start();}
    //if(strcmp(pch,"Stop")==0){Stop();}

    if(strcmp(pch,"teste2")==0){teste2();}
    if(strcmp(pch,"teste")==0){teste();}

    if(strcmp(pch,"invade")==0){}
}

//TODO: acrescentar fun��es para controlar a simula��o.


void ControleComandos::escrever(char * str){
    //TODO retirar /n da �ltima linha, ocasionando bug de leitura para alguns programas externos.
    dataoutput->write(str,strlen(str));
    dataoutput->flush();
}

char * ControleComandos::getNextToken(){
    return std::strtok(NULL, " ,-");
}

int ControleComandos::getNextTokenInt(){
    char * pch;
    pch = getNextToken();
    if(pch == nullptr){throw cRuntimeError(" Argumento nao encontrado - getNextTokenInt(ControleComandos.cc)");}
    return atoi( pch );
}

ControleComandos::~ControleComandos() {

}

////////////////Funcionalidades////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ControleComandos::Tinfecta(){
    char str[256];
    char *nome2 = (char *)malloc(sizeof(char)*256);
    char *nome1 = getNextToken();
    if(nome1 == nullptr){throw cRuntimeError(" Primeiro Argumento nao encontrado - ipcomputador(ControleComandos.cc)");}
    strcpy(nome2,".");
    nome2 = strcat(nome2,nome1);
    nome2 = strcat(nome2, "..tcpApp[1]");
    BotnetApp* mod = check_and_cast<BotnetApp*>(ModSim->getModuleByPath(nome2));
    if(mod == nullptr){throw cRuntimeError(" Modulo nao encontrado no ipcomputador(ControleComandos.cc)");}

    IPv4RoutingTable* tabela2 = (IPv4RoutingTable*) ModSim->getModuleByPath(".inicial.routingTable");
    if(tabela2 == nullptr){throw cRuntimeError(" Modulo nao encontrado no ipcomputador(ControleComandos.cc)");}
    L3Address ip = tabela2->getRouterIdAsGeneric();

    mod->Global1(0,(void*)&ip);
    sprintf(str, "%s infectado com sucesso\n", nome1);
    escrever(str);

}

void ControleComandos::BotInfecta(){

}

void ControleComandos::pintaDeVermelho(){
    char str[256];
    char *nome2 = (char *)malloc(sizeof(char)*256);
    char *nome1 = getNextToken();
    if(nome1 == nullptr){throw cRuntimeError(" Primeiro Argumento nao encontrado - ipcomputador(ControleComandos.cc)");}
    strcpy(nome2,".");
    nome2 = strcat(nome2,nome1);
    nome2 = strcat(nome2, "..tcpApp[1]");
    BotnetApp* mod = (BotnetApp*) ModSim->getModuleByPath(nome2);
    if(mod == nullptr){throw cRuntimeError(" Modulo nao encontrado no ipcomputador(ControleComandos.cc)");}
    mod->mudaIconeBotnet();
    sprintf(str, "%s infectado com sucesso\n", nome1);
    escrever(str);

}

void ControleComandos::autoScript(){
    std::string tmp1,tmp3;
    std::fstream datainput,scriptinput;

    std::vector<char *> buffer;
    char *buf;

    tmp1.assign(path).append("i").append(name).append(".fl");
    tmp3.assign(path).append("script").append(name).append(".fl");

    datainput.open (tmp1.c_str(),std::fstream::out);
    scriptinput.open (tmp3.c_str(), std::fstream::in | std::fstream::out);


    while(!scriptinput.eof()){
        buf = (char*)malloc(sizeof(char)*258);
        datainput.getline(buf,256);
        strcat(buf,"\n");
        buffer.push_back(buf);
    }
    scriptinput.close();

    while(!buffer.empty()){
        datainput.write(buffer[0],strlen(buffer[0]));
        free(buffer[0]);
        buffer.erase(buffer.begin());
    }
    datainput.close();

    char str[256];
    sprintf(str, "Script Carregado\n");
    escrever(str);
}

void ControleComandos::proximoComando(){
    std::string tmp1,tmp3;
    std::fstream datainput,controlinput;

    std::vector<char *> buffer;
    char *buf;

    tmp1.assign(path).append("i").append(name).append(".fl");
    tmp3.assign(path).append("c").append(name).append(".fl");

    datainput.open (tmp1.c_str(), std::fstream::in | std::fstream::out);
    controlinput.open (tmp3.c_str(), std::fstream::in | std::fstream::out);

    //TODO verificar , colocar prote��o
    /*
    cccontrol- omnet escreve controle.
    ccontrol- omnet l� controle.
    datainput - omnet l� dados.
    dataoutput- omnet escreve dados.
    */

    char p[258];
    while(!datainput.eof()){
        datainput.getline(p,256);
        if(strcmp(p,"##")==0){break;}
    }

    while(!datainput.eof()){
        buf = (char*)malloc(sizeof(char)*258);
        datainput.getline(buf,256);
        strcat(buf,"\n");
        buffer.push_back(buf);
    }
    datainput.close();
    datainput.open (tmp1.c_str(), std::fstream::out);

    while(!buffer.empty()){
        datainput.write(buffer[0],strlen(buffer[0]));
        free(buffer[0]);
        buffer.erase(buffer.begin());
    }
    datainput.close();

    char t[258];
    controlinput.getline(t,256);
    controlinput.clear();
    controlinput.seekg(0, std::ios::beg);
    int id = strtol (t,NULL,10)+1;
    sprintf(t,"%d",id);
    controlinput.write(t,strlen(t));
    controlinput.close();

    char str[256];
    sprintf(str, "Execucao automatica\n");
    escrever(str);
}


void ControleComandos::sniffcompn(){
    //Comando: sniffcompn [nome do computador]
    //Retorna o hist�rico de trafego (entrada e sa�da) do computador desde da �ltima requisi��o desta fun��o.
    char str[256];
    char *nome1 = (char *)malloc(sizeof(char)*256);
    char *nome2 = getNextToken();
    //int tamanho = getNextTokenInt();
    if(nome2 == nullptr){throw cRuntimeError(" Primeiro Argumento nao encontrado - ipcomputador(ControleComandos.cc)");}

    strcpy(nome1,".");
    nome1 = strcat(nome1,nome2);
    nome1 = strcat(nome1, ".sniffer");
    Sniffer* mod = (Sniffer*) ModSim->getModuleByPath(nome1);
    if(mod == nullptr){throw cRuntimeError(" Modulo nao encontrado no snifcomp(ControleComandos.cc)");}

    mod->escreve(mod->dados_size);
    sprintf(str, "sniffcompn %s %ld \n", nome2, mod->dados_size);
    escrever(str);
}

void ControleComandos::sniffcomp(){
    //Comando: sniffcomp [nome do computador]
    //Retorna todo o hist�rico de trafego (entrada e sa�da) do computador desde o come�o da simula��o.
    char str[256];
    char *nome2 = (char *)malloc(sizeof(char)*256);
    char *nome1 = getNextToken();
    if(nome1 == nullptr){throw cRuntimeError(" Primeiro Argumento nao encontrado - ipcomputador(ControleComandos.cc)");}
    strcpy(nome2,".");
    nome2 = strcat(nome2,nome1);
    nome2 = strcat(nome2, ".sniffer");
    Sniffer* mod = (Sniffer*) ModSim->getModuleByPath(nome2);
    if(mod == nullptr){throw cRuntimeError(" Modulo nao encontrado no snifcomp(ControleComandos.cc)");}
    mod->escreve(0);
    sprintf(str, "sniffcomp %s %ld \n", nome2, mod->dados_size);
    escrever(str);
}

void ControleComandos::ipbotmaster(){
    //Comando: ipbotmaster
    //Retorna o IP do botmaster.
    char str[256];
    IPv4RoutingTable* mod = (IPv4RoutingTable*) ModSim->getModuleByPath(".inicial.routingTable");
    if(mod == nullptr){throw cRuntimeError(" Modulo nao encontrado no ipbotmaster(ControleComandos.cc)");}
    IPv4Address meuip = mod->getRouterId();
    sprintf(str, "ipbotmaster %s %d \n", meuip.str().c_str(), meuip.getInt());
    escrever(str);
}

void ControleComandos::ipcomputador(){
    //Comando: ipcomputador [nome do computador]
    //Retorna o IP do computador.
    char str[256];
    char *nome2 = (char *)malloc(sizeof(char)*256);
    char *nome1 = getNextToken();
    if(nome1 == nullptr){throw cRuntimeError(" Primeiro Argumento nao encontrado - ipcomputador(ControleComandos.cc)");}
    strcpy(nome2,".");
    nome2 = strcat(nome2,nome1);
    nome2 = strcat(nome2, ".routingTable");
    IPv4RoutingTable* mod = (IPv4RoutingTable*) ModSim->getModuleByPath(nome2);
    if(mod == nullptr){throw cRuntimeError(" Modulo nao encontrado no ipcomputador(ControleComandos.cc)");}
    IPv4Address meuip = mod->getRouterId();
    sprintf(str, "ipcomputador %s %s %d \n", nome1, meuip.str().c_str(), meuip.getInt());
    escrever(str);
}

void ControleComandos::Finish(){
    throw cTerminationException("Simulacao Terminada por ControleComandos.");
}

/*
void ControleComandos::Stop(){
    //Depreciado.

    //cSimulation *Sim = cSimulation::getActiveSimulation();
    //cScheduler *esc = Sim->getScheduler();
    //esc->endRun();
    //cSimulation::setActiveSimulation(NULL);
}

void ControleComandos::Start(){
    //Depreciado.
    //cSimulation::setActiveSimulation(ControleComandos::Sim);
}
*/

void ControleComandos::teste(){
    int num = getNextTokenInt();
    char str[256];
    sprintf(str, "Teste bem sucedido %ld\n", num);
    escrever(str);
}

void ControleComandos::teste2(){
    char str[256];
    //int num1 = getNextTokenInt();
    //int num2 = getNextTokenInt();
    IPv4RoutingTable* mod = (IPv4RoutingTable*) ModSim->getModuleByPath(".inicial.routingTable");
    if(mod == nullptr){throw cRuntimeError(" Modulo nao encontrado no ModSim(ControleComandos.cc)");}
    IPv4Address meuip = mod->getRouterId();
    sprintf(str, "Teste 2 bem sucedido %s %d \n", meuip.str().c_str(), meuip.getInt());
    /*
    cProperties *props = mod->getProperties();
    cProperty *prop = props->get(5);
    std::string meuip = prop->info();
    */
    escrever(str);
}


//////////////////////////////////////////////////////////////////////////////////

} /* namespace inet */
