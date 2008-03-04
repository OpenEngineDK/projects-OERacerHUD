
# Project source code list
SET( OERACER_SOURCES
  # Add all the cpp source files here
  main.cpp
  GameFactory.cpp
  MoveHandler.cpp
)

# todo get rid of this!@#!
# Include if using SDL under Mac OS X
IF(APPLE)
  SET(OERACER_SOURCES ${OERACER_SOURCES} ${SDL_MAIN_FOR_MAC})
ENDIF(APPLE)

# Project executable
ADD_EXECUTABLE(OERacer ${OERACER_SOURCES})

# Project dependencies
TARGET_LINK_LIBRARIES(OERacer
  OpenEngine_Core
  OpenEngine_Logging
  OpenEngine_Display
  OpenEngine_Devices
  OpenEngine_Renderers
  OpenEngine_Renderers_OpenGL
  OpenEngine_Resources
  OpenEngine_Scene
  OpenEngine_Utils
  # Extensions
  Extensions_SDL
  Extensions_FixedTimeStepPhysics
  ${BOOST_SERIALIZATION_LIB}
)
