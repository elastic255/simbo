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


#include "inet/applications/simbo/Modulos/ControleSuper/ControleComunicacao.h"

#include "inet/applications/simbo/Modulos/ControleSuper/ControleComandos.h"
//#include <ControleComunicacao.h>
namespace inet {

ControleComunicacao::ControleComunicacao() {
    idcontrol = 0;
    idcomunicacao = ControleComunicacao::getNewIdNumber();
    habilitado  = false;
    operante = false;
}

ControleComunicacao::ControleComunicacao(const char *patht, const char *name) {

    idcontrol = 0;
    habilitado  = false;
    operante = false;
    nome.assign(name);
    path.assign(patht);

    //TODO renomear tmp para sua função.

    tmp1.assign(path).append("i").append(name).append(".fl");
    tmp2.assign(path).append("o").append(name).append(".fl");
    tmp3.assign(path).append("c").append(name).append(".fl");
    tmp4.assign(path).append("cc").append(name).append(".fl");

    /*
    cccontrol- omnet escreve controle.
    ccontrol- omnet lê controle.
    datainput - omnet lê dados.
    dataoutput- omnet escreve dados.
    */

    //limpar o arquivo.
    controlinput.open (tmp3.c_str(),std::fstream::out);
    controlinput.close();

    datainput.open (tmp1.c_str(), std::fstream::in | std::fstream::out);
    dataoutput.open (tmp2.c_str(),std::fstream::out);
    controlinput.open (tmp3.c_str(), std::fstream::in | std::fstream::out);
    controloutput.open (tmp4.c_str(),std::fstream::out);

    if(!datainput.is_open()){throw cRuntimeError(tmp1.append(" -- Arquivo nao abriu.").c_str() );}
    if(!dataoutput.is_open()){throw cRuntimeError(tmp2.append(" -- Arquivo nao abriu.").c_str() );}
    if(!controlinput.is_open()){throw cRuntimeError(tmp3.append(" -- Arquivo nao abriu.").c_str() );}
    if(!controloutput.is_open()){throw cRuntimeError(tmp4.append(" -- Arquivo nao abriu.").c_str() );}

    escreveArquivoControle(controlinput,0);
    escreveArquivoControle(controloutput,0);
    dataoutput.close();
    habilitado = true;      //Flag se os arquivos estão abertos.
    operante = true;        //Flag se os novos comandos foram lidos.

}


ControleComunicacao& ControleComunicacao::next(){
    //Escreve token no arquivo de controle.

    if(habilitado && operante){
        if(controloutput.is_open() ){
            idcontrol++;
            escreveArquivoControle(controloutput,idcontrol);
            return *this;
        }else{
            throw cRuntimeError("Erro na funcao next - contreoutput nao abriu");
        }
    }else{
        throw cRuntimeError("Erro ao usar operador ++ ControleCommunicacao, objeto nao habilitado");
    }
}


bool ControleComunicacao::clone(ControleComunicacao tmp){
    //TODO Terminar de criar este método.
    nome.assign(tmp.nome);
    path.assign(tmp.path);
    habilitado = tmp.habilitado;
    operante = tmp.operante;
    idcontrol = tmp.idcontrol;
    idcomunicacao =  tmp.idcomunicacao;
    return true;
}

void ControleComunicacao::readControl(){
    //Lê o arquivo de controle e verifica se o token é o esperado.

    if(habilitado && operante){
        if( controlinput.is_open()){
            long int id = 0;
            char linha[258];
            controlinput.getline(linha,256);
            controlinput.clear();
            controlinput.seekg(0, std::ios::beg);

            id = strtol (linha,NULL,10);
            if(id == idcontrol){
                printf("id: %d idinterno: %d \n",id,idcontrol);
                readInput();
                operante = false;
            }
        }else{throw cRuntimeError("ControleComunicacao::readControl - Erro ao abrir arquivos de controle!");}
    }else{
        throw cRuntimeError("Erro de habilitacao. Funcao ControleComunicacao::readfile");

    }
}

void ControleComunicacao::readInput(){
    //Lê os comandos do arquivo.
    //TODO: Colocar alerta para o dataoutput.open.

    dataoutput.open(tmp2.c_str(),std::fstream::out);

    if( datainput.is_open()){
        char linha[258];
        ControleComandos comandos(nome,path,dataoutput);
        while(!datainput.eof()){
           datainput.getline(linha,256);
           comandos.exec(linha);
           if(strcmp(linha,"##")==0){break;}
        }
        datainput.flush();
        datainput.clear();
        datainput.seekp(0, std::ios::beg);

        dataoutput.flush();
        dataoutput.close();

        //dataoutput.clear();
        //dataoutput.seekp(0, std::ios::beg);

    }else{throw cRuntimeError("ControleComunicacao::readControl - Erro ao abrir Arquivo de Dados Input!");}

}

void ControleComunicacao::reflesh(){
    //Reset da flag para iniciar uma nova rodada.

    operante = true;
}

void ControleComunicacao::escreveArquivoControle(std::fstream &arquivo,long int id){
    //Escreve o token no arquivo de controle.

    char str[256];
    sprintf(str, "%ld", id);
    arquivo.write(str,strlen(str));
    arquivo.flush();
    arquivo.clear();
    arquivo.seekg(0, std::ios::beg);
}

void ControleComunicacao::escreveArquivoControle(std::ofstream &arquivo,long int id){
    //Escreve o token no arquivo de controle.

    char str[256];
    sprintf(str, "%ld", id);
    arquivo.write(str,strlen(str));
    arquivo.flush();
    arquivo.clear();
    arquivo.seekp(0, std::ios::beg);
}

void ControleComunicacao::fechaArquivos(){
    //Fecha todos os arquivos.

        if(datainput.is_open()){datainput.close();}
        if(dataoutput.is_open()){dataoutput.close();}
        if(controlinput.is_open()){controlinput.close();}
        if(controloutput.is_open()){controloutput.close();}
        habilitado = false;
}

bool ControleComunicacao::isOperante(){
    return operante;
}

ControleComunicacao::~ControleComunicacao() {
    fechaArquivos();
}

///////////////STATIC//////////////////////////////

int ControleComunicacao::objcriados = 0;

int ControleComunicacao::getIdNumber(){
    //Retorna o número atual de conexões criadas.

    return ControleComunicacao::objcriados;
}

int ControleComunicacao::getNewIdNumber(){
    //Retorna um novo id para conexão, que é o número de conexões criadas +1.

    int a;
    a = ControleComunicacao::objcriados;
    ControleComunicacao::objcriados++;
    return a;
}


} /* namespace inet */
