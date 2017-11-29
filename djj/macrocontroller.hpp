#pragma once

#include "../hlt/location.hpp"
#include "../hlt/move.hpp"
#include "../hlt/map.hpp"
#include "ship.hpp"
#include "navigator.hpp"
#include "objective.hpp"
#include <vector>
#include <queue>
#include<map>

#define INF 100000000

namespace djj {
    struct Macrocontroller {
        std::unordered_map<int,int> shipsLastSeen;
        std::unordered_map<int,djj::Ship> shipsByID;
        std::map<Objective,int> objectiveLastNeeded;
        std::vector<hlt::Location> enemyLocs;
        hlt::Map curMap;
        Navigator nav;
        int player_id;

        static Macrocontroller newMacrocontroller(const hlt::Map& m, int pid){
            std::unordered_map<int,int> sls;
            std::unordered_map<int,djj::Ship> sbid;
            std::map<Objective,int> oln;
            std::vector<hlt::Location> el;
            Navigator n = Navigator::newNavigator(m);

            for(const hlt::Ship& ship : m.ships.at(pid)){
                sls[ship.entity_id]= -1;
                sbid[ship.entity_id] = Ship::makeShip(ship.entity_id,ship.location,ship.location);
            }
            for(const hlt::Planet& planet : m.planets){
                oln[Objective::newObjective(ObjType::dockUnownedPlanet,planet.location,planet.radius)] = -1;
            }
            return {sls,sbid,oln,el,m,n,pid};
        }

        void updateMapInfo(const hlt::Map& m, int turn){
            for(const hlt::Ship& ship : m.ships.at(player_id)){
                if(shipsLastSeen.find(ship.entity_id)==shipsLastSeen.end()){
                    shipsByID[ship.entity_id] = Ship::makeShip(ship.entity_id,ship.location,ship.location);
                }
                shipsByID[ship.entity_id].setLocation(ship.location);
                shipsByID[ship.entity_id].setDocked(ship.docking_status != hlt::ShipDockingStatus::Undocked); 
                shipsLastSeen[ship.entity_id] = turn;
            }
            for(const hlt::Planet& planet : m.planets){
                //TODO: remove dead planets from nav map 
                std::vector<Objective> objs;
                if(!planet.owned){
                    objs.push_back(Objective::newObjective(ObjType::dockUnownedPlanet,planet.location,planet.radius));
                }
                else{
                    std::vector<hlt::EntityId> docked_ships = planet.docked_ships;

                    if(shipsLastSeen.find(docked_ships[0]) != shipsLastSeen.end()){
                        objs.push_back(Objective::newObjective(ObjType::defendPlanet,planet.location,planet.radius));
                        if(!planet.is_full()){
                            objs.push_back(Objective::newObjective(ObjType::dockOwnedPlanet,planet.location,planet.radius));
                        }
                    }
                    else{
                        objs.push_back(Objective::newObjective(ObjType::harassPlanet,planet.location,planet.radius));
                    }
                }
                for(Objective o: objs){
                    auto it = objectiveLastNeeded.find(o);
                    if(it == objectiveLastNeeded.end()){
                        objectiveLastNeeded[o] = turn;
                    }
                    else{
                        objectiveLastNeeded[it->first] = turn;
                    }
                }
            }
            for(auto it = shipsLastSeen.begin(); it != shipsLastSeen.end();){
                if(it->second!=turn){
                    Ship s = shipsByID.find(it->first);
                    nav.removeDock(s.myLoc);
                    shipsByID.erase(s);
                    shipsLastSeen.erase(it++);                
                }
                else ++it;
            }
            for(auto it = objectiveLastNeeded.begin(); it != objectiveLastNeeded.end();){
                if(it->second!=turn){
                    objectiveLastNeeded.erase(++it);
                }
                else ++it;
            }
            curMap = m;
            this->updateEnemyLocs();
            this->updateObjectives();
        }

        void updateEnemyLocs(){
            enemyLocs.clear();
            for(auto s: curMap.ships){
                if(s.first!=player_id){
                    enemyLocs.push_back(s.second[0].location);
                }
            }
        }

        void updateObjectives(){
            for(auto it = objectiveLastNeeded.begin(); it != objectiveLastNeeded.end(); it++){
                Objective o = it->first;
                //remove dead ships from myShips
                for(auto s: o.myShips){
                    if(shipsByID.find(s)==shipsByID.end()){
                        o.removeShip(s);
                    }
                }
                //update enemy ships near me
                o.clearEnemyLocs();
                for(const hlt::Location& loc: enemyLocs){
                    o.addEnemyLoc(loc);
                }
                //update priority
                o.updatePriority();
            }
        }

        void assignObjective(Ship s){
            int maxScore = -INF; 
            Objective bestObj = Objective::newObjective(ObjType::noop, s.myLoc, 0);
            for(auto it: objectiveLastNeeded){
                Objective o = it.first;
                int myScore = o.priority - s.myLoc.get_distance_to(o.targetLoc);
                if(myScore>maxScore){
                    maxScore = myScore;
                    bestObj = o;
                }
            }
            s.setObjective(bestObj);
            bestObj.addShip(s.ID);
            bestObj.updatePriority();
        }

        std::vector<hlt::Move> getMoves(int turn){
            std::vector<hlt::Move> moves;
            //assign objectives to ships without them, mark docked ships
            for(auto it: shipsByID){
                Ship s = it.second;
                if(s.docked){
                    nav.markDock(s.myLoc);
                    continue;
                }
                nav.removeDock(s.myLoc);
                Objective myObj = it.second.obj;
                if(objectiveLastNeeded.find(myObj) == objectiveLastNeeded.end()){
                    assignObjective(it.second);
                }
            }
            //notify navigators of enemy ship locations
            //for groups of ships within 19 of enemy ship, create microcontroller and get moves from that
            //for ships which didn't get added to a microcontroller, add moves  
            return moves;
        }

    };
}
