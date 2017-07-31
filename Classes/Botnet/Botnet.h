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



#ifndef __INET_BOTNET_H_
#define __INET_BOTNET_H_





#include "inet/common/INETDefs.h"
#include "inet/applications/tcpapp/TelnetApp.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/transportlayer/contract/tcp/TCPSocket.h"
#include "inet/networklayer/common/InterfaceTable.h"
#include "inet/networklayer/common/InterfaceEntry.h"
#include "inet/networklayer/ipv4/IPv4InterfaceData.h"


#include "inet/applications/simbo/Modulos/BotnetApp/BotnetApp.h"
#include "inet/applications/simbo/Modulos/BotnetPing/BotnetPing.h"

#include "inet/applications/simbo/Classes/Botnet/BotnetInterface.h"
#include "inet/applications/simbo/Classes/Topologia/Topologia.h"

#include <cstring>
#include <omnetpp.h>

namespace inet {

class INET_API Botnet : public BotnetInterface {
public:
    Botnet(BotnetApp *);
    virtual ~Botnet();

    BotnetApp *obj; //Guarda referência do módulo simples.

   virtual void prospectaTopologia();
   virtual bool terminoProspectaTopologia();
   virtual bool verificaProspectaTopologia();

   void prospectaSuperficies();

   virtual void invadeSistemas();
   virtual bool aliveFeedback(int connId);
   virtual bool recebeComando(void *);

   virtual void invadeUmSistema(L3Address ip);
   void redefinirCC(L3Address ip);
   virtual bool infectaApp(int, void*);

   virtual L3Address myip();
   virtual TCPSocket setSocket(L3Address ip);


};

} /* namespace inet */

#endif /* __INET_BOTNET_H_ */
