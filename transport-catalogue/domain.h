#pragma once

#include "geo.h"

#include <string>
#include <vector>
#include <set>
#include <map>

namespace transport_catalogue
{
    struct Stop
    {
        std::string name_stop;
        geo::Coordinates coordinates;
        std::set<std::string> passing_buses;
    };

    struct Bus
    {
        std::string name_bus;
        std::vector<const Stop *> stops_for_bus;
        bool is_roundtrip;
    };

    struct InfoRoute
    {
        std::string name_route;
        size_t stops_on_route;
        size_t unique_stops;
        int route_length;
        double curvature;
    };

    struct ParseStops
    {
        std::string_view name_stop;
        geo::Coordinates coordinates;
        std::map<std::string_view, int> stops_and_distances;
    };

    struct ParseBus
    {
        std::string_view name_bus;
        std::vector<const transport_catalogue::Stop *> stops;
        bool is_roundtrip;
    };
}