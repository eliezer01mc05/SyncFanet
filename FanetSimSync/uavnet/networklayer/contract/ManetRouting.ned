package uavnet.networklayer.contract;

moduleinterface ManetRouting {
    @display("i=block/routing");
    gates:
        input ipIn;
        output ipOut;
}