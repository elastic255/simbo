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

#include "inet/applications/simbo/Modulos/ControleSuper/ControleComunicacaoSet.h"

namespace inet {

ControleComunicacaoSet::ControleComunicacaoSet() {
    tamanho = 0;    //Número de conexões.
}

ControleComunicacaoSet::ControleComunicacaoSet(ControleComunicacao *a) {
    tamanho = 0;
    add(a);
}

void ControleComunicacaoSet::add(ControleComunicacao *com) {
    //Adiciona a conexão à lista de conexões.

    listaModulos.push_back(com);
    estado.push_back(ControleComunicacaoSet::RecemCriado);
    tamanho++;
}

void ControleComunicacaoSet::criar(const char *a) {
    //Cria objeto para controlar a comunicação com um dado programa externo e adiciona à lista de comunicações ativas.

    ControleComunicacao *com =  new ControleComunicacao(".\\io\\controles\\",a);
    add(com);
}

void ControleComunicacaoSet::iniciar() {
    //Inicia todas as conexões.
    for (int i=0;i<tamanho;i++){
        listaModulos[i]->next();
    }
}

void ControleComunicacaoSet::loop() {
    //Todas as conexões verificam se estão autorizadas a atuar.
    for (int i=0;i<tamanho;i++){
        if(listaModulos[i]->isOperante()){
            listaModulos[i]->readControl();
        }
    }
}

bool ControleComunicacaoSet::isprosseguir() {
    //Verifica se a simulação pode continuar.

    bool verifica = false;
    for (int i=0;i<tamanho;i++){
        if(listaModulos[i]->isOperante()){
            verifica = true;
        }
    }
    return verifica;
}

void ControleComunicacaoSet::reflesh() {
    //Reset das flags das conexões para a próxima rodada.

    for (int i=0;i<tamanho;i++){
            listaModulos[i]->reflesh();
    }
}


void ControleComunicacaoSet::fechar() {
    //Fecha todas as conexões.

    for (int i=0;i<tamanho;i++){
        listaModulos[i]->fechaArquivos();
    }
    tamanho = 0;
}

void ControleComunicacaoSet::finalizar() {
    //Encerra o objeto.

    fechar();
    for (int i=0;i<tamanho;i++){
        delete listaModulos[i];
    }
}


ControleComunicacaoSet::~ControleComunicacaoSet() {

}


} /* namespace inet */
