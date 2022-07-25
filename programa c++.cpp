#include "iostream"
#include "vector"
#include "utility"
#include "string"
#include "crow.h"
#include "cpp_redis/core/client.hpp"
int main(){

    crow::SimpleApp app;
    cpp_redis::client redisClient;
        redisClient.connect();

        CROW_ROUTE(app, "/health")([](){
            return "Fine";
        });

    CROW_ROUTE(app, "/api/blogs").methods(crow::HTTPMethod::POST)
    ([&](const crow::request& req){
        auto body = crow::json::load(req.body);
        if (!body)
            return crow::response(400, "Invalid order");
        std::string id, user;
    try { 
        id = body["id"].s();
        user = body["user"].s();
        }
    catch (const std::runtime_error &err) {
        return crow::response(400, "Invalid order");
    }

    try {
        redisClient.lpush("IDs", { id });
        redisClient.lpush("Users", { user });
        redisClient.sync_commit();
        } 
    catch (const std::runtime_error &ex) {
        return crow::response(500, "Internal Server Error");
    }
    return crow::response(200, "Order added");
    }); 

    CROW_ROUTE(app, "/api/blogs")
    ([&]()
    {
        auto r1 = redisClient.lrange("IDs", 0, -1);
        auto r2 = redisClient.lrange("Users", 0, -1);
        redisClient.sync_commit();
        r1.wait();
        r2.wait();
        auto r = r1.get().as_array();
        std::vector<std::string> IDs(r.size()), Users(r.size());
        std::transform(r.begin(), r.end(), IDs.begin(), [](const cpp_redis::reply
        &rep) { return rep.as_string(); });
        auto rC = r2.get().as_array();
        std::transform(rC.begin(), rC.end(), Users.begin(), [](const
        cpp_redis::reply &rep) { return rep.as_string(); });
        std::vector<crow::json::wvalue> blogs;
        
        for (int i = 0; i < 1000000; i++){
            blogs.push_back(crow::json::wvalue{{"id", IDs[i]},
            {"user", Users[i]}  });

        }
    return crow::json::wvalue{{"Orders", blogs}};
    });
    app.port(3000).multithreaded().run();
}
