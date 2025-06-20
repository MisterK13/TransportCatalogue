#pragma once

#include "domain.h"

#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <stdexcept>
#include <set>
#include <optional>
#include <map>

namespace transport_catalogue
{
    class TransportCatalogue
    {
    public:
        struct HaherDistanceStop
        {
            size_t operator()(const std::pair<const Stop *, const Stop *> two_stops) const
            {
                size_t hash_first = std::hash<const void *>{}(two_stops.first);
                size_t hash_second = std::hash<const void *>{}(two_stops.second);
                return hash_first + hash_second * 37;
            }
        };

        void AddStop(const std::string_view &name_stop, const geo::Coordinates &coordinates);
        void AddRoute(const std::string_view &name_bus, const std::vector<const Stop *> &stops_for_bus, bool is_roundtrip);
        const Stop *FindStop(const std::string &name_stop) const;
        const Bus *FindBus(const std::string &name_bus) const;
        const std::optional<InfoRoute> InformationRoute(const std::string &name_route) const;
        const std::set<std::string> *InformationStop(const std::string &name_stop) const;
        void AddDistance(const Stop *from, const Stop *to, const int distance);
        int GetDistance(const Stop *from, const Stop *to) const;
        const std::map<std::string_view, const Bus *> GetSortedBuses() const;
        const std::map<std::string_view, const Stop *> GetSortedStops() const;

    private:
        std::deque<Stop> stops_;
        std::unordered_map<std::string_view, Stop *> stopname_to_stop_;
        std::deque<Bus> buses_;
        std::unordered_map<std::string_view, Bus *> busname_to_bus_;
        std::unordered_map<std::pair<const Stop *, const Stop *>, int, HaherDistanceStop> distance_stops_;
    };
}