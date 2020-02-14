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

package inet.tutorials.configurator.configuratortest;

import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;
import inet.node.ethernet.Eth100M;
import inet.node.ethernet.Eth10G;
import inet.node.ethernet.Eth1G;
import inet.node.ethernet.EtherSwitch;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import inet.visualizer.networklayer.RoutingTableCanvasVisualizer;


network nineteen
{
    @display("bgb=2504.3225,1393.2375");
    submodules:
        Area1Subnet1Host0: StandardHost {
            @display("p=380.2925,376.7875");
        }
        Area1Subnet1Host1: StandardHost {
            @display("p=380.2925,227.82501");
        }
        Area1Subnet1Host2: StandardHost {
            @display("p=380.2925,59.585003");
        }
        Area1Switch1: EtherSwitch {
            @display("p=676.465,257.61752");
        }
        Area2Subnet1Host1: StandardHost {
            @display("p=953.36005,1307.3651");
        }
        Area2Subnet1Host2: StandardHost {
            @display("p=718.525,1307.3651");
        }
        Area2Subnet1Host3: StandardHost {
            @display("p=476.68002,1307.3651");
        }
        Area2Switch1: EtherSwitch {
            @display("p=807.9025,1100.5701");
        }
        Area3Subnet1Host1: StandardHost {
            @display("p=2216.9126,986.65753");
        }
        Area3Subnet1Host2: StandardHost {
            @display("p=2244.9526,755.3275");
        }
        Area3Subnet1Host3: StandardHost {
            @display("p=2265.9827,601.10754");
        }
        Area3Switch1: EtherSwitch {
            @display("p=1838.3726,623.89");
        }
        Area1Subnet2Host1: StandardHost {
            @display("p=368.02502,601.10754");
        }
        Area1Subnet2Host2: StandardHost {
            @display("p=368.02502,730.79254");
        }
        Area1Subnet2Host3: StandardHost {
            @display("p=332.975,914.80505");
        }
        Area1Switch2: EtherSwitch {
            @display("p=608.11755,765.8425");
        }
        Area2Subnet2Host1: StandardHost {
            @display("p=1961.0476,1281.0775");
        }
        Area2Subnet2Host2: StandardHost {
            @display("p=1659.6176,1281.0775");
        }
        Area2Subnet2Host3: StandardHost {
            @display("p=1377.4651,1307.3651");
        }
        Area2Switch2: EtherSwitch {
            @display("p=1659.6176,1055.005");
        }
        Area3Subnet2Host1: StandardHost {
            @display("p=2080.2175,117.4175");
        }
        Area3Subnet2Host2: StandardHost {
            @display("p=2197.635,212.0525");
        }
        Area3Subnet2Host3: StandardHost {
            @display("p=2216.9126,336.48");
        }
        Area3Switch2: EtherSwitch {
            @display("p=1878.68,346.99503");
        }
        Area1Router: Router {
            @display("p=1051.5,502.96753");
        }
        Area2Router: Router {
            @display("p=1309.1176,914.80505");
        }
        Area3Router: Router {
            @display("p=1591.27,483.69003");
        }
        Backbonerouter: Router {
            @display("p=1309.1176,686.98004;i=abstract/router2");
        }
        configurator: IPv4NetworkConfigurator {
            @display("p=124.427505,89.3775");
        }
        visualizer: RoutingTableCanvasVisualizer {
            @display("p=124.427505,282.15253");
        }
    connections:
        Area2Subnet1Host1.ethg++ <--> Eth100M <--> Area2Switch1.ethg++;
        Area2Subnet1Host2.ethg++ <--> Eth100M <--> Area2Switch1.ethg++;
        Area2Subnet1Host3.ethg++ <--> Eth100M <--> Area2Switch1.ethg++;
        Area1Switch2.ethg++ <--> Eth100M <--> Area1Subnet2Host1.ethg++;
        Area1Subnet2Host2.ethg++ <--> Eth100M <--> Area1Switch2.ethg++;
        Area1Switch2.ethg++ <--> Eth100M <--> Area1Subnet2Host3.ethg++;
        Area1Switch1.ethg++ <--> Eth100M <--> Area1Subnet1Host0.ethg++;
        Area1Switch1.ethg++ <--> Eth100M <--> Area1Subnet1Host1.ethg++;
        Area1Switch1.ethg++ <--> Eth100M <--> Area1Subnet1Host2.ethg++;
        Area1Switch1.ethg++ <--> Eth100M <--> Area1Router.ethg++;
        Area1Switch2.ethg++ <--> Eth100M <--> Area1Router.ethg++;
        Area1Router.ethg++ <--> Eth100M <--> Backbonerouter.ethg++;
        Area2Switch1.ethg++ <--> Eth100M <--> Area2Router.ethg++;
        Area2Router.ethg++ <--> Eth100M <--> Area2Switch2.ethg++;
        Area2Switch2.ethg++ <--> Eth100M <--> Area2Subnet2Host1.ethg++;
        Area2Switch2.ethg++ <--> Eth100M <--> Area2Subnet2Host2.ethg++;
        Area2Switch2.ethg++ <--> Eth100M <--> Area2Subnet2Host3.ethg++;
        Area3Switch1.ethg++ <--> Eth100M <--> Area3Subnet1Host1.ethg++;
        Area3Switch1.ethg++ <--> Eth100M <--> Area3Subnet1Host2.ethg++;
        Area3Switch1.ethg++ <--> Eth100M <--> Area3Subnet1Host3.ethg++;
        Area3Switch1.ethg++ <--> Eth100M <--> Area3Router.ethg++;
        Area3Router.ethg++ <--> Eth100M <--> Area3Switch2.ethg++;
        Area3Switch2.ethg++ <--> Eth100M <--> Area3Subnet2Host1.ethg++;
        Area3Switch2.ethg++ <--> Eth100M <--> Area3Subnet2Host2.ethg++;
        Area3Switch2.ethg++ <--> Eth100M <--> Area3Subnet2Host3.ethg++;
        Area3Router.ethg++ <--> Eth100M <--> Backbonerouter.ethg++;
        Area2Router.ethg++ <--> Eth100M <--> Backbonerouter.ethg++;
}
