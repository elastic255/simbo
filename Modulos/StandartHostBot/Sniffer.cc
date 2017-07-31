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


#include "inet/applications/simbo/Modulos/StandartHostBot/Sniffer.h"
#include "inet/applications/simbo/Modulos/ControleSuper/ControleSuper.h"

namespace inet {

Define_Module(Sniffer);

Sniffer::Sniffer() {}

void Sniffer::initialize(int stage){


    cSimpleModule::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {

    nome = (char*)malloc(sizeof(char)*100);
    this->getParentModule()->subscribe("packetSentToLower",this);
    this->getParentModule()->subscribe("packetReceivedFromLower",this);

    //ControleSuper::fsniffers(this,&this->dados_size); //ControleSuper Aqui !!!

    }

}

L3Address Sniffer::myip(){
    //Retorna IP do server.

    InterfaceTable* interfacetab =(InterfaceTable*) this->getParentModule()->getSubmodule("interfaceTable");
    InterfaceEntry *interface =  interfacetab->getInterface(1);
    L3Address temp1 = L3Address(interface->ipv4Data()->getIPAddress());
    return temp1;
}

void Sniffer::handleMessage(cMessage *msg)
{

}

void Sniffer::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details)
{

    //TODO consertar a cambiarra das 3 linhas abaixo. e mudar para o construtor.
    L3Address meuip = myip();
    strcpy(nome,"bala");
    strcpy(nome,meuip.str().c_str());
    //-------------------

    EthernetIIFrame *msg = dynamic_cast<EthernetIIFrame *>(obj);
    //cPacket *packet = dynamic_cast<cPacket *>(obj);
    if(msg){
        //printf("host: %s \n",nome);
        //printf("pacote recebido %s \n",msg->getFullName());
        //printf("pacote recebido %s \n",msg->getClassName());
        //printf("dados: %d \n",msg->getBitLength());

        IPv4Datagram *dta = dynamic_cast<IPv4Datagram *>(msg->getEncapsulatedPacket());
        if(dta){
            DadosRedeHost *msgatual;
            msgatual = (DadosRedeHost *)malloc(sizeof(DadosRedeHost));

            //printf("destino: %s \n",dta->getDestinationAddress().str().c_str());


            msgatual->ipdestino = dta->getDestinationAddress();
            msgatual->ipremetente = dta->getSourceAddress();
            msgatual->bytelength = msg->getByteLength();
            msgatual->nome = (char*)malloc( strlen( msg->getFullName() )+3 );
            strcpy(msgatual->nome,msg->getFullName());
            msgatual->hora_recebimento = msg->getArrivalTime().dbl();

            tcp::TCPSegment *dtb = dynamic_cast<tcp::TCPSegment *>(dta->getEncapsulatedPacket());
            if(dtb){
                msgatual->portadestino = dtb->getDestPort();
                msgatual->portaremetente = dtb->getSrcPort();
            }
            dados.push_back(msgatual);
        }
    }
}

//TODO salvar os dados em arquivo personalizaveis. ???

long int Sniffer::escreve(){
    //Escreve tudo que tiver no arquivo. Todos os registros desde do começo.

    return escreve(0);
}

long int Sniffer::escreve(long int ponto){
    //Escreve no arquivo do ponto indicado até o fim (atual).

    long int s,i;
    char txt[500];
    char str[30];
    char arquivo[60];

    strcpy(arquivo,".\\io\\sniffer\\");
    strcat(arquivo,nome);
    strcat(arquivo,".txt");

    s = dados.size();
    std::ofstream outfile (arquivo);
   // if(!outfile.is_open()){throw cRuntimeError(arquivo);}
    if(!outfile.is_open()){throw cRuntimeError("problema para abrir arquivo no sniffer (existe pasta io/sniffer ?)");}

    for(i=ponto;i<s;i++){
        strcpy(txt,dados[i]->nome);
        strcat(txt,";");
        strcat(txt, dados[i]->ipdestino.str().c_str());
        strcat(txt,";");
        strcat(txt, dados[i]->ipremetente.str().c_str());
        strcat(txt,";");
        sprintf(str, "%d", dados[i]->portadestino);
        strcat(txt,str);
        strcat(txt,";");
        sprintf(str, "%d", dados[i]->portaremetente);
        strcat(txt,str);
        strcat(txt,";");
        sprintf(str, "%d", dados[i]->bytelength);
        strcat(txt, str);
        strcat(txt,";");
        sprintf(str, "%f", dados[i]->hora_recebimento);
        strcat(txt, str);
        strcat(txt,";\n");
        outfile.write(txt,strlen(txt));
        outfile.flush();
    }


    outfile.close();
    dados_size = s;
    return s;
}


void Sniffer::desalocaDados(){
    //Limpa os registro da memória.

    long int s,i;
    s = dados.size();
    for(i=0;i<s;i++){
        free(dados[i]->nome);
        free(dados[i]);
    }
    dados.clear();
    if( dados.size() != 0){throw cRuntimeError("problema para limpar os dados no sniffer)");}
}

void Sniffer::finish(){
    //Termina as operações do módulo.
    escreve();
    desalocaDados();
}

Sniffer::~Sniffer() {
    desalocaDados();
}

} /* namespace inet */
