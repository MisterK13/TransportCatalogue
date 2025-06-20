#include "transport_catalogue.h"

namespace transport_catalogue
{
    void TransportCatalogue::AddStop(const std::string_view &name_stop, const geo::Coordinates &coordinates)
    {

        stops_.push_back({std::string(name_stop), coordinates, {}});
        stopname_to_stop_[stops_.back().name_stop] = &stops_.back();
    }

    void TransportCatalogue::AddRoute(const std::string_view &name_bus, const std::vector<const Stop *> &stops_for_bus, bool is_roundtrip)
    {
        for (auto &stop_for_bus : stops_for_bus)
        {
            for (auto &stop : stops_)
            {
                if (stop.name_stop == stop_for_bus->name_stop)
                    stop.passing_buses.insert(std::string(name_bus));
            }
        }

        buses_.push_back({std::string(name_bus), stops_for_bus, is_roundtrip});
        busname_to_bus_[buses_.back().name_bus] = &buses_.back();
    }

    const Stop *TransportCatalogue::FindStop(const std::string &name_stop) const
    {
        auto it = stopname_to_stop_.find(name_stop);

        if (it != stopname_to_stop_.end())
            return it->second;
        else
            return nullptr;
    }

    const Bus *TransportCatalogue::FindBus(const std::string &name_bus) const
    {
        auto it = busname_to_bus_.find(name_bus);

        if (it != busname_to_bus_.end())
            return it->second;
        else
            return nullptr;
    }

    const std::optional<InfoRoute> TransportCatalogue::InformationRoute(const std::string &name_route) const
    {
        InfoRoute info{};
        const Bus *bus = FindBus(name_route);

        if (!bus)
            throw std::invalid_argument("bus not found");

        info.name_route = name_route;

        if (bus->is_roundtrip)
            info.stops_on_route = bus->stops_for_bus.size();
        else
            info.stops_on_route = bus->stops_for_bus.size() * 2 - 1;

        std::unordered_set<std::string> uniq_stops;

        for (const auto &stop : bus->stops_for_bus)
        {
            uniq_stops.insert(stop->name_stop);
        }

        info.unique_stops = uniq_stops.size();

        int route_length = 0;
        double geo_length = 0.0;

        for (size_t i = 0; i < bus->stops_for_bus.size() - 1; ++i)
        {
            const Stop *stop_from = bus->stops_for_bus[i];
            const Stop *stop_to = bus->stops_for_bus[i + 1];
            if (bus->is_roundtrip)
            {
                route_length += GetDistance(stop_from, stop_to);
                geo_length += geo::ComputeDistance(stop_from->coordinates, stop_to->coordinates);
            }
            else
            {
                route_length += GetDistance(stop_from, stop_to) + GetDistance(stop_to, stop_from);
                geo_length += geo::ComputeDistance(stop_from->coordinates, stop_to->coordinates) * 2;
            }
        }

        info.route_length = route_length;
        info.curvature = static_cast<double>(route_length) / geo_length;

        return std::make_optional(info);
    }

    const std::set<std::string> *TransportCatalogue::InformationStop(const std::string &name_stop) const
    {
        const Stop *stop = FindStop(name_stop);

        if (!stop)
            return nullptr;

        return &stop->passing_buses;
    }

    void TransportCatalogue::AddDistance(const Stop *from, const Stop *to, const int distance)
    {
        distance_stops_[{from, to}] = distance;
    }

    int TransportCatalogue::GetDistance(const Stop *from, const Stop *to) const
    {
        if (distance_stops_.count({from, to}))
            return distance_stops_.at({from, to});
        else if (distance_stops_.count({to, from}))
            return distance_stops_.at({to, from});
        else
            return 0;
    }

    const std::map<std::string_view, const Bus *> TransportCatalogue::GetSortedBuses() const
    {
        std::map<std::string_view, const Bus *> result;
        for (const auto &bus : busname_to_bus_)
        {
            result.emplace(bus);
        }
        return result;
    }

    const std::map<std::string_view, const Stop *> TransportCatalogue::GetSortedStops() const
    {
        std::map<std::string_view, const Stop *> result;
        for (const auto &stop : stopname_to_stop_)
        {
            result.emplace(stop);
        }
        return result;
    }

}