#include "map_renderer.h"

namespace renderer
{
    std::vector<svg::Polyline> MapRenderer::GetRouteLines(const std::map<std::string_view, const transport_catalogue ::Bus *> &buses, const SphereProjector &sphere_projector) const
    {
        std::vector<svg::Polyline> result;
        size_t color = 0;

        for (const auto &[bus_name, bus] : buses)
        {
            if (bus->stops_for_bus.empty())
                continue;

            std::vector<const transport_catalogue::Stop *> stops_route{bus->stops_for_bus.begin(), bus->stops_for_bus.end()};

            if (bus->is_roundtrip == false)
                stops_route.insert(stops_route.end(), std::next(bus->stops_for_bus.rbegin()), bus->stops_for_bus.rend());

            svg::Polyline line;

            for (const auto &stop : stops_route)
            {
                line.AddPoint(sphere_projector(stop->coordinates));
            }

            line.SetStrokeColor(render_settings_.color_palette[color]);
            line.SetFillColor("none");
            line.SetStrokeWidth(render_settings_.line_width);
            line.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
            line.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            if (color < (render_settings_.color_palette.size() - 1))
                ++color;
            else
                color = 0;

            result.push_back(line);
        }
        return result;
    }

    std::vector<svg::Text> MapRenderer::GetNamesRoute(const std::map<std::string_view, const transport_catalogue::Bus *> &buses, const SphereProjector &sp) const
    {
        std::vector<svg::Text> result;
        size_t color = 0;

        for (const auto &[bus_name, bus] : buses)
        {
            if (bus->stops_for_bus.empty())
                continue;
            svg::Text text;
            text.SetPosition(sp(bus->stops_for_bus[0]->coordinates));
            text.SetOffset(render_settings_.bus_label_offset);
            text.SetFontSize(static_cast<uint32_t>(render_settings_.bus_label_font_size));
            text.SetFontFamily("Verdana");
            text.SetFontWeight("bold");
            text.SetData(bus->name_bus);
            text.SetFillColor(render_settings_.color_palette[color]);

            if (color < (render_settings_.color_palette.size() - 1))
                ++color;
            else
                color = 0;

            svg::Text substrate;
            substrate.SetPosition(sp(bus->stops_for_bus[0]->coordinates));
            substrate.SetOffset(render_settings_.bus_label_offset);
            substrate.SetFontSize(static_cast<uint32_t>(render_settings_.bus_label_font_size));
            substrate.SetFontFamily("Verdana");
            substrate.SetFontWeight("bold");
            substrate.SetData(bus->name_bus);
            substrate.SetFillColor(render_settings_.underlayer_color);
            substrate.SetStrokeColor(render_settings_.underlayer_color);
            substrate.SetStrokeWidth(render_settings_.underlayer_width);
            substrate.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
            substrate.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            result.push_back(substrate);
            result.push_back(text);

            if (!bus->is_roundtrip && bus->stops_for_bus[0] != bus->stops_for_bus.back())
            {
                svg::Text text2{text};
                svg::Text substrate2{substrate};
                text2.SetPosition(sp(bus->stops_for_bus.back()->coordinates));
                substrate2.SetPosition(sp(bus->stops_for_bus.back()->coordinates));

                result.push_back(substrate2);
                result.push_back(text2);
            }
        }

        return result;
    }

    std::vector<svg::Circle> MapRenderer::GetStopCircle(const std::map<std::string_view, const transport_catalogue::Stop *> &stops, const SphereProjector &sp) const
    {
        std::vector<svg::Circle> result;

        for (const auto &[stop_name, stop] : stops)
        {
            svg::Circle circle;
            circle.SetCenter(sp(stop->coordinates));
            circle.SetRadius(render_settings_.stop_radius);
            circle.SetFillColor("white");
            result.push_back(circle);
        }

        return result;
    }

    std::vector<svg::Text> MapRenderer::GetNamesStops(const std::map<std::string_view, const transport_catalogue::Stop *> &stops, const SphereProjector &sp) const
    {
        std::vector<svg::Text> result;

        for (const auto &[stop_name, stop] : stops)
        {
            if (stop->passing_buses.empty())
                continue;
            svg::Text text;
            text.SetPosition(sp(stop->coordinates));
            text.SetOffset(render_settings_.stop_label_offset);
            text.SetFontSize(static_cast<uint32_t>(render_settings_.stop_label_font_size));
            text.SetFontFamily("Verdana");
            text.SetData(stop->name_stop);
            text.SetFillColor("black");

            svg::Text substrate;
            substrate.SetPosition(sp(stop->coordinates));
            substrate.SetOffset(render_settings_.stop_label_offset);
            substrate.SetFontSize(static_cast<uint32_t>(render_settings_.stop_label_font_size));
            substrate.SetFontFamily("Verdana");
            substrate.SetData(stop->name_stop);
            substrate.SetFillColor(render_settings_.underlayer_color);
            substrate.SetStrokeColor(render_settings_.underlayer_color);
            substrate.SetStrokeWidth(render_settings_.underlayer_width);
            substrate.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
            substrate.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

            result.push_back(substrate);
            result.push_back(text);
        }

        return result;
    }

    svg::Document MapRenderer::GetDocumentSVG(const std::map<std::string_view, const transport_catalogue::Bus *> &buses) const
    {
        svg::Document result;
        std::vector<geo::Coordinates> coordinates_route;
        std::map<std::string_view, const transport_catalogue::Stop *> stops;

        for (const auto &[bus_name, bus] : buses)
        {
            if (bus->stops_for_bus.empty())
                continue;
            for (const auto &stop : bus->stops_for_bus)
            {
                coordinates_route.push_back(stop->coordinates);
                stops[stop->name_stop] = stop;
            }
        }

        SphereProjector sphere_projector(coordinates_route.begin(), coordinates_route.end(), render_settings_.width, render_settings_.height, render_settings_.padding);

        for (const auto &polyline : GetRouteLines(buses, sphere_projector))
        {
            result.Add(polyline);
        }

        for (const auto &text : GetNamesRoute(buses, sphere_projector))
        {
            result.Add(text);
        }

        for (const auto &circle : GetStopCircle(stops, sphere_projector))
        {
            result.Add(circle);
        }

        for (const auto &text : GetNamesStops(stops, sphere_projector))
        {
            result.Add(text);
        }

        return result;
    }
}