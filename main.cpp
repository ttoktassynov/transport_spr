#include "request.h"
#include "route_manager.h"
#include "json.h"

void TestUpdateRequests();
void TestReadRequests();
void TestResponses();


int main(){
    freopen("input/other_input.json", "r", stdin);
    freopen("output/output5.json", "w", stdout);
    //TestRunner tr;
    //RUN_TEST(tr, TestUpdateRequests);
    //RUN_TEST(tr, TestReadRequests);
    //RUN_TEST(tr, TestResponses);
    
    std::stringstream input_info;

    RouteManager manager;
    Json::Document document = Json::Load(std::cin);
    const Json::Node node = document.GetRoot();
    const auto routing_settings = ReadSettings(node, input_info);
    const auto base_requests = ReadRequests<0>(node);
    ProcessRequests(base_requests, manager);

    manager.RunGraphBuilder(routing_settings);
    
    const auto stat_requests = ReadRequests<1>(node);
    const auto responses = ProcessRequests(stat_requests, manager);
    PrintResponses(responses, input_info);
}
