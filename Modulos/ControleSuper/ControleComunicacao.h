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

#ifndef CONTROLECOMUNICACAO_H_
#define CONTROLECOMUNICACAO_H_

//#include "inet/applications/simbo/Classes/includeall/all.h"
#include "inet/common/INETDefs.h"
#include <fstream>
#include <string>
#include <algorithm>

namespace inet {

class INET_API ControleComunicacao {
public:

    std::string tmp1,tmp2,tmp3,tmp4;
    std::fstream datainput,controlinput;
    std::ofstream dataoutput,controloutput;
    std::string nome,path;  //Nome de referência e caminho de onde estão os arquivos.
    bool habilitado;        //Flag se os arquivos estão abertos.
    bool operante;          //Flag se os novos comandos foram lidos.
    long int idcontrol;     //id dos arquivos / token externo
    int idcomunicacao;      //id do objeto   / token interno



    virtual void escreveArquivoControle(std::fstream &arquivo,long int id);
    virtual void escreveArquivoControle(std::ofstream &arquivo,long int id);
    virtual void fechaArquivos();
    virtual ControleComunicacao& next();
    virtual bool clone(ControleComunicacao tmp);
    virtual void readControl();
    virtual void readInput();
    virtual void reflesh();

    virtual bool isOperante();

    ControleComunicacao();
    ControleComunicacao(const char *path2, const char *name);

    virtual ~ControleComunicacao();
////////////STATIC//////////////////////////////
    static int objcriados;      //Número de conexões criadas.
    static int getIdNumber();
    static int getNewIdNumber();

protected:



};

} /* namespace inet */

#endif /* CONTROLECOMUNICAÇÃO_H_ */
