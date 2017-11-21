#pragma once

#include "../hlt/location.hpp"
#include "../hlt/move.hpp"
#include "../hlt/map.hpp"
#include "../hlt/planet.hpp"
#include "ship.hpp"
#include "navigator.hpp"
#include "objective.hpp"
#include <vector>
#include <queue>
#include <unordered_set>

namespace djj {
    enum class ObjType {
        noop = 0,
        dockUnownedPlanet,
        dockOwnedPlanet,
        harassPlanet,
        defendPlanet,
    };

    struct Objective {
        std::unordered_set<int> myShips;
        std::vector<hlt::Location> targetLocs;
        double targetRad;
        ObjType type;        

        static Objective newObjective(ObjType t, const hlt::Location& loc, double rad){
            std::unordered_set<int> ms;
            std::vector<hlt::Location> tl;
            tl.push_back(loc);
            //hlt::Log::log("hi");
            return { ms,tl,rad,t };
        }

        bool operator<(const Objective& o) const{
            bool xLess = targetLocs[0].pos_x < o.targetLocs[0].pos_x;
            bool xeq = targetLocs[0].pos_x == o.targetLocs[0].pos_x;
            bool yLess = targetLocs[0].pos_y < o.targetLocs[0].pos_y;
            bool yeq = targetLocs[0].pos_y == o.targetLocs[0].pos_y;
            bool typeLess = static_cast<int>(type) < static_cast<int>(o.type);
            return xLess || (xeq && yLess) || (xeq && yeq && typeLess);
        }

    };

    bool operator==(const Objective& o1, const Objective& o2){
        return o1.targetLocs[0] == o2.targetLocs[0] && o1.type == o2.type;
    }

}
