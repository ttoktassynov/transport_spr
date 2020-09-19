#pragma once
#include "response.h"
#include "graph.h"
#include "router.h"

#include <optional>
#include <string>
#include <vector>
#include <unordered_map>
#include <iomanip>
#include <set>

class RouteManager{
public:
    using DistInfo = std::vector<std::pair<int, std::string> >;
    using RouteInfo = std::pair<std::vector<std::string>, bool>;
    using StopInfo = std::set<std::string>;

    using StopPair = std::pair<std::string, std::string>;
    using RoutesData = std::unordered_map<std::string, RouteInfo>;
    using StopsData = std::unordered_map<std::string, Coordinate>;

    ResponseHolder ReadRoute(std::string route, int request_id) const;
    ResponseHolder ReadStop(std::string stop, int request_id) const;
    ResponseHolder ReadRouteSearch(std::string from, std::string to, int request_id) const;

    void AddStop(std::string stop, double lat, double lon, std::optional<DistInfo> other_stops);
    void AddRoute(std::string route, std::vector<std::string> stops, bool is_roundtrip);
    void RunGraphBuilder(std::pair<int, double> routing_settings);

private:
    
    struct StopsHasher {
        size_t operator() (const StopPair& stop_pair) const
        {
            size_t r1 = hs(stop_pair.first);
            size_t r2 = hs(stop_pair.second);
            size_t x = 1'000'193;
            return r1 * x  + r2 ;
        }
        std::hash<std::string> hs;
    };

    std::unordered_map<std::string, Coordinate> stops_;
    std::unordered_map<std::string, RouteInfo> route_to_stops_;
    std::unordered_map<std::string, StopInfo> stop_to_routes_;
    std::unordered_map<StopPair, int, StopsHasher > distances_;
    using Distances = std::unordered_map<StopPair, int, StopsHasher>;

    class GraphBuilder {
        using WeightType = double;
        using Router = Graph::Router<WeightType>;
    public:
        GraphBuilder(const RouteManager * manager, const std::pair<int, double> setInfo) : 
                graph(2 * manager->stops_.size()),
                stop_id_to_name_(InitStopIdToNameMaps(manager->stops_)),
                name_to_stop_id_(InitNameToStopIdMaps(manager->stops_, this)),
                edge_id_to_route(InitEdgeIdToRouteName(manager, setInfo)),
                router(graph) {}

        Graph::DirectedWeightedGraph<double> graph;
        const std::vector<std::string> stop_id_to_name_;
        const std::unordered_map<std::string, int> name_to_stop_id_;
        const std::unordered_map<int, std::string> edge_id_to_route;
        Router router;

    private:
        static std::vector<std::string> 
        InitStopIdToNameMaps(const StopsData& stops_){
            std::vector<std::string> result;
            result.resize(stops_.size() * 2);
            int i = 0;
            for (const auto& [stop_name, coord] : stops_) {
                result[i] = stop_name;
                result[i + 1] = stop_name;
                i += 2;
            }
            return result;
        }
        static std::unordered_map<std::string, int>
        InitNameToStopIdMaps(const StopsData& stops_, const GraphBuilder* builder) {
            std::unordered_map<std::string, int> result;
            int i = 0;
            for (int i = 0;i < builder->stop_id_to_name_.size(); ++i) {
                if (i % 2 == 0)
                    result[builder->stop_id_to_name_[i]] = i;
            }
            return result;
        }
        
        std::unordered_map<int, std::string>
        InitEdgeIdToRouteName(const RouteManager * manager, const std::pair<int, double> setInfo) {
            std::unordered_map<int, std::string> result;

            for (const auto& [route_name, route_stops] : manager->route_to_stops_) {
                const auto& stops = route_stops.first;
                // build edges for roundtrip route
                if (route_stops.second){
                    for (int i = 0; i < stops.size(); ++i) {
                        unsigned long stop_id_from = name_to_stop_id_.at(stops[i]) + 1;
                        
                        size_t edge_id = graph.AddEdge({stop_id_from - 1, stop_id_from, static_cast<double>(setInfo.first)});
                        result.emplace(edge_id, route_name);

                        for (int j = i ; j < stops.size(); ++j) {
                            unsigned long  stop_id_to = name_to_stop_id_.at(stops[j]);
                            int dist = RouteManager::ComputeRealDistForTwoVertices(stops, manager->distances_, i, j);
                            size_t edge_id = graph.AddEdge({stop_id_from, stop_id_to, dist / setInfo.second});
                            result.emplace(edge_id, route_name);
                        }
                    }
                }
                // build edges for ordinary route
                else {
                    int n = stops.size();
                    for (int i = 0; i < n; ++i) {
                        unsigned long  stop_id_from = name_to_stop_id_.at(stops[i]) + 1;
                        
                        size_t edge_id = graph.AddEdge({stop_id_from - 1, stop_id_from, static_cast<double>(setInfo.first)});
                        result.emplace(edge_id, route_name);

                        for (int j = i + 1; j < n; ++j) {
                            unsigned long  stop_id_to = name_to_stop_id_.at(stops[j]);
                            int dist = RouteManager::ComputeRealDistForTwoVertices(stops, manager->distances_, i, j);
                            size_t edge_id = graph.AddEdge({stop_id_from, stop_id_to, dist / setInfo.second});
                            result.emplace(edge_id, route_name);    
                        }
                    }
                    for (int i = n - 1; i >= 0; i--) {
                        unsigned long stop_id_from = name_to_stop_id_.at(stops[i]) + 1;

                        for (int j = i - 1; j >= 0; j--) {
                            unsigned long  stop_id_to = name_to_stop_id_.at(stops[j]);
                            int dist = RouteManager::ComputeRealDistForTwoVertices(stops, manager->distances_, i, j);
                            size_t edge_id = graph.AddEdge({stop_id_from, stop_id_to, dist / setInfo.second});
                            result.emplace(edge_id, route_name);  
                        }
                    }
                }
            }
            return result;
        }
    };
    std::optional<GraphBuilder> graphBuilder = std::nullopt;

    double ComputeRouteGeoDistance(const std::vector<std::string>& stops, 
            bool is_roundtrip) const;
    int ComputeRouteRealDistance(const std::vector<std::string>& stops,
            bool is_roundtrip) const;
public:
    static int ComputeRealDistForTwoVertices(const std::vector<std::string>& stops, 
            const Distances& distances, const int stop_a , int stop_b);

    static int ComputeSpanCountOnEdge(const std::string_view& route_name, 
            const std::vector<std::string>& stop_id_to_name, 
            const RoutesData& route_to_stops, int vertexA, int vertexB);
};

