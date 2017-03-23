#ifndef BOTNETCOMMANDSTRUCT_H_
#define BOTNETCOMMANDSTRUCT_H_

#include "inet/networklayer/common/L3Address.h"

namespace inet {
enum TypeCommandStruct: int  {TCS_INVASION,
                              TCS_ALIVE,
                              TCS_CHANGECC};

typedef struct BotnetCommandStruct{
    int id;
    TypeCommandStruct command;
    L3Address ip1;
    L3Address ip2;
}BotnetCommandStruct;

}

#endif /* TOPOLOGIA_H_ */
