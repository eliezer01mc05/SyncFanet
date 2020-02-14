//
// Copyright (C) 2010 Alfonso Ariza, Malaga University
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include <omnetpp.h>
#include "dijktraKShortest.h"
#include "inet/networklayer/common/ModuleIdAddress.h"

namespace inet{

DijkstraKshortest::CostVector DijkstraKshortest::minimumCost;
DijkstraKshortest::CostVector DijkstraKshortest::maximumCost;

DijkstraKshortest::State::State()
{
    idPrev = UndefinedAddr;
    idPrevIdx=-1;
    label=tent;
}

DijkstraKshortest::State::State(const CostVector &costData)
{
    idPrev = UndefinedAddr;
    idPrevIdx=-1;
    label=tent;
    cost = costData;
}

DijkstraKshortest::State::~State() {
    cost.clear();
}

void DijkstraKshortest::State::setCostVector(const CostVector &costData)
{
    cost=costData;
}

void DijkstraKshortest::addCost (CostVector &val, const CostVector & a, const CostVector & b)
{
    val.clear();
    for (unsigned int i=0;i<a.size();i++)
    {
        Cost aux;
        aux.metric=a[i].metric;
        switch(aux.metric)
        {
            case aditiveMin:
            case aditiveMax:
                aux.value =a[i].value+b[i].value;
                break;
            case concaveMin:
                aux.value = std::min (a[i].value,b[i].value);
                break;
            case concaveMax:
                aux.value = std::max (a[i].value,b[i].value);
                break;
        }
        val.push_back(aux);
    }
}


void DijkstraKshortest::initMinAndMax()
{
   CostVector defaulCost;
   Cost costData;
   costData.metric=aditiveMin;
   costData.value=0;
   minimumCost.push_back(costData);
   costData.metric=aditiveMin;
   costData.value=0;
   minimumCost.push_back(costData);
   costData.metric=concaveMax;
   costData.value=100e100;
   minimumCost.push_back(costData);
   costData.metric=aditiveMin;
   costData.value=0;
   minimumCost.push_back(costData);

   costData.metric=aditiveMin;
   costData.value=10e100;
   maximumCost.push_back(costData);
   costData.metric=aditiveMin;
   costData.value=1e100;
   maximumCost.push_back(costData);
   costData.metric=concaveMax;
   costData.value=0;
   maximumCost.push_back(costData);
   costData.metric=aditiveMin;
   costData.value=10e100;
   maximumCost.push_back(costData);
}

void DijkstraKshortest::initMinAndMaxWs()
{
   CostVector defaulCost;
   Cost costData;
   costData.metric=aditiveMin;
   costData.value=0;
   minimumCost.push_back(costData);
   costData.metric=concaveMax;
   costData.value=100e100;
   minimumCost.push_back(costData);

   costData.metric=aditiveMin;
   costData.value=10e100;
   maximumCost.push_back(costData);
   costData.metric=concaveMax;
   costData.value=0;
   maximumCost.push_back(costData);

}

void DijkstraKshortest::initMinAndMaxSw()
{

    CostVector defaulCost;
    Cost costData;
    costData.metric=aditiveMin;
    costData.value=10e100;
    maximumCost.push_back(costData);
    costData.metric=concaveMax;
    costData.value=0;
    maximumCost.push_back(costData);

    costData.metric=aditiveMin;
    costData.value=0;
    minimumCost.push_back(costData);
    costData.metric=concaveMax;
    costData.value=100e100;
    minimumCost.push_back(costData);
}


DijkstraKshortest::DijkstraKshortest(int limit)
{
    initMinAndMax();
    K_LIMITE = limit;
    resetLimits();
}

void DijkstraKshortest::cleanLinkArray()
{
    for (auto it=linkArray.begin();it!=linkArray.end();it++)
        while (!it->second.empty())
        {
            delete it->second.back();
            it->second.pop_back();
        }
    linkArray.clear();
}


DijkstraKshortest::~DijkstraKshortest()
{
    cleanLinkArray();
    kRoutesMap.clear();
}

void DijkstraKshortest::setLimits(const std::vector<double> & vectorData)
{

    limitsData =maximumCost;
    for (unsigned int i=0;i<vectorData.size();i++)
    {
        if (i>=limitsData.size()) continue;
        limitsData[i].value=vectorData[i];
    }
}



void DijkstraKshortest::addEdgeWs (const NodeId & originNode, const NodeId & last_node,double cost,double bw)
{
    auto it = linkArray.find(originNode);
    if (it!=linkArray.end())
    {
         for (unsigned int i=0;i<it->second.size();i++)
         {
             if (last_node == it->second[i]->last_node_)
             {
                  it->second[i]->Cost()=cost;
                  it->second[i]->Bandwith()=bw;
                  return;
             }
         }
    }
    EdgeWs *link = new EdgeWs;
    // The last hop is the interface in which we have this neighbor...
    link->last_node() = last_node;
    // Also record the link delay and quality..
    link->Cost()=cost;
    link->Bandwith()=bw;
    linkArray[originNode].push_back(link);
}

void DijkstraKshortest::addEdgeSw (const NodeId & originNode, const NodeId & last_node,double cost,double bw)
{
    auto it = linkArray.find(originNode);
    if (it!=linkArray.end())
    {
         for (unsigned int i=0;i<it->second.size();i++)
         {
             if (last_node == it->second[i]->last_node_)
             {
                  it->second[i]->Cost()=cost;
                  it->second[i]->Bandwith()=bw;
                  return;
             }
         }
    }
    EdgeSw *link = new EdgeSw;
    // The last hop is the interface in which we have this neighbor...
    link->last_node() = last_node;
    // Also record the link delay and quality..
    link->Cost()=cost;
    link->Bandwith()=bw;
    linkArray[originNode].push_back(link);
}

void DijkstraKshortest::addEdge (const NodeId & originNode, const NodeId & last_node,double cost,double delay,double bw,double quality)
{
    auto it = linkArray.find(originNode);
    if (it!=linkArray.end())
    {
         for (unsigned int i=0;i<it->second.size();i++)
         {
             if (last_node == it->second[i]->last_node_)
             {
                  it->second[i]->Cost()=cost;
                  it->second[i]->Delay()=delay;
                  it->second[i]->Bandwith()=bw;
                  it->second[i]->Quality()=quality;
                  return;
             }
         }
    }
    Edge *link = new Edge;
    // The last hop is the interface in which we have this neighbor...
    link->last_node() = last_node;
    // Also record the link delay and quality..
    link->Cost()=cost;
    link->Delay()=delay;
    link->Bandwith()=bw;
    link->Quality()=quality;
    linkArray[originNode].push_back(link);
}

void DijkstraKshortest::setRoot(const NodeId & dest_node)
{
    rootNode = dest_node;

}

void DijkstraKshortest::run()
{
    std::multiset<SetElem> heap;
    routeMap.clear();

    // include routes in the map
    kRoutesMap.clear();

    auto it = linkArray.find(rootNode);
    if (it==linkArray.end())
        throw cRuntimeError("Node not found");
    for (int i=0;i<K_LIMITE;i++)
    {
        State state(minimumCost);
        state.label=perm;
        routeMap[rootNode].push_back(state);
    }
    SetElem elem;
    elem.iD = rootNode;
    elem.idx = 0;
    elem.cost=minimumCost;
    heap.insert(elem);
    while (!heap.empty())
    {
        SetElem elem=*heap.begin();
        heap.erase(heap.begin());
        auto  it = routeMap.find(elem.iD);
        if (it==routeMap.end())
            throw cRuntimeError("node not found in routeMap");

        if (elem.iD != rootNode)
        {
            if (it->second.size() > elem.idx && it->second[elem.idx].label == perm)
                continue; // set
            if ((int)it->second.size() == K_LIMITE)
            {
                bool continueLoop = true;
                for (int i=0;i<K_LIMITE;i++)
                {
                    if (it->second[i].label != perm)
                    {
                        continueLoop = false;
                        break;
                    }
                }
                if (continueLoop)
                    continue; // nothing to do with this element
            }
        }

        if ((int)it->second.size()<=elem.idx)
        {
            for (int i = elem.idx -((int)it->second.size()-1);i>=0;i--)
            {
                State state(maximumCost);
                state.label=tent;
                it->second.push_back(state);
            }
        }

        /// Record the route in the map
        auto itAux = it;
        Route pathActive;
        Route pathNode;
        int prevIdx = elem.idx;
        NodeId currentNode =  elem.iD;
        while (currentNode!=rootNode)
        {
            pathActive.push_back(currentNode);
            currentNode = itAux->second[prevIdx].idPrev;
            prevIdx = itAux->second[prevIdx].idPrevIdx;
            itAux = routeMap.find(currentNode);
            if (itAux == routeMap.end())
                throw cRuntimeError("error in data");
            if (prevIdx >= (int) itAux->second.size())
                throw cRuntimeError("error in data");
        }

        bool routeExist = false;
        if (!pathActive.empty()) // valid path, record in the map
        {
            while (!pathActive.empty())
            {
                pathNode.push_back(pathActive.back());
                pathActive.pop_back();
            }
            auto itKroutes = kRoutesMap.find(elem.iD);
            if (itKroutes == kRoutesMap.end())
            {
                Kroutes kroutes;
                kroutes.push_back(pathNode);
                kRoutesMap.insert(std::make_pair(elem.iD, kroutes));
            }
            else
            {
                for (unsigned int j = 0; j < itKroutes->second.size(); j++)
                {
                    if (pathNode == itKroutes->second[j])
                    {
                        routeExist = true;
                        break;
                    }
                }
                if (!routeExist)
                {
                    if ((int)itKroutes->second.size() < K_LIMITE)
                        itKroutes->second.push_back(pathNode);
                }
            }
        }

        if (routeExist)
            continue; // next
        it->second[elem.idx].label=perm;

        // next hop
        auto linkIt=linkArray.find(elem.iD);
        if (linkIt == linkArray.end())
            throw cRuntimeError("Error link not found in linkArray");

        for (unsigned int i=0;i<linkIt->second.size();i++)
        {
            Edge *current_edge= (linkIt->second)[i];
            CostVector cost;
            CostVector maxCost = maximumCost;
            int nextIdx;

            // check if the node is in the path
            if (std::find(pathNode.begin(),pathNode.end(),current_edge->last_node())!=pathNode.end())
                continue;

            auto itNext = routeMap.find(current_edge->last_node());

            addCost(cost,current_edge->cost,(it->second)[elem.idx].cost);
            if (!limitsData.empty())
            {
                if (limitsData<cost)
                    continue;
            }

            if (itNext==routeMap.end() || (itNext!=routeMap.end() && (int)itNext->second.size()<K_LIMITE))
            {
                State state;
                state.idPrev=elem.iD;
                state.idPrevIdx=elem.idx;
                state.cost=cost;
                state.label=tent;
                routeMap[current_edge->last_node()].push_back(state);
                nextIdx = routeMap[current_edge->last_node()].size()-1;
                SetElem newElem;
                newElem.iD=current_edge->last_node();
                newElem.idx=nextIdx;
                newElem.cost=cost;
                heap.insert(newElem);
            }
            else
            {
                bool permanent = true;
                for (unsigned i=0;i<itNext->second.size();i++)
                {
                    if ((maxCost<itNext->second[i].cost)&&(itNext->second[i].label==tent))
                    {
                        maxCost = itNext->second[i].cost;
                        nextIdx=i;
                        permanent = false;
                    }
                }
                if (cost<maxCost && !permanent)
                {
                    itNext->second[nextIdx].cost=cost;
                    itNext->second[nextIdx].idPrev=elem.iD;
                    itNext->second[nextIdx].idPrevIdx=elem.idx;
                    SetElem newElem;
                    newElem.iD=current_edge->last_node();
                    newElem.idx=nextIdx;
                    newElem.cost=cost;
                    for (auto it = heap.begin(); it != heap.end(); ++it)
                    {
                        if (it->iD == newElem.iD && it->idx == newElem.idx && it->cost > newElem.cost)
                        {
                            heap.erase(it);
                            break;
                        }
                    }
                    heap.insert(newElem);
                }
            }
        }
    }
}

void DijkstraKshortest::runUntil (const NodeId &target)
{
    std::multiset<SetElem> heap;
    routeMap.clear();

    auto it = linkArray.find(rootNode);
    if (it==linkArray.end())
        throw cRuntimeError("Node not found");
    for (int i=0;i<K_LIMITE;i++)
    {
        State state(minimumCost);
        state.label=perm;
        routeMap[rootNode].push_back(state);
    }
    SetElem elem;
    elem.iD=rootNode;
    elem.idx=0;
    elem.cost=minimumCost;
    heap.insert(elem);
    while (!heap.empty())
    {
        SetElem elem=*heap.begin();
        heap.erase(heap.begin());
        auto it = routeMap.find(elem.iD);
        if (it==routeMap.end())
            throw cRuntimeError("node not found in routeMap");

        if (elem.iD != rootNode)
        {
            if (it->second.size() > elem.idx && it->second[elem.idx].label == perm)
                continue; // set
            if ((int)it->second.size() == K_LIMITE)
            {
                bool continueLoop = true;
                for (int i=0;i<K_LIMITE;i++)
                {
                    if (it->second[i].label != perm)
                    {
                        continueLoop = false;
                        break;
                    }
                }
                if (continueLoop)
                    continue;
            }
        }

        if ((int)it->second.size()<=elem.idx)
        {
            for (int i = elem.idx -((int)it->second.size()-1);i>=0;i--)
            {
                State state(maximumCost);
                state.label=tent;
                it->second.push_back(state);
            }
        }
        (it->second)[elem.idx].label=perm;
        if ((int)it->second.size()==K_LIMITE && target==elem.iD)
            return;
        auto linkIt=linkArray.find(elem.iD);
        if (linkIt == linkArray.end())
            throw cRuntimeError("Error link not found in linkArray");
        for (unsigned int i=0;i<linkIt->second.size();i++)
        {
            Edge* current_edge= (linkIt->second)[i];
            CostVector cost;
            CostVector maxCost = maximumCost;
            int nextIdx;
            auto itNext = routeMap.find(current_edge->last_node());
            addCost(cost,current_edge->cost,(it->second)[elem.idx].cost);
            if (!limitsData.empty())
            {
                if (limitsData<cost)
                    continue;
            }
            if ((itNext==routeMap.end()) ||  (itNext!=routeMap.end() && (int)itNext->second.size()<K_LIMITE))
            {
                State state;
                state.idPrev=elem.iD;
                state.idPrevIdx=elem.idx;
                state.cost=cost;
                state.label=tent;
                routeMap[current_edge->last_node()].push_back(state);
                nextIdx = routeMap[current_edge->last_node()].size()-1;
                SetElem newElem;
                newElem.iD=current_edge->last_node();
                newElem.idx=nextIdx;
                newElem.cost=cost;
                heap.insert(newElem);
            }
            else
            {
                bool permanent = true;
                for (unsigned i=0;i<itNext->second.size();i++)
                {
                    if ((maxCost<itNext->second[i].cost)&&(itNext->second[i].label==tent))
                    {
                        maxCost = itNext->second[i].cost;
                        nextIdx=i;
                        permanent = false;
                    }
                }
                if (cost<maxCost && !permanent)
                {
                    itNext->second[nextIdx].cost = cost;
                    itNext->second[nextIdx].idPrev = elem.iD;
                    itNext->second[nextIdx].idPrevIdx = elem.idx;
                    SetElem newElem;
                    newElem.iD=current_edge->last_node();
                    newElem.idx=nextIdx;
                    newElem.cost=cost;
                    for (auto it = heap.begin(); it != heap.end(); ++it)
                    {
                        if (it->iD == newElem.iD && it->idx == newElem.idx && it->cost > newElem.cost)
                        {
                            heap.erase(it);
                            break;
                        }
                    }
                    heap.insert(newElem);
                }
            }
        }
    }
}

int DijkstraKshortest::getNumRoutes(const NodeId &nodeId)
{
    auto it = routeMap.find(nodeId);
    if (it==routeMap.end())
        return -1;
    return (int)it->second.size();
}

bool DijkstraKshortest::getRoute(const NodeId &nodeId,std::vector<NodeId> &pathNode,int k)
{
    auto it = routeMap.find(nodeId);
    if (it == routeMap.end())
        return false;
    if (k>=(int)it->second.size())
        return false;
    std::vector<NodeId> path;
    NodeId currentNode = nodeId;
    int idx=it->second[k].idPrevIdx;
    while (currentNode!=rootNode)
    {
        path.push_back(currentNode);
        currentNode = it->second[idx].idPrev;
        idx=it->second[idx].idPrevIdx;
        it = routeMap.find(currentNode);
        if (it==routeMap.end())
            throw cRuntimeError("error in data");
        if (idx>=(int)it->second.size())
            throw cRuntimeError("error in data");
    }
    pathNode.clear();
    while (!path.empty())
    {
        pathNode.push_back(path.back());
        path.pop_back();
    }
    return true;
}

void DijkstraKshortest::setFromTopo(const cTopology *topo)
{
    for (int i=0; i<topo->getNumNodes(); i++)
    {
    	cTopology::Node *node = const_cast<cTopology*>(topo)->getNode(i);
    	NodeId id(ModuleIdAddress(node->getModuleId()));
    	for (int j=0; j<node->getNumOutLinks(); j++)
    	{

    		NodeId idNex(ModuleIdAddress(node->getLinkOut(j)->getRemoteNode()->getModuleId()));
    		double cost=node->getLinkOut(j)->getWeight();
    		addEdge (id,idNex,cost,0,1000,0);
    	}
    }
}

void DijkstraKshortest::setRouteMapK()
{
    kRoutesMap.clear();
    std::vector<NodeId> pathNode;
    for (auto it = routeMap.begin();it != routeMap.begin();++it)
    {
        for (int i = 0; i < (int)it->second.size();i++)
        {
            std::vector<NodeId> path;
            NodeId currentNode = it->first;
            int idx=it->second[i].idPrevIdx;
            while (currentNode!=rootNode)
            {
                path.push_back(currentNode);
                currentNode = it->second[idx].idPrev;
                idx=it->second[idx].idPrevIdx;
                it = routeMap.find(currentNode);
                if (it==routeMap.end())
                    throw cRuntimeError("error in data");
                if (idx>=(int)it->second.size())
                    throw cRuntimeError("error in data");
            }
            pathNode.clear();
            while (!path.empty())
            {
                pathNode.push_back(path.back());
                path.pop_back();
            }
            kRoutesMap[it->first].push_back(pathNode);
        }
    }
}


void DijkstraKshortest::getRouteMapK(const NodeId &nodeId, Kroutes &routes)
{
    routes.clear();
    auto it = kRoutesMap.find(nodeId);
    if (it == kRoutesMap.end())
        return;
    routes = it->second;
}

}
