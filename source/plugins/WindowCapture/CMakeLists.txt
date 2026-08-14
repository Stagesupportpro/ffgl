add_library(ffgl-plugin-windowcapture STATIC)
add_library(ffgl::plugin::windowcapture ALIAS ffgl-plugin-windowcapture)
target_sources(ffgl-plugin-windowcapture PRIVATE
    WindowCaptureSource.h WindowCaptureSource.cpp
    WindowEnumerator.h WindowEnumerator.cpp
    ScreenCapture.h ScreenCapture.cpp
    PluginMain.cpp
)
target_link_libraries(ffgl-plugin-windowcapture PRIVATE ffgl::sdk)
target_link_libraries(ffgl-plugin-windowcapture PRIVATE User32 Gdi32)

install(
    TARGETS     ffgl-plugin-windowcapture
    EXPORT      ffgl--plugin-windowcapture-targets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
