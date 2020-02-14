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


network MoreLevels_2
{
    @display("bgb=907,526");
    submodules:
        configurator: IPv4NetworkConfigurator {
            @display("p=107,87");
        }
        visualizer: RoutingTableCanvasVisualizer {
            @display("p=112,167");
        }
        Area1router: Router {
            @display("p=390,205");
        }
        Area2router: Router {
            @display("p=376,375");
        }
        Router1: Router {
            @display("p=512,268");
        }
        Router2: Router {
            @display("p=634,268");
        }
        Area3router: Router {
            @display("p=755,194");
        }
        Area4router: Router {
            @display("p=748,375");
        }
        Area1_Host1: StandardHost {
            @display("p=280,124");
        }
        Area1_Host2: StandardHost {
            @display("p=271,263");
        }
        Area2_Host1: StandardHost {
            @display("p=263,377");
        }
        Area2_Host2: StandardHost {
            @display("p=260,479");
        }
        Area3_Host1: StandardHost {
            @display("p=841,121");
        }
        Area3_Host2: StandardHost {
            @display("p=849,226");
        }
        Area4_Host1: StandardHost {
            @display("p=833,343");
        }
        Area4_Host2: StandardHost {
            @display("p=831,470");
        }
    connections:

        Area2_Host2.ethg++ <--> Eth100M <--> Area2router.ethg++;
        Area2_Host1.ethg++ <--> Eth100M <--> Area2router.ethg++;
        Area1_Host2.ethg++ <--> Eth100M <--> Area1router.ethg++;
        Area1_Host1.ethg++ <--> Eth100M <--> Area1router.ethg++;
        Area1router.ethg++ <--> Eth100M <--> Router1.ethg++;
        Area2router.ethg++ <--> Eth100M <--> Router1.ethg++;
        Router1.ethg++ <--> Eth100M <--> Router2.ethg++;
        Router2.ethg++ <--> Eth100M <--> Area3router.ethg++;
        Area3router.ethg++ <--> Eth100M <--> Area3_Host1.ethg++;
        Area3router.ethg++ <--> Eth100M <--> Area3_Host2.ethg++;
        Area4router.ethg++ <--> Eth100M <--> Area4_Host1.ethg++;
        Area4router.ethg++ <--> Eth100M <--> Area4_Host2.ethg++;
        Router2.ethg++ <--> Eth100M <--> Area4router.ethg++;
}
