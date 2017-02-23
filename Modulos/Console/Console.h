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

#ifndef CONSOLE_H_
#define CONSOLE_H_

#include "inet/applications/simbo/Modulos/Console/include/jni.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include <windows.h>

#include "inet/common/INETDefs.h"

namespace inet {

class INET_API Console : public cSimpleModule{
public:

    static JNIEnv* env;
    static JavaVM* jvm;
    static jint res;
    enum ConsoleMsg: int{CONSOLE_INICIA_JVM,CONSOLE_ESPERA};

    static char *pastaclasse;
    static char *pastajava;
    static bool JVMAtivada;

    virtual void initialize(int );
    virtual void handleMessage(cMessage *);
    virtual void finish();

    static int levantaJVM(const char *, const char *);
    static void destroy();
    static void ConsoleInput();
    static void write(const char *);

    virtual bool print();
    virtual void remarca(Console::ConsoleMsg);

    Console();
    virtual ~Console();
};

} /* namespace inet */

#endif /* CONSOLE_H_ */
