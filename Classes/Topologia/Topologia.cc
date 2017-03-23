/*
 * Topologia.cpp
 *
 *  Created on: Feb 10, 2016
 *      Author: BalaPC
 */

#include "inet/applications/simbo/Classes/Topologia/Topologia.h"

namespace inet {


Topologia::Topologia() {


}

int Topologia::addIp(L3Address address){

    if(!hasAddress(address)){
        CellTopo temp;
        temp.id = setId();
        temp.ip = address;
        topo.push_back(temp);
        it = topo.begin();
        return temp.id;
    }
    return -1; //Fixme Certo seria localiar e retornar o id já que o ip já está na lista
}

int Topologia::addIp(std::vector<L3Address> vec){
    if(vec.empty()){throw cRuntimeError("Topologia::addIp, vetor chegou vazio.");}
    int lastid;
    std::vector<L3Address>::iterator itl3;
    L3Address address;
    for(itl3=vec.begin(); itl3 <= vec.end(); itl3++){
        address = *itl3;
        lastid = addIp(address);
    }
    return lastid;
}

void Topologia::addVulnerabilidadeAll(int vulnerabilidade){
    if(topo.empty()){return;}
    for(it=topo.begin(); it< topo.end(); it++){
        (*it).vulnerabilidade.push_back(vulnerabilidade);
    }
}

int Topologia::setId(){
    //return ++unitId;
    int i = unitId;
    unitId++;
    return i;
}

L3Address Topologia::getAddress(int connId){
    L3Address ret;
    if(topo.empty()){return ret;}
    for(it=topo.begin();it <= topo.end(); it++){
        if(connId == (*it).connId){return (*it).ip;}
    }
    return ret;
}

bool Topologia::apagarConnId(int connId){
    if(topo.empty()){return false;}
    for(it=topo.begin();it <= topo.end(); it++){
        if(connId == (*it).connId){
            (*it).vulnerabilidade.clear();
            topo.erase(it);
            return true;
        }
    }
    return false;
}

bool Topologia::apagarId(int id){
    if(topo.empty()){return false;}
    for(it=topo.begin();it <= topo.end(); it++){
        if(id == (*it).id){
            (*it).vulnerabilidade.clear();
            topo.erase(it);
            return true;
        }
    }
    return false;
}

bool Topologia::apagarAddress(L3Address address){
    if(topo.empty()){return false;}
    for(it=topo.begin();it <= topo.end(); it++){
        if(address == (*it).ip){
            (*it).vulnerabilidade.clear();
            topo.erase(it);
            return true;
        }
    }
    return false;
}


L3Address Topologia::getAddressId(int id){
    L3Address ret;
    if(topo.empty()){return ret;}
    for(it=topo.begin();it <= topo.end(); it++){
        if(id == (*it).id){return (*it).ip;}
    }
    return ret;
}

TCPSocket *Topologia::getSocketPtr(int connId){
    if(topo.empty()){return nullptr;}
    for(it=topo.begin();it <= topo.end(); it++){
        if(connId == (*it).connId){return &(*it).socket;}
    }
    return nullptr;
}

bool Topologia::getEstruturaConnId(int connId, CellTopo* oi){
    if(topo.empty()){return false;}
    for(it=topo.begin();it <= topo.end(); it++){
        if(connId == (*it).connId){
            (*oi) = (*it);
                    //topo[it];
            return true;
        }
    }
    return false;
}

bool Topologia::getEstruturaId(int id, CellTopo* oi){
    if(topo.empty()){return false;}
    for(it=topo.begin();it <= topo.end(); it++){
        if(id == (*it).id){
            (*oi) = (*it);
                    //topo[it];
            return true;}
    }
    return false;
}

CellTopo* Topologia::getTopoById(int id){
    if(topo.empty()){return nullptr;}
    for(it=topo.begin();it <= topo.end(); it++){
        if(id == (*it).id){return &(*it);}
    }
    return nullptr;
}

bool Topologia::hasAddress(L3Address address){
    if(topo.empty()){return false;}
    for(it=topo.begin();it <= topo.end(); it++){
        if(address == (*it).ip){return true;}
    }
return false;
}

bool Topologia::hasConnId(int connId){
    if(topo.empty()){return false;}
    for(it=topo.begin();it <= topo.end(); it++){
        if(connId == (*it).connId){return true;}
    }
return false;
}

bool Topologia::hasVulnerabilidadeConnId(int vulnerabilidade, int connId){
    if(topo.empty()){return false;}
    std::vector<int>::iterator vit;

    for(it=topo.begin();it <= topo.end(); it++){
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
    for(it=topo.begin();it <= topo.end(); it++){
        (*it).vulnerabilidade.clear();
    }
    topo.clear();
}


Topologia::~Topologia() {
apagarTudo();
}
} /* namespace inet */
