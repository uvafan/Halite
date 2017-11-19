#pragma once

#include "hlt/hlt.hpp"
#include "../hlt/location.hpp"
#include "../hlt/map.hpp"
#include <vector>
#include <set>

namespace djj {
    struct Navigator {
        int R;
        int C;
        std::vector<std::vector<std::set<int> > > map;

        static Navigator newNavigator(const hlt::Map& init_map) {
            int rows = init_map.map_height;
            int cols = init_map.map_width;
            std::vector<std::vector<std::set<int> > > myMap(rows,std::vector<std::set<int> >(cols,std::set<int>()));
            for(auto p: init_map.planets){
                hlt::Location ploc = p.location;
                double rad = p.radius;
                int minX = (int)(ploc.pos_x - rad); int maxX = (int)(ploc.pos_x + rad + 0.5);
                int minY = (int)(ploc.pos_y - rad); int maxY = (int)(ploc.pos_y + rad + 0.5);
                for(int x = minX; x <= maxX; x++){
                    if(x<0||x>=rows)continue;
                    for(int y = minY; y <= maxY; y++){
                        if(y<0||y>=cols)continue;
                        if(ploc.get_distance_to(hlt::Location::newLoc(x,y))<=rad){
                            myMap[x][y].insert(-1);
                        }
                    }
                }
            }
            return {rows, cols, myMap};
        }
    };
}
