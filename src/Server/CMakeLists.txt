project(LiteMQServer)

add_executable(LiteMQServer
  main.cpp
  )

target_link_libraries(LiteMQServer
  Gears
  LiteMQ
  ${CMAKE_THREAD_LIBS_INIT}
  )

install(TARGETS LiteMQServer DESTINATION bin)
