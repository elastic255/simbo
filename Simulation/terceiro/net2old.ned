package inet.applications.simbo.Simulation.terceiro;


import inet.applications.simbo.Modulos.ControleSuper.ControleSuper;
import inet.applications.simbo.Modulos.StandartHostBot.StandardHostBot;
import inet.applications.simbo.Modulos.Console.Console;
import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;
import inet.node.ethernet.EtherSwitch;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import ned.DatarateChannel;


network net2old
{
    parameters:
        @display("bgb=871,327");
    types:
        channel C extends DatarateChannel
        {
            datarate = 10Mbps;
            delay = 0.1us;
        }
    submodules:
        inicial: StandardHostBot {
            parameters:
                @display("p=247.1875,30.625;i=device/pc2");
        }
        R1: Router {
            parameters:
                @display("p=381.71875,114.84375");
        }
        configurator: IPv4NetworkConfigurator {
            parameters:
                config = xmldoc("config.xml");
                @display("p=402.5,30.625");
        }
        switch: EtherSwitch {
            @display("p=505.3125,113.75");
        }
        server: StandardHost {
            @display("p=505.3125,30.625");
        }

        comp: StandardHost {
            @display("p=125.78125,114.84375");
        }
        etherSwitch: EtherSwitch {
            @display("p=248,114");
        }
        comp1: StandardHost {
            @display("p=595,114.84375");
        }
    connections:


        comp.ethg++ <--> C <--> etherSwitch.ethg++;
        comp.ethg++ <--> C <--> etherSwitch.ethg++;
        comp1.ethg++ <--> C <--> switch.ethg++;

        switch.ethg++ <--> C <--> server.ethg++;
        inicial.ethg++ <--> C <--> etherSwitch.ethg++;
        etherSwitch.ethg++ <--> C <--> R1.ethg++;

        R1.ethg++ <--> C <--> switch.ethg++;
}
