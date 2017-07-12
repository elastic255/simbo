#ifndef BOTNETCOMMANDSTRUCT_H_
#define BOTNETCOMMANDSTRUCT_H_

#include "inet/networklayer/common/L3Address.h"

namespace inet {
	
	//fixme: em desenvolvimento.
	//Estrutura para fazer pivoteamento da cadeia de comando da botnet. Define comando.
enum TypeCommandStruct: int  {TCS_INVASION,
                              TCS_ALIVE,
                              TCS_CHANGECC};

							  
							 
							 
	//fixme: em desenvolvimento.
	//Estrutura para fazer pivoteamento da cadeia de comando da botnet. Define Estrutura.
typedef struct BotnetCommandStruct{
    int id;
    TypeCommandStruct command;
    L3Address ip1;
    L3Address ip2;
}BotnetCommandStruct;

}

#endif /* TOPOLOGIA_H_ */
