package inet.applications.simbo.Simulation.Athena;

import inet.applications.httptools.configurator.HttpController;
import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;
import inet.node.ethernet.EtherSwitch;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;

network Athena
{
    parameters:
        int numberOfHosts = default(1);
        int numberOfLocalNet = default(1);
        double connectedness = default(0.5);
        @display("bgb=474,374");
    types:
        channel gigabitline extends ned.DatarateChannel
        {
            parameters:
                delay = 0.1us;
                datarate = 1000Mbps;
        }
    submodules:
        configurator: IPv4NetworkConfigurator {
            parameters:
                //                moduleTypes = "StandardHost";
                //                nonIPModuleTypes = "";
                //config = xml("<config><interface hosts='**' address='143.x.x.x' netmask='255.x.x.x'/></config>");
                @display("i=block/cogwheel;p=42,93");
        }
        controller: HttpController {
            parameters:
                @display("p=42,22;i=block/cogwheel");
        }
        C2: StandardHost {
            parameters:
                @display("p=45,300;i=device/server_l");
        }
        router: Router {
            @display("p=133,206");
        }
        srv: StandardHost {
            parameters:
                @display("p=133,30;i=device/server_l");
        }
        etherSwitch: EtherSwitch {
            @display("p=133,115");
        }
        etherSwitch1: EtherSwitch {
            @display("p=133,305");
        }
        localRouter[numberOfLocalNet]: Router {
        }
        localSwitch[numberOfLocalNet]: EtherSwitch {
        }
        cli[numberOfHosts]: StandardHost {
        }
    connections allowunconnected:
        for i=0..numberOfLocalNet-2 {
            localRouter[i].ethg++ <--> gigabitline <--> localRouter[i+1].ethg++ if uniform(0,1) < connectedness;
        }
        localRouter[0].ethg++ <--> gigabitline <--> localRouter[numberOfLocalNet-1].ethg++ if numberOfLocalNet > 1 && uniform(0,1) < connectedness;
        for i=0..numberOfHosts-1 {
            cli[i].ethg++ <--> gigabitline <--> localSwitch[intuniform(0, numberOfLocalNet-1)].ethg++ ;
        }
        for j=0..numberOfLocalNet-1 {
            localSwitch[j].ethg++ <--> gigabitline <--> localRouter[j].ethg++;
            localRouter[j].ethg++ <--> gigabitline <--> router.ethg++ if uniform(0,1) < connectedness;
        }
        localRouter[0].ethg++ <--> gigabitline <--> router.ethg++;
        srv.ethg++ <--> gigabitline <--> etherSwitch.ethg++;
        etherSwitch.ethg++ <--> gigabitline <--> router.ethg++;
        etherSwitch1.ethg++ <--> gigabitline <--> router.ethg++;
        C2.ethg++ <--> gigabitline <--> etherSwitch1.ethg++;
}


