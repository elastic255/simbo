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

typedef struct CellTopo{
    int id;
    L3Address ip;
    int connId;
    TCPSocket socket;
    std::vector<int> vulnerabilidade;
}CellTopo;

class Topologia {
public:

    int classid = 1;
    std::vector<CellTopo> topo;
    std::vector<CellTopo>::iterator it;

    virtual void addIp(L3Address);
    virtual void addIp(std::vector<L3Address>);

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
