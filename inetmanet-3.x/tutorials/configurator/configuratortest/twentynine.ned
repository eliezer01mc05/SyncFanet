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
import inet.node.inet.WirelessHost;
import inet.physicallayer.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.visualizer.integrated.IntegratedCanvasVisualizer;


network twentynine
{
    @display("bgb=1031,655");
    submodules:
        configurator: IPv4NetworkConfigurator {
            @display("p=45,48");
        }
        visualizer: IntegratedCanvasVisualizer {
            @display("p=43,117");
        }
        wirelessHost: WirelessHost {
            @display("p=226,263");
        }
        wirelessHost1: WirelessHost {
            @display("p=350,271");
        }
        wirelessHost2: WirelessHost {
            @display("p=442,338");
        }
        wirelessHost3: WirelessHost {
            @display("p=568,338");
        }
        wirelessHost4: WirelessHost {
            @display("p=652,266");
        }
        wirelessHost5: WirelessHost {
            @display("p=774,267");
        }
        radioMedium: Ieee80211ScalarRadioMedium {
            @display("p=52,188");
        }
}
