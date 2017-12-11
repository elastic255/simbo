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
#include <arpa/inet.h>
#include <vector>
#include <string>

#include <omnetpp/platdep/sockets.h>

namespace inet {

//namespace athena{

//Max HTTP Packet Stuff
#define MAX_HTTP_PACKET_LENGTH          5000

class AthenaCC : public httptools::HttpServerBase, public TCPSocket::CallbackInterface {
private:
    char recvBuffer[4000];
    int numRecvBytes;

    const char *srvAddr;
    int usPort;
protected:
    typedef struct {
        std::string botid;
        int newbot;
        std::string country;
        std::string country_code;
        std::string ip;
        std::string os;
        int cpu;
        int type;
        int cores;
        std::string version;
        std::string net;
        int botskilled;
        int files;
        int regkey;
        int admin;
        int ram;
        int busy;
        int lastseen;
    } BOT;

    std::vector<BOT> botlist;

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

    void processData(char *str);
    httptools::HttpReplyMessage *generateReply(httptools::HttpRequestMessage *request, std::string &outDataMarker);
    std::string urldecode(std::string str);
    const char *BASE64_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    void _base64_encode_triple(unsigned char triple[3], char result[4]);
    int base64_encode(unsigned char *source, size_t sourcelen, char *target, size_t targetlen);
    int _base64_char_value(char base64char);
    int _base64_decode_triple(char quadruple[4], unsigned char *result);
    size_t base64_decode(const char *source, char *target, size_t targetlen);
    void strtr(char *cSource, const char *cCharArrayA, const char *cCharArrayB);
public:
    AthenaCC();
    virtual ~AthenaCC();
};

//} /* namespace athena */

} /* namespace inet */

#endif /* INET_APPLICATIONS_SIMBO_MODULOS_ATHENACC_ATHENACC_H_ */
