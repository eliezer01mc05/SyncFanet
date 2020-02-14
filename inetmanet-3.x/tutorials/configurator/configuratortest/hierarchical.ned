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


network Hierarchical
{
    @display("bgb=1045,532");
    submodules:
        configurator: IPv4NetworkConfigurator {
            @display("p=100,100");
        }
        visualizer: RoutingTableCanvasVisualizer {
            @display("p=100,200");
        }
        Host0: StandardHost {
            @display("p=422,60");
        }
        Host1: StandardHost {
            @display("p=593,60");
        }
        Host2: StandardHost {
            @display("p=873,83");
        }
        Router0: Router {
            @display("p=511,185");
        }
        Router1: Router {
            @display("p=756,184");
        }
        Router2: Router {
            @display("p=638,280");
        }
        Host3: StandardHost {
            @display("p=568,441");
        }
        Router3: Router {
            @display("p=440,350");
        }
        Host4: StandardHost {
            @display("p=251,251");
        }
        Host5: StandardHost {
            @display("p=257,441");
        }
        Host6: StandardHost {
            @display("p=774,441");
        }
    connections:
        Host5.ethg++ <--> Eth100M <--> Router3.ethg++;
        Host4.ethg++ <--> Eth100M <--> Router3.ethg++;
        Router3.ethg++ <--> Eth100M <--> Router2.ethg++;
        Router2.ethg++ <--> Eth100M <--> Router0.ethg++;
        Router0.ethg++ <--> Eth100M <--> Host0.ethg++;
        Router0.ethg++ <--> Eth100M <--> Host1.ethg++;
        Router2.ethg++ <--> Eth100M <--> Router1.ethg++;
        Router1.ethg++ <--> Eth100M <--> Host2.ethg++;
        Router2.ethg++ <--> Eth100M <--> Host6.ethg++;
        Router2.ethg++ <--> Eth100M <--> Host3.ethg++;
}
