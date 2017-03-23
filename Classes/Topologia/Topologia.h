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
    int NumDropOut = 0;
}CellTopo;

class Topologia {
public:

    int unitId = 1;
    std::vector<CellTopo> topo;
    std::vector<CellTopo>::iterator it;

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
