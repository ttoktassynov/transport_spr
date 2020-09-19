#include "route_manager.h"
#include <cmath>
#include <algorithm>
using namespace std;

void RouteManager::RunGraphBuilder(std::pair<int, double> routing_settings) {
    graphBuilder.emplace(this, routing_settings);
}
ResponseHolder RouteManager::ReadRoute(string route, int request_id) const{
    ReadRouteResponse response;

    response.request_id = request_id;
    response.route = route;

    if (route_to_stops_.count(route)){  
        const vector<string>& stops = route_to_stops_.at(route).first;
        bool is_roundtrip = route_to_stops_.at(route).second;
        int n = stops.size();
        size_t num_of_stops = (is_roundtrip) 
                ? n : 2 * n - 1;

        vector<string> tmp(stops);
        sort(begin(tmp), end(tmp));
        auto last = unique(begin(tmp), end(tmp));
        tmp.erase(last, end(tmp));

        size_t num_of_unique_stops = tmp.size();

        int route_real_dist = ComputeRouteRealDistance(stops, is_roundtrip);
        double route_geo_dist = ComputeRouteGeoDistance(stops, is_roundtrip);
        double curvature = route_real_dist / route_geo_dist;
        
        response.stats = RouteStats{num_of_stops, 
                                    num_of_unique_stops, 
                                    route_real_dist, 
                                    curvature};
    }
    else{
        response.stats = nullopt;
    }
    return make_unique<ReadRouteResponse>(response);
}

ResponseHolder RouteManager::ReadStop(string stop, int request_id) const {
    ReadStopResponse response;

    response.request_id = request_id;
    response.stop = stop;
    response.hasStop = stops_.count(stop);

    if (stop_to_routes_.count(stop)){
        response.stats = StopStats{stop_to_routes_.at(stop)};
    }
    else{
        response.stats = nullopt;
    }
    return make_unique<ReadStopResponse>(response);
}

ResponseHolder RouteManager::ReadRouteSearch(std::string from, std::string to, int request_id) const {
    ReadRouteSearchResponse response;
    // do smth;
    response.request_id = request_id;
    response.from = from;
    response.to = to;
    response.total_time = 0;

    size_t vertex_from = graphBuilder->name_to_stop_id_.at(from);
    size_t vertex_to = graphBuilder->name_to_stop_id_.at(to);

    const auto route_info = graphBuilder->router.BuildRoute(vertex_from, vertex_to);
    vector<RouteSearchStatsHolder> temp;
    response.stats = move(temp);
    //response.stats->reserve(route_info->edge_count);
    
    if (route_info) {
        for (int i = 0 ;i < route_info->edge_count; ++i) {
            int edge_id = graphBuilder->router.GetRouteEdge(route_info->id, i);
            const auto& edge = graphBuilder->graph.GetEdge(edge_id);
            if (edge.from % 2 == 0) {
                string_view stop_name = graphBuilder->stop_id_to_name_[edge.from];
                response.stats->push_back(make_unique<WaitRouteSearchStats>(stop_name, edge.weight));
                response.total_time += edge.weight;
            }
            else {
                string_view bus_name = graphBuilder->edge_id_to_route.at(edge_id);
                int span_count = ComputeSpanCountOnEdge(bus_name, 
                    graphBuilder->stop_id_to_name_, route_to_stops_, edge.from, edge.to);
                response.stats->push_back(make_unique<BusRouteSearchStats>(bus_name, span_count, edge.weight));
                response.total_time += edge.weight;
            }
        }
    }
    else {
        response.stats = nullopt;
    }
    return make_unique<ReadRouteSearchResponse>(move(response));
}


void RouteManager::AddStop(string stop, double lat, double lon, optional<DistInfo> other_stops){
    stops_[stop] = {lat, lon};
    if (other_stops){
        for (auto& [distance, other_stop] : *other_stops){
            distances_[make_pair(stop, other_stop)] = distance;
            if (!distances_.count(make_pair(other_stop, stop)))
                distances_[make_pair(other_stop, stop)] = distance;
        }
    }
}
void RouteManager::AddRoute(string route, vector<string> stops, 
    bool is_roundtrip ){
    for (const auto& stop : stops){
        stop_to_routes_[stop].insert(route);
    }
    route_to_stops_[route] = make_pair(move(stops), is_roundtrip);
}

double RouteManager::ComputeRouteGeoDistance(const vector<string>& stops,
        bool is_roundtrip) const{
    double total = 0;

    for (int i = 1; i < stops.size(); ++i){
        const Coordinate start = stops_.at(stops[i - 1]);
        const Coordinate end = stops_.at(stops[i]);
        total += DistanceBetweenCoordinates(start, end);
    }
    if (!is_roundtrip) {
        for (int i = stops.size() - 2; i >= 0; --i) {
            const Coordinate start = stops_.at(stops[i + 1]);
            const Coordinate end = stops_.at(stops[i]);
            total += DistanceBetweenCoordinates(start, end);
        }
    }
    return total;
}

int RouteManager::ComputeRouteRealDistance(const std::vector<std::string>& stops,
        bool is_roundtrip) const {
    int total = 0;
    for (int i = 1; i < stops.size(); ++i){
        if (distances_.count(make_pair(stops[i - 1], stops[i]))){
            total += distances_.at(make_pair(stops[i - 1], stops[i]));
        }
    }
    if (!is_roundtrip) {
        for (int i = stops.size() - 2; i >= 0; --i) {
            if (distances_.count(make_pair(stops[i + 1], stops[i]))){
                total += distances_.at(make_pair(stops[i + 1], stops[i]));
            }
        }
    }
    return total;
}
int RouteManager::ComputeRealDistForTwoVertices(const std::vector<std::string>& stops, 
        const Distances& distances, int stop_a , int stop_b )  {

    if (stop_a == stop_b)
        return 0;
    int total = 0;
    
    if (stop_a < stop_b) {
        for (int i = stop_a + 1; i <= stop_b; ++i){
            if (distances.count(make_pair(stops[i - 1], stops[i]))){
                total += distances.at(make_pair(stops[i - 1], stops[i]));
            }
        }
    }
    else {
        for (int i = stop_a - 1 ; i >= stop_b; --i){
            if (distances.count(make_pair(stops[i + 1], stops[i]))){
                total += distances.at(make_pair(stops[i + 1], stops[i]));
            }
        }
    }
    return total;
}

int RouteManager::ComputeSpanCountOnEdge(const std::string_view& route_name, 
        const vector<string>& stop_id_to_name, const RoutesData& route_to_stops, 
        int vertexA, int vertexB)  {

    const auto& route = route_to_stops.at(string(route_name));
    bool is_roundtrip = route.second;
    const auto& stops = route.first;

    string_view stop_a, stop_b;
    
    stop_a = stop_id_to_name[vertexA];
    stop_b = stop_id_to_name[vertexB];

    if (stop_a == stop_b)
        return 0;

    int ans = 2'000'000'000;

    vector<string> all_stops = stops;
    if (!is_roundtrip)
        for (int i = stops.size() - 1; i >= 0; --i)
            all_stops.push_back(stops[i]);

    vector<int> a_index, b_index;
    for (int i = 0; i < all_stops.size(); ++i) {
        if (all_stops[i] == stop_a)
            a_index.push_back(i);
        if (all_stops[i] == stop_b)
            b_index.push_back(i);
    }
    ans = abs(a_index.front() - b_index.back());

    for (int i = 0; i < a_index.size(); ++i) {
        for (int j = 0; j < b_index.size(); ++j) {
            if (b_index[j] >= a_index[i])
                ans = min(ans, abs(a_index[i] - b_index[j]));
        }
    }

    return ans;
}