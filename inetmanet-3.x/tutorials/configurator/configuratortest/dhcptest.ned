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
import inet.node.inet.WirelessHost;
import inet.node.wireless.AccessPoint;
import inet.physicallayer.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;


network dhcptest
{
    @display("bgb=400,400");
    submodules:
        configurator: IPv4NetworkConfigurator {
            @display("p=45,46");
        }
        radioMedium: Ieee80211ScalarRadioMedium {
            @display("p=37,106");
        }
        router: Router {
            @display("p=169,183");
        }
        accessPoint: AccessPoint {
            @display("p=101,186");
        }
        standardHost: StandardHost {
            @display("p=122,119");
        }
        wirelessHost: WirelessHost {
            @display("p=101,254");
        }
    connections:
        router.ethg++ <--> Eth100M <--> accessPoint.ethg++;
        accessPoint.ethg++ <--> Eth100M <--> standardHost.ethg++;
}
