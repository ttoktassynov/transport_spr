#include "response.h"
#include <iostream>



std::ostream& operator << (std::ostream& output, 
    const ReadRouteResponse data){
    using std::operator""s;
    output << std::fixed << std::setprecision(6);

    if (!data.stats){
        output << "\t\t\"" << "request_id"s << "\"" << ": ";
        output << data.request_id << ",\n";
        output << "\t\t\"" << "error_message"s << "\"" << ": ";
        return output << "\"" << "not found"s << "\"";
    }
    return output << "\t\t\"" << "curvature"s << "\"" << ": "
                  << data.stats->curvature << ",\n"
                  << "\t\t\"" << "request_id"s << "\"" << ": "
                  << data.request_id << ",\n"
                  << "\t\t\"" << "unique_stop_count"s << "\"" << ": "
                  << data.stats->unique_stops << ",\n"
                  << "\t\t\"" << "stop_count"s << "\"" << ": "
                  << data.stats->stops << ",\n"
                  << "\t\t\"" << "route_length"s << "\"" << ": "
                  << data.stats->length;
}

std::ostream& operator << (std::ostream& output,
    const ReadStopResponse data){
    using std::operator""s;
    output << "\t\t\"" << "request_id"s << "\"" << ": ";
    output << data.request_id << ",\n";

    if (!data.stats){
        if (!data.hasStop){
            output << "\t\t\"" << "error_message"s << "\"" << ": ";
            output << "\"" << "not found"s << "\"";
        }
        else {
            output << "\t\t\"" << "buses"s << "\"" << ": []";
        }
    }
    else{
        output << "\t\t\"" << "buses"s << "\"" << ": [\n";
        size_t i = 0;
        for (const std::string& route : data.stats->routes){
            output << "\t\t\t\"" << route << "\"";
            if (i < data.stats->routes.size() - 1) 
              output << ",\n";
            else 
              output << "\n";
            ++i;
        }
        output << "\t\t]";
    }
    return output;
}


std::ostream& operator << (std::ostream& output,
    const ReadRouteSearchResponse& data) {
    //do stmth;
    using std::operator""s;
    output << std::fixed << std::setprecision(25);
    //output.unsetf(std::ios_base::fixed);
    
    output << "\t\t\"" << "request_id"s << "\"" << ": ";
    output << data.request_id << ",\n";
    if (!data.stats){
        output << "\t\t\"" << "error_message"s << "\"" << ": ";
        return output << "\"" << "not found"s << "\"";
    }

    output << "\t\t\"" << "total_time"s << "\"" << ": "
           << data.total_time << ",\n"
           << "\t\t\"" << "items"s << "\"" << ": [\n";
    int i = 0;
    for (const auto& item : *data.stats) {
        output << "\t\t\t{\n";
        if (item->type_ == RouteSearchStats::Type::WAIT) {
            const auto& cur_stat = static_cast<const WaitRouteSearchStats&>(*item);
            output << "\t\t\t\t\"" << "type"s << "\"" << ": \""  << "Wait"s << "\",\n";
            output << "\t\t\t\t\"" << "stop_name"s << "\"" << ": \"" << cur_stat.stop_name_ << "\",\n";
            output << "\t\t\t\t\"" << "time"s << "\"" << ": " << static_cast<int>(cur_stat.time_) << "\n";
        }
        else if (item->type_ == RouteSearchStats::Type::BUS) {
            const auto& cur_stat = static_cast<const BusRouteSearchStats&>(*item);
            output << "\t\t\t\t\"" << "type"s << "\"" << ": \""  << "Bus"s << "\",\n";
            output << "\t\t\t\t\"" << "bus"s << "\"" << ": \"" << cur_stat.bus_name_ << "\",\n";
            output << "\t\t\t\t\"" << "span_count"s << "\"" << ": " << cur_stat.span_count_ << ",\n";
            output << "\t\t\t\t\"" << "time"s << "\"" << ": " << cur_stat.time_ << "\n";
        }
        if (i == data.stats->size() - 1)
            output << "\t\t\t}\n";
        else
            output << "\t\t\t},\n";
        ++i;
    }
    output << "\t\t]";
    return output;
}
double ConvertToRad(double val){
    return val * PI / 180;
}

double DistanceBetweenCoordinates(const Coordinate& x, const Coordinate& y){
    double lat_x_r = ConvertToRad(x.lat);
    double lon_x_r = ConvertToRad(x.lon);

    double lat_y_r = ConvertToRad(y.lat);
    double lon_y_r = ConvertToRad(y.lon);

    return acos(sin(lat_x_r) * sin(lat_y_r) + 
            cos(lat_x_r) * cos(lat_y_r) * 
            cos(std::abs(lon_x_r - lon_y_r))) * RADIUS;
}

ResponseHolder Response::Create(Response::Type type) {
  switch (type) {
    case Response::Type::SEND_ROUTE:
      return std::make_unique<ReadRouteResponse>();
    case Response::Type::SEND_STOP:
      return std::make_unique<ReadStopResponse>();
    default:
      return nullptr;
  }
}