#include <CLI/CLI.hpp>
#include <uWebSockets/App.h>
#include <csignal>
#include <iostream>
#include <sstream>
#include <utility>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libuv.h>
#include "usocket_internal.h"
#include "charts_page.h"

#define APP_VERSION "1.0.0"

uWS::App *globalApp;
redisAsyncContext *globalRedisContext;
std::string redisStreamKeyStr;
const char* redisStreamKeyC;
int streamInitlen = 50;

/* ws->getUserData returns one of these */
struct PerSocketData {
    /* Fill with user data */
};

void onMessage(redisAsyncContext *c, void *replyPtr, void *) {
    auto *reply = static_cast<redisReply *>(replyPtr);
    redisReply *retReply;

    if ((reply == nullptr) || (c->err)) {
        std::cout << "error: [ctl] redisCommand reply is error, " << c->errstr << std::endl;
        redisAsyncCommand(c, onMessage, nullptr, "XREAD block 10000 streams %s $", redisStreamKeyC);
        return;
    }

    if (reply->type == REDIS_REPLY_ARRAY && reply->elements > 0) {
        retReply = reply->element[0];
        if (retReply->type == REDIS_REPLY_ARRAY && retReply -> elements > 0) {
            //std::cout << retReply->element[0]->type << " " << retReply->element[0]->str << std::endl;  // stream name

            retReply = retReply->element[1]->element[0];
            if (retReply->type == REDIS_REPLY_ARRAY && retReply -> elements > 0) {
                //std::cout << retReply->element[0]->type << " " << retReply->element[0]->str << std::endl;  // id name

                retReply = retReply->element[1];
                if (retReply->type == REDIS_REPLY_ARRAY && retReply -> elements > 0) {
                    //std::cout << retReply->element[0]->type << " " << retReply->element[0]->str << std::endl;  // key 1
                    //std::cout << retReply->element[1]->type << " " << retReply->element[1]->str << std::endl;  // value 1
                    globalApp->publish("broadcast", std::string_view(retReply->element[1]->str, retReply->element[1]->len), uWS::OpCode::TEXT, false);
                }
            }
        }
    }

    // loop
    redisAsyncCommand(c, onMessage, nullptr, "XREAD block 10000 streams %s $", redisStreamKeyC);
}

void onColdStartup(redisAsyncContext *c, void *replyPtr, void *privdata) {
    auto *r = static_cast<redisReply *>(replyPtr);
    redisReply *reply = r;
    redisReply *retReply;

    if ((reply == nullptr) || (c->err)) {
        std::cout << "error: [ctl] redisCommand reply is error, " << c->errstr << std::endl;
        return;
    }

    auto ws = static_cast<uWS::WebSocket<false, true, PerSocketData> *>(privdata);

    if (reply->elements > 0) {
        std::ostringstream stream;
        stream << R"({"type":"init","data":[)";
        auto lastIndex = reply->elements - 1;
        for (int i = 0; i < reply->elements; i++) {
            //std::cout << reply->element[i]->type << " " << reply->element[i]->elements << std::endl;
            retReply = reply->element[i];
            if (retReply->type == REDIS_REPLY_ARRAY && retReply->elements > 0) {
                //std::cout << retReply->element[0]->type << " " << retReply->element[0]->str << std::endl;  // id name

                retReply = retReply->element[1];
                if (retReply->type == REDIS_REPLY_ARRAY && retReply->elements > 0) {
                    //std::cout << retReply->element[0]->type << " " << retReply->element[0]->str << std::endl;  // key 1
                    //std::cout << retReply->element[1]->type << " " << retReply->element[1]->str << std::endl;  // value 1
                    stream << retReply->element[1]->str;
                    if (i < lastIndex) {
                        stream << ',';
                    }
                }
            }
        }
        stream << "]" << R"(,"initlen":)" << streamInitlen << "}";

        // send
        //std::string string = stream.str();
        //std::cout << string << std::endl;
        ws->send(stream.str(), uWS::OpCode::TEXT, true);
    }

    ws->subscribe("broadcast");
}

int initServer(std::string bindIp, int bindPort) {
    signal(SIGPIPE, SIG_IGN);

    // prepare for web service
    auto app = uWS::App()
            .get("/", [](auto *res, uWS::HttpRequest *req) {
                res->writeStatus(uWS::HTTP_200_OK);
                res->writeHeader("Content-Type", "text/html");
                res->end(CHARTS_PAGE_HTML_SOURCE);
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

                    //ws->subscribe("broadcast");
                    /* Open event here, you may access ws->getUserData() which points to a PerSocketData struct */
                    redisAsyncCommand(globalRedisContext, onColdStartup, ws, "XREVRANGE %s + - COUNT %i", redisStreamKeyC, streamInitlen);
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
            .listen(std::move(bindIp), bindPort, [](auto *listen_socket) {
                if (listen_socket) {
                    std::cout << "Listening on port " << 3000 << std::endl;
                }
            });

    // hack libuv loop
    auto *us_loop = (struct us_loop_t_copy *) uWS::Loop::get();
    // install redis async client to libuv
    redisLibuvAttach(globalRedisContext, us_loop->uv_loop);

    // start message watcher
    redisAsyncCommand(globalRedisContext, onMessage, nullptr, "XREAD block 10000 streams %s $", redisStreamKeyC);

    globalApp = &app;
    app.run();

    std::cout << "Failed to listen on port 3000" << std::endl;
    return 0;
}

void initRedis(const char *bindIp, int bindPort, const char *auth) {
    // connect redis
    globalRedisContext = redisAsyncConnect(bindIp, bindPort);
    if (globalRedisContext->err) {
        std::cout << "connect redis error: " << globalRedisContext->errstr << std::endl;
        exit(1);
    }
    if (auth != nullptr) {
        redisAsyncCommand(globalRedisContext, nullptr, nullptr, "auth %s", auth);
    }
}

int main(int argc, const char *argv[]) {
    CLI::App app("Realtime Dashboard");
    // add version output
    app.set_version_flag("-V,--version", std::string(APP_VERSION));
    std::string bindIp = "127.0.0.1";
    app.add_option("-B,--bind-ip", bindIp, "Web server bind ip");
    int bindPort = 3000;
    app.add_option("-P,--bind-port", bindPort, "Web server bind port");
    std::string redisHost = "127.0.0.1";
    app.add_option("-r,--redis-host", redisHost, "Redis host");
    int redisPort = 6379;
    app.add_option("-p,--redis-port", redisPort, "Redis port");
    std::string redisAuth;
    app.add_option("-a,--redis-auth", redisAuth, "Redis auth password");
    app.add_option("-s,--redis-stream", redisStreamKeyStr, "Redis stream key")->required();
    redisStreamKeyC = redisStreamKeyStr.c_str();
    app.add_option("-l,--stream-init-len", streamInitlen, "Redis stream length when a new connection");
    CLI11_PARSE(app, argc, argv)

    initRedis(redisHost.c_str(), redisPort, redisAuth.empty() ? nullptr : redisAuth.c_str());
    initServer(bindIp, bindPort);

    return 0;
}
