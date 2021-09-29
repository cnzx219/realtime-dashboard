#include <uWebSockets/App.h>
#include <csignal>
#include <fstream>
#include <iostream>
#include <sstream>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libuv.h>
#include "usocket_internal.h"

uWS::App *globalApp;
std::string charts_html;

void onMessage(redisAsyncContext *c, void *reply, void *privdata) {
    auto *r = static_cast<redisReply *>(reply);
    if (reply == nullptr) return;

    if (r->type == REDIS_REPLY_ARRAY) {
        if (r->element[2]) {
            if (r->element[2]->type == 1) {
                globalApp->publish("broadcast", std::string_view(r->element[2]->str, r->element[2]->len), uWS::OpCode::TEXT, false);
            }
//            std::cout << r->element[2]->type <<std::endl;
//            for (int j = 0; j < r->elements; j++) {
//                printf("%u) %s\n", j, r->element[j]->str);
//            }
        }
    }
}

int main() {
    signal(SIGPIPE, SIG_IGN);

    std::filesystem::path cwd = std::filesystem::current_path() / "../../charts.html";
    std::ifstream inFile;
    inFile.open(cwd.string()); //open the input file
    std::stringstream strStream;
    strStream << inFile.rdbuf();
    charts_html = strStream.str();
    inFile.close();

    /* ws->getUserData returns one of these */
    struct PerSocketData {
        /* Fill with user data */
    };

    auto app = uWS::App()
            .get("/", [](auto *res, uWS::HttpRequest *req) {
                res->writeStatus(uWS::HTTP_200_OK);
                res->writeHeader("Content-Type", "text/html");
                res->end(charts_html);
            })
            .get("/status", [](auto *res, uWS::HttpRequest *req) {
                res->writeStatus(uWS::HTTP_200_OK);
                res->writeHeader("Content-Type", "application/json");
                // std::cout << req->getHeader("cookie") << std::endl;
                res->end(R"({"m":"Works!"})");
            })
            .ws<PerSocketData>("/ws", {
                /* Settings */
                .compression = uWS::DEDICATED_COMPRESSOR_4KB,
                .maxPayloadLength = 100 * 1024 * 1024,
                .idleTimeout = 16,
                .maxBackpressure = 100 * 1024 * 1024,
                .closeOnBackpressureLimit = false,
                .resetIdleTimeoutOnSend = false,
                .sendPingsAutomatically = true,
                /* Handlers */
                .upgrade = nullptr,
                .open = [](auto *ws) {
                    std::cout << "open" << std::endl;
                    ws->subscribe("broadcast");
                    /* Open event here, you may access ws->getUserData() which points to a PerSocketData struct */
                },
                .message = [](auto *ws, std::string_view message, uWS::OpCode opCode) {
                    //ws->send(message, opCode, true);
                },
                .drain = [](auto */*ws*/) {
                    /* Check ws->getBufferedAmount() here */
                },
                .ping = [](auto */*ws*/, std::string_view) {
                    std::cout << "receive ping" << std::endl;
                },
                .pong = [](auto */*ws*/, std::string_view) {
                    std::cout << "receive pong" << std::endl;
                },
                .close = [](auto */*ws*/, int /*code*/, std::string_view /*message*/) {
                    std::cout << "close" << std::endl;
                }
            })
            .listen(3000, [](auto *listen_socket) {
                if (listen_socket) {
                    std::cout << "Listening on port " << 3000 << std::endl;
                }
            });

    // connect redis
    redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 6379);
    if (c->err) {
        std::cout << "error: " << c->errstr << std::endl;
        return -1;
    }
    auto *us_loop = (struct us_loop_t_copy *) uWS::Loop::get();

    // install redis async client to libuv
    redisLibuvAttach(c, us_loop->uv_loop);
    redisAsyncCommand(c, onMessage, nullptr, "SUBSCRIBE testtopic");

    globalApp = &app;
    app.run();

    std::cout << "Failed to listen on port 3000" << std::endl;
    return 0;
}
