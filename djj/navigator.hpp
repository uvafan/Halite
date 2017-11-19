#pragma once

#include "../hlt/hlt.hpp"
#include "../hlt/location.hpp"
#include "../hlt/map.hpp"
#include "../hlt/move.hpp"
#include <vector>
#include <set>
#include <math.h>
#include <queue>
#include <stack>
#include <ctime>

#define PI 3.14159265
#define NUM_DIRS 8

namespace djj {
    struct node {
        double x;
        double y;
        double f;
        double g;
        node* p;
        bool operator<(const node& cmp) const{
            return f > cmp.f;
        }
    };

    struct Navigator {
        int R;
        int C;
        std::vector<std::vector<std::set<int> > > map;

        static Navigator newNavigator(const hlt::Map& init_map) {
            int rows = init_map.map_height;
            int cols = init_map.map_width;
            std::vector<std::vector<std::set<int> > > myMap(cols,std::vector<std::set<int> >(rows,std::set<int>()));
            for(auto p: init_map.planets){
                hlt::Location ploc = p.location;
                double rad = p.radius;
                int minX = (int)(ploc.pos_x - rad); int maxX = (int)(ploc.pos_x + rad + 1);
                int minY = (int)(ploc.pos_y - rad); int maxY = (int)(ploc.pos_y + rad + 1);
                for(int x = minX; x <= maxX; x++){
                    if(x<0||x>=cols)continue;
                    for(int y = minY; y <= maxY; y++){
                        if(y<0||y>=rows)continue;
                        if(ploc.get_distance_to(hlt::Location::newLoc(x,y))<=rad){
                            myMap[x][y].insert(-1);
                        }
                    }
                }
            }
            hlt::Log::log("finished initializing");
            return {rows, cols, myMap};
        }
        
        //get plan to get within radius rad - A*, then add plan to map
        std::queue<hlt::Move> getPlan(int ID, const hlt::Location& source, const hlt::Location& target, double rad, int turn){
            std::ostringstream initial;
            initial << "getting plan source = " << source.pos_x << " " << source.pos_y
                    << " target = " << target.pos_x << " " << target.pos_y << " rad = " << rad;
            hlt::Log::log(initial.str());
            std::priority_queue<node> open;
            std::vector<std::vector<int> > openMap(C,std::vector<int>(R,0));  
            std::vector<std::vector<int> > closedMap(C,std::vector<int>(R,0));  
            node start = {source.pos_x,source.pos_y,0,0,nullptr};
            open.push(start);
            bool done = false;
            while(!open.empty()&&!done){
                std::ostringstream debug;
                node q = open.top(); open.pop();
                debug << "processing x = " << q.x << " y = " << q.y << " g = " << q.g << " f = " << q.f;
                hlt::Log::log(debug.str());
                for(int i = 0; i < NUM_DIRS; i++){
                    std::ostringstream timeinfo;
                    clock_t begin = clock();
                    double dx = 7 * cos(((double)(i)/NUM_DIRS)*2*PI);
                    double dy = 7 * sin(((double)(i)/NUM_DIRS)*2*PI);
                    double nx = q.x+dx; double ny = q.y+dy;
                    if(nx<0.5||nx>=(C-.5)||ny<0.5||ny>=(C-.5))continue;
                    hlt::Move move = hlt::Move::thrust_rad(ID,7,((double)(i)/NUM_DIRS)*2*PI);    
                    hlt::Location loc = hlt::Location::newLoc(q.x,q.y);
                    double e1 = double(clock()-begin) / CLOCKS_PER_SEC;
                    if(!checkMove(move,loc,turn+q.g,false,false))continue;
                    double e2 = double(clock()-begin) / CLOCKS_PER_SEC;
                    hlt::Location nextLoc = hlt::Location::newLoc(nx,ny);
                    double dist = nextLoc.get_distance_to(target);
                    if(dist<=rad){
                        std::stack<hlt::Move> s;
                        int toX = nx; int toY = ny;
                        do{
                            s.push(getMove(ID,q.x,q.y,toX,toY));
                            toX = q.x;
                            toY = q.y;
                            q = *q.p;
                        }while(q.p!=nullptr);
                        std::queue<hlt::Move> qu;
                        while(!s.empty()){
                            qu.push(s.top()); s.pop();
                        }
                        addPlan(qu,source,turn);
                        return qu;
                    }
                    double sg = q.g+1;
                    double sf = sg+dist;
                    if(openMap[int(nx+.5)][int(ny+.5)]&&openMap[int(nx+.5)][int(ny+.5)]<sf)continue;
                    if(closedMap[int(nx+.5)][int(ny+.5)]&&closedMap[int(nx+.5)][int(ny+.5)]<sf)continue;
                    openMap[int(nx+.5)][int(ny+.5)] = sf;
                    node successor = {nx,ny,sf,sg,&q};
                    open.push(successor);
                    double e3 = double(clock()-begin) / CLOCKS_PER_SEC;
                    timeinfo<< "e1 = " << e1 << " e2 = " << e2 << " e3 = " << e3;
                    hlt::Log::log(timeinfo.str());
                }
                closedMap[int(q.x+.5)][int(q.y+.5)] = q.f;
            }
            std::queue<hlt::Move> rip;
            return rip;
        }

        static hlt::Move getMove(int ID, double x1, double y1, double x2, double y2){
            double dx = x2-x1; double dy = y2-y1;
            double thrust = sqrt(dx*dx+dy*dy);
            double angle = atan(dy/dx);
            return hlt::Move::thrust_rad(ID,thrust,angle);
        }

        //removes all moves in plan from map
        void removePlan(std::queue<hlt::Move> plan, const hlt::Location& start, int turn){
            hlt::Location track = start;
            while(!plan.empty()){
                if(!this->checkMove(plan.front(),track,turn,false,true))hlt::Log::log("Error when removing plan.");
                track = nextLoc(plan.front(),track);
                plan.pop();
                turn++;
            }
        }
        
        //adds all moves in plan from map
        void addPlan(std::queue<hlt::Move> plan, const hlt::Location& start, int turn){
            hlt::Location track = start;
            while(!plan.empty()){
                if(this->checkMove(plan.front(),track,turn,true,false))hlt::Log::log("Error when adding plan!");
                track = nextLoc(plan.front(),track);
                plan.pop();
                turn++;
            }
        }

        //alters path of move in some way
        bool checkMove(hlt::Move move, const hlt::Location& start, int turn, bool add, bool remove){
            int subturn = turn*7;
            std::pair<double,double> dxdy = movetodxdy(move);
            double stepx = dxdy.first/7.0;
            double stepy = dxdy.second/7.0;
            double curX = start.pos_x;
            double curY = start.pos_y;
            for(int i = 0;i < 7;i++){
                subturn++;
                curX += stepx; curY += stepy;
                if(!markPos(hlt::Location::newLoc(curX,curY), subturn, add, remove))return false;
            }
            return true;
        }   

        //mark all positions in map within ship radius of loc
        bool markPos(const hlt::Location& loc, int turn, bool add, bool remove){
            std::vector<int> xchecks, ychecks;
            xchecks.push_back((int)(loc.pos_x)); xchecks.push_back((int)(loc.pos_x+1));
            ychecks.push_back((int)(loc.pos_y)); xchecks.push_back((int)(loc.pos_y+1));
            for(int i = 0; i < 2; i++){
                for(int j= 0; j < 2; j++){
                    if(xchecks[i]<0||xchecks[i]>=C||ychecks[j]<0||ychecks[j]>=R)continue;
                    if(loc.get_distance_to(hlt::Location::newLoc(xchecks[i],ychecks[j])) <= 0.5){
                        if((!remove && map[xchecks[i]][ychecks[j]].find(turn) != map[xchecks[i]][ychecks[j]].end()) ||
                               map[xchecks[i]][ychecks[j]].find(-1) != map[xchecks[i]][ychecks[j]].end())  return false;
                        if(add){
                            map[xchecks[i]][ychecks[j]].insert(turn);
                        }
                        else if(remove) map[xchecks[i]][ychecks[j]].erase(turn);
                    }
                }
            }
            return true;
        } 

        static std::pair<double,double> movetodxdy(hlt::Move move){
            double dx = move.move_thrust * cos(move.move_angle_deg*PI/180);
            double dy = move.move_thrust * sin(move.move_angle_deg*PI/180);
            return std::pair<double,double>(dx,dy);
        }

        static hlt::Location nextLoc(hlt::Move move, hlt::Location& start){
            std::pair<double,double> dxdy = movetodxdy(move);
            return hlt::Location::newLoc(start.pos_x+dxdy.first,start.pos_y+dxdy.second);
        }

    };
}
