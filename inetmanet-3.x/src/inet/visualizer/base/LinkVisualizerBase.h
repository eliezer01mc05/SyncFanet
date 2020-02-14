//
// Copyright (C) OpenSim Ltd.
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

#ifndef __INET_LINKVISUALIZERBASE_H
#define __INET_LINKVISUALIZERBASE_H

#include "inet/common/geometry/common/Coord.h"
#include "inet/visualizer/base/VisualizerBase.h"
#include "inet/visualizer/util/AnimationPosition.h"
#include "inet/visualizer/util/InterfaceFilter.h"
#include "inet/visualizer/util/LineManager.h"
#include "inet/visualizer/util/NetworkNodeFilter.h"
#include "inet/visualizer/util/PacketFilter.h"

namespace inet {

namespace visualizer {

class INET_API LinkVisualizerBase : public VisualizerBase, public cListener
{
  protected:
    class INET_API LinkVisualization : public LineManager::ModuleLine {
      public:
        mutable AnimationPosition lastUsageAnimationPosition;

      public:
        LinkVisualization(int sourceModuleId, int destinationModuleId);
        virtual ~LinkVisualization() {}
    };

  protected:
    /** @name Parameters */
    //@{
    bool displayLinks = false;
    NetworkNodeFilter nodeFilter;
    InterfaceFilter interfaceFilter;
    PacketFilter packetFilter;
    cFigure::Color lineColor;
    cFigure::LineStyle lineStyle;
    double lineWidth = NaN;
    double lineShift = NaN;
    const char *lineShiftMode = nullptr;
    double lineContactSpacing = NaN;
    const char *lineContactMode = nullptr;
    const char *fadeOutMode = nullptr;
    double fadeOutTime = NaN;
    double fadeOutAnimationSpeed = NaN;
    //@}

    LineManager *lineManager = nullptr;
    /**
     * Maps packet to last module.
     */
    std::map<int, int> lastModules;
    /**
     * Maps source/destination module ids to link visualizations.
     */
    std::map<std::pair<int, int>, const LinkVisualization *> linkVisualizations;

  protected:
    virtual void initialize(int stage) override;
    virtual void handleParameterChange(const char *name) override;
    virtual void refreshDisplay() const override;

    virtual void subscribe();
    virtual void unsubscribe();

    virtual bool isLinkEnd(cModule *module) const = 0;

    virtual const LinkVisualization *createLinkVisualization(cModule *source, cModule *destination) const = 0;
    virtual const LinkVisualization *getLinkVisualization(std::pair<int, int> linkVisualization);
    virtual void addLinkVisualization(std::pair<int, int> sourceAndDestination, const LinkVisualization *linkVisualization);
    virtual void removeLinkVisualization(const LinkVisualization *linkVisualization);
    virtual void removeAllLinkVisualizations();
    virtual void setAlpha(const LinkVisualization *linkVisualization, double alpha) const = 0;

    virtual cModule *getLastModule(int treeId);
    virtual void setLastModule(int treeId, cModule *lastModule);
    virtual void removeLastModule(int treeId);

    virtual void updateLinkVisualization(cModule *source, cModule *destination);

  public:
    virtual ~LinkVisualizerBase();

    virtual void receiveSignal(cComponent *source, simsignal_t signal, cObject *object, cObject *details) override;
};

} // namespace visualizer

} // namespace inet

#endif // ifndef __INET_LINKVISUALIZERBASE_H

