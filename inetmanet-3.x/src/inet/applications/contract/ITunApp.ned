package inet.applications.contract;

//
// Not an actual module type. It only shows which gates a TUN app should have
// to be able to be used in StandardHost
//
moduleinterface ITunApp
{
    parameters:
        @display("i=block/app");
    gates:
        input tunIn @labels(TunInterface/up);
        output tunOut @labels(TunInterface/down);
}
