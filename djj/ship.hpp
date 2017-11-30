#pragma once

#include "../hlt/location.hpp"
#include "../hlt/move.hpp"
#include "objective.hpp"
#include <vector>
#include <queue>

namespace djj {
    struct Ship {
        int ID;
        hlt::Location myLoc;
        std::queue<hlt::Move> plan;
        bool docked;
        Objective obj;

        static Ship makeShip(int myID, const hlt::Location& loc){
            return {myID,loc,std::queue<hlt::Move>(),false,Objective::newObjective(ObjType::noop,loc,0)};
        }

        void setPlan(std::queue<hlt::Move> p){
            plan = p;
        }

        void setDocked(bool d){
            docked = d;
        }

        void setLocation(const hlt::Location& loc){
            myLoc = loc;
        }

        void setObjective(Objective o){
            obj = o;
        }

        bool operator <(const Ship& s) const{
            return ID < s.ID;
        }
    };
}
