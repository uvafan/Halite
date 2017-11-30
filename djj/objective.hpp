#pragma once

#include "../hlt/location.hpp"
#include "../hlt/move.hpp"
#include "../hlt/map.hpp"
#include "../hlt/planet.hpp"
#include "../hlt/types.hpp"
#include "navigator.hpp"
#include "objective.hpp"
#include <vector>
#include <queue>
#include <unordered_set>

#define INF 100000000

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
        std::set<hlt::Ship> enemyShips;
        double targetRad;
        double enemyRelevanceRad;
        int priority;
        ObjType type;
        int myDocked;        
        int mySpaces;

        static Objective newObjective(ObjType t, const hlt::Location& loc, double rad, int docked, int cap){
            std::unordered_set<int> ms;
            std::set<hlt::Ship> es;
            return { ms,loc,es,rad,35,100,t,docked,cap };
        }

        bool operator<(const Objective& o) const{
            bool xLess = targetLoc.pos_x < o.targetLoc.pos_x;
            bool xeq = targetLoc.pos_x == o.targetLoc.pos_x;
            bool yLess = targetLoc.pos_y < o.targetLoc.pos_y;
            bool yeq = targetLoc.pos_y == o.targetLoc.pos_y;
            bool typeLess = static_cast<int>(type) < static_cast<int>(o.type);
            return xLess || (xeq && yLess) || (xeq && yeq && typeLess);
        }

        void updatePriority() {
            int myUndocked = myShips.size();
            int enemyUndocked = enemyShips.size();
            if(type == ObjType::harassPlanet) enemyUndocked -= myDocked;
            else myUndocked -= myDocked;
            if(type == ObjType::noop)priority = -INF;
            else if(type == ObjType::dockUnownedPlanet) priority = (mySpaces-myUndocked) * 10;
            else if(type == ObjType::dockOwnedPlanet) priority = (mySpaces-myDocked-myUndocked) * 5;
            else if(type == ObjType::harassPlanet) priority = 100 + myUndocked * 10 - enemyUndocked * 10;
            else{
                priority = 50 * (enemyUndocked - myUndocked);
            }
        }
        
        void addShip(int ship) {
            myShips.insert(ship);
        }

        void removeShip(int ship){
            myShips.erase(ship);
        }
        
        void addEnemyShip(const hlt::Ship& ship){
            if(ship.location.get_distance_to(targetLoc) <= enemyRelevanceRad)
                enemyShips.insert(ship);
        }
           
        void removeEnemyShip(const hlt::Ship& ship){
            enemyShips.erase(ship);
        }

        void clearEnemyShips(){
            enemyShips.clear();
        }
        
    };

    std::string ts(ObjType o){
        if(o==ObjType::dockUnownedPlanet)return "dock unowned";
        else if(o == ObjType::dockOwnedPlanet) return "dock owned";
        else if(o == ObjType::harassPlanet) return "harass";
        else if(o == ObjType::defendPlanet) return "defend";
        return "noop";
    }

    inline std::ostream & operator<<(std::ostream & Str, Objective const & o){
        Str << "objective of type " << ts(o.type) << " priority " << o.priority << " targetLoc " << o.targetLoc.pos_x << " " << o.targetLoc.pos_y; 
        return Str;
    }

    bool operator==(const Objective& o1, const Objective& o2){
        return o1.targetLoc == o2.targetLoc && o1.type == o2.type;
    }

}
