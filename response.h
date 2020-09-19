#pragma once
#include <set>
#include <string>
#include <memory>
#include <sstream>
#include <optional>
#include <iomanip>
#include <cmath>
#include <vector>

struct RouteSearchStats;
using RouteSearchStatsHolder = std::unique_ptr<RouteSearchStats>;

const double PI = 3.1415926535;
const double RADIUS = 6371000;

struct RouteStats{
    size_t stops;
    size_t unique_stops;
    int length;
    double curvature;
};

struct StopStats{
    std::set<std::string> routes;
};

struct RouteSearchStats {
  enum class Type {
    WAIT,
    BUS
  };

  RouteSearchStats(Type type, double time) : type_(type), time_(time) {}
  virtual ~RouteSearchStats() = default;
  const Type type_;
public:
  double time_;
};

struct WaitRouteSearchStats : RouteSearchStats {
    WaitRouteSearchStats (std::string_view stop_name, double time) : 
        RouteSearchStats(RouteSearchStats::Type::WAIT, time),
        stop_name_(stop_name) {}
    std::string_view stop_name_;
};

struct BusRouteSearchStats : RouteSearchStats {
    BusRouteSearchStats (std::string_view bus_name, int span_count, double time) : 
        RouteSearchStats(RouteSearchStats::Type::BUS, time),
        bus_name_(bus_name),
        span_count_(span_count) {}
    std::string_view bus_name_;
    int span_count_;
};


struct Response;
using ResponseHolder = std::unique_ptr<Response>;

struct Response{
    enum class Type {
        SEND_ROUTE,
        SEND_STOP,
        SEND_ROUTE_SEARCH
    };

    Response(Type type) : type(type) {}
    static ResponseHolder Create(Type type);
    virtual ~Response() = default;

    const Type type;
    int request_id;
};

struct ReadRouteResponse : Response {
    ReadRouteResponse () : Response(Response::Type::SEND_ROUTE) {}
    std::string route;
    std::optional<RouteStats> stats;
};

struct ReadStopResponse : Response {
    ReadStopResponse () : Response(Response::Type::SEND_STOP) {}
    std::string stop;
    bool hasStop;
    std::optional<StopStats> stats;
};

struct ReadRouteSearchResponse : Response {
    ReadRouteSearchResponse () : Response(Response::Type::SEND_ROUTE_SEARCH) {} ;
    std::string from, to;
    std::optional<std::vector<RouteSearchStatsHolder> > stats;
    double total_time;
};

struct Coordinate{
    double lat;
    double lon;
};

double DistanceBetweenCoordinates(const Coordinate& lhs, const Coordinate& rhs);

std::ostream& operator << (std::ostream& output, 
    const ReadRouteResponse data);

std::ostream& operator << (std::ostream& output,
    const ReadStopResponse data);

std::ostream& operator << (std::ostream& output,
    const ReadRouteSearchResponse& data);

double ConvertToRad(double val);
