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

#ifndef SNIFFER_H_
#define SNIFFER_H_

#include "inet/common/INETDefs.h"
#include "inet/networklayer/ipv4/IPv4Datagram.h"
#include "inet/transportlayer/tcp_common/TCPSegment.h"
#include "inet/linklayer/ethernet/Ethernet.h"
#include "inet/linklayer/ethernet/EtherFrame_m.h"
#include "inet/networklayer/common/InterfaceTable.h"
#include "inet/networklayer/common/InterfaceEntry.h"
#include "inet/networklayer/ipv4/IPv4InterfaceData.h"

#include <fstream>

namespace inet {

//Registro de dados de saída ou entrada.
typedef struct DadosRedeHost{
        L3Address ipdestino;
        L3Address ipremetente;
        unsigned short portadestino;
        unsigned short portaremetente;
        double hora_recebimento;
        int64_t bytelength;
        char *nome;
    }DadosRedeHost;

class INET_API Sniffer : public cSimpleModule, protected cListener{
public:

    std::vector<DadosRedeHost*> dados;  //Vetor com todos os dados que entraram e saíram do host.
    long int dados_size;                //Contagem de todos os dados que entraram e sairam.
    char *nome;                         //Nome do host.

    Sniffer();
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details) override;
    virtual void finish() override;

    virtual long int escreve();
    virtual long int escreve(long int ponto);
    virtual L3Address myip();

    virtual void desalocaDados();
    virtual ~Sniffer();
};

} /* namespace inet */

#endif /* SNIFFER_H_ */
