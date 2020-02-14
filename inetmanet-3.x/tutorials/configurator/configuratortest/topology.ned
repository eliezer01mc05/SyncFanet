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
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import inet.visualizer.networklayer.RoutingTableCanvasVisualizer;


network Topology
{
    @display("bgb=988,400");
    submodules:
        Host0: StandardHost {
            @display("p=254,146");
        }
        Host1: StandardHost {
            @display("p=315,299");
        }
        Host2: StandardHost {
            @display("p=516,185");
        }
        Host3: StandardHost {
            @display("p=863,310");
        }
        Host4: StandardHost {
            @display("p=883,82");
        }
        Router0: Router {
            @display("p=380,201");
        }
        Router1: Router {
            @display("p=771,191");
        }
        Router2: Router {
            @display("p=574,332");
        }
        configurator: IPv4NetworkConfigurator {
            @display("p=100,100");
        }
        visualizer: RoutingTableCanvasVisualizer {
            @display("p=100,200");
        }
    connections:
        Host1.ethg++ <--> Eth100M <--> Router0.ethg++;
        Host0.ethg++ <--> Eth100M <--> Router0.ethg++;
        Host2.ethg++ <--> Eth100M <--> Router0.ethg++;
        Router0.ethg++ <--> Eth100M <--> Router2.ethg++;
        Router2.ethg++ <--> Eth100M <--> Router1.ethg++;
        Router1.ethg++ <--> Eth100M <--> Host3.ethg++;
        Router1.ethg++ <--> Eth100M <--> Host4.ethg++;
}
