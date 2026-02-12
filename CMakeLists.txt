cmake_minimum_required(VERSION 3.16)
project(widgetsboxmodel VERSION 9.0.0)

find_package(Albert REQUIRED)

albert_plugin(QT Widgets StateMachine Svg)

install(
    DIRECTORY "styles/"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/albert/${PROJECT_NAME}/styles"
    REGEX "styles\\/\\..+" EXCLUDE  # exclude hidden files
)
