

package inet.applications.simbo.Modulos.ControleSuper;
//package felipe;
//package Jteste12;

import inet.applications.contract.ITCPApp;


simple ControleSuper
{
    parameters:
        bool matlab = default(false);
        bool jacamo = default(false);
        bool javaconsole = default(false);
        int periodoComunicacao = default(60);	//Define quanto tempo deve passar na simulação por rodada.
}

