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

#ifndef INET_APPLICATIONS_SIMBO_MODULOS_ATHENACC_ATHENACC_H_
#define INET_APPLICATIONS_SIMBO_MODULOS_ATHENACC_ATHENACC_H_

#include "inet/applications/httptools/server/HttpServerBase.h"
#include "inet/transportlayer/contract/tcp/TCPSocket.h"
#include "inet/transportlayer/contract/tcp/TCPSocketMap.h"

namespace inet {

//namespace athena{

// Criar uma fila de requisições (talvez nem seja necessário)
// Primeiro evento que vem de fora -> requisição

class AthenaCC : public httptools::HttpServerBase, public TCPSocket::CallbackInterface {
private:
    char recvBuffer[4000];
    int numRecvBytes;

    int addr;
    int srvAddr;

    std::queue<httptools::HttpRequestMessage> requestsWaiting;

protected:
    TCPSocket listensocket;
    TCPSocketMap sockCollection;
    unsigned long numBroken = 0;
    unsigned long socketsOpened = 0;

protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void finish() override;
    virtual void handleMessage(cMessage *msg) override;
    cPacket *handleReceivedMessage(cMessage *msg);

    virtual void socketEstablished(int connId, void *yourPtr) override;
    virtual void socketDataArrived(int connId, void *yourPtr, cPacket *msg, bool urgent) override;
    virtual void socketPeerClosed(int connId, void *yourPtr) override;
    virtual void socketClosed(int connId, void *yourPtr) override;
    virtual void socketFailure(int connId, void *yourPtr, int code) override;

    httptools::HttpReplyMessage *handlePostRequest(httptools::HttpRequestMessage *request);

    /**
     * For Athena bot only
     */
    int _base64_char_value(char base64char);
    int _base64_decode_triple(char quadruple[4], unsigned char *result);
    size_t base64_decode(const char *source, char *target, size_t targetlen);
    void strtr(char *cSource, char *cCharArrayA, char *cCharArrayB);
public:
    AthenaCC();
    virtual ~AthenaCC();
};

//} /* namespace athena */

} /* namespace inet */

#endif /* INET_APPLICATIONS_SIMBO_MODULOS_ATHENACC_ATHENACC_H_ */
