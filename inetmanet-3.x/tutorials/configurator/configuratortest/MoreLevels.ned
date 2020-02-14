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


network MoreLevels
{
    @display("bgb=940,501");
    submodules:
        configurator: IPv4NetworkConfigurator {
            @display("p=73,72");
        }
        visualizer: RoutingTableCanvasVisualizer {
            @display("p=65,153");
        }
        Host0: StandardHost {
            @display("p=96,419");
        }
        Host1: StandardHost {
            @display("p=271,177");
        }
        Host2: StandardHost {
            @display("p=104,307");
        }
        Host3: StandardHost {
            @display("p=271,80");
        }
        Router0: Router {
            @display("p=219,379");
        }
        Router1: Router {
            @display("p=393,136");
        }
        Router2: Router {
            @display("p=422,307");
        }
        Host4: StandardHost {
            @display("p=377,442");
        }
        Router3: Router {
            @display("p=571,226");
        }
        Host5: StandardHost {
            @display("p=610,346");
        }
        Host6: StandardHost {
            @display("p=623,103");
        }
        Router4: Router {
            @display("p=787,286");
        }
        Host7: StandardHost {
            @display("p=717,435");
        }
        Host8: StandardHost {
            @display("p=863,435");
        }
    connections:
        Host0.ethg++ <--> Eth100M <--> Router0.ethg++;
        Host2.ethg++ <--> Eth100M <--> Router0.ethg++;
        Host3.ethg++ <--> Eth100M <--> Router1.ethg++;
        Host1.ethg++ <--> Eth100M <--> Router1.ethg++;
        Router0.ethg++ <--> Eth100M <--> Router2.ethg++;
        Host4.ethg++ <--> Eth100M <--> Router2.ethg++;
        Router1.ethg++ <--> Eth100M <--> Router3.ethg++;
        Router2.ethg++ <--> Eth100M <--> Router3.ethg++;
        Host5.ethg++ <--> Eth100M <--> Router3.ethg++;
        Host6.ethg++ <--> Eth100M <--> Router3.ethg++;
        Router4.ethg++ <--> Eth100M <--> Host7.ethg++;
        Router4.ethg++ <--> Eth100M <--> Host8.ethg++;
        Router4.ethg++ <--> Eth100M <--> Router3.ethg++;
}
