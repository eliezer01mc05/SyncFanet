package inet.node.inet;

import inet.node.base.ApplicationLayerNodeBase;

module SensorNode extends ApplicationLayerNodeBase {
    parameters:
        @display("i=misc/node");
        @figure[submodules];
        numWlanInterfaces = default(1);
        energyStorage.typename = default("IdealEpEnergyStorage");
        wlan[*].typename = default("Ieee802154NarrowbandInterface");
        wlan[*].radio.energyConsumer.typename = default("SensorStateBasedEpEnergyConsumer"); //FIXME does this consumer exist???
}