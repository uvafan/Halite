#pragma once

#include "../hlt/location.hpp"
#include "../hlt/move.hpp"
#include "../hlt/map.hpp"
#include "../hlt/planet.hpp"
#include "../hlt/types.hpp"
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
        hlt::Location targetLoc;
        std::set<hlt::Location> enemyLocs;
        double targetRad;
        double enemyRelevanceRad;
        int priority;
        ObjType type;        

        static Objective newObjective(ObjType t, const hlt::Location& loc, double rad){
            std::unordered_set<int> ms;
            std::set<hlt::Location> el;
            return { ms,loc,el,rad,35,100,t };
        }

        bool operator<(const Objective& o) const{
            bool xLess = targetLoc.pos_x < o.targetLoc.pos_x;
            bool xeq = targetLoc.pos_x == o.targetLoc.pos_x;
            bool yLess = targetLoc.pos_y < o.targetLoc.pos_y;
            bool yeq = targetLoc.pos_y == o.targetLoc.pos_y;
            bool typeLess = static_cast<int>(type) < static_cast<int>(o.type);
            return xLess || (xeq && yLess) || (xeq && yeq && typeLess);
        }

        void updatePriority(){
            //TODO this 
            priority = 100;
        }
        
        void addShip(int ship){
            myShips.insert(ship);
        }

        void removeShip(int ship){
            myShips.erase(ship);
        }
        
        void addEnemyLoc(const hlt::Location& loc){
            if(loc.get_distance_to(targetLoc) <= enemyRelevanceRad)
                enemyLocs.insert(loc);
        }
           
        void removeEnemyLoc(const hlt::Location& loc){
            enemyLocs.erase(loc);
        }

        void clearEnemyLocs(){
            enemyLocs.clear();
        }

    };

    bool operator==(const Objective& o1, const Objective& o2){
        return o1.targetLoc == o2.targetLoc && o1.type == o2.type;
    }

}
