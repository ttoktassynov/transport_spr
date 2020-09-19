#include "test_runner.h"
#include "request.h"
#include "route_manager.h"
#include <vector>
#include <string>
#include <sstream>

using namespace std;

void TestUpdateRequests(){
    const string input = "10\n"
        "Stop Tolstopaltsevo: 55.611087, 37.20829\n"
        "Stop Marushkino: 55.595884, 37.209755\n"
        "Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > "
        "Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye\n"
        "Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka\n"
        "Stop Rasskazovka: 55.632761, 37.333324\n"
        "Stop Biryulyovo Zapadnoye: 55.574371, 37.6517\n"
        "Stop Biryusinka: 55.581065, 37.64839\n"
        "Stop Universam: 55.587655, 37.645687\n"
        "Stop Biryulyovo Tovarnaya: 55.592028, 37.653656\n"
        "Stop Biryulyovo Passazhirskaya: 55.580999, 37.659164";
    istringstream input_stream(input);
    const auto update_requests = ReadRequests<0>(input_stream);
    ASSERT_EQUAL(update_requests.size(), 10);
    {
        const auto& request = static_cast<const AddStopRequest&>(*update_requests[0]);
        ASSERT_EQUAL(request.lat, 55.611087);
        ASSERT_EQUAL(request.lon, 37.20829);
        ASSERT_EQUAL(request.stop, "Tolstopaltsevo");
    }
    
    {
        const vector<string> stops = {
            "Biryulyovo Zapadnoye",
            "Biryusinka",
            "Universam",
            "Biryulyovo Tovarnaya",
            "Biryulyovo Passazhirskaya",
            "Biryulyovo Zapadnoye"
        };
        const auto& request = static_cast<const AddRouteRequest&>(*update_requests[2]);
        ASSERT_EQUAL(request.route, "256");
        ASSERT_EQUAL(request.stops, stops);
    }
    {
        const vector<string> stops = {
            "Tolstopaltsevo",
            "Marushkino",
            "Rasskazovka",
            "Marushkino",
            "Tolstopaltsevo"
        };
        const auto& request = static_cast<const AddRouteRequest&>(*update_requests[3]);
        ASSERT_EQUAL(request.route, "750");
        ASSERT_EQUAL(request.stops, stops);
    }
}


void TestReadRequests(){
    const string input = "5\n"
                        "Bus 256\n"
                        "Bus 750\n"
                        "Bus 751 2 3\n"
                        "Stop xyz\n"
                        "Stop yu iu";
    istringstream input_stream(input);
    const auto update_requests = ReadRequests<1>(input_stream);
    ASSERT_EQUAL(update_requests.size(), 5);
    {
        const auto& request = static_cast<const ReadRouteRequest&>(*update_requests[0]);
        ASSERT_EQUAL(request.route, "256");
    }
    {
        const auto& request = static_cast<const ReadRouteRequest&>(*update_requests[2]);
        ASSERT_EQUAL(request.route, "751 2 3");
    }
    {
        const auto& request = static_cast<const ReadStopRequest&>(*update_requests[3]);
        ASSERT_EQUAL(request.stop, "xyz");
    }
    {
        const auto& request = static_cast<const ReadStopRequest&>(*update_requests[4]);
        ASSERT_EQUAL(request.stop, "yu iu");
    }
}

void TestResponses(){
    const string input = "13\n"
        "Stop Tolstopaltsevo: 55.611087, 37.20829\n"
        "Stop Marushkino: 55.595884, 37.209755\n"
        "Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > "
        "Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye\n"
        "Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka\n"
        "Stop Rasskazovka: 55.632761, 37.333324\n"
        "Stop Biryulyovo Zapadnoye: 55.574371, 37.6517\n"
        "Stop Biryusinka: 55.581065, 37.64839\n"
        "Stop Universam: 55.587655, 37.645687\n"
        "Stop Biryulyovo Tovarnaya: 55.592028, 37.653656\n"
        "Stop Biryulyovo Passazhirskaya: 55.580999, 37.659164\n"
        "Bus 828: Biryulyovo Zapadnoye > Universam > Rossoshanskaya ulitsa > Biryulyovo Zapadnoye\n"
        "Stop Rossoshanskaya ulitsa: 55.595579, 37.605757\n"
        "Stop Prazhskaya: 55.611678, 37.603831\n"
        "6\n"
        "Bus 256\n"
        "Bus 750\n"
        "Bus 751\n"
        "Stop Samara\n"
        "Stop Prazhskaya\n"
        "Stop Biryulyovo Zapadnoye";

    istringstream input_stream(input);
    const auto update_requests = ReadRequests<0>(input_stream);
    RouteManager manager;

    ProcessRequests(update_requests, manager);
    const auto read_requests = ReadRequests<1>(input_stream);
    const auto responses = ProcessRequests(read_requests, manager);

    const string expected = "Bus 256: 6 stops on route, 5 unique stops, 4371.017264 route length\n"
                            "Bus 750: 5 stops on route, 3 unique stops, 20939.483047 route length\n"
                            "Bus 751: not found\n"
                            "Stop Samara: not found\n"
                            "Stop Prazhskaya: no buses\n"
                            "Stop Biryulyovo Zapadnoye: buses 256 828 \n";
    stringstream output_stream;
    PrintResponses(responses, output_stream);
    ASSERT_EQUAL(output_stream.str(), expected);
}