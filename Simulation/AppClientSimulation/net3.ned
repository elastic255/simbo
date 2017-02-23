package inet.applications.simbo.Simulation.AppClientSimulation;


import inet.applications.simbo.Modulos.ControleSuper.ControleSuper;
import inet.applications.simbo.Modulos.StandartHostBot.StandardHostBot;
import inet.applications.simbo.Modulos.Console.Console;
import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;
import inet.node.ethernet.EtherSwitch;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import ned.DatarateChannel;


network net3
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
        R1: Router {
            parameters:
                @display("p=461,122");
        }
        configurator: IPv4NetworkConfigurator {
            parameters:
                config = xmldoc("config.xml");
                @display("p=62,31");
        }
        switch: EtherSwitch {
            @display("p=792,121");
        }
        comp[3]: StandardHost;
        etherSwitch: EtherSwitch {
            @display("p=248,114");
        }
        comp1[3]: StandardHost;
        server1: StandardHost;
        server2: StandardHost;
        server3: StandardHost;
    connections:

		server1.ethg++ <--> C <--> switch.ethg++;
		server2.ethg++ <--> C <--> switch.ethg++;
		server3.ethg++ <--> C <--> switch.ethg++;
        for i=0..2 {
            comp[i].ethg++ <--> C <--> etherSwitch.ethg++;
        }
        for i=0..2 {
            comp[i].ethg++ <--> C <--> etherSwitch.ethg++;
            comp1[i].ethg++ <--> C <--> switch.ethg++;
        }

        etherSwitch.ethg++ <--> C <--> R1.ethg++;

        R1.ethg++ <--> C <--> switch.ethg++;
}
