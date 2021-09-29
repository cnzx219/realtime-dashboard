//
// Created by Hiro on 2021/9/29.
//

#ifndef REALTIME_DASHBOARD_BACKEND_USOCKET_INTERNAL_H
#define REALTIME_DASHBOARD_BACKEND_USOCKET_INTERNAL_H

// https://github.com/uNetworking/uWebSockets/blob/v19.3.0/src/LoopData.h
struct us_internal_loop_data_t_copy {
    struct us_timer_t *sweep_timer;
    struct us_internal_async *wakeup_async;
    int last_write_failed;
    struct us_socket_context_t *head;
    struct us_socket_context_t *iterator;
    char *recv_buf;
    void *ssl_data;
    void (*pre_cb)(struct us_loop_t *);
    void (*post_cb)(struct us_loop_t *);
    struct us_socket_t *closed_head;
    /* We do not care if this flips or not, it doesn't matter */
    long long iteration_nr;
};

// https://github.com/uNetworking/uSockets/blob/v0.7.1/src/internal/eventing/libuv.h
struct us_loop_t_copy {
    alignas(LIBUS_EXT_ALIGNMENT) struct us_internal_loop_data_t_copy data;

    uv_loop_t *uv_loop;
    int is_default;

    uv_prepare_t *uv_pre;
    uv_check_t *uv_check;
};

#endif //REALTIME_DASHBOARD_BACKEND_USOCKET_INTERNAL_H
