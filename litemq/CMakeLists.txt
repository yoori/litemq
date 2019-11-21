project(LiteMQ)

# projects executable name
set(TARGET_NAME LiteMQ)

set(
  LITEMQ_SOURCE_FILES
    AcceptorDescriptorHandler.cpp
    DescriptorHandlerPoller.cpp
    Connection.cpp
    Context.cpp
)

add_library(${TARGET_NAME} SHARED ${LITEMQ_SOURCE_FILES})

INSTALL(TARGETS ${TARGET_NAME} DESTINATION lib)
