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
import inet.node.ethernet.EtherSwitch;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import inet.visualizer.networklayer.RoutingTableCanvasVisualizer;


network twentysix
{
    @display("bgb=1442,808");
    submodules:
        Router1: Router {
            @display("p=661,333");
        }
        Router3: Router {
            @display("p=918,333");
        }
        Router2: Router {
            @display("p=661,525");
        }
        Router4: Router {
            @display("p=918,525");
        }
        Area1Subet1Switch: EtherSwitch {
            @display("p=555,183");
        }
        Area1Subnet2Switch: EtherSwitch {
            @display("p=471,270");
        }
        Area2Subnet1Switch: EtherSwitch {
            @display("p=510,572");
        }
        Area2Subnet2Switch: EtherSwitch {
            @display("p=622,660");
        }
        Area3Subnet1Switch: EtherSwitch {
            @display("p=985,169");
        }
        Area3Subnet2Switch: EtherSwitch {
            @display("p=1091,261");
        }
        Area4Subnet1Switch: EtherSwitch {
            @display("p=1091,553");
        }
        Area4Subnet2Switch: EtherSwitch {
            @display("p=1005,642");
        }
        Area1Subnet1Host1: StandardHost {
            @display("p=489,59");
        }
        Area1Subnet1Host2: StandardHost {
            @display("p=415,131");
        }
        Area1Subnet2Host1: StandardHost {
            @display("p=297,274");
        }
        Area1Subnet2Host2: StandardHost {
            @display("p=337,403");
        }
        Area2Subnet1Host1: StandardHost {
            @display("p=343,541");
        }
        Area2Subnet1Host2: StandardHost {
            @display("p=372,654");
        }
        Area2Subnet2Host1: StandardHost {
            @display("p=549,753");
        }
        Area2Subnet2Host2: StandardHost {
            @display("p=742,753");
        }
        Area3Subnet1Host1: StandardHost {
            @display("p=945,69");
        }
        Area3Subnet1Host2: StandardHost {
            @display("p=1056,87");
        }
        Area3Subnet2Host1: StandardHost {
            @display("p=1175,188");
        }
        Area3Subnet2Host2: StandardHost {
            @display("p=1211,300");
        }
        Area4Subnet1Host1: StandardHost {
            @display("p=1192,494");
        }
        Area4Subnet1Host2: StandardHost {
            @display("p=1204,588");
        }
        Area4Subnet2Host2: StandardHost {
            @display("p=1104,703");
        }
        Area4Subnet2Host1: StandardHost {
            @display("p=967,730");
        }
        configurator: IPv4NetworkConfigurator {
            @display("p=63,60");
        }
        visualizer: RoutingTableCanvasVisualizer {
            @display("p=64,150");
        }
    connections:
        Router1.ethg++ <--> Eth100M <--> Router3.ethg++;
        Router2.ethg++ <--> Eth100M <--> Router4.ethg++;
        Router1.ethg++ <--> Eth100M <--> Router2.ethg++;
        Router3.ethg++ <--> Eth100M <--> Router4.ethg++;
        Router1.ethg++ <--> Eth100M <--> Area1Subet1Switch.ethg++;
        Router1.ethg++ <--> Eth100M <--> Area1Subnet2Switch.ethg++;
        Router3.ethg++ <--> Eth100M <--> Area3Subnet1Switch.ethg++;
        Router3.ethg++ <--> Eth100M <--> Area3Subnet2Switch.ethg++;
        Router4.ethg++ <--> Eth100M <--> Area4Subnet1Switch.ethg++;
        Router4.ethg++ <--> Eth100M <--> Area4Subnet2Switch.ethg++;
        Router2.ethg++ <--> Eth100M <--> Area2Subnet2Switch.ethg++;
        Area2Subnet1Switch.ethg++ <--> Eth100M <--> Router2.ethg++;
        Area1Subet1Switch.ethg++ <--> Eth100M <--> Area1Subnet1Host1.ethg++;
        Area1Subet1Switch.ethg++ <--> Eth100M <--> Area1Subnet1Host2.ethg++;
        Area1Subnet2Switch.ethg++ <--> Eth100M <--> Area1Subnet2Host1.ethg++;
        Area1Subnet2Switch.ethg++ <--> Eth100M <--> Area1Subnet2Host2.ethg++;
        Area2Subnet1Switch.ethg++ <--> Eth100M <--> Area2Subnet1Host1.ethg++;
        Area2Subnet1Switch.ethg++ <--> Eth100M <--> Area2Subnet1Host2.ethg++;
        Area2Subnet2Switch.ethg++ <--> Eth100M <--> Area2Subnet2Host1.ethg++;
        Area2Subnet2Switch.ethg++ <--> Eth100M <--> Area2Subnet2Host2.ethg++;
        Area4Subnet2Switch.ethg++ <--> Eth100M <--> Area4Subnet2Host1.ethg++;
        Area4Subnet2Switch.ethg++ <--> Eth100M <--> Area4Subnet2Host2.ethg++;
        Area4Subnet1Switch.ethg++ <--> Eth100M <--> Area4Subnet1Host1.ethg++;
        Area4Subnet1Switch.ethg++ <--> Eth100M <--> Area4Subnet1Host2.ethg++;
        Area3Subnet2Switch.ethg++ <--> Eth100M <--> Area3Subnet2Host1.ethg++;
        Area3Subnet2Switch.ethg++ <--> Eth100M <--> Area3Subnet2Host2.ethg++;
        Area3Subnet1Switch.ethg++ <--> Eth100M <--> Area3Subnet1Host1.ethg++;
        Area3Subnet1Switch.ethg++ <--> Eth100M <--> Area3Subnet1Host2.ethg++;
}
