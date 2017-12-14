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
#include <chrono>

#define INF 100000000

namespace djj {
    struct Macrocontroller {
        std::unordered_map<int,Ship> shipsByID;
        std::set<Objective> objs;
        std::set<hlt::Ship> enemyShips;
        hlt::Map curMap;
        Navigator nav;
        int player_id;
        int numPlayers;

        static Macrocontroller newMacrocontroller(const hlt::Map& m, int pid){
            std::unordered_map<int,Ship> sbid;
            std::set<Objective> os;
            std::set<hlt::Ship> es;
            Navigator n = Navigator::newNavigator(m);

            for(const hlt::Ship& ship : m.ships.at(pid)){
                sbid[ship.entity_id] = Ship::makeShip(ship.entity_id,ship.location);
            }
            for(const hlt::Planet& planet : m.planets){
                os.insert(Objective::newObjective(ObjType::dockUnownedPlanet,planet.location,planet.radius+4,0,planet.docking_spots));
            }
            return {sbid,os,es,m,n,pid,(int)(m.ship_map.size())};
        }

        void updateMapInfo(const hlt::Map& m, int turn){
            hlt::Log::log("updating ships");
            std::vector<int> myShips;
            for(const hlt::Ship& ship : m.ships.at(player_id)){
                if(shipsByID.find(ship.entity_id)==shipsByID.end()){
                    shipsByID[ship.entity_id] = Ship::makeShip(ship.entity_id,ship.location);
                    //hlt::Log::log("hello");
                }
                shipsByID[ship.entity_id].setLocation(ship.location);
                shipsByID[ship.entity_id].setDocked(ship.docking_status != hlt::ShipDockingStatus::Undocked); 
                std::ostringstream attendance;
                attendance << ship.entity_id << " in attendance.";
                hlt::Log::log(attendance.str());
                myShips.push_back(ship.entity_id);
            }
            hlt::Log::log("updating objectives");
            objs.clear();
            for(const hlt::Planet& planet : m.planets){
                if(!planet.owned){
                    objs.insert(Objective::newObjective(ObjType::dockUnownedPlanet,planet.location,planet.radius+4,0,planet.docking_spots));
                }
                else{
                    if(planet.owner_id == player_id){
                        Objective o = Objective::newObjective(ObjType::defendPlanet,planet.location,planet.radius+4,planet.docked_ships.size(),planet.docking_spots);
                        objs.insert(o);
                        if(!planet.is_full()){
                            Objective o2 = Objective::newObjective(ObjType::dockOwnedPlanet,planet.location,planet.radius+4,planet.docked_ships.size(),planet.docking_spots);
                            objs.insert(o2); 
                        }
                    }
                    else{
                        Objective o = Objective::newObjective(ObjType::harassPlanet,planet.location,planet.radius+4,planet.docked_ships.size(),planet.docking_spots);
                        objs.insert(o);
                    }
                }
            }
            hlt::Log::log("removing ships");
            std::unordered_map<int,Ship> nextsid;
            for(int sid: myShips){
                nextsid[sid] = shipsByID[sid];
                std::ostringstream sdebug;
                sdebug << "sid " << shipsByID[sid].obj;
                //hlt::Log::log(sdebug.str());
            }
            shipsByID = nextsid;
            curMap = m;
            this->updateEnemyShips();
            this->updateObjectives();
        }

        void updateEnemyShips(){
            enemyShips.clear();
            for(int i=0;i<numPlayers;i++){
                if(i==player_id)continue;
                for(const hlt::Ship& s: curMap.ships.at(i)){
                    enemyShips.insert(s);
                }
            }
        }

        void updateObjectives(){
            std::set<Objective> oldObjs = objs;
            objs.clear();
            for(Objective o: oldObjs){
                for(auto s: shipsByID){
                    if(s.second.obj == o){
                        o.addShip(s.second.ID);
                    }
                }
                //update enemy ships near me
                o.clearEnemyShips();
                for(const hlt::Ship& s: enemyShips){
                    //std::ostringstream debug;
                    //debug << "adding enemy ship " << s.entity_id;
                    //hlt::Log::log(debug.str());
                    o.addEnemyShip(s);
                }
                //update priority
                o.updatePriority();
                objs.insert(o);
            }
        }

        std::vector<hlt::Move> doMicro(Objective o, std::vector<hlt::Move> moves, int turn){
            std::ostringstream microd;
            microd << "doing micro ";
            int myShipCount = o.myShips.size() - o.myDocked;
            int pid = -1;
            hlt::Planet toDock = curMap.planets[0];
            for(const hlt::Planet& p: curMap.planets){
                if(p.location.pos_x == o.targetLoc.pos_x && p.location.pos_y == o.targetLoc.pos_y && !p.is_full()) {
                    pid = p.entity_id;
                    toDock = p;
                }
            }
            if(o.type == ObjType::dockOwnedPlanet){
                for(Objective o2: objs){
                    if(o2.targetLoc.pos_x == o.targetLoc.pos_x && o2.targetLoc.pos_y == o.targetLoc.pos_y && o2.type == ObjType::defendPlanet){
                        myShipCount += o2.myShips.size() - o.myDocked;
                    }
                }
            }
            int theirShipCount = o.enemyShips.size();
            for(const hlt::Ship& s: enemyShips){
                if(s.docking_status != hlt::ShipDockingStatus::Undocked)theirShipCount--;
            }
            int aggressionFactor = myShipCount - theirShipCount;
            microd << "aggFactor = " << aggressionFactor;
            hlt::Location target = o.getMicroTarget();
            microd << " target = " << target.pos_x << " " << target.pos_y;
            if(aggressionFactor>0){
                //int shipsSwarming = 0;
                //hlt::Location swarmLoc = target;
                for(int sid: o.myShips){
                    Ship s = shipsByID[sid];
                    if(s.docked)continue;
                    if(o.type != ObjType::harassPlanet && aggressionFactor > 0 && pid > -1 && s.myLoc.get_distance_to(toDock.location) < 4 + toDock.radius && !toDock.is_full()){
                        moves.push_back(hlt::Move::dock(sid,pid));
                        shipsByID[sid].setObjective(Objective::newObjective(ObjType::defendPlanet,toDock.location,0,0,0));
                        microd << " ship " << sid << " docks ";
                        nav.markDock(s.myLoc);
                        aggressionFactor--;
                        continue;
                    }
                    std::pair<hlt::Move,hlt::Location> info = nav.getAggressiveMove(s.myLoc,target,turn,sid);
                    microd << " added move for ship " << sid << " with end loc " << info.second.pos_x << " " << info.second.pos_y;
                    moves.push_back(info.first);
                    //swarmLoc = updateSL(shipsSwarming,swarmLoc,info.second);
                    //microd << " new SL = " << swarmLoc.pos_x << " " << swarmLoc.pos_y;
                    //shipsSwarming++;
                }
            }
            else{
                for(int sid: o.myShips){
                    Ship s = shipsByID[sid];
                    if(s.docked)continue;
                    shipsByID[sid].setPlan(std::queue<hlt::Move>());
                    hlt::Move move = nav.getPassiveMove(s.myLoc,target,turn,sid);
                    microd << " added move for ship " << sid;
                    moves.push_back(move);
                }
            }
            hlt::Log::log(microd.str());
            return moves;
        }

        static hlt::Location updateSL(int numShips, hlt::Location med, hlt::Location toAdd){
            if(!numShips) return toAdd;
            double newX = (med.pos_x*numShips+toAdd.pos_x) / (numShips+1);
            double newY = (med.pos_y*numShips+toAdd.pos_y) / (numShips+1);
            return hlt::Location::newLoc(newX,newY);
        }

        std::vector<hlt::Move> determineMoves(Objective o, std::vector<hlt::Move> moves, int turn){
            //if the distance from targetLoc to enemy is too close, do micro shit; otherwise, move as safely as possible
            if(!o.myShips.size())return moves;
            std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
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
                    if(s.docked){
                        debug << " docked at " << s.myLoc.pos_x << " " << s.myLoc.pos_y;
                        hlt::Log::log(debug.str());
                        continue;
                    }
                    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(end-start);
                    if(time_span.count() < 0.5 && (s.plan.empty() || !nav.checkMove(s.plan.front(),s.myLoc,-1,false,false))){
                        bool stop = false;
                        if(o.type == ObjType::dockUnownedPlanet || o.type == ObjType::dockOwnedPlanet){
                            for(const hlt::Planet& p: curMap.planets){
                                if(s.myLoc.get_distance_to(p.location) < 4 + p.radius && !p.is_full() && p.location.pos_x == targetLoc.pos_x && p.location.pos_y == targetLoc.pos_y){
                                    stop = true;
                                    moves.push_back(hlt::Move::dock(sid,p.entity_id));
                                    debug << " docking";
                                    nav.markDock(s.myLoc);
                                    shipsByID[sid].setObjective(Objective::newObjective(ObjType::defendPlanet,p.location,0,0,0));
                                    hlt::Log::log(debug.str());
                                    break;
                                }
                            }
                        }
                        if(stop)continue;
                        debug << "calculating new plan from " << s.myLoc.pos_x << " " << s.myLoc.pos_y << " to " << targetLoc.pos_x << " " << targetLoc.pos_y;
                        bool addDockToPlan = (o.type == ObjType::dockUnownedPlanet || o.type == ObjType::dockOwnedPlanet);
                        shipsByID[sid].setPlan(nav.getPlan(sid,s.myLoc,targetLoc,o.targetRad,turn,addDockToPlan));
                    }
                    if(!shipsByID[sid].plan.empty()){
                        hlt::Move move = shipsByID[sid].plan.front();
                        debug << " moving from plan with thrust " << move.move_thrust << " and angle " << move.move_angle_deg;
                        moves.push_back(move);
                        shipsByID[sid].plan.pop();
                    }
                    else{
                        debug << " no move found";
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
            std::unordered_map<int,Ship> newsid;
            for(auto it: shipsByID){
                Ship s = it.second;
                Objective myObj = s.obj;
                if(objs.find(myObj) == objs.end()){
                    std::ostringstream assignDebug;
                    double maxScore = -INF; 
                    Objective bestObj = Objective::newObjective(ObjType::noop, s.myLoc, 0,0,0);
                    for(Objective o: objs){
                        double myScore = o.priority - s.myLoc.get_distance_to(o.targetLoc);
                        assignDebug << "considering " << o << " score = " << myScore << std::endl;
                        if(myScore>maxScore){
                            maxScore = myScore;
                            bestObj = o;
                        }
                    }
                    s.setObjective(bestObj);
                    if(!s.plan.empty())nav.removePlan(s.plan,s.myLoc,turn);
                    //in case we were planning to dock
                    nav.checkMove(hlt::Move::noop(),s.myLoc,turn,false,true);
                    s.setPlan(std::queue<hlt::Move>());
                    objs.erase(bestObj);
                    bestObj.addShip(s.ID);
                    bestObj.updatePriority();
                    objs.insert(bestObj);
                    assignDebug << "assigning new " << bestObj << " to ship " << s.ID << " old was " << myObj;
                    hlt::Log::log(assignDebug.str());
                }
                newsid[s.ID] = s;
            }
            shipsByID = newsid;
            for(auto it: shipsByID){
                std::ostringstream objdebug;
                objdebug << "ship = " << it.first << " " << it.second.obj;
                //hlt::Log::log(objdebug.str());
            }
            hlt::Log::log("notifying navigator of enemy ships");
            //notify navigator of enemy ship locations
            nav.clearEnemies();
            for(const hlt::Ship& s: enemyShips){
                if(s.docking_status == hlt::ShipDockingStatus::Undocked)
                    nav.addEnemyShip(s,turn,true);    
                else
                    nav.addEnemyShip(s,turn,false);

            }
            //for each objective, get moves
            hlt::Log::log("getting moves");
            for(Objective o: objs){
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
