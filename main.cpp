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
redisAsyncContext *globalRedisContext;

void onMessage(redisAsyncContext *c, void *replyPtr, void *privdata) {
    auto *r = static_cast<redisReply *>(replyPtr);
    redisReply *reply = r;
    redisReply *retReply;
    if (reply == nullptr) return;

    if ((reply == NULL) || (c->err)) {
        std::cout << "error: [ctl] redisCommand reply is error, " << c->errstr << std::endl;
        if (reply) {
//            freeReplyObject(reply);
        }
        redisAsyncCommand(c, onMessage, nullptr, "XREAD block 10000 streams %s $", "teststream");
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
//    freeReplyObject(reply);

    // loop
    redisAsyncCommand(c, onMessage, nullptr, "XREAD block 10000 streams %s $", "teststream");
}

void onColdStartup(redisAsyncContext *c, void *replyPtr, void *privdata) {
    auto *r = static_cast<redisReply *>(replyPtr);
    redisReply *reply = r;
    redisReply *retReply;
    if (reply == nullptr) return;

    if ((reply == NULL) || (c->err)) {
        std::cout << "error: [ctl] redisCommand reply is error, " << c->errstr << std::endl;
        if (reply) {
//            freeReplyObject(reply);
        }
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
                    std::cout << retReply->element[0]->type << " " << retReply->element[0]->str << std::endl;  // key 1
                    std::cout << retReply->element[1]->type << " " << retReply->element[1]->str << std::endl;  // value 1
                }
            }
        }
    }
}

int main() {
    signal(SIGPIPE, SIG_IGN);

    // connect redis
    redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 6379);
    globalRedisContext = c;
    if (c->err) {
        std::cout << "connect redis error: " << c->errstr << std::endl;
        exit(1);
    }

    // buffer chart page
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

                    //ws->subscribe("broadcast");
                    /* Open event here, you may access ws->getUserData() which points to a PerSocketData struct */
                    redisAsyncCommand(globalRedisContext, onMessage, ws, "XREVRANGE %s + - COUNT %s", "teststream", "100");
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

    // hack libuv loop
    auto *us_loop = (struct us_loop_t_copy *) uWS::Loop::get();
    // install redis async client to libuv
    redisLibuvAttach(c, us_loop->uv_loop);

    // start message watcher
    //redisAsyncCommand(c, onMessage, nullptr, "XREAD block 10000 streams %s $", "teststream");
    redisAsyncCommand(c, onColdStartup, nullptr, "XREVRANGE %s + - COUNT %s", "teststream", "2");

    globalApp = &app;
    app.run();

    std::cout << "Failed to listen on port 3000" << std::endl;
    return 0;
}
