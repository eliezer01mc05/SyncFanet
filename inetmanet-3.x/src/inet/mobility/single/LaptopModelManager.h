//
// Copyright (C) 2013 Alfonso Ariza. Universidad de Malaga
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

#ifndef LaptopModelManager_H_
#define LaptopModelManager_H_
#include <vector>
#include <map>
#include <omnetpp.h>
#include "inet/common/INETDefs.h"
#include "inet/common/geometry/common/Coord.h"

namespace inet {

class LaptopModelManager : public cSimpleModule
{
    private:
        class Timer {
          public:
            int index;
            LaptopModelManager *module;
            Timer(int index, LaptopModelManager *module): index(index),module(module){}
            ~Timer();
            virtual void expire();
            virtual void removeQueueTimer();
            virtual void removeTimer();
            virtual void resched(double time);
            virtual void resched(simtime_t time);
            virtual bool isScheduled();
        };

        class NodeInf
        {
            public:
            cModule *module;
            simtime_t startLife;
            simtime_t endLife;
        };
    private:
        std::vector<NodeInf> nodeList;
        typedef std::multimap <simtime_t, Timer *> TimerMultiMap;
        TimerMultiMap timerMultimMap;
        bool checkTimer(cMessage *msg);
        void scheduleEvent();
        cMessage *timerMessagePtr;
    public:
        LaptopModelManager();
        virtual ~LaptopModelManager();
        virtual void initialize();
       // virtual void finish();
        virtual void handleMessage(cMessage *msg);
        void newNode(const char *name, const char * nodeId, bool setCoor, const Coord& position, simtime_t, int);
        void deleteNode(const int &, const simtime_t &);
};

}

#endif /* DINAMICWIRELESSNODEMANAGER_H_ */
