import inet.common.lifecycle.LifecycleController;
import inet.common.scenario.ScenarioManager;
import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.physicallayer.ieee80211.packetlevel.*;
import inet.physicallayer.pathloss.*;

network UAVNetwork {
    
    parameters:
        int numHosts;

    submodules:
        radioMedium: Ieee80211ScalarRadioMedium {
            parameters:
                @display("p=100,50");
        }
        configurator: IPv4NetworkConfigurator {
            parameters:
                config = xml("<config><interface hosts='*' address='192.168.1.x' netmask='255.255.255.0'/></config>");
                @display("p=100,100");
        }
        routingTableRecorder: RoutingTableRecorder {
            parameters:
                @display("p=100,150");
        }
        lifecycleController: LifecycleController {
            parameters:
                @display("p=100,200");
        }
        scenarioManager: ScenarioManager {
            parameters:
                script = default(xml("<scenario/>"));
                @display("p=100,250");
        }
        host[numHosts]: UAVHost {
            parameters:
                @display("p=360,200");
        }
    connections allowunconnected:
}