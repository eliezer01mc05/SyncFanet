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


network Simple
{
    @display("bgb=1028,400");
    submodules:
        configurator: IPv4NetworkConfigurator {
            @display("p=100,100");
        }
        visualizer: RoutingTableCanvasVisualizer {
            @display("p=100,200");
        }
        Host0: StandardHost {
            @display("p=274,69");
        }
        Host1: StandardHost {
            @display("p=266,294");
        }
        Host2: StandardHost {
            @display("p=957,294");
        }
        Host3: StandardHost {
            @display("p=957,62");
        }
        Switch0: EtherSwitch {
            @display("p=453,186");
        }
        Switch1: EtherSwitch {
            @display("p=798,186");
        }
    connections:

        Host1.ethg++ <--> Eth100M <--> Switch0.ethg++;
        Host0.ethg++ <--> Eth100M <--> Switch0.ethg++;
        Switch0.ethg++ <--> Eth100M <--> Switch1.ethg++;
        Switch1.ethg++ <--> Eth100M <--> Host3.ethg++;
        Switch1.ethg++ <--> Eth100M <--> Host2.ethg++;
}
