project(LiteMQClient)

add_executable(LiteMQClient
  main.cpp
  )

target_link_libraries(LiteMQClient
  Gears
  LiteMQ
  ${CMAKE_THREAD_LIBS_INIT}
  )

install(TARGETS LiteMQClient DESTINATION bin)
