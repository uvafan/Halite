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

namespace djj {
    struct Macrocontroller {
        std::unordered_map<int,int> shipsLastSeen;
        std::unordered_map<int,djj::Ship> shipsByID;
        std::map<Objective,int> objectiveLastNeeded;
        hlt::Map curMap;
        int player_id;

        static Macrocontroller newMacrocontroller(const hlt::Map& m, int pid){
            std::unordered_map<int,int> sls;
            std::unordered_map<int,djj::Ship> sbid;
            std::map<Objective,int> oln;
            for(const hlt::Ship& ship : m.ships.at(pid)){
                sls[ship.entity_id]= -1;
                sbid[ship.entity_id] = Ship::makeShip(ship.entity_id,ship.location);
            }
            for(const hlt::Planet& planet : m.planets){
                oln[Objective::newObjective(ObjType::dockUnownedPlanet,planet.location,planet.radius)] = -1;
            }
            return {sls,sbid,oln,m,pid};
        }

        void updateMapInfo(const hlt::Map& m, int turn){
            for(const hlt::Ship& ship : m.ships.at(player_id)){
                shipsLastSeen[ship.entity_id] = turn;
            }
            for(const hlt::Planet& planet : m.planets){
                //TODO: change type depending on circumstances
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
            //remove all the stuff in shipsLastSeen and objectiveLastNeeded and shipsByID that don't don't have val turn
            curMap = m;
        }

        std::vector<hlt::Move> getMoves(int turn){
            std::vector<hlt::Move> moves;
            return moves;
        }

    };
}
