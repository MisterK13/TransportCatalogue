#pragma once

#include "transport_catalogue.h"
#include "json.h"
#include "map_renderer.h"
#include "json_builder.h"
#include "transport_router.h"

#include <string>
#include <string_view>
#include <vector>
#include <sstream>

class JsonReader
{
public:
    JsonReader(std::istream &input) : doc_(json::Load(input)) {}

    const json::Node &GetBaseRequests() const;
    const json::Node &GetStatRequests() const;
    const json::Node &GetRenderSettings() const;
    const json::Node &GetRoutingSettings() const;

    void AddCatalogue(transport_catalogue::TransportCatalogue &catalogue);

    void PrintFunction(const transport_catalogue::TransportCatalogue &catalogue, const transport_catalogue::Router &router) const;

    svg::Document RenderMap(const transport_catalogue::TransportCatalogue &catalogue) const;

    transport_catalogue::RouteSettings FillRoutingSettings(const json::Node &settings) const;

private:
    json::Document doc_;
    json::Node ntr_ = nullptr;

    transport_catalogue::ParseStops ParseStopWithDistances(const json::Dict &request_map) const;
    transport_catalogue::ParseBus ParseBus(const json::Dict &request_map, transport_catalogue::TransportCatalogue &catalogue) const;
    renderer::MapRenderer ParseRenderSettings(const json::Dict &request_map) const;

    const json::Node PrintBus(const json::Dict &request_map, const transport_catalogue::TransportCatalogue &catalogue) const;
    const json::Node PrintStop(const json::Dict &request_map, const transport_catalogue::TransportCatalogue &catalogue) const;
    const json::Node PrintMap(const json::Dict &request_map, const transport_catalogue::TransportCatalogue &catalogue) const;
    const json::Node PrintRouting(const json::Dict &request_map, const transport_catalogue::Router &router) const;

    std::optional<transport_catalogue::InfoRoute> GetBusStat(const std::string_view &bus_name, const transport_catalogue::TransportCatalogue &catalogue) const;
    const std::set<std::string> GetBusesByStop(const std::string_view &stop_name, const transport_catalogue::TransportCatalogue &catalogue) const;
};