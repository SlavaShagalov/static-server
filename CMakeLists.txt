cmake_minimum_required(VERSION 3.22)
project(static-server C)

set(CMAKE_C_STANDARD 11)

add_executable(server
        main.c
        tpool/t_pool.c
        tpool/t_pool.h
        queue/task.h
        constants.h
        queue/queue.c
        queue/queue.h
        log/log.c
        log/log.h
        server/server.c
        server/server.h
        net/net.c
        net/net.h
        server/request.c
        server/request.h
        server/responses.h
        server/content_type.c
        server/content_type.h
)
