#pragma once

#include "../hlt/location.hpp"
#include "../hlt/move.hpp"
#include "../hlt/map.hpp"
#include "ship.hpp"
#include "navigator.hpp"
#include "objective.hpp"
#include <vector>
#include <queue>
#include <map>

#define INF 100000000

namespace djj {
    struct Macrocontroller {
        std::unordered_map<int,int> shipsLastSeen;
        std::unordered_map<int,Ship> shipsByID;
        std::map<Objective,int> objectiveLastNeeded;
        std::set<hlt::Ship> enemyShips;
        hlt::Map curMap;
        Navigator nav;
        int player_id;

        static Macrocontroller newMacrocontroller(const hlt::Map& m, int pid){
            std::unordered_map<int,int> sls;
            std::unordered_map<int,Ship> sbid;
            std::map<Objective,int> oln;
            std::set<hlt::Ship> es;
            Navigator n = Navigator::newNavigator(m);

            for(const hlt::Ship& ship : m.ships.at(pid)){
                sls[ship.entity_id]= -1;
                sbid[ship.entity_id] = Ship::makeShip(ship.entity_id,ship.location);
            }
            for(const hlt::Planet& planet : m.planets){
                oln[Objective::newObjective(ObjType::dockUnownedPlanet,planet.location,planet.radius+4)] = -1;
            }
            return {sls,sbid,oln,es,m,n,pid};
        }

        void updateMapInfo(const hlt::Map& m, int turn){
            hlt::Log::log("updating ships");
            for(const hlt::Ship& ship : m.ships.at(player_id)){
                if(shipsLastSeen.find(ship.entity_id)==shipsLastSeen.end()){
                    shipsByID[ship.entity_id] = Ship::makeShip(ship.entity_id,ship.location);
                }
                shipsByID[ship.entity_id].setLocation(ship.location);
                shipsByID[ship.entity_id].setDocked(ship.docking_status != hlt::ShipDockingStatus::Undocked); 
                std::ostringstream attendance;
                attendance << ship.entity_id << " in attendance.";
                hlt::Log::log(attendance.str());
                shipsLastSeen[ship.entity_id] = turn;
            }
            hlt::Log::log("updating objectives");
            for(const hlt::Planet& planet : m.planets){
                //TODO: remove dead planets from nav map 
                std::vector<Objective> objs;
                if(!planet.owned){
                    objs.push_back(Objective::newObjective(ObjType::dockUnownedPlanet,planet.location,planet.radius+4));
                }
                else{
                    std::vector<hlt::EntityId> docked_ships = planet.docked_ships;

                    if(shipsLastSeen.find(docked_ships[0]) != shipsLastSeen.end()){
                        objs.push_back(Objective::newObjective(ObjType::defendPlanet,planet.location,planet.radius+4));
                        if(!planet.is_full()){
                            objs.push_back(Objective::newObjective(ObjType::dockOwnedPlanet,planet.location,planet.radius+4));
                        }
                    }
                    else{
                        objs.push_back(Objective::newObjective(ObjType::harassPlanet,planet.location,planet.radius+4));
                    }
                }
                for(Objective o: objs){
                    auto it = objectiveLastNeeded.find(o);
                    if(it == objectiveLastNeeded.end()){
                        std::ostringstream debugobjadd;
                        debugobjadd << "adding " << o;
                        objectiveLastNeeded[o] = turn;
                    }
                    else{
                        objectiveLastNeeded[it->first] = turn;
                    }
                }
            }
            hlt::Log::log("removing ships");
            for(auto it = shipsLastSeen.cbegin(); it != shipsLastSeen.cend();){
                if(it->second!=turn){
                    std::ostringstream debugshipr;
                    auto it2 = shipsByID.find(it->first);
                    debugshipr << "removing ship " << it->first;
                    nav.removeDock(it2->second.myLoc);
                    shipsByID.erase(it2);
                    shipsLastSeen.erase(it++);                
                    hlt::Log::log(debugshipr.str());
                }
                else ++it;
            }
            hlt::Log::log("removing objectives");
            for(auto it = objectiveLastNeeded.cbegin(); it != objectiveLastNeeded.cend();){
                if(it->second!=turn){
                    std::ostringstream debugrip;
                    debugrip << "removing " << it->first;
                    hlt::Log::log(debugrip.str());
                    objectiveLastNeeded.erase(it++);
                }
                else ++it;
            }
            curMap = m;
            this->updateEnemyShips();
            this->updateObjectives();
        }

        void updateEnemyShips(){
            enemyShips.clear();
            for(auto s: curMap.ships){
                if(s.first!=player_id){
                    enemyShips.insert(s.second[0]);
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
                o.clearEnemyShips();
                for(const hlt::Ship& s: enemyShips){
                    o.addEnemyShip(s);
                }
                //update priority
                o.updatePriority();
            }
        }

        std::vector<hlt::Move> doMicro(Objective o, std::vector<hlt::Move> moves, int turn){
            int myShipCount = o.myShips.size();
            for(int sid: o.myShips){
                Ship s = shipsByID[sid];
                if(s.docked)myShipCount--;
            }
            int theirShipCount = o.enemyShips.size();
            for(const hlt::Ship& s: enemyShips){
                if(s.docking_status != hlt::ShipDockingStatus::Undocked)theirShipCount--;
            }
            bool aggressive = (myShipCount > theirShipCount);
            if(aggressive){
            }
            else{
            }
            return moves;
        }

        std::vector<hlt::Move> determineMoves(Objective o, std::vector<hlt::Move> moves, int turn){
            //if the distance from targetLoc to enemy is too close, do micro shit; otherwise, move as safely as possible
            if(!o.myShips.size())return moves;
            std::ostringstream objdebug;
            objdebug << o;
            hlt::Log::log(objdebug.str());
            hlt::Location targetLoc = o.targetLoc;
            bool microMode = o.enemyShips.size();
            if(microMode){
                moves = doMicro(o,moves,turn);
            }
            else{
                for(int sid: o.myShips){
                    //normal nav
                    std::ostringstream debug;
                    debug << "ship " << sid << " reporting for duty ";
                    Ship s = shipsByID[sid];
                    if(s.plan.empty() || !nav.checkMove(s.plan.front(),s.myLoc,-1,false,false)){
                        bool stop = false;
                        if(o.type == ObjType::dockUnownedPlanet || o.type == ObjType::dockOwnedPlanet){
                            for(const hlt::Planet& p: curMap.planets){
                                if(s.myLoc.get_distance_to(p.location) < 4 + p.radius && !p.is_full()){
                                    stop = true;
                                    moves.push_back(hlt::Move::dock(sid,p.entity_id));
                                    break;
                                }
                            }
                        }
                        if(stop)continue;
                        debug << "calculating new plan from " << s.myLoc.pos_x << " " << s.myLoc.pos_y << " to " << targetLoc.pos_x << " " << targetLoc.pos_y;
                        shipsByID[sid].setPlan(nav.getPlan(sid,s.myLoc,targetLoc,o.targetRad,turn));
                    }
                    if(!s.plan.empty()){
                        moves.push_back(s.plan.front());
                        shipsByID[sid].plan.pop();
                    }
                    hlt::Log::log(debug.str());
                }
            }
            return moves;
        }

        std::vector<hlt::Move> getMoves(int turn){
            std::vector<hlt::Move> moves;
            //assign objectives to ships without them, mark docked ships
            hlt::Log::log("assigning objectives");
            for(auto it: shipsByID){
                Ship s = it.second;
                if(s.docked){
                    nav.markDock(s.myLoc);
                    //TODO: determine when to undock
                    continue;
                }
                nav.removeDock(s.myLoc);
                Objective myObj = s.obj;
                if(objectiveLastNeeded.find(myObj) == objectiveLastNeeded.end()){
                    std::ostringstream assignDebug;
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
                    it.second.setObjective(bestObj);
                    it.second.setPlan(std::queue<hlt::Move>());
                    objectiveLastNeeded.erase(objectiveLastNeeded.find(bestObj));
                    bestObj.addShip(s.ID);
                    bestObj.updatePriority();
                    objectiveLastNeeded[bestObj] = turn;
                    assignDebug << "assigning new objective to ship " << s.ID;
                    hlt::Log::log(assignDebug.str());
                }
            }
            hlt::Log::log("notifying navigator of enemy ships");
            //notify navigator of enemy ship locations
            nav.clearEnemies();
            for(const hlt::Ship& s: enemyShips){
                if(s.docking_status == hlt::ShipDockingStatus::Undocked)
                    nav.addEnemyLoc(s.location,turn,true);    
                else
                    nav.addEnemyLoc(s.location,turn,false);

            }
            //for each objective, get moves
            hlt::Log::log("getting moves");
            for(auto it: objectiveLastNeeded){
                Objective o = it.first;
                moves = determineMoves(o,moves,turn);
            }
            //remove moves from nav
            hlt::Log::log("removing moves");
            for(hlt::Move& m: moves){
                if(m.type == hlt::MoveType::Thrust){
                    hlt::Location loc = shipsByID[m.ship_id].myLoc;
                    nav.checkMove(m,loc,turn,false,true);
                }
            }
            return moves;
        }

    };
}
