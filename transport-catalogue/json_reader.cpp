#include "json_reader.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <iostream>

const json::Node &JsonReader::GetBaseRequests() const
{
    if (!doc_.GetRoot().AsMap().count("base_requests"))
        return ntr_;
    return doc_.GetRoot().AsMap().at("base_requests");
}

const json::Node &JsonReader::GetStatRequests() const
{
    if (!doc_.GetRoot().AsMap().count("stat_requests"))
        return ntr_;
    return doc_.GetRoot().AsMap().at("stat_requests");
}

const json::Node &JsonReader::GetRenderSettings() const
{
    if (!doc_.GetRoot().AsMap().count("render_settings"))
        return ntr_;
    return doc_.GetRoot().AsMap().at("render_settings");
}

const json::Node &JsonReader::GetRoutingSettings() const
{
    if (!doc_.GetRoot().AsMap().count("routing_settings"))
        return ntr_;
    return doc_.GetRoot().AsMap().at("routing_settings");
}

void JsonReader::AddCatalogue(transport_catalogue::TransportCatalogue &catalogue)
{
    const json::Array &array = GetBaseRequests().AsArray();

    std::vector<json::Node> buses_v;
    buses_v.reserve(array.size());
    std::vector<json::Node> stops_v;
    stops_v.reserve(array.size());

    for (const auto &request : array)
    {
        const auto &base_request = request.AsMap();
        const auto &type = base_request.at("type").AsString();

        if (type == "Stop")
        {
            auto [stop_name, coordinates, stop_distances] = ParseStopWithDistances(base_request);
            catalogue.AddStop(stop_name, coordinates);
            stops_v.push_back(request);
        }

        if (type == "Bus")
        {
            buses_v.push_back(request);
        }
    }

    for (const auto &request : stops_v)
    {
        const auto &base_request = request.AsMap();
        auto [stop_from_name, coordinates, stop_distances] = ParseStopWithDistances(base_request);

        for (auto &[stop_to_name, dist] : stop_distances)
        {
            auto stop_from = catalogue.FindStop(std::string(stop_from_name));
            auto stop_to = catalogue.FindStop(std::string(stop_to_name));
            catalogue.AddDistance(stop_from, stop_to, dist);
        }
    }

    for (const auto &request : buses_v)
    {
        const auto &base_requset = request.AsMap();
        auto [bus_number, stops, circular_route] = ParseBus(base_requset, catalogue);
        catalogue.AddRoute(bus_number, stops, circular_route);
    }
}

transport_catalogue::ParseStops JsonReader::ParseStopWithDistances(const json::Dict &request_map) const
{
    transport_catalogue::ParseStops result;
    result.name_stop = request_map.at("name").AsString();
    result.coordinates = {request_map.at("latitude").AsDouble(), request_map.at("longitude").AsDouble()};
    std::map<std::string_view, int> stop_distances;
    auto &distances = request_map.at("road_distances").AsMap();

    for (auto &[stop_to_name, distance] : distances)
    {
        stop_distances.emplace(stop_to_name, distance.AsInt());
    }

    result.stops_and_distances = stop_distances;

    return result;
}

transport_catalogue::ParseBus JsonReader::ParseBus(const json::Dict &request_map, transport_catalogue::TransportCatalogue &catalogue) const
{
    transport_catalogue::ParseBus result;
    result.name_bus = request_map.at("name").AsString();
    std::vector<const transport_catalogue::Stop *> stops;

    for (auto &stop : request_map.at("stops").AsArray())
    {
        stops.push_back(catalogue.FindStop(stop.AsString()));
    }

    result.stops = stops;
    result.is_roundtrip = request_map.at("is_roundtrip").AsBool();

    return result;
}

void JsonReader::PrintFunction(const transport_catalogue::TransportCatalogue &catalogue, const transport_catalogue::Router &router) const
{
    json::Array result;
    const json::Array array = GetStatRequests().AsArray();

    for (const auto &request : array)
    {
        const auto &base_request = request.AsMap();
        const auto &type = base_request.at("type").AsString();

        if (type == "Stop")
        {
            result.push_back(PrintStop(base_request, catalogue).AsMap());
        }

        if (type == "Bus")
        {
            result.push_back(PrintBus(base_request, catalogue).AsMap());
        }

        if (type == "Map")
        {
            result.push_back(PrintMap(base_request, catalogue).AsMap());
        }

        if (type == "Route")
        {
            result.push_back(PrintRouting(base_request, router).AsMap());
        }
    }
    json::Print(json::Document{result}, std::cout);
}

const json::Node JsonReader::PrintBus(const json::Dict &request_map, const transport_catalogue::TransportCatalogue &catalogue) const
{
    json::Node result;
    const std::string &bus_name = request_map.at("name").AsString();
    const int id = request_map.at("id").AsInt();

    if (!catalogue.FindBus(bus_name))
    {
        result = json::Builder{}
                     .StartDict()
                     .Key("request_id")
                     .Value(id)
                     .Key("error_message")
                     .Value("not found")
                     .EndDict()
                     .Build();
    }
    else
    {
        const auto &result_info = GetBusStat(bus_name, catalogue);
        result = json::Builder{}
                     .StartDict()
                     .Key("request_id")
                     .Value(id)
                     .Key("curvature")
                     .Value(result_info->curvature)
                     .Key("route_length")
                     .Value(result_info->route_length)
                     .Key("stop_count")
                     .Value(static_cast<int>(result_info->stops_on_route))
                     .Key("unique_stop_count")
                     .Value(static_cast<int>(result_info->unique_stops))
                     .EndDict()
                     .Build();
    }

    return result;
}

const json::Node JsonReader::PrintStop(const json::Dict &request_map, const transport_catalogue::TransportCatalogue &catalogue) const
{
    json::Node result;
    const std::string &stop_name = request_map.at("name").AsString();
    const int id = request_map.at("id").AsInt();

    if (!catalogue.FindStop(stop_name))
    {
        result = json::Builder{}
                     .StartDict()
                     .Key("request_id")
                     .Value(id)
                     .Key("error_message")
                     .Value("not found")
                     .EndDict()
                     .Build();
    }
    else
    {
        json::Array buses;

        for (auto &bus : GetBusesByStop(stop_name, catalogue))
        {
            buses.push_back(bus);
        }

        result = json::Builder{}
                     .StartDict()
                     .Key("request_id")
                     .Value(id)
                     .Key("buses")
                     .Value(buses)
                     .EndDict()
                     .Build();
    }

    return result;
}

const json::Node JsonReader::PrintMap(const json::Dict &request_map, const transport_catalogue::TransportCatalogue &catalogue) const
{
    json::Node result;

    const int id = request_map.at("id").AsInt();
    std::ostringstream strm;
    svg::Document map = RenderMap(catalogue);
    map.Render(strm);
    result = json::Builder{}
                 .StartDict()
                 .Key("request_id")
                 .Value(id)
                 .Key("map")
                 .Value(strm.str())
                 .EndDict()
                 .Build();

    return result;
}

renderer::MapRenderer JsonReader::ParseRenderSettings(const json::Dict &request_map) const
{
    renderer::RenderSettings render_settings;
    render_settings.width = request_map.at("width").AsDouble();
    render_settings.height = request_map.at("height").AsDouble();
    render_settings.padding = request_map.at("padding").AsDouble();
    render_settings.stop_radius = request_map.at("stop_radius").AsDouble();
    render_settings.line_width = request_map.at("line_width").AsDouble();
    render_settings.bus_label_font_size = request_map.at("bus_label_font_size").AsInt();
    const json::Array &bus_label_offset = request_map.at("bus_label_offset").AsArray();
    render_settings.bus_label_offset = {bus_label_offset[0].AsDouble(), bus_label_offset[1].AsDouble()};
    render_settings.stop_label_font_size = request_map.at("stop_label_font_size").AsInt();
    const json::Array &stop_label_offset = request_map.at("stop_label_offset").AsArray();
    render_settings.stop_label_offset = {stop_label_offset[0].AsDouble(), stop_label_offset[1].AsDouble()};
    render_settings.underlayer_width = request_map.at("underlayer_width").AsDouble();

    const auto &underlayer_color_json = request_map.at("underlayer_color");
    if (underlayer_color_json.IsString())
    {
        render_settings.underlayer_color = underlayer_color_json.AsString();
    }
    else if (underlayer_color_json.IsArray())
    {
        const json::Array &underlayer_color = underlayer_color_json.AsArray();
        if (underlayer_color.size() == 3)
        {
            render_settings.underlayer_color = svg::Rgb(static_cast<uint8_t>(underlayer_color[0].AsInt()), static_cast<uint8_t>(underlayer_color[1].AsInt()), static_cast<uint8_t>(underlayer_color[2].AsInt()));
        }
        else if (underlayer_color.size() == 4)
        {
            render_settings.underlayer_color = svg::Rgba(static_cast<uint8_t>(underlayer_color[0].AsInt()), static_cast<uint8_t>(underlayer_color[1].AsInt()), static_cast<uint8_t>(underlayer_color[2].AsInt()), underlayer_color[3].AsDouble());
        }
        else
        {
            throw std::logic_error("wrong underlayer colortype");
        }
    }
    else
    {
        throw std::logic_error("wrong underlayer color");
    }

    const json::Array &color_palette = request_map.at("color_palette").AsArray();
    for (const auto &color_element : color_palette)
    {
        if (color_element.IsString())
        {
            render_settings.color_palette.push_back(color_element.AsString());
        }
        else if (color_element.IsArray())
        {
            const json::Array &color_type = color_element.AsArray();
            if (color_type.size() == 3)
            {
                render_settings.color_palette.push_back(svg::Rgb(static_cast<uint8_t>(color_type[0].AsInt()), static_cast<uint8_t>(color_type[1].AsInt()), static_cast<uint8_t>(color_type[2].AsInt())));
            }
            else if (color_type.size() == 4)
            {
                render_settings.color_palette.push_back(svg::Rgba(static_cast<uint8_t>(color_type[0].AsInt()), static_cast<uint8_t>(color_type[1].AsInt()), static_cast<uint8_t>(color_type[2].AsInt()), color_type[3].AsDouble()));
            }
            else
            {
                throw std::logic_error("wrong color palette type");
            }
        }
        else
        {
            throw std::logic_error("wrong color palette");
        }
    }
    return render_settings;
}

const json::Node JsonReader::PrintRouting(const json::Dict &request_map, const transport_catalogue::Router &router) const
{
    json::Node result;
    const int id = request_map.at("id").AsInt();
    const std::string_view stop_from = request_map.at("from").AsString();
    const std::string_view stop_to = request_map.at("to").AsString();
    const auto &graph_router_info = router.FindInfoRoute(stop_from, stop_to);

    if (!graph_router_info.route_setting)
    {
        result = json::Builder{}
                     .StartDict()
                     .Key("request_id")
                     .Value(id)
                     .Key("error_message")
                     .Value("not found")
                     .EndDict()
                     .Build();
    }
    else
    {
        json::Array items;
        double total_time = 0.0;
        items.reserve(graph_router_info.route_setting.value().edges.size());
        for (auto &edge : graph_router_info.edges)
        {
            if (edge.quality == 0)
            {
                items.emplace_back(json::Node(json::Builder{}
                                                  .StartDict()
                                                  .Key("stop_name")
                                                  .Value(edge.name)
                                                  .Key("time")
                                                  .Value(edge.weight)
                                                  .Key("type")
                                                  .Value("Wait")
                                                  .EndDict()
                                                  .Build()));

                total_time += edge.weight;
            }
            else
            {
                items.emplace_back(json::Node(json::Builder{}
                                                  .StartDict()
                                                  .Key("bus")
                                                  .Value(edge.name)
                                                  .Key("span_count")
                                                  .Value(static_cast<int>(edge.quality))
                                                  .Key("time")
                                                  .Value(edge.weight)
                                                  .Key("type")
                                                  .Value("Bus")
                                                  .EndDict()
                                                  .Build()));

                total_time += edge.weight;
            }
        }

        result = json::Builder{}
                     .StartDict()
                     .Key("request_id")
                     .Value(id)
                     .Key("total_time")
                     .Value(total_time)
                     .Key("items")
                     .Value(items)
                     .EndDict()
                     .Build();
    }

    return result;
}

std::optional<transport_catalogue::InfoRoute> JsonReader::GetBusStat(const std::string_view &bus_name, const transport_catalogue::TransportCatalogue &catalogue) const
{
    return catalogue.InformationRoute(std::string(bus_name));
}

const std::set<std::string> JsonReader::GetBusesByStop(const std::string_view &stop_name, const transport_catalogue::TransportCatalogue &catalogue) const
{
    return catalogue.FindStop(std::string(stop_name))->passing_buses;
}

svg::Document JsonReader::RenderMap(const transport_catalogue::TransportCatalogue &catalogue) const
{
    renderer::MapRenderer result(ParseRenderSettings(GetRenderSettings().AsMap()));
    return result.GetDocumentSVG(catalogue.GetSortedBuses());
}

transport_catalogue::RouteSettings JsonReader::FillRoutingSettings(const json::Node &settings) const
{
    transport_catalogue::RouteSettings routing_settings{settings.AsMap().at("bus_wait_time").AsInt(), settings.AsMap().at("bus_velocity").AsDouble()};
    return routing_settings;
}