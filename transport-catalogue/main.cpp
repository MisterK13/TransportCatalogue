#include "json_reader.h"
#include "map_renderer.h"

int main()
{
#ifdef _WIN64
    freopen("input.json", "r", stdin);
    freopen("output.json", "w", stdout);
   
#endif

    transport_catalogue::TransportCatalogue catalogue;
    JsonReader requests(std::cin);
    requests.AddCatalogue(catalogue);

    const auto &routing_settings = requests.FillRoutingSettings(requests.GetRoutingSettings());
    const transport_catalogue::Router router = {routing_settings, catalogue};

    requests.PrintFunction(catalogue, router);
}
