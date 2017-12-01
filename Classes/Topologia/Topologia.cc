/*
 * Topologia.cpp
 *
 *  Created on: Feb 10, 2016
 *      Author: BalaPC
 */

#include "inet/applications/simbo/Classes/Topologia/Topologia.h"

namespace inet {

//Estrutu que representa o conhecimento do bot sobre a rede. E as configura��es do pr�prio bot.
//Armazena os n�s ativos e as conex�es abertas. 

Topologia::Topologia() {

}

int Topologia::addIp(L3Address address){
	//Adiciona o ip a estrutura.
	//Representa que o bot agora sabe que o computador com esse ip est� vivo.

    if(!hasAddress(address)){
        CellTopo temp;
        temp.id = setId();
        temp.ip = address;
        printf("address2: %s\n", address.str().c_str());
        topo.push_back(temp);
        it = topo.begin();
        return temp.id;
    }
    return -1; //Fixme Certo seria localiar e retornar o id j� que o ip j� est� na lista
}

int Topologia::addIp(std::vector<L3Address> vec){
	//Adiciona um vetor de ips a estrutura.
	
    if(vec.empty()){throw cRuntimeError("Topologia::addIp, vetor chegou vazio.");}
    int lastid;
    std::vector<L3Address>::iterator itl3;
    L3Address address;
    for(itl3=vec.begin(); itl3 != vec.end(); itl3++){
        address = *itl3;
        lastid = addIp(address);
    }
    return lastid;
}

void Topologia::addVulnerabilidadeAll(int vulnerabilidade){
	//Adiciona vulnerabilidade a este computador.
	//Define que vulnerabilidades este computador ter�.
	
    if(topo.empty()){return;}
    for(it=topo.begin(); it< topo.end(); it++){
        (*it).vulnerabilidade.push_back(vulnerabilidade);
    }
}

int Topologia::setId(){
	//Cria e retorna um id para cada n� encontrado na topologia.
	//Esse ser� um identificador de f�cil acesso para se referir a um computador dentro da estrutura topologia.
	
    //return ++unitId;
    int i = unitId;
    unitId++;
    return i;
}

L3Address Topologia::getAddress(int connId){
	//Retorna o endere�o do n� (se conhecido) baseado no identificador da conex�o.
	
    L3Address ret;
    if(topo.empty()){return ret;}
    for(it=topo.begin();it != topo.end(); it++){
        if(connId == (*it).connId){return (*it).ip;}
    }
    return ret;
}

bool Topologia::apagarConnId(int connId){
	//Apaga registro referente ao n� (se conhecido) baseado no identificador da conex�o.
	
    if(topo.empty()){return false;}
    for(it=topo.begin();it != topo.end(); it++){
        if(connId == (*it).connId){
            (*it).vulnerabilidade.clear();
            topo.erase(it);
            return true;
        }
    }
    return false;
}

bool Topologia::apagarId(int id){
	//Apaga registro referente ao n� (se conhecido) baseado no identificador interno.
	
    if(topo.empty()){return false;}
    for(it=topo.begin();it != topo.end(); it++){
        if(id == (*it).id){
            (*it).vulnerabilidade.clear();
            topo.erase(it);
            return true;
        }
    }
    return false;
}

bool Topologia::apagarAddress(L3Address address){
	//Apaga registro referente ao n� (se conhecido) baseado no endere�o.
	
    if(topo.empty()){return false;}
    for(it=topo.begin();it != topo.end(); it++){
        if(address == (*it).ip){
            (*it).vulnerabilidade.clear();
            topo.erase(it);
            return true;
        }
    }
    return false;
}


L3Address Topologia::getAddressId(int id){
	//Retorna o endere�o do n� (se conhecido) baseado no identificador interno.
	
    L3Address ret;
    if(topo.empty()){return ret;}
    for(it=topo.begin();it != topo.end(); it++){
        if(id == (*it).id){return (*it).ip;}
    }
    return ret;
}

TCPSocket *Topologia::getSocketPtr(int connId){
	//Retorna o socket da conex�o (se conhecido) baseado no identificador de conex�o.
	
    if(topo.empty()){return nullptr;}
    for(it=topo.begin();it != topo.end(); it++){
        if(connId == it->connId){
            printf("(%d %d)\n", connId, it->connId);
            printf("(%d)\n", &(it->socket));return &(it->socket);}
    }
    return nullptr;
}

bool Topologia::getEstruturaConnId(int connId, CellTopo* oi){
	//Retorna todo o registro refer�nte ao identificador de conex�o, se conhecido, por passagem de par�metro.
	//Retorna verdadeiro se o identificador de conex�o for conhecido e falso caso contr�trio.
	
    if(topo.empty()){return false;}
    for(it=topo.begin();it != topo.end(); it++){
        if(connId == (*it).connId){
            (*oi) = (*it);
                    //topo[it];
            return true;
        }
    }
    return false;
}

bool Topologia::getEstruturaId(int id, CellTopo* oi){
	//Retorna todo o registro refer�nte ao identificador interno, se conhecido, por passagem de par�metro.
	//Retorna verdadeiro se o identificador interno for conhecido e falso caso contr�trio.
	
    if(topo.empty()){return false;}
    for(it=topo.begin();it != topo.end(); it++){
        if(id == (*it).id){
            (*oi) = (*it);
                    //topo[it];
            return true;}
    }
    return false;
}

CellTopo* Topologia::getTopoById(int id){
	//Retorna todo o registro refer�nte ao identificador interno, se conhecido, por passagem de par�metro.
	
    if(topo.empty()){return nullptr;}
    for(it=topo.begin();it != topo.end(); it++){
        if(id == (*it).id){return &(*it);}
    }
    return nullptr;
}

bool Topologia::hasAddress(L3Address address){
	//Verifica se o endere�o � conhecido pela estrutura.
	
    if(topo.empty()){return false;}
    for(it=topo.begin();it != topo.end(); it++){
        if(address == (*it).ip){return true;}
    }
return false;
}

bool Topologia::hasConnId(int connId){
	//Verifica se o identificador de conex�o � conhecido pela estrutura.
	
    if(topo.empty()){return false;}
    for(it=topo.begin();it != topo.end(); it++){
        if(connId == (*it).connId){return true;}
    }
return false;
}

bool Topologia::hasVulnerabilidadeConnId(int vulnerabilidade, int connId){
	//Verifica se � sabido que o computador na conex�o refer�nte ao identificador de conex�o tem esta vulnerabilidade.
	
    if(topo.empty()){return false;}
    std::vector<int>::iterator vit;

    for(it=topo.begin();it != topo.end(); it++){
        if(connId == (*it).connId){
            if( (*it).vulnerabilidade.empty() ){return false;}
            for( vit=(*it).vulnerabilidade.begin(); vit <= (*it).vulnerabilidade.end(); vit++){
                if( (*vit) == vulnerabilidade){return true;}
            }
        }
    }
return false;
}

void Topologia::apagarTudo(){
	//Apaga toda a estrutura topologia.
	//Utilizado na destrui��o do objeto topologia para limpar a mem�ria.
	
    for(it=topo.begin();it != topo.end(); it++){
        (*it).vulnerabilidade.clear();
    }
    topo.clear();
}


Topologia::~Topologia() {
apagarTudo();
}
} /* namespace inet */
