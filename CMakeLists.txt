
# Project source code list
SET( OERACERHUD_SOURCES
  # Add all the cpp source files here
  main.cpp
)

# todo get rid of this!@#!
# Include if using SDL under Mac OS X
IF(APPLE)
  SET(OERACERHUD_SOURCES ${OERACERHUD_SOURCES} ${SDL_MAIN_FOR_MAC})
ENDIF(APPLE)

# Project executable
ADD_EXECUTABLE(OERacerHUD ${OERACERHUD_SOURCES})

# Project dependencies
TARGET_LINK_LIBRARIES(OERacerHUD
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
  Extensions_TGAResource
  Extensions_OBJResource
  Extensions_FixedTimeStepPhysics
  Extensions_GenericHandlers
  Extensions_OEGUI	
  ${BOOST_SERIALIZATION_LIB}
)
