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
/*

#include "inet/applications/simbo/Modulos/Console/Console.h"

namespace inet {

Define_Module(Console);

bool Console::JVMAtivada = false;
char *Console::pastaclasse = NULL;
char *Console::pastajava = NULL;

JNIEnv* Console::env = NULL;
JavaVM* Console::jvm = NULL;
jint Console::res = 0;

Console::Console() {
    // TODO Auto-generated constructor stub
}

void Console::initialize(int stage){

    cSimpleModule::initialize(stage);
    cMessage *timer = new cMessage("console_msg");
    simtime_t d = simTime();
    timer->setKind(Console::CONSOLE_INICIA_JVM);
    scheduleAt(d+2, timer);

}

void Console::remarca(Console::ConsoleMsg tipo){
    cMessage *timer = new cMessage("console_msg");
    simtime_t d = simTime();
    timer->setKind(tipo);
    scheduleAt(d+2, timer);
}

void Console::handleMessage(cMessage *msg){

    //int rc;
    //pthread_t threads;

    switch(msg->getKind()){

        case Console::CONSOLE_INICIA_JVM:
            Console::levantaJVM( "D:\\desenvolvimento\\omnet\\omnetpp-5.0b2\\samples\\inet\\src\\inet\\applications\\felipe\\Modulos\\Console\\JavaFiles",
                        "C:\\Program Files (x86)\\Java\\jdk1.8.0_65\\jre\\bin\\server\\jvm.dll"
                        );

            Console::write("oi!!!!!!!!!!!!!!!!!!!!!!!!!!!");
            //rc = pthread_create(&threads, NULL,teste, NULL);
            remarca(Console::CONSOLE_ESPERA);
            break;

        case Console::CONSOLE_ESPERA:
            remarca(Console::CONSOLE_ESPERA);
            break;

        default:
            throw cRuntimeError("Formato não válido para handle Console");
    }

    delete msg;
}

void Console::ConsoleInput(){

    if(!Console::JVMAtivada){return;}
    jclass cls = env->FindClass("JavaConsole");

    if (cls == NULL) { // can't find class.
            printf("falha1\n");
            return;
    }

    jmethodID mid = env->GetStaticMethodID(cls, "console","()V");

    if (mid == NULL) { // no main method.
        printf("falha2\n");
        return;
    }


    env->CallStaticVoidMethod(cls, mid,NULL);

}

void Console::write(const char *frase){

    if(!Console::JVMAtivada){return;}
    jclass cls = env->FindClass("JavaConsole");
    jclass stringClass = env->FindClass("java/lang/String");

    if (cls == NULL) { // can't find class.
            printf("falha1\n");
            return;
    }
    if (stringClass == NULL) { // can't find class.
            printf("falha1b\n");
            return;
    }

    jmethodID mid = env->GetStaticMethodID(cls, "write","(Ljava/lang/String;)V");
    if (mid == NULL) { // no main method.
        printf("falha2\n");
        return;
    }

    jstring arg = env->NewStringUTF(frase);
    if (arg == NULL) {
        printf("falha3\n");
    }

    env->CallStaticVoidMethod(cls, mid,arg);

}


int Console::levantaJVM(const char *pastaclasse2, const char *pastajava2){

    if(!Console::JVMAtivada){
        //PastaClasse
        char pastaclasse3[strlen(pastaclasse2)+30]="";
        strcat(pastaclasse3,"-Djava.class.path=");
        strcat(pastaclasse3,pastaclasse2);
        strcat(pastaclasse3,"\\.");

        //PastaJava
        char pastajava3[strlen(pastajava2)];
        strcpy(pastajava3,pastajava2);

        //Salva as os caminhos
        Console::pastaclasse = pastaclasse3;
        Console::pastajava = pastajava3;

        //Configura as opções JVM
        JavaVMInitArgs vm_args;
        JavaVMOption options[1];
        options[0].optionString = pastaclasse3;
        vm_args.version = JNI_VERSION_1_6;
        vm_args.options = options;
        vm_args.nOptions = 1;
        vm_args.ignoreUnrecognized = JNI_TRUE;

        HINSTANCE hinstLib = NULL;
        hinstLib = LoadLibrary(TEXT(pastajava2));
        int erro =  GetLastError();

        if(hinstLib != NULL){

            typedef jint (JNICALL *PtrCreateJavaVM)(JavaVM **, void **, void *);
            PtrCreateJavaVM ptrCreateJavaVM = (PtrCreateJavaVM)GetProcAddress(hinstLib,"JNI_CreateJavaVM");
            erro =  GetLastError();
            if(ptrCreateJavaVM == NULL){
                printf("hisnst = %d\n", (int)hinstLib);
                printf("Erro = %d\n", erro);
                printf("funct = %d\n", (int)ptrCreateJavaVM);
                return(erro);
            }
            res = ptrCreateJavaVM(&jvm, (void**)&env, &vm_args);
            Console::JVMAtivada = true;
            //Confere se a JVM foi criada.
            if (res > 0)
            {
                return 0;
            }else{return(-1);}
        }else{return erro;}
    }
  return -2;
}

bool Console::print(){

    jclass cls = env->FindClass("JavaConsole");
    jclass stringClass = env->FindClass("java/lang/String");


    if (cls == NULL) { // can't find class.
            printf("falha1\n");
    }

    if (stringClass == NULL) { // can't find class.
            printf("falha1b\n");
            return false;
    }


    jmethodID mid = env->GetStaticMethodID(cls, "Bob","([Ljava/lang/String;)I");


    if (mid == NULL) { // no main method.
        printf("falha2\n");
        return false;
    }

    jstring jstr = env->NewStringUTF(" from C++ !!!!!!!");
    if (jstr == NULL) {
        printf("falha3\n");
    }



    jobjectArray args = env->NewObjectArray(1, stringClass, jstr);

    if (args == NULL) {
        printf("falha4\n");
        return false;
    }

         int a = (int) env->CallStaticIntMethod(cls, mid,args);

    return true;
}


void Console::destroy() {
  if(!Console::JVMAtivada){return;}
  if (env->ExceptionOccurred()) {
    env->ExceptionDescribe();
  }
  jvm->DestroyJavaVM();
  Console::JVMAtivada = false;
  printf("máquina virtual destruida");
}

void Console::finish(){
    Console::destroy();
}

Console::~Console() {
}

} */  /* namespace inet */

