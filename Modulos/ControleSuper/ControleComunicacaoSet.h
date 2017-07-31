//
//  Status Code:    2 - recem criado.
//                  0 - usado.
//                  1 - pronto para o loop.
//
// 

#ifndef CONTROLECOMUNICACAOSET_H_
#define CONTROLECOMUNICACAOSET_H_

#include "inet/applications/simbo/Modulos/ControleSuper/ControleComunicacao.h"

namespace inet {

class ControleComunicacaoSet {
public:

    //Não utilizado em plenitude, precisa desenvolver.
    enum Status: int {  Ativo,
                        Inativo,
                        RecemCriado};


    std::vector<ControleComunicacao*> listaModulos; //Lista todas as conexões.
    std::vector<Status> estado;                     //Não utilizado em plenitude, precisa desenvolver.
    int tamanho;                                    //Número de conexões ativas.



    ControleComunicacaoSet();
    ControleComunicacaoSet(ControleComunicacao *a);
    virtual void add(ControleComunicacao *com);
    virtual void criar(const char *a);
    virtual void fechar();
    virtual void iniciar();
    virtual bool isprosseguir();
    virtual void loop();
    virtual void reflesh();
    virtual void finalizar();



    virtual ~ControleComunicacaoSet();
};

} /* namespace inet */

#endif /* CONTROLECOMUNICACAOSET_H_ */
