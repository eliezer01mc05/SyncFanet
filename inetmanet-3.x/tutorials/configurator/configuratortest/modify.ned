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

import inet.node.ethernet.Eth100M;
import inet.node.ethernet.EtherSwitch;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import inet.node.inet.WirelessHost;
import inet.node.wireless.AccessPoint;


network modify
{
    @display("bgb=2293.2998,1382.1");

    submodules:
        etherSwitch1: EtherSwitch {
            @display("p=1139,1064.2");
        }
        accessPoint: AccessPoint {
            @display("p=285.59998,1064.2");
        }
        wirelessHost: WirelessHost {
            @display("p=103.7,1205.2999");
        }
        wirelessHost1: WirelessHost {
            @display("p=278.8,1193.3999");
        }
        wirelessHost2: WirelessHost {
            @display("p=464.09998,1220.6");
        }
        standardHost: StandardHost {
            @display("p=251.59999,877.19995");
        }
        standardHost1: StandardHost {
            @display("p=462.4,878.89996");
        }
        standardHost3: StandardHost {
            @display("p=980.89996,1235.8999");
        }
        standardHost4: StandardHost {
            @display("p=1173,1232.5");
        }
        standardHost5: StandardHost {
            @display("p=1365.1,1234.2");
        }
        router: Router {
            @display("p=809.19995,1067.6");
        }
        router1: Router {
            @display("p=984.3,586.5;i=abstract/router2");
        }
        router2: Router {
            @display("p=1207,384.19998");
        }
        router3: Router {
            @display("p=1363.3999,571.19995");
        }
    connections:
        standardHost3.ethg++ <--> Eth100M <--> etherSwitch1.ethg++;
        standardHost4.ethg++ <--> Eth100M <--> etherSwitch1.ethg++;
        etherSwitch1.ethg++ <--> Eth100M <--> standardHost5.ethg++;
        accessPoint.ethg++ <--> Eth100M <--> standardHost.ethg++;
        accessPoint.ethg++ <--> Eth100M <--> standardHost1.ethg++;
        accessPoint.ethg++ <--> Eth100M <--> router.ethg++;
        etherSwitch1.ethg++ <--> Eth100M <--> router.ethg++;
        router.ethg++ <--> Eth100M <--> router1.ethg++;
        router1.ethg++ <--> Eth100M <--> router2.ethg++;
        router3.ethg++ <--> Eth100M <--> router2.ethg++;
}
