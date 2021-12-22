# Realtime Dashboard

A minimalist, frustration-free, single-node websocket server, and continuously broadcasts redis stream every message to every connection from. Provide a simple real chart page. And it will add more chart type in future.

## Quickstart

Type the following command in your terminal:

```bash
./realtime_dashboard_backend --redis-stream teststream
```

And then, open browser `http://localhost:3000/`

## Installing Dependencies

Debug environment

```bash
conan install . -s build_type=Debug --install-folder=cmake-build-debug
```

Release environment

```bash
conan install . -s build_type=Release --install-folder=cmake-build-release
```

## TODO

- [x] Simple demo, broadcast subscribe messages from redis to every websocket.
- [x] Cache history data.
- [x] Easy to use and copy
- [ ] Simple authentication.
- [ ] Charts configurable. (Such as multicharts, layouts, types)

## License

This project is licensed under the MIT license.

