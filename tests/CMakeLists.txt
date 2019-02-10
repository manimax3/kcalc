
## TODO https://community.kde.org/Guidelines_and_HOWTOs/CMake/Library

set(kcalc_lib_src
    ${CMAKE_SOURCE_DIR}/bitbutton.cpp
    ${CMAKE_SOURCE_DIR}/kcalc_bitset.cpp
    ${CMAKE_SOURCE_DIR}/kcalc_button.cpp
    ${CMAKE_SOURCE_DIR}/kcalc_const_menu.cpp
    ${CMAKE_SOURCE_DIR}/kcalc_core.cpp
    ${CMAKE_SOURCE_DIR}/kcalc_parser.cpp
    ${CMAKE_SOURCE_DIR}/kcalcdisplay2.cpp
    ${CMAKE_SOURCE_DIR}/kcalc_statusbar.cpp
    ${CMAKE_SOURCE_DIR}/bitbutton.cpp)

add_library(kcalc_lib ${kcalc_lib_src})
add_executable(kcalcparsertest kcalcparsertest.cpp)

target_link_libraries(kcalc_lib
    Qt5::Core
    Qt5::Widgets
    KF5::ConfigWidgets
    KF5::GuiAddons
    KF5::I18n
    KF5::Notifications
    KF5::XmlGui
    KF5::Crash
    ${GMP_LIBRARIES}
    ${MPFR_LIBRARIES}
)

target_link_libraries(kcalcparsertest kcalc_lib)
