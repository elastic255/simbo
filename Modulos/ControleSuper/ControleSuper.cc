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

#include "inet/applications/simbo/Modulos/ControleSuper/ControleSuper.h"
#include "inet/applications/simbo/Modulos/StandartHostBot/Sniffer.h"
#include "inet/applications/simbo/Modulos/BotnetApp/BotnetApp.h"
#include "inet/applications/simbo/Classes/Botnet/Botnet.h"
#include "inet/applications/simbo/Modulos/ControleSuper/ControleComunicacao.h"
#include "inet/applications/simbo/Modulos/ControleSuper/ControleComandos.h"
#include <cstring>
//#define PItempo 60

namespace inet {

Define_Module(ControleSuper);

std::vector<DadosSniffer *> ControleSuper::sniffers;    //Vetor com referência a todos os sniffers presentes na simulação.


ControleSuper::ControleSuper() {

}

void ControleSuper::initialize(int stage){
    cSimpleModule::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        remarca(0, par("periodoComunicacao").longValue());

        //TODO: criar uma maneira de inserção na lista dinâmica, sem depender de hardcode.
        if(par("matlab").boolValue() == true){
            lista.criar("matlab");
        }

        if(par("javaconsole").boolValue() == true){
            lista.criar("javaconsole");
        }

        if(par("jacamo").boolValue() == true){
            lista.criar("jacamo");
        }

        setModuloSim2();
    }

}

void ControleSuper::setModuloSim2(){
    //Pega o módulo da simulação e passa a sua referência para a classe ControleComandos.

    ControleComandos::setModuloSim(this->getParentModule());
}

void ControleSuper::EndAll(){
    //Termina a simulação

    cSimpleModule end;
    end.endSimulation();
}

void ControleSuper::remarca(int f, simtime_t g){
    //Agenda um timer/alarme.

        timerCS = new cMessage("ControleSuper");
        simtime_t d = simTime()+g;
        timerCS->setKind(f);
        scheduleAt(d, timerCS);
}

void ControleSuper::handleMessage(cMessage *msg)
{
    //Recebe mensagem e verifica se aquela mensagem foi enviada por esse objeto,
    //se foi é um alarme e deve ser passado a função handleTimer.

    if (msg->isSelfMessage())
        handleTimer(msg);
    else{
        throw cRuntimeError("ControleSuper nao deve receber mensagens externas");
    }

}

void ControleSuper::handleTimer(cMessage *msg){
    //Recebe um aviso do alarme que o tempo já se passou,
    //e é hora de verificar as entradas dos programas externos.

    lista.iniciar();
    while(lista.isprosseguir()){
        lista.loop();
    }
    lista.reflesh();

    cancelAndDelete(msg);
    remarca(0, par("periodoComunicacao").longValue());
}

void ControleSuper::escreveArquivoDadosSniffer(){
    //Escreve nos arquivos os dados coletados pelos sniffers.

    int i,n;
    long int size;
    n = ControleSuper::sniffers.size();
    for(i=0;i<n;i++){
        //size = *(ControleSuper::sniffers[i]->size );
        size = 0;
        ControleSuper::sniffers[i]->obj->escreve(size);
        printf("Size %d\n",size);
    }
}

void ControleSuper::fsniffers(Sniffer* obji, long int* i){
    //Adiciona um elemento no vetor de sniffers.
    //TODO: Melhorar essa descrição / fazer diagrama.

    DadosSniffer *kl;
    kl = (DadosSniffer*)malloc(sizeof(DadosSniffer));
    kl->obj = obji;
    kl->size = i;
    ControleSuper::sniffers.push_back(kl);
}



void ControleSuper::escreveArquivoControle(std::fstream &arquivo,long int id){
    //Escreve no arquivo de controle, avisando que a operação foi concluida.

    char str[256];
    sprintf(str, "%d", id);
    arquivo.write(str,strlen(str));
    arquivo.flush();
    arquivo.clear();
    arquivo.seekg(0, std::ios::beg);
}

void ControleSuper::escreveArquivoControle(std::ofstream &arquivo,long int id){
    //Escreve no arquivo de controle, avisando que a operação foi concluida.

    char str[256];
    sprintf(str, "%d", id);
    arquivo.write(str,strlen(str));
    arquivo.flush();
    arquivo.clear();
    arquivo.seekp(0, std::ios::beg);
}

void ControleSuper::finish(){
    lista.finalizar();
    cancelAndDelete(timerCS);
}


ControleSuper::~ControleSuper() {
}

} /* namespace inet */
