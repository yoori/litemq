project(Gears)

# projects executable name
set(TARGET_NAME Gears)

set(
  GEARS_SOURCE_FILES
    StringManip.cpp
    AsciiStringManip.cpp
    Time.cpp
    Condition.cpp
    ActiveObject.cpp
    CompositeActiveObject.cpp 
    StringManip.cpp
    ThreadRunner.cpp
    Time.cpp
    Logger.cpp
    StreamLogger.cpp
    SimpleLogger.cpp
    ActiveObjectCallback.cpp
)

add_library(${TARGET_NAME} SHARED ${GEARS_SOURCE_FILES})

INSTALL(TARGETS ${TARGET_NAME} DESTINATION lib)
