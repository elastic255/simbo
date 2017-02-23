package inet.applications.simbo.Simulation.Quinto;


import inet.applications.simbo.Modulos.ControleSuper.ControleSuper;
import inet.applications.simbo.Modulos.StandartHostBot.StandardHostBot;
import inet.applications.simbo.Modulos.Console.Console;
import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;
import inet.node.ethernet.EtherSwitch;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import ned.DatarateChannel;


network net2
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
                @display("p=331.40625,114.84375");
        }
        configurator: IPv4NetworkConfigurator {
            parameters:
                config = xmldoc("config.xml");
                @display("p=402.5,30.625");
        }
        switch: EtherSwitch {
            @display("p=420,113.75");
        }
        server: StandardHostBot {
            @display("p=462.65625,30.625");
        }

        comp[3]: StandardHostBot;
        etherSwitch: EtherSwitch {
            @display("p=248,114");
        }
        comp1[2]: StandardHostBot {
        }
        controleSuper: ControleSuper {
            @display("p=320.46875,30.625");
        }
    connections:

        for i=0..2 {
            comp[i].ethg++ <--> C <--> etherSwitch.ethg++;
        }
        for i=0..1 {
            comp[i].ethg++ <--> C <--> etherSwitch.ethg++;
            comp1[i].ethg++ <--> C <--> switch.ethg++;
        }

        switch.ethg++ <--> C <--> server.ethg++;
        inicial.ethg++ <--> C <--> etherSwitch.ethg++;
        etherSwitch.ethg++ <--> C <--> R1.ethg++;

        R1.ethg++ <--> C <--> switch.ethg++;
}
