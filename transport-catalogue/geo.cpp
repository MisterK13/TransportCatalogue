#define _USE_MATH_DEFINES
#include "geo.h"

#include <cmath>

namespace geo
{
    double ComputeDistance(Coordinates from, Coordinates to)
    {
        using namespace std;
        if (from == to)
        {
            return 0;
        }
        static const double dr = M_PI / 180.;
        return acos(sin(from.lat * dr) * sin(to.lat * dr) + cos(from.lat * dr) * cos(to.lat * dr) * cos(abs(from.lng - to.lng) * dr)) * RADIUS_EARCH;
    }
}