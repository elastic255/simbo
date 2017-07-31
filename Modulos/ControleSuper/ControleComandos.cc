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

cModule * ControleComandos::ModSim=NULL;    //Modulo que representa a simulacao, está no topo e contem todos os módulos dos hosts.

ControleComandos::ControleComandos(std::string name2, std::string path2, std::ofstream &dout) {
    name = name2; // somente necessário para proximoComando e autoscript. Nome dos arquivos, correspondente ao programa externo.
    path = path2; //somente necessário para proximoComando e autoscript.  Caminho dos arquivos, correspondente ao programa externo.
    dataoutput = &dout;     //refêrencia ao arquivo de escrita para a saída da resposta.

}

void ControleComandos::setModuloSim(cModule *mod){
    //Aloca uma referência para o módulo da simulação.
    ModSim = mod;
}

void ControleComandos::exec(char *s){
    //Funciona como um índice que relaciona o comando escrito com a função correspondente.

    //TODO: acrescentar funções para controlar a simulação.
    //TODO: pensar numa forma mais programática de fazer isso.

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




void ControleComandos::escrever(char * str){
    //Escreve no arquivo de saída a resposta do comando.
    //TODO retirar /n da última linha, ocasionando bug de leitura para alguns programas externos.

    dataoutput->write(str,strlen(str));
    dataoutput->flush();
}

char * ControleComandos::getNextToken(){
    //Busca o próximo token, pode ser um comando ou argumento de um comando.

    return std::strtok(NULL, " ,-");
}

int ControleComandos::getNextTokenInt(){
    //Busca o próximo argumento de um comando que seja numérico.

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
    // Uso: Tinfecta nome_host
    // Descrição: Torna este host imediatamente infectado.

    char str[256];
    char *nome2 = (char *)malloc(sizeof(char)*256);
    char *nome1 = getNextToken();
    if(nome1 == nullptr){throw cRuntimeError(" Primeiro Argumento nao encontrado - ipcomputador(ControleComandos.cc)");}
    strcpy(nome2,".");
    nome2 = strcat(nome2,nome1);
    nome2 = strcat(nome2, "..tcpApp[1]");
    BotnetApp* mod = check_and_cast<BotnetApp*>(ModSim->getModuleByPath(nome2));
    if(mod == nullptr){throw cRuntimeError(" BotnetApp nao encontrado no ipcomputador(ControleComandos.cc)");}

    IPv4RoutingTable* tabela2 = (IPv4RoutingTable*) ModSim->getModuleByPath(".inicial.routingTable");
    if(tabela2 == nullptr){throw cRuntimeError(" IPv4RoutingTable nao encontrado no ipcomputador(ControleComandos.cc)");}
    L3Address ip = tabela2->getRouterIdAsGeneric();

    mod->Global1(0,(void*)&ip);
    sprintf(str, "%s infectado com sucesso\n", nome1);
    escrever(str);

}

void ControleComandos::BotInfecta(){
    // Uso: BotInfecta nome_host
    // Descrição: Ordena que o bot ataque este host.

}

void ControleComandos::pintaDeVermelho(){
    //Serviu de teste.
    //Pinta um host de vermelho e verifica se os comandos estão funcionando.
    //Todo: Criar um comanda que pinta os host de qualquer cor, para criar demarcadores.

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
    //Executa todos os comandos presentes em um arquivo.

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
    //Pega o próximo comando.

    std::string tmp1,tmp3;
    std::fstream datainput,controlinput;

    std::vector<char *> buffer;
    char *buf;

    tmp1.assign(path).append("i").append(name).append(".fl");
    tmp3.assign(path).append("c").append(name).append(".fl");

    datainput.open (tmp1.c_str(), std::fstream::in | std::fstream::out);
    controlinput.open (tmp3.c_str(), std::fstream::in | std::fstream::out);

    //TODO verificar , colocar proteção
    /*
    cccontrol- omnet escreve controle.
    ccontrol- omnet lê controle.
    datainput - omnet lê dados.
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
    // Uso: sniffcompn nome_do_computador
    // Descrição: Retorna o histórico de trafego (entrada e saída) do computador desde da última requisição desta função.

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
    //Uso: sniffcomp nome_do_computador
    //Descrição: o histórico de trafego (entrada e saída) do computador desde o começo da simulação.
    //TODO: verificar.

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
    //Uso: ipbotmaster
    //Descrição: Retorna o IP do botmaster.

    char str[256];
    IPv4RoutingTable* mod = (IPv4RoutingTable*) ModSim->getModuleByPath(".inicial.routingTable");
    if(mod == nullptr){throw cRuntimeError(" Modulo nao encontrado no ipbotmaster(ControleComandos.cc)");}
    IPv4Address meuip = mod->getRouterId();
    sprintf(str, "ipbotmaster %s %d \n", meuip.str().c_str(), meuip.getInt());
    escrever(str);
}

void ControleComandos::ipcomputador(){
    //Uso: ipcomputador [nome do computador]
    //Descrição: Retorna o IP do computador.
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
    //Uso: Finish
    //Descrição: Encerra a simulação.

    throw cTerminationException(E_ENDSIM);
    //throw cTerminationException("Simulacao Terminada por ControleComandos.");
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
    //Teste para verificar se é possível ler comando.

    int num = getNextTokenInt();
    char str[256];
    sprintf(str, "Teste bem sucedido %ld\n", num);
    escrever(str);
}

void ControleComandos::teste2(){
    //Teste para verificar se é possível selecionar um módulo na simulação.

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
