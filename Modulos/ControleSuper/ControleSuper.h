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

#ifndef CONTROLESUPER_H_
#define CONTROLESUPER_H_

#include "inet/common/INETDefs.h"
#include <fstream>
#include <stdlib.h>
#include "inet/applications/simbo/Modulos/ControleSuper/ControleComunicacaoSet.h"

namespace inet {

class ControleComunicacaoSet;
class Sniffer;

typedef struct DadosSniffer{
        Sniffer* obj;
        long int* size;
    }DadosSniffer;

class INET_API ControleSuper : public cSimpleModule  {
public:

      ControleComunicacaoSet lista;     //Objeto que controla a comunicação com o programa externo.
      cMessage *timerCS;    //Timer padrão desta classe.

private:
    static std::vector<DadosSniffer *> sniffers; //Vetor com referência a todos os sniffers presentes na simulação.


public:
    virtual void finish() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void initialize(int stage) override;
    virtual void setModuloSim2();

    virtual void handleTimer(cMessage *msg);
    virtual void remarca(int f, simtime_t g);
    virtual void escreveArquivoControle(std::fstream &arquivo, long int id);
    virtual void escreveArquivoControle(std::ofstream &arquivo,long int id);
    virtual void escreveArquivoDadosSniffer();

    static void fsniffers(Sniffer* obji, long int* i);
    static void EndAll();

    ControleSuper();
    virtual ~ControleSuper();
};

} /* namespace inet */

#endif /* CONTROLESUPER_H_ */
