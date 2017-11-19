#pragma once

#include "../hlt/location.hpp"
#include "../hlt/move.hpp"
#include <vector>

namespace djj {
    struct Ship {
        int ID;
        hlt::Location dest;
        std::vector<hlt::Move> plan;

        static Ship makeShip(int myID, const hlt::Location& loc){
            return {myID,loc,std::vector<hlt::Move>()};
        }

        bool operator <(const Ship& s) const{
            return ID < s.ID;
        }
    };
}
