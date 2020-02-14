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
//

#include "inet/mobility/single/LaptopModelManager.h"
#include "inet/mobility/base/MobilityBase.h"

namespace inet {

Define_Module(LaptopModelManager);

void LaptopModelManager::Timer::removeTimer()
{
    removeQueueTimer();
}

LaptopModelManager::Timer::~Timer()
{
    removeTimer();
}

void LaptopModelManager::Timer::removeQueueTimer()
{
    TimerMultiMap::iterator it;
    for (it = module->timerMultimMap.begin(); it != module->timerMultimMap.end(); it++ )
    {
        if (it->second==this)
        {
            module->timerMultimMap.erase(it);
            return;
        }
    }
}

void LaptopModelManager::Timer::expire()
{
    SimTime t;
    if (module->nodeList[index].module)
    {
        t =  simTime() + module->par("startLife").doubleValue();
        module->deleteNode(index, t);
    }
    else
    {
        t =  simTime() + module->par("endLife").doubleValue();
        Coord position;
        module->newNode(module->par("nodeName").stringValue(), module->par("nodeType").stringValue(), false, position, t, index);
    }
    module->timerMultimMap.insert(std::pair<simtime_t, Timer *>(t, this));
}

void LaptopModelManager::Timer::resched(double time)
{
    removeQueueTimer();
    if (simTime()+time<=simTime())
        throw cRuntimeError("LaptopModelManager::resched message timer in the past");
    module->timerMultimMap.insert(std::pair<simtime_t, Timer *>(simTime()+time, this));
}

void LaptopModelManager::Timer::resched(simtime_t time)
{
    removeQueueTimer();
    if (time<=simTime())
        throw cRuntimeError("LaptopModelManager::resched message timer in the past");
    module->timerMultimMap.insert(std::pair<simtime_t, Timer *>(time, this));
}

bool LaptopModelManager::Timer::isScheduled()
{
    TimerMultiMap::iterator it;
    for (it = module->timerMultimMap.begin() ; it != module->timerMultimMap.end(); it++ )
    {
        if (it->second==this)
        {
            return true;
        }
    }
    return false;
}


LaptopModelManager::LaptopModelManager()
{
    // TODO Auto-generated constructor stub
    timerMessagePtr = new cMessage();
}

LaptopModelManager::~LaptopModelManager()
{
    // TODO Auto-generated destructor stub
    cancelAndDelete(timerMessagePtr);
}

void LaptopModelManager::initialize()
{
    unsigned int numNodes = par("NumNodes");
    for (unsigned int i = 0; i < numNodes; i++)
    {
        NodeInf info;
        info.module = nullptr;
        info.startLife = par("startLife");
        nodeList.push_back(info);
        LaptopModelManager::Timer *timer = new LaptopModelManager::Timer(i, this);
        timer->resched(info.startLife);
    }
    scheduleEvent();
}

void LaptopModelManager::newNode(const char * name, const char * type,bool setCoor, const Coord& position, simtime_t endLife, int index)
{
    cModule* parentmod = getParentModule();
    int nodeVectorIndex = index;
    if (!parentmod) error("Parent Module not found");

    cModuleType* nodeType = cModuleType::get(type);
    if (!nodeType) error("Module Type \"%s\" not found", type);

    //TODO: this trashes the vectsize member of the cModule, although nobody seems to use it
    cModule* mod = nodeType->create(name, parentmod, nodeVectorIndex, nodeVectorIndex);
    mod->finalizeParameters();
    mod->buildInside();
    mod->scheduleStart(simTime());

    if (setCoor)
    {
        MobilityBase* mm = nullptr;
        for (cModule::SubmoduleIterator iter(mod); !iter.end(); iter++) {
            cModule* submod = *iter;
            mm = dynamic_cast<MobilityBase*>(submod);
            if (!mm)
                continue;
            mm->par("initialX").setDoubleValue(position.x);
            mm->par("initialY").setDoubleValue(position.y);
            mm->par("initialZ").setDoubleValue(position.z);
            break;
        }
        if (!mm)
            error("Mobility modele not found");
    }
    NodeInf info;
    info.endLife = endLife;
    info.module = mod;
    nodeList[index] = info;
    mod->callInitialize();
}


void LaptopModelManager::deleteNode(const int &index, const simtime_t &startTime)
{
    cModule* mod = nodeList[index].module;

    if (!mod) error("no node with Id \"%i\" not found", index);

    if (!mod->getSubmodule("notificationBoard")) error("host has no submodule notificationBoard");

    mod->callFinish();
    mod->deleteModule();
    nodeList[index].module = nullptr;
    nodeList[index].startLife = startTime;
}

void LaptopModelManager::scheduleEvent()
{
    if (!timerMessagePtr)
        return;
    if (timerMultimMap.empty()) // nothing to do
    {
        if (timerMessagePtr->isScheduled())
            cancelEvent(timerMessagePtr);
        return;
    }
    TimerMultiMap::iterator e = timerMultimMap.begin();
    while (timerMultimMap.begin()->first<=simTime())
    {
        timerMultimMap.erase(e);
        e->second->expire();
        if (timerMultimMap.empty())
            break;
        e = timerMultimMap.begin();
    }

    if (timerMessagePtr->isScheduled())
    {
        if (e->first <timerMessagePtr->getArrivalTime())
        {
            cancelEvent(timerMessagePtr);
            scheduleAt(e->first,timerMessagePtr);
        }
        else if (e->first>timerMessagePtr->getArrivalTime()) // Possible error, or the first event has been canceled
        {
            cancelEvent(timerMessagePtr);
            scheduleAt(e->first,timerMessagePtr);
            EV << "timer Queue problem";
            // throw cRuntimeError("timer Queue problem");
        }
    }
    else
    {
        scheduleAt(e->first, timerMessagePtr);
    }
}

bool LaptopModelManager::checkTimer(cMessage *msg)
{
    if (msg != timerMessagePtr)
        return false;
    if (timerMultimMap.empty())
        return true;
    TimerMultiMap::iterator it = timerMultimMap.begin();
    while (it->first <= simTime())
    {
        Timer * timer = it->second;
        if (timer == nullptr)
            throw cRuntimeError ("timer owner is bad");
        timerMultimMap.erase(it);
        timer->expire();
        if (timerMultimMap.empty())
            break;
        it = timerMultimMap.begin();
    }
    return true;
}

void LaptopModelManager::handleMessage(cMessage *msg)
{
    if(!checkTimer(msg))
    {
        delete msg;
    }
    scheduleEvent();
}

}
