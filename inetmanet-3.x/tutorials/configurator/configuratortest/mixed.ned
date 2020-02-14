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
import inet.node.inet.WirelessHost;
import inet.node.wireless.AccessPoint;
import inet.physicallayer.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.visualizer.integrated.IntegratedCanvasVisualizer;


network mixed
{
    @display("bgb=986,606");
    submodules:
        router: Router {
            @display("p=345,265");
        }
        wirelessHost: WirelessHost {
            @display("p=124,177");
        }
        wirelessHost1: WirelessHost {
            @display("p=262,265");
        }
        wirelessHost2: WirelessHost {
            @display("p=228.9375,176.96251");
        }
        standardHost: StandardHost {
            @display("p=745,497");
        }
        standardHost1: StandardHost {
            @display("p=621,493");
        }
        standardHost2: StandardHost {
            @display("p=503,503");
        }
        router1: Router {
            @display("p=606,265");
        }
        etherSwitch: EtherSwitch {
            @display("p=630,395");
        }
        configurator: IPv4NetworkConfigurator {
            @display("p=77,46");
        }
        radioMedium: Ieee80211ScalarRadioMedium {
            @display("p=81,107");
        }
        visualizer: IntegratedCanvasVisualizer {
            @display("p=276,46");
        }
    connections:
        standardHost2.ethg++ <--> Eth100M <--> etherSwitch.ethg++;
        standardHost1.ethg++ <--> Eth100M <--> etherSwitch.ethg++;
        etherSwitch.ethg++ <--> Eth100M <--> standardHost.ethg++;
        etherSwitch.ethg++ <--> Eth100M <--> router1.ethg++;
        router1.ethg++ <--> Eth100M <--> router.ethg++;
        router.ethg++ <--> Eth100M <--> wirelessHost1.ethg++;
}
