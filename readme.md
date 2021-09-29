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
- [ ] Simple authentication.
- [ ] Charts configurable. (Such as layouts, types, complex)
- [ ] Rich data source.