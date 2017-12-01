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



#ifndef __INET_BOTNETINTERFACE_H_
#define __INET_BOTNETINTERFACE_H_





#include "inet/common/INETDefs.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/transportlayer/contract/tcp/TCPSocket.h"

#include "inet/applications/simbo/Modulos/BotnetApp/BotnetApp.h"
#include "inet/applications/simbo/Classes/Topologia/Topologia.h"


namespace inet {

class INET_API BotnetInterface {
public:


    BotnetInterface(BotnetApp *){};
    BotnetInterface(){};
    virtual ~BotnetInterface(){};

   //BotnetApp *obj;
    Topologia topologia;
    std::vector<CellTopo> subBotnet;        //fixme: em desenvolvimento.
    std::vector<CellTopo>::iterator it;     //fixme: em desenvolvimento.
    typedef std::vector<CellTopo>::iterator SubBotIterator;
    int maxNumDropOut;     //fixme:Em deenvolvimento. Tempo m�ximo para um bot ser considerado desconectado da botnet pelos CC ou botmaster.

   //Ciclo tradicional
   virtual void inicia(){}

   virtual void prospectaTopologia(){}
   virtual bool terminoProspectaTopologia(){return false;}
   virtual bool verificaProspectaTopologia(){return true;}

   virtual void prospectaSuperficies(){}
   virtual int prospectaSuperficie(L3Address){return 0;}
   virtual bool terminoProspectaSuperficie(){return false;}
   virtual bool verificaProspectaSuperficie(){return true;}

   virtual void invadeSistemas(){}
   virtual TCPSocket * invadeSistema(L3Address){return NULL;}
   virtual bool terminoInvadeSistemas(){return false;}
   virtual bool verificaInvadeSistemas(){return true;}

   virtual int isSetTimerAliveFeedback(){return 3;}     //Regula de quanto em quanto tempo o bot vai enviar notifica��o de vida para o centro de comando ou botmaster.

   virtual void finaliza(){}


   //Sinais externos
   virtual bool infectaApp(int vulnerabilidade, void *ip){return true;}
   virtual bool aliveFeedback(int connId){return false;}
   virtual bool recebeComando(void *){return false;}

   //Ciclo de comandos
   virtual bool repassaComando(){return false;}
   virtual bool executaComando(){return false;}

   //Auxiliares
   virtual bool verificaIdConexoesAbertas(int  connId){return false;}
   virtual TCPSocket * getConexoesAberta(int connId){return NULL;}


};

} /* namespace inet */

#endif /* __INET_BOTNET_H_ */
