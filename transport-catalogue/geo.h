#pragma once

#include <cmath>

const double RADIUS_EARCH = 6371000.;

namespace geo
{
    struct Coordinates
    {
        double lat;
        double lng;
        bool operator==(const Coordinates &other) const
        {
            return lat == other.lat && lng == other.lng;
        }
        bool operator!=(const Coordinates &other) const
        {
            return !(*this == other);
        }
    };

    double ComputeDistance(Coordinates from, Coordinates to);
}