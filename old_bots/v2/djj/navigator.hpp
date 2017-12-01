#pragma once

#include "../hlt/hlt.hpp"
#include "../hlt/location.hpp"
#include "../hlt/map.hpp"
#include "../hlt/move.hpp"
#include <vector>
#include <set>
#include <unordered_set>
#include <math.h>
#include <queue>
#include <algorithm>
#include <stack>
#include <ctime>
#include <chrono>
#include <memory>

#define SUBTURNS 14 
#define PI 3.14159265
#define NUM_DIRS 12 
#define COMBAT_NUM_DIRS 20
#define COLLISION_THRESHOLD 1.0
#define INF 100000000

namespace djj {
    struct node {
        double x;
        double y;
        double f;
        double g;
        int p;
        bool operator<(const node& cmp) const{
            return f > cmp.f;
        }
    };

    struct Navigator {
        int R;
        int C;
        std::vector<std::vector<std::unordered_set<int> > > map;
        std::vector<std::vector<int> > enemyDockedMap;
        std::vector<std::vector<std::vector<std::pair<std::set<hlt::Ship>,std::set<hlt::Ship> > > > > enemyMap;

        static Navigator newNavigator(const hlt::Map& init_map) {
            int rows = init_map.map_height;
            int cols = init_map.map_width;
            std::vector<std::vector<int> > myEDMap (cols,std::vector<int>(rows,0));
            std::vector<std::vector<std::unordered_set<int> > > myMap(cols,std::vector<std::unordered_set<int> >(rows,std::unordered_set<int>()));
            std::vector<std::vector<std::vector<std::pair<std::set<hlt::Ship>,std::set<hlt::Ship> > > > > myEnemyMap(cols,std::vector<std::vector<std::pair<std::set<hlt::Ship>,std::set<hlt::Ship> > > >(rows,std::vector<std::pair<std::set<hlt::Ship>,std::set<hlt::Ship> > >(SUBTURNS,std::make_pair(std::set<hlt::Ship>(),std::set<hlt::Ship>()))));
            for(auto p: init_map.planets){
                hlt::Location ploc = p.location;
                double rad = p.radius;
                int minX = (int)(ploc.pos_x - rad); int maxX = (int)(ploc.pos_x + rad + 1);
                int minY = (int)(ploc.pos_y - rad); int maxY = (int)(ploc.pos_y + rad + 1);
                for(int x = minX; x <= maxX; x++){
                    if(x<0||x>=cols)continue;
                    for(int y = minY; y <= maxY; y++){
                        if(y<0||y>=rows)continue;
                        if(ploc.get_distance_to(hlt::Location::newLoc(x,y))<=rad+.5){
                            myMap[x][y].insert(-1);
                        }
                    }
                }
            }
            hlt::Log::log("finished initializing");
            return {rows, cols, myMap, myEDMap, myEnemyMap};
        }

        static bool compare(node a, node b){
            return a.f > b.f;
        }

        //get plan to get within radius rad - A*, then add plan to map
        std::queue<hlt::Move> getPlan(int ID, const hlt::Location& source, const hlt::Location& target, double rad, int turn){
            std::ostringstream initial;
            //initial << "getting plan source = " << source.pos_x << " " << source.pos_y
            //    << " target = " << target.pos_x << " " << target.pos_y << " rad = " << rad;
            //hlt::Log::log(initial.str());
            int thrusts[] = {4,7};
            std::priority_queue<node,std::vector<node>,std::function<bool(node,node)> > open(compare);
            std::vector<std::vector<int> > openMap(C,std::vector<int>(R,0));  
            std::vector<std::vector<int> > closedMap(C,std::vector<int>(R,0));  
            node start = {source.pos_x,source.pos_y,0,0,-1};
            open.push(start);
            bool done = false;
            std::queue<hlt::Move> ret;
            //if(source.get_distance_to(target)<=rad) return ret;
            std::vector<node> closed;
            while(!open.empty()&&!done){
                std::ostringstream debug;
                node q = open.top(); open.pop();
                //debug << "processing x = " << q.x << " y = " << q.y << " g = " << q.g << " f = " << q.f;
                //hlt::Log::log(debug.str());
                hlt::Location loc = hlt::Location::newLoc(q.x,q.y);
                for(int i = 0; i < NUM_DIRS; i++){
                    for(auto thrust: thrusts){
                        //std::ostringstream timeinfo;
                        double dx = thrust * cos(((double)(i)/NUM_DIRS)*2*PI);
                        double dy = thrust * sin(((double)(i)/NUM_DIRS)*2*PI);
                        double nx = q.x+dx; double ny = q.y+dy;
                        if(nx<0.5||nx>=(C-.5)||ny<0.5||ny>=(C-.5))continue;
                        hlt::Move move = hlt::Move::thrust_rad(ID,thrust,((double)(i)/NUM_DIRS)*2*PI);    
                        if(!checkMove(move,loc,turn+q.g,false,false))continue;
                        hlt::Location nextLoc = hlt::Location::newLoc(nx,ny);
                        double dist = nextLoc.get_distance_to(target);
                        if(dist<=rad){
                            std::stack<hlt::Move> s;
                            double toX = nx; double toY = ny;
                            //std::ostringstream success;
                            //success<<"success! final x = "<<nx<<" final y = "<<ny;
                            //hlt::Log::log(success.str());
                            while(1){
                                s.push(getMove(ID,q.x,q.y,toX,toY));
                                toX = q.x;
                                toY = q.y;
                                if(q.p==-1)break;
                                q = closed[q.p];
                                //std::ostringstream IDinf;
                                //IDinf<<"q.x = " << q.x;
                                //hlt::Log::log(IDinf.str());
                            }
                            while(!s.empty()){
                                ret.push(s.top()); s.pop();
                            }
                            addPlan(ret,source,turn);
                            done = true;
                            //delete q;
                            break;
                        }
                        double sg = q.g+1;
                        double sf = sg+dist;
                        if(openMap[int(nx+.5)][int(ny+.5)]&&openMap[int(nx+.5)][int(ny+.5)]<sf)continue;
                        if(closedMap[int(nx+.5)][int(ny+.5)]&&closedMap[int(nx+.5)][int(ny+.5)]<sf)continue;
                        openMap[int(nx+.5)][int(ny+.5)] = sf;
                        node successor = {nx,ny,sf,sg,(int)(closed.size())};
                        //std::ostringstream rip;
                        //hlt::Log::log(rip.str());
                        open.push(successor);
                    }
                    closedMap[int(q.x+.5)][int(q.y+.5)] = q.f;
                    closed.push_back(q);
                }
            }
            return ret;
        }

        static hlt::Move getMove(int ID, double x1, double y1, double x2, double y2){
            /*std::ostringstream moveinf;
              moveinf << "getting move from " << x1 << " " << y1 << " " << x2 << " " << y2;
              hlt::Log::log(moveinf.str());*/
            double dx = x2-x1; double dy = y2-y1;
            double thrust = sqrt(dx*dx+dy*dy);
            double angle = atan2(dy,dx);
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
                if(!this->checkMove(plan.front(),track,turn,true,false))hlt::Log::log("Error when adding plan!");
                track = nextLoc(plan.front(),track);
                plan.pop();
                turn++;
            }
        }

        //alters path of move in some way
        bool checkMove(hlt::Move move, const hlt::Location& start, int turn, bool add, bool remove){
            int subturn = turn*SUBTURNS;
            std::pair<double,double> dxdy = movetodxdy(move);
            double stepx = dxdy.first/(double)(SUBTURNS);
            double stepy = dxdy.second/(double)(SUBTURNS);
            double curX = start.pos_x;
            double curY = start.pos_y;
            for(int i = 0;i < SUBTURNS;i++){
                subturn++;
                curX += stepx; curY += stepy;
                if(!markPos(hlt::Location::newLoc(curX,curY), subturn, add, remove))return false;
            }
            return true;
        }  
        
        void clearEnemies(){
            enemyMap.assign(C,std::vector<std::vector<std::pair<std::set<hlt::Ship>,std::set<hlt::Ship> > > >(R,std::vector<std::pair<std::set<hlt::Ship>,std::set<hlt::Ship> > >(SUBTURNS,std::make_pair(std::set<hlt::Ship>(),std::set<hlt::Ship>()))));
            enemyDockedMap.assign(C,std::vector<int>(R,0));
        }

        void addEnemyShip(const hlt::Ship& s, int turn, bool undocked){
            hlt::Location loc = s.location;
            if(undocked){
                double stepDist = 7.0/SUBTURNS;
                int minX = std::max(0,int(loc.pos_x-12)); int maxX = std::min(C-1,int(loc.pos_x+13));
                int minY = std::max(0,int(loc.pos_y-12)); int maxY = std::min(R-1,int(loc.pos_y+13));
                for(int x = minX; x <= maxX; x++){
                    for(int y = minY; y <= maxY; y++){
                        //min turn at which enemy can attack me
                        double distToGo = std::max(0.0,loc.get_distance_to(hlt::Location::newLoc(x,y))-5);
                        int subturnsToGo = int(distToGo/stepDist);
                        for(int i=subturnsToGo; i<SUBTURNS; i++){
                            enemyMap[x][y][i].first.insert(s);
                        }
                    }
                }
            }
            else{
                int minX = std::max(0,int(loc.pos_x-5)); int maxX = std::min(C-1,int(loc.pos_x+6));
                int minY = std::max(0,int(loc.pos_y-5)); int maxY = std::min(R-1,int(loc.pos_y+6));
                for(int x = minX; x <= maxX; x++){
                    for(int y = minY; y <= maxY; y++){
                        //min turn at which enemy can attack me
                        double distToShip = loc.get_distance_to(hlt::Location::newLoc(x,y));
                        if(distToShip <= 0.5){ //don't collide!
                            for(int i=0;i<SUBTURNS;i++){
                                enemyDockedMap[x][y]=1;
                            }
                        }
                        else if(distToShip <= 4.5){ //kill him!
                            for(int i=0;i<SUBTURNS;i++){
                                enemyMap[x][y][i].second.insert(s);
                            }

                        }
                    }
                }

            }
        }

        double scoreMove(hlt::Move move, hlt::Location start, bool aggressive){
            std::pair<double,double> dxdy = movetodxdy(move);
            double stepx = dxdy.first/(double)(SUBTURNS);
            double stepy = dxdy.second/(double)(SUBTURNS);
            double curX = start.pos_x;
            double curY = start.pos_y;
            std::set<hlt::Ship> potentialDamagers;
            std::set<hlt::Ship> damagedDockers;
            for(int i = 0;i < SUBTURNS;i++){
                curX += stepx; curY += stepy;
                hlt::Location loc = hlt::Location::newLoc(curX,curY);
                std::vector<int> xchecks, ychecks;
                xchecks.push_back((int)(curX)); xchecks.push_back((int)(curX+1));
                ychecks.push_back((int)(curX)); ychecks.push_back((int)(curX+1));
                for(int i = 0; i < 2; i++){
                    for(int j= 0; j < 2; j++){
                        int x = xchecks[i]; int y = ychecks[j];
                        if(x<0||x>=C||y<0||y>=R)continue;
                        if(loc.get_distance_to(hlt::Location::newLoc(x,y)) <= COLLISION_THRESHOLD){
                            if(enemyDockedMap[x][y])return -INF;
                            for(hlt::Ship s: enemyMap[x][y][i].first){
                                potentialDamagers.insert(s);
                            }
                            for(hlt::Ship s: enemyMap[x][y][i].second){
                                damagedDockers.insert(s);
                            }

                        }
                    }
                }
            }
            if(aggressive){
                return damagedDockers.size()*100 - potentialDamagers.size()*20;
            }
            return 100 - potentialDamagers.size()*20; 
        }

        std::pair<hlt::Move,hlt::Location> getAggressiveMove(hlt::Location s, hlt::Location t, hlt::Location swarm, int turn, int ID){
            int thrusts[] = {2,4,7};
            hlt::Move bestMove = hlt::Move::noop();
            hlt::Location nextLoc = s;
            double bestScore = -INF;
            for(int i = 0; i < COMBAT_NUM_DIRS; i++){
                for(auto thrust: thrusts){
                    double dx = thrust * cos(((double)(i)/NUM_DIRS)*2*PI);
                    double dy = thrust * sin(((double)(i)/NUM_DIRS)*2*PI);
                    double nx = s.pos_x+dx; double ny = s.pos_y+dy;
                    if(nx<0.5||nx>=(C-.5)||ny<0.5||ny>=(C-.5))continue;
                    hlt::Move move = hlt::Move::thrust_rad(ID,thrust,((double)(i)/NUM_DIRS)*2*PI);    
                    if(!checkMove(move,s,turn,false,false))continue;
                    hlt::Location nextL = hlt::Location::newLoc(nx,ny);
                    double distToT = nextL.get_distance_to(t);
                    double distToSwarm = nextL.get_distance_to(swarm);
                    double score = scoreMove(move,s,true);
                    if(score - distToT - distToSwarm > bestScore){
                        bestScore = score - distToT - distToSwarm;
                        bestMove = move;
                        nextLoc = nextL;
                    }
                }
            }
            checkMove(bestMove,s,turn,true,false);
            return std::make_pair(bestMove,nextLoc);
        }

        hlt::Move getPassiveMove(hlt::Location s, hlt::Location t, int turn, int ID){
            int thrusts[] = {2,4,7};
            hlt::Move bestMove = hlt::Move::noop();
            double bestScore = -INF;
            for(int i = 0; i < COMBAT_NUM_DIRS; i++){
                for(auto thrust: thrusts){
                    double dx = thrust * cos(((double)(i)/NUM_DIRS)*2*PI);
                    double dy = thrust * sin(((double)(i)/NUM_DIRS)*2*PI);
                    double nx = s.pos_x+dx; double ny = s.pos_y+dy;
                    if(nx<0.5||nx>=(C-.5)||ny<0.5||ny>=(C-.5))continue;
                    hlt::Move move = hlt::Move::thrust_rad(ID,thrust,((double)(i)/NUM_DIRS)*2*PI);    
                    if(!checkMove(move,s,turn,false,false))continue;
                    hlt::Location nextL = hlt::Location::newLoc(nx,ny);
                    double distToT = nextL.get_distance_to(t);
                    double score = scoreMove(move,s,true);
                    if(score - distToT > bestScore){
                        bestScore = score - distToT;
                        bestMove = move;
                    }
                }
            }
            checkMove(bestMove,s,turn,true,false);
            return bestMove;
        }

        void markDock(const hlt::Location& loc){
            markPos(loc,-1,true,false);
        }

        void removeDock(const hlt::Location& loc){
            markPos(loc,-1,false,true);
        }

        //mark all positions in map within ship radius of loc
        bool markPos(const hlt::Location& loc, int turn, bool add, bool remove){
            std::vector<int> xchecks, ychecks;
            xchecks.push_back((int)(loc.pos_x)); xchecks.push_back((int)(loc.pos_x+1));
            ychecks.push_back((int)(loc.pos_y)); ychecks.push_back((int)(loc.pos_y+1));
            //std::ostringstream d;
            //d << "markPos called with loc " << loc.pos_x << " " << loc.pos_y;
            //hlt::Log::log(d.str());
            for(int i = 0; i < 2; i++){
                for(int j= 0; j < 2; j++){
                    int x = xchecks[i]; int y = ychecks[j];
                    if(x<0||x>=C||y<0||y>=R)continue;
                    if(loc.get_distance_to(hlt::Location::newLoc(x,y)) <= COLLISION_THRESHOLD){
                        //std::ostringstream debug;
                        //debug << "checking " << x << " " << y;
                        //hlt::Log::log(debug.str());
                        if((!remove && map[x][y].find(turn) != map[x][y].end())){
                            //hlt::Log::log("position already claimed");
                            return false;
                        }
                        else if(!remove && map[x][y].find(-1) != map[x][y].end()){
                            //hlt::Log::log("position occupied by planet or docked ship");
                            return false;
                        }
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
