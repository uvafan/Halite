#include "hlt/hlt.hpp"
#include "hlt/navigation.hpp"
#include "hlt/location.hpp"
#include "djj/navigator.hpp"
#include "djj/ship.hpp"
#include "djj/macrocontroller.hpp"
#include <map>
#include <chrono>

int main() {
    const hlt::Metadata metadata = hlt::initialize("v2.2");
    const hlt::PlayerId player_id = metadata.player_id;

    const hlt::Map& initial_map = metadata.initial_map;

    // We now have 1 full minute to analyse the initial map.
    std::ostringstream initial_map_intelligence;
    initial_map_intelligence
        << "width: " << initial_map.map_width
        << "; height: " << initial_map.map_height
        << "; players: " << initial_map.ship_map.size()
        << "; my ships: " << initial_map.ship_map.at(player_id).size()
        << "; planets: " << initial_map.planets.size();
    hlt::Log::log(initial_map_intelligence.str());
    djj::Navigator nav = djj::Navigator::newNavigator(initial_map); 
    djj::Macrocontroller mc = djj::Macrocontroller::newMacrocontroller(initial_map,player_id);
    std::vector<hlt::Move> moves;
    int turn = -1;
    //int plans;
    for (;;) {
        //std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
        //plans = 0;
        turn++;
        const hlt::Map map = hlt::in::get_map();
        mc.updateMapInfo(map,turn);
        moves = mc.getMoves(turn);
        
        if (!hlt::out::send_moves(moves)) {
            hlt::Log::log("send_moves failed; exiting");
            break;
        }
    }
}
