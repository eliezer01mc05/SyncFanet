package src.modules.power.battery;

import base.modules.BaseBattery;

// a simple battery module
//
// Note that resolution should be much smaller than the expected time to
// first node failure.
//
simple SimpleBattery extends BaseBattery {    
    parameters:
        @class(SimpleBattery);
        bool debug = default(false);
        // nominal battery capacity
        volatile double nominal @unit(mAh);
        // battery capacity
        double capacity @unit(mAh); 
        // nominal voltage 
        volatile double voltage @unit(V);
        // capacity is updated at least every resolution time
        volatile double resolution @unit(s);
        
        // (0..1): capacity is published each time it is
		// observed to have changed by publishDelta * nominal_capacity
        volatile double publishDelta;
        
        // if > 0, capacity is published to the BB each publishTime 
		// interval		
        volatile double publishTime @unit(s);
        
        // number of modules that will draw energy from the 
		// battery
        int numDevices;

}