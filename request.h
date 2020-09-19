#pragma once
#include "route_manager.h"
#include "response.h"
#include "json.h"

#include <string>
#include <string_view>
#include <optional>
#include <deque>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <sstream>

std::pair<std::string_view, std::optional<std::string_view>> SplitTwoStrict(std::string_view s, 
                    std::string_view delimiter = " ");

std::pair<std::string_view, std::string_view> SplitTwo(std::string_view s, 
                    std::string_view delimiter = " ");

std::string_view ReadToken(std::string_view& s, std::string_view delimiter = " ");

int ConvertToInt(std::string_view str);

double ConvertToDouble(std::string_view str);

struct Request;
using RequestHolder = std::unique_ptr<Request>;
using RequestMap  = std::map<std::string, Json::Node>;
using RequestArray = std::vector<Json::Node>;

struct Request {
  enum class Type {
    ADD_STOP,
    ADD_ROUTE,
    READ_ROUTE,
    READ_STOP,
    READ_SEARCH_ROUTE
  };

  Request(Type type) : type(type) {}

  virtual void ParseFrom(const RequestMap& map) = 0;
  virtual ~Request() = default;
  static RequestHolder Create(Type type);

  const Type type;
};

const std::unordered_map<std::string_view, Request::Type> STR_TO_UPDATE_REQUEST_TYPE = {
    {"Stop", Request::Type::ADD_STOP},
    {"Bus", Request::Type::ADD_ROUTE}
};

const std::unordered_map<std::string_view, Request::Type> STR_TO_READ_REQUEST_TYPE = {
    {"Bus", Request::Type::READ_ROUTE},
    {"Stop", Request::Type::READ_STOP},
    {"Route", Request::Type::READ_SEARCH_ROUTE}
};

template <typename ResultType>
struct StatRequest : Request {
  using Request::Request;
  virtual ResultType Process(const RouteManager& manager) const = 0;

  int request_id;
};

struct BaseRequest : Request {
  using Request::Request;
  virtual void Process(RouteManager& manager) const = 0;
};

struct ReadRouteRequest : StatRequest<ResponseHolder> {
  ReadRouteRequest() : StatRequest(Type::READ_ROUTE) {}

  void ParseFrom(const RequestMap& map) override{
    route = map.at("name").AsString();
    request_id = static_cast<int>(map.at("id").AsDouble());
  }
  ResponseHolder Process(const RouteManager& manager) const override{
    return manager.ReadRoute(route, request_id);
  }

  std::string route;
};

struct ReadStopRequest : StatRequest<ResponseHolder> {
  ReadStopRequest() : StatRequest(Type::READ_STOP) {}

  void ParseFrom(const RequestMap& map) override {
    stop = map.at("name").AsString();
    request_id = static_cast<int>(map.at("id").AsDouble());
  }
  ResponseHolder Process(const RouteManager& manager) const override {
    return manager.ReadStop(stop, request_id);
  }

  std::string stop;
};

struct ReadRouteSearchRequest : StatRequest<ResponseHolder> {
  ReadRouteSearchRequest() : StatRequest(Type::READ_SEARCH_ROUTE) {}

  void ParseFrom(const RequestMap& map) override {
    //do smth
    from = map.at("from").AsString();
    to = map.at("to").AsString();
    request_id = static_cast<int>(map.at("id").AsDouble());
  }
  ResponseHolder Process(const RouteManager& manager) const override {
    return manager.ReadRouteSearch(from, to, request_id);
  }
  std::string from, to;
};

struct AddStopRequest : BaseRequest {
  AddStopRequest() : BaseRequest(Type::ADD_STOP) {}
  void ParseFrom(const RequestMap& map) override;
  void Process(RouteManager& manager) const override;

  double lat;
  double lon;
  std::string stop;
  RouteManager::DistInfo other_stops;
};

struct AddRouteRequest : BaseRequest {
  AddRouteRequest() : BaseRequest(Type::ADD_ROUTE) {}
  void ParseFrom(const RequestMap& map) override;
  void Process(RouteManager& manager) const override;

  std::string route;
  std::vector<std::string> stops;
  bool is_roundtrip;
};

std::optional<Request::Type> ConvertBaseRequestType(std::string_view type_str);

std::optional<Request::Type> ConvertStatRequestType(std::string_view type_str);

template<int SIGN>
RequestHolder ParseRequest(const RequestMap& map) {
  const auto request_type = SIGN == 0 ? 
        ConvertBaseRequestType(map.at("type").AsString()) :
        ConvertStatRequestType(map.at("type").AsString());
    
  if (!request_type) {
    return nullptr;
  }
  RequestHolder request = Request::Create(*request_type);
  if (request) {
    request->ParseFrom(map);
  };
  return request;
}

template<int SIGN>
std::vector<RequestHolder> ReadRequests(const Json::Node& document) {
  static_assert(SIGN == 0 || SIGN == 1);
  
  const RequestArray& json_requests = (SIGN == 0) ?
    document.AsMap().at("base_requests").AsArray() :
    document.AsMap().at("stat_requests").AsArray();

  std::vector<RequestHolder> requests;
  requests.reserve(json_requests.size());
  
  for (const auto& node : json_requests) {
    if (auto request = ParseRequest<SIGN>(node.AsMap())) {
      requests.push_back(move(request));
    }
  }
  return requests;
}

std::vector<ResponseHolder> ProcessRequests(const std::vector<RequestHolder>& requests,
    RouteManager& manager);

void PrintResponses(const std::vector<ResponseHolder>& responses, std::stringstream& input_info, std::ostream& stream = std::cout);

std::pair<int, double> ReadSettings(const Json::Node& document, std::stringstream& input_info);