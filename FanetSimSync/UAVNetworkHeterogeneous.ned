import base.connectionManager.ConnectionManager;
//import inet.world.radio.ChannelControl;
import inet.node.inet.WirelessHost;
import samples.hybrid.WirelessHostWithBattery;


network UAVNetworkHeterogeneous
{
    parameters:
        double playgroundSizeX @unit(m); // x size of the area the nodes are in (in meters)
        double playgroundSizeY @unit(m); // y size of the area the nodes are in (in meters)
        double playgroundSizeZ @unit(m); // z size of the area the nodes are in (in meters)
        @display("bgb=639,465,white;bgp=10,50");
    submodules:
        channelWifi: ConnectionManager {
            parameters:
                @display("p=132,25");
        }
        channelZigbee: ConnectionManager {
            parameters:
                @display("p=264,25");
        }
        channelBLE: ConnectionManager {
            parameters:
                @display("p=413,28");
        }
        //channelControl: ChannelControl {
          //  @display("p=583,24");
        //}
        
        
        hostWifi_Master: WirelessHost {
            @display("r=,,#707070;p=328,93");
        }
        hostWifi_Zigbee: WirelessHost {
            @display("r=,,#707070;p=158,208");
        }
        hostWifi_Ble: WirelessHost {
            @display("r=,,#707070;p=502,193");
        }
        hostWifi: WirelessHost {
            @display("r=,,#707070;p=322,201");
        }
        hostZigbee_1: WirelessHostWithBattery {
            @display("p=49,346");
        }
        hostZigbee_2: WirelessHostWithBattery {
            @display("p=225,346");
        }
        hostBle_1: WirelessHost {
            @display("r=,,#707070;p=389,346");
        }
        hostBle_2: WirelessHost {
            @display("r=,,#707070;p=558,352");
        }
    connections:
        

}
