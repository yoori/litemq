project(ServerGearsHashServer)

add_executable(ServerGearsHashServer
  main.cpp
  )

target_link_libraries(ServerGearsHashServer
  Gears
  LiteMQ
  pthread)

install(TARGETS ServerGearsHashServer DESTINATION bin)
