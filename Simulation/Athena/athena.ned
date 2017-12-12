package inet.applications.simbo.Simulation.Athena;

import inet.applications.httptools.configurator.HttpController;
import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;
import inet.node.inet.StandardHost;

network Athena
{
    parameters:
        int numberOfHosts = default(1);
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
                config = xml("<config><interface hosts='*' address='145.236.x.x' netmask='255.255.x.x'/></config>");
                @display("i=block/cogwheel");
        }
        controller: HttpController {
            parameters:
                @display("p=62,60;i=block/cogwheel");
        }
        srv: StandardHost {
            parameters:
                @display("p=170,188;i=device/server_l");
        }
        cli[numberOfHosts]: StandardHost {
            parameters:
                @display("p=316,188;i=device/laptop_l");
        }
    connections:
        //cli[*].ethg++ <--> gigabitline <--> srv.ethg++;
        for i=0..numberOfHosts-1 {
                cli[i].ethg++ <--> gigabitline <--> srv.ethg++;
        }
}


