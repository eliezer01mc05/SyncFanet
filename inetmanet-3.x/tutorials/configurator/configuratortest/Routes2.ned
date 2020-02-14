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
import inet.node.ethernet.Eth10M;
import inet.node.ethernet.EtherSwitch;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import inet.visualizer.networklayer.RoutingTableCanvasVisualizer;


network Routes2
{
    @display("bgb=1610.5125,874.96497");
    submodules:
        configurator: IPv4NetworkConfigurator {
            @display("p=100,100");
        }
        visualizer: RoutingTableCanvasVisualizer {
            @display("p=100,200");
        }
        Host0: StandardHost {
            @display("p=179.48,352.55");
        }
        Host1: StandardHost {
            @display("p=179.48,520.8125");
        }
        Host2: StandardHost {
            @display("p=179.48,693.88245");
        }
        Host3: StandardHost {
            @display("p=536.83746,99.354996");
        }
        Router0: Router {
            @display("p=578.5025,576.89996");
        }
        Router1: Router {
            @display("p=882.9775,576.89996");
        }
        Router2: Router {
            @display("p=730.74,370.1775");
        }
        Host4: StandardHost {
            @display("p=778.81494,99.354996");
        }
        Host5: StandardHost {
            @display("p=977.52496,99.354996");
        }
        Host6: StandardHost {
            @display("p=1249.95,370.1775");
        }
        Host7: StandardHost {
            @display("p=1272.3849,549.6575");
        }
        Host8: StandardHost {
            @display("p=1261.1675,737.14996");
        }
        Switch0: EtherSwitch {
            @display("p=427.8675,575.2975");
        }
        Switch1: EtherSwitch {
            @display("p=1072.0725,575.2975");
        }
        Switch2: EtherSwitch {
            @display("p=730.74,227.555");
        }
    connections:
        Host2.ethg++ <--> Eth100M <--> Switch0.ethg++;
        Host1.ethg++ <--> Eth100M <--> Switch0.ethg++;
        Host0.ethg++ <--> Eth100M <--> Switch0.ethg++;
        Switch0.ethg++ <--> Eth100M <--> Router0.ethg++;
        Router0.ethg++ <--> Eth100M <--> Router2.ethg++;
        Router2.ethg++ <--> Eth100M <--> Switch2.ethg++;
        Switch2.ethg++ <--> Eth100M <--> Host3.ethg++;
        Switch2.ethg++ <--> Eth100M <--> Host4.ethg++;
        Switch2.ethg++ <--> Eth100M <--> Host5.ethg++;
        Router2.ethg++ <--> Eth100M <--> Router1.ethg++;
        Router1.ethg++ <--> Eth100M <--> Switch1.ethg++;
        Switch1.ethg++ <--> Eth100M <--> Host6.ethg++;
        Switch1.ethg++ <--> Eth100M <--> Host7.ethg++;
        Switch1.ethg++ <--> Eth100M <--> Host8.ethg++;
        Router0.ethg++ <--> Eth10M <--> Router1.ethg++;
}
