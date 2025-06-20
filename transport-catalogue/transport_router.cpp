#include "transport_router.h"

namespace transport_catalogue
{
    const graph::DirectedWeightedGraph<double> &Router::BuildGraph(const TransportCatalogue &catalogue)
    {
        const auto &all_stops = catalogue.GetSortedStops();
        const auto &all_buses = catalogue.GetSortedBuses();
        graph::DirectedWeightedGraph<double> stops_graph(all_stops.size() * 2);
        std::map<std::string, graph::VertexId> stop_ids;
        graph::VertexId vertex_id = 0;

        for (const auto &[stop_name, stop_info] : all_stops)
        {
            stop_ids[stop_info->name_stop] = vertex_id;
            stops_graph.AddEdge({stop_info->name_stop,
                                 0,
                                 vertex_id,
                                 ++vertex_id,
                                 static_cast<double>(bus_wait_time_)});
            ++vertex_id;
        }
        stop_ids_ = std::move(stop_ids);

        for (const auto &[bus_name, bus_info] : all_buses)
        {
            const auto &stops = bus_info->stops_for_bus;
            size_t stops_count = stops.size();

            for (size_t i = 0; i < stops_count; ++i)
            {
                for (size_t j = i + 1; j < stops_count; ++j)
                {
                    int dist_sum = 0;
                    int dist_sum_inverse = 0;
                    for (size_t k = i + 1; k <= j; ++k)
                    {
                        dist_sum += catalogue.GetDistance(stops[k - 1], stops[k]);
                        dist_sum_inverse += catalogue.GetDistance(stops[k], stops[k - 1]);
                    }

                    const double METERS_PER_MINUTE = 1000.0 / 60.0;
                    double weight = dist_sum / (bus_velocity_ * METERS_PER_MINUTE);
                    double weight_inverse = dist_sum_inverse / (bus_velocity_ * METERS_PER_MINUTE);

                    size_t stop_from_id = stop_ids_.at(stops[i]->name_stop);
                    size_t stop_to_id = stop_ids_.at(stops[j]->name_stop);

                    stops_graph.AddEdge({bus_info->name_bus, j - i, stop_from_id + 1, stop_to_id, weight});

                    if (!bus_info->is_roundtrip)
                    {
                        stops_graph.AddEdge({bus_info->name_bus, j - i, stop_to_id + 1, stop_from_id, weight_inverse});
                    }
                }
            }
        }

        graph_ = std::move(stops_graph);
        router_ = std::make_unique<graph::Router<double>>(graph_);

        return graph_;
    }

    const transport_catalogue::GraphRouteInfo Router::FindInfoRoute(const std::string_view stop_from, const std::string_view stop_to) const
    {
        GraphRouteInfo result;
        result.route_setting = router_->BuildRoute(stop_ids_.at(std::string(stop_from)), stop_ids_.at(std::string(stop_to)));

        if (result.route_setting)
        {
            for (auto &edge_id : result.route_setting.value().edges)
            {
                result.edges.push_back(GetGraph().GetEdge(edge_id));
            }
        }

        return result;
    }

    const graph::DirectedWeightedGraph<double> &Router::GetGraph() const
    {
        return graph_;
    }
}