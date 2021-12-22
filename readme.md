# Realtime Dashboard

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
- [ ] Charts configurable. (Such as layouts, types, complex)


## Usage

Use the following command:

```bash
./realtime_dashboard_backend --redis-stream teststream
```

