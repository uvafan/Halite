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
                //hlt::Log::log("h");
                oln[Objective::newObjective(ObjType::dockUnownedPlanet,planet.location,planet.radius)] = -1;
                //hlt::Log::log("b");
            }
            return {sls,sbid,oln,pid};
        }

        void updateMapInfo(const hlt::Map& m, int turn){
            for(const hlt::Ship& ship : m.ships.at(player_id)){
                shipsLastSeen[ship.entity_id] = turn;
            }
            for(const hlt::Planet& planet : m.planets){
                //TODO: change type depending on circumstances
                Objective o = Objective::newObjective(ObjType::dockUnownedPlanet,planet.location,planet.radius);
                auto it = objectiveLastNeeded.find(o);
                if(it == objectiveLastNeeded.end()){
                    objectiveLastNeeded[o] = turn;
                }
                else{
                    objectiveLastNeeded[it->first] = turn;
                }
            }
        }
        
        std::vector<hlt::Move> getMoves(int turn){
            std::vector<hlt::Move> moves;
            return moves;
        }

    };
}
