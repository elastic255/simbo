/*
 * Topologia.h
 *
 *  Created on: Feb 10, 2016
 *      Author: BalaPC
 */

#ifndef TOPOLOGIA_H_
#define TOPOLOGIA_H_
#include "inet/networklayer/common/L3Address.h"
#include "inet/transportlayer/contract/tcp/TCPSocket.h"

namespace inet {

//Registro que armazena toda a informa��o sabida do bot sobre aquele ip.
typedef struct CellTopo{
    int id;
    L3Address ip;						//Endere�o do host (IP)
    int connId;							//Identificador de Conexão entre o bot e esse computador.
    TCPSocket socket;					//Socket aberto entre o bot e esse computador.
    std::vector<int> vulnerabilidade;	//Vulnerabilidades conhecidas deste computador.
    int NumDropOut = 0;					//Marca quanto tempo não se tem notícia do bot nesse computador (só usado por CC ou botmaster). 
}CellTopo;

class Topologia {
public:

    long unitId = 1;					//Indica quantos registros a estrutura já adicionou em seu tempo de vida. (não quantos ela possui no momento).
    std::vector<CellTopo> topo;			//Vetor de Registro referênte a todos os nós da rede conhecidos.
    std::vector<CellTopo>::iterator it;	//Iterador do vetor.

    virtual int addIp(L3Address);
    virtual int addIp(std::vector<L3Address>);

    bool apagarConnId(int connId);
    bool apagarId(int id);
    bool apagarAddress(L3Address address);

    CellTopo* getTopoById(int id);
    bool getEstruturaConnId(int connId, CellTopo* oi);
    bool getEstruturaId(int id, CellTopo* oi);

    virtual void addVulnerabilidadeAll(int vulnerabilidade);

    virtual bool hasAddress(L3Address address);
    virtual bool hasConnId(int connId);
    virtual bool hasVulnerabilidadeConnId(int vulnerabilidade, int connId);
    virtual int setId();

    virtual L3Address getAddress(int connId);
    virtual L3Address getAddressId(int id);
    virtual TCPSocket *getSocketPtr(int connId);

    virtual void apagarTudo();


    Topologia();
    virtual ~Topologia();
};

} /* namespace inet */

#endif /* TOPOLOGIA_H_ */
