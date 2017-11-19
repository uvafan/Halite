#include "hlt/hlt.hpp"
#include "hlt/navigation.hpp"
#include "hlt/location.hpp"
#include "djj/navigator.hpp"
#include "djj/ship.hpp"
#include <map>

int main() {
    const hlt::Metadata metadata = hlt::initialize("Botv1.0");
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
    std::map<int, djj::Ship> myShips;
    std::vector<hlt::Move> moves;
    std::vector<hlt::Location> locs;
    int turn = -1;
    for (;;) {
        turn++;
        moves.clear();
        locs.clear();
        const hlt::Map map = hlt::in::get_map();

        for (const hlt::Ship& ship : map.ships.at(player_id)) {
            
            if (ship.docking_status != hlt::ShipDockingStatus::Undocked) {
                continue;
            }
            
            djj::Ship dship;
            if(myShips.find(ship.entity_id)==myShips.end()){
                dship = djj::Ship::makeShip(ship.entity_id,ship.location);
            }
            else{
                //hlt::Log::log("found a ship in myShips");
                dship = myShips[ship.entity_id];
            }
            for (const hlt::Planet& planet : map.planets) {
                if (planet.owned) {
                    continue;
                }

                if (ship.can_dock(planet)) {
                    moves.push_back(hlt::Move::dock(ship.entity_id, planet.entity_id));
                    break;
                }

                hlt::Location dest = planet.location;
                std::ostringstream destinfo;
                destinfo  << "my dest = " << dship.dest.pos_x << " " << dship.dest.pos_y 
                         << " new dest = " << dest.pos_x << " " << dest.pos_y
                         << " dist to dest = " << ship.location.get_distance_to(dest)
                         << " dest radius = " << planet.radius
                         << " my loc = " << ship.location.pos_x << " " << ship.location.pos_y;

                hlt::Log::log(destinfo.str());
                if(dest.pos_x != dship.dest.pos_x || dest.pos_y != dship.dest.pos_y) {
                    nav.removePlan(dship.plan, ship.location, turn);
                    dship.plan = nav.getPlan(ship.entity_id,ship.location,dest,planet.radius+4,turn);
                    hlt::Log::log("Found new plan");
                    dship.dest = dest;
                }
                if(!dship.plan.empty()){
                    moves.push_back(dship.plan.front());
                    dship.plan.pop();
                    locs.push_back(ship.location);
                }
                else{
                    hlt::Log::log("Found no moves :(.");
                    moves.push_back(hlt::Move::noop());
                }

                break;
            }
            myShips[ship.entity_id] = dship;
        }
        //remove moves from map
        int locIndex = 0;
        for(int i = 0; i < moves.size(); i++){
            hlt::Move move = moves[i];
            if(move.type!=hlt::MoveType::Thrust)continue;
            hlt::Location loc = locs[locIndex];
            locIndex++;
            nav.checkMove(move,loc,turn,false,true);
        }

        if (!hlt::out::send_moves(moves)) {
            hlt::Log::log("send_moves failed; exiting");
            break;
        }
    }
}
