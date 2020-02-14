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


network Routes
{
    @display("bgb=1278.39,639.83997");
    submodules:
        configurator: IPv4NetworkConfigurator {
            @display("p=100,100");
        }
        visualizer: RoutingTableCanvasVisualizer {
            @display("p=100,200");
        }
        Host0: StandardHost {
            @display("p=273.48,358.62");
        }
        Host1: StandardHost {
            @display("p=273.48,531.48");
        }
        Host2: StandardHost {
            @display("p=966.20996,531.48");
        }
        Host3: StandardHost {
            @display("p=966.20996,358.62");
        }
        Router0: Router {
            @display("p=482.46,441.18");
        }
        Router1: Router {
            @display("p=788.19,441.18");
        }
        Router2: Router {
            @display("p=634.68,234.78");
        }
        Host4: StandardHost {
            @display("p=519.87,68.37");
        }
        Host5: StandardHost {
            @display("p=737.88,68.37");
        }
    connections:
        Host0.ethg++ <--> Eth100M <--> Router0.ethg++;
        Host1.ethg++ <--> Eth100M <--> Router0.ethg++;
        Host2.ethg++ <--> Eth100M <--> Router1.ethg++;
        Host3.ethg++ <--> Eth100M <--> Router1.ethg++;
        Router0.ethg++ <--> Eth100M <--> Router1.ethg++;
        Router0.ethg++ <--> Eth100M <--> Router2.ethg++;
        Router1.ethg++ <--> Eth100M <--> Router2.ethg++;
        Router2.ethg++ <--> Eth100M <--> Host4.ethg++;
        Router2.ethg++ <--> Eth100M <--> Host5.ethg++;
}
