#pragma once

#include "router.h"
#include "transport_catalogue.h"

#include <memory>
#include <vector>

namespace transport_catalogue
{
    struct RouteSettings
    {
        int bus_wait_time = 0;
        double bus_velocity = 0.0;
    };

    struct GraphRouteInfo
    {
        std::optional<graph::Router<double>::RouteInfo> route_setting;
        std::vector<graph::Edge<double>> edges;
    };

    class Router
    {
    public:
        Router() = default;

        Router(const RouteSettings &settings, const TransportCatalogue &catalogue)
        {
            bus_wait_time_ = settings.bus_wait_time;
            bus_velocity_ = settings.bus_velocity;
            BuildGraph(catalogue);
        }

        const transport_catalogue::GraphRouteInfo FindInfoRoute(const std::string_view stop_from, const std::string_view stop_to) const;

    private:
        int bus_wait_time_ = 0;
        double bus_velocity_ = 0.0;

        graph::DirectedWeightedGraph<double> graph_;
        std::map<std::string, graph::VertexId> stop_ids_;
        std::unique_ptr<graph::Router<double>> router_;

        const graph::DirectedWeightedGraph<double> &BuildGraph(const TransportCatalogue &catalogue);
        const graph::DirectedWeightedGraph<double> &GetGraph() const;
    };
}