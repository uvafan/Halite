#pragma once

#include "../hlt/location.hpp"
#include "../hlt/move.hpp"
#include "objective.hpp"
#include <vector>
#include <queue>

namespace djj {
    struct Ship {
        int ID;
        hlt::Location dest;
        std::queue<hlt::Move> plan;
        Objective obj;

        static Ship makeShip(int myID, const hlt::Location& loc){
            return {myID,loc,std::queue<hlt::Move>(),Objective::newObjective(ObjType::noop,loc,0)};
        }

        bool operator <(const Ship& s) const{
            return ID < s.ID;
        }
    };
}
