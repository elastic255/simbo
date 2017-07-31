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

#ifndef CONTROLECOMANDOS_H_
#define CONTROLECOMANDOS_H_

#include "inet/common/INETDefs.h"
#include "inet/networklayer/contract/ipv4/IPv4Address.h"
#include "inet/networklayer/ipv4/IPv4RoutingTable.h"

#include "inet/applications/simbo/Modulos/StandartHostBot/Sniffer.h"

#include <string.h>
#include <stdlib.h>
#include <fstream>

namespace inet {

class INET_API ControleComandos {
//Classe
public:
    static void setModuloSim(cModule *mod);
    static cModule *ModSim;     //Guarda referência ao módulo de simulação (pai de todos os módulos, como hosts).

//Objeto
public:
    ControleComandos(std::string name, std::string path2,std::ofstream &dataoutput);
    virtual void exec(char *s);
    virtual ~ControleComandos();

private:
    std::ofstream *dataoutput;  //Referência de arquivo de saída para resposta do comando.
    std::string name,path;      //Nome de referência e caminho dos arquivos.
    virtual char * getNextToken();
    virtual int getNextTokenInt();
    virtual void escrever(char * st);

    //Funcionalidades
    virtual void teste();
    virtual void teste2();
    virtual void ipcomputador();
    virtual void ipbotmaster();
    virtual void sniffcomp();
    virtual void sniffcompn();
    virtual void proximoComando();
    virtual void autoScript();
    virtual void Tinfecta();
    virtual void BotInfecta();
    virtual void pintaDeVermelho();

    virtual void Finish();
    //virtual void Stop();
    //virtual void Start();

};

} /* namespace inet */

#endif /* CONTROLECOMANDOS_H_ */
