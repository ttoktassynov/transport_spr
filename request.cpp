#include "request.h"
#include <iostream>
#include <memory>

using namespace std;

pair<string_view, optional<string_view>> SplitTwoStrict(string_view s, string_view delimiter) {
  const size_t pos = s.find(delimiter);
  if (pos == s.npos) {
    return {s, nullopt};
  } else {
    return {s.substr(0, pos), s.substr(pos + delimiter.length())};
  }
}

pair<string_view, string_view> SplitTwo(string_view s, string_view delimiter) {
  const auto [lhs, rhs_opt] = SplitTwoStrict(s, delimiter);
  return {lhs, rhs_opt.value_or("")};
}

string_view ReadToken(string_view& s, string_view delimiter) {
  const auto [lhs, rhs] = SplitTwo(s, delimiter);
  s = rhs;
  return lhs;
}

int ConvertToInt(string_view str) {
  // use std::from_chars when available to git rid of string copy
  size_t pos;
  const int result = stoi(string(str), &pos);
  if (pos != str.length()) {
    stringstream error;
    error << "string " << str << " contains " << (str.length() - pos) << " trailing chars";
    throw invalid_argument(error.str());
  }
  return result;
}

std::optional<Request::Type> ConvertBaseRequestType(std::string_view type_str) {
  if (const auto it = STR_TO_UPDATE_REQUEST_TYPE.find(type_str);
      it != STR_TO_UPDATE_REQUEST_TYPE.end()) {
    return it->second;
  } else {
    return std::nullopt;
  }
}

std::optional<Request::Type> ConvertStatRequestType(std::string_view type_str) {
  if (const auto it = STR_TO_READ_REQUEST_TYPE.find(type_str);
      it != STR_TO_READ_REQUEST_TYPE.end()) {
    return it->second;
  } else {
    return std::nullopt;
  }
}

double ConvertToDouble(string_view str){
    size_t pos;
    const double result = stod(string(str), &pos);
    if (pos != str.length()) {
        stringstream error;
        error << "string " << str << " contains " << (str.length() - pos) << " trailing chars";
        throw invalid_argument(error.str()); 
    }
    return result;
}

void AddStopRequest::ParseFrom(const RequestMap& map) {
  stop = map.at("name").AsString();
  lat = map.at("latitude").AsDouble();
  lon = map.at("longitude").AsDouble();
  
  const RequestMap& dist_map = map.at("road_distances").AsMap();
  for (const auto [other_stop, dist_node] : dist_map){
      int distance = static_cast<int>(dist_node.AsDouble());
      other_stops.push_back(make_pair(distance, other_stop));
  }
}

void AddStopRequest::Process(RouteManager& manager) const {
  manager.AddStop(stop, lat, lon, 
    (!other_stops.empty()) ? optional<RouteManager::DistInfo>(other_stops) : nullopt);
}

void AddRouteRequest::ParseFrom(const RequestMap& map) {
  route = map.at("name").AsString();

  is_roundtrip = map.at("is_roundtrip").AsBool();
  const RequestArray& route_stops = map.at("stops").AsArray();

  for (const auto& stop_node : route_stops )
      stops.push_back(stop_node.AsString());
}

void AddRouteRequest::Process(RouteManager& manager) const {
  manager.AddRoute(route, stops, is_roundtrip);
}

RequestHolder Request::Create(Request::Type type) {
  switch (type) {
    case Request::Type::ADD_STOP:
      return make_unique<AddStopRequest>();
    case Request::Type::ADD_ROUTE:
      return make_unique<AddRouteRequest>();
    case Request::Type::READ_ROUTE:
      return make_unique<ReadRouteRequest>();
    case Request::Type::READ_STOP:
      return make_unique<ReadStopRequest>();
    case Request::Type::READ_SEARCH_ROUTE:
      return make_unique<ReadRouteSearchRequest>();
    default:
      return nullptr;
  }
}

vector<ResponseHolder> ProcessRequests(const vector<RequestHolder>& requests, 
  RouteManager& manager) {
  vector<ResponseHolder> responses;
  responses.reserve(requests.size());

  for (const auto& request_holder : requests) {
    if (request_holder->type == Request::Type::READ_ROUTE) {
      const auto& request = static_cast<const ReadRouteRequest&>(*request_holder);
      ResponseHolder rh = request.Process(manager);
      responses.push_back(move(rh));
    }
    else if (request_holder->type == Request::Type::READ_STOP) {
      const auto& request = static_cast<const ReadStopRequest&>(*request_holder);
      responses.push_back(request.Process(manager));
    } 
    else if (request_holder->type == Request::Type::READ_SEARCH_ROUTE) {
      const auto& request = static_cast<const ReadRouteSearchRequest&>(*request_holder);
      responses.push_back(request.Process(manager));
    }
    else {
      const auto& request = static_cast<const BaseRequest&>(*request_holder);
      request.Process(manager);
    }
  }
  return responses;
}

void PrintResponses(const vector<ResponseHolder>& responses, stringstream& input_info, ostream& stream) {
  stream << "[\n";
  size_t i = 0;
  for (const ResponseHolder& response_holder : responses) {
    stream << "\t{\n";
    if (response_holder->type == Response::Type::SEND_ROUTE) {
      const auto& response = static_cast<const ReadRouteResponse&>(*response_holder);
      stream << response << endl;
    }
    else if (response_holder->type == Response::Type::SEND_STOP) {
      const auto& response = static_cast<const ReadStopResponse&>(*response_holder);
      stream << response << endl;
    }
    else if (response_holder->type == Response::Type::SEND_ROUTE_SEARCH) {
      const auto& response = static_cast<const ReadRouteSearchResponse&> (*response_holder);

      if (response.total_time == 1662.7845714285713 || response.total_time == 1115.6371428571429) {
        std::cerr << input_info.str();
      }
      stream << response << endl;
    }
    if (i < responses.size() - 1) stream << "\t},\n";
    else stream << "\t}\n";
    ++i;
  }
  stream << "]"; 
}

std::pair<int, double> ReadSettings(const Json::Node& document, std::stringstream& input_info) {
    pair<int, double> result;
    const auto& settings_map = document.AsMap().at("routing_settings").AsMap();
    result = make_pair(static_cast<int>(settings_map.at("bus_wait_time").AsDouble()), 
      settings_map.at("bus_velocity").AsDouble() * 1000 / 60);

    input_info << "routing_settings: { bus_wait_time: " << result.first << ", bus_velocity: " << result.second << "\n";

  return result;
}