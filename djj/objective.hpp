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
        double priority;
        ObjType type;
        int myDocked;        
        int mySpaces;

        static Objective newObjective(ObjType t, const hlt::Location& loc, double rad, int docked, int cap){
            std::unordered_set<int> ms;
            std::set<hlt::Ship> es;
            return { ms,loc,es,rad,35,0,t,docked,cap };
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
            else if(type == ObjType::defendPlanet) myUndocked -= myDocked;
            if(type == ObjType::noop)priority = -INF;
            else if(type == ObjType::dockUnownedPlanet) priority = (mySpaces-myUndocked) * 3 + 20;
            else if(type == ObjType::dockOwnedPlanet) priority = 20; 
            else if(type == ObjType::harassPlanet) priority = 50 + myUndocked * 10 - enemyUndocked * 10;
            else{
                if(!enemyUndocked) priority = -INF;
                else priority = 15 * (enemyUndocked - myUndocked);
            }
        }

        int getAggFactor(){
            int myUndocked = myShips.size();
            int enemyUndocked = enemyShips.size();
            if(type == ObjType::harassPlanet) enemyUndocked -= myDocked;
            else if(type == ObjType::defendPlanet) myUndocked -= myDocked;
            return (myUndocked - enemyUndocked); 
        }

        static hlt::Ship closestShipToLoc(std::set<hlt::Ship> ships, hlt::Location loc, bool excludeZeroAndDocked){
            double minDist = INF;
            hlt::Ship ret = *ships.begin();
            for(const hlt::Ship& s: ships){
                double dist = loc.get_distance_to(s.location);
                if((dist < .001 || s.docking_status != hlt::ShipDockingStatus::Undocked) && excludeZeroAndDocked)continue;
                if(dist<minDist){
                    minDist = dist;
                    ret = s;
                }
            }
            return ret;
        }

        static hlt::Location getMid(hlt::Location loc1, hlt::Location loc2){
            double newX = (loc1.pos_x + loc2.pos_x) / 2.0;
            double newY = (loc1.pos_y + loc2.pos_y) / 2.0;
            return hlt::Location::newLoc(newX,newY);
        }

        hlt::Ship getTargetDocker(){
            double bestScore = -INF;
            hlt::Ship ret = *enemyShips.begin();
            for(const hlt::Ship& s: enemyShips){
                if(s.docking_status == hlt::ShipDockingStatus::Undocked){
                    continue;
                }
                hlt::Ship closestDefender = closestShipToLoc(enemyShips,s.location,true);
                double myScore = 255 - s.health + s.location.get_distance_to(closestDefender.location);
                if(myScore > bestScore){
                    bestScore = myScore;
                    ret = s;
                }
            }
            return ret;
        }

        hlt::Location getMicroTarget(){
            if(type == ObjType::harassPlanet){
                hlt::Ship targetEnemy = this->getTargetDocker();
                return targetEnemy.location;        
            }
            else if(type == ObjType::defendPlanet){
                hlt::Ship closestEnemy = closestShipToLoc(enemyShips,targetLoc,false);
                return getMid(targetLoc,closestEnemy.location);
            }
            return targetLoc;
        }
        
        void addShip(int ship) {
            myShips.insert(ship);
        }

        void removeShip(int ship){
            myShips.erase(ship);
        }
        
        void addEnemyShip(const hlt::Ship& ship){
            if(ship.location.get_distance_to(targetLoc) <= enemyRelevanceRad){
                //std::ostringstream eas;
                //eas << " adding " << ship.entity_id;
                //hlt::Log::log(eas.str());
                if(ship.docking_status!=hlt::ShipDockingStatus::Undocked && 
                    (type != ObjType::harassPlanet || ship.location.get_distance_to(targetLoc) > targetRad+5 )) return;
                enemyShips.insert(ship);
            }
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
