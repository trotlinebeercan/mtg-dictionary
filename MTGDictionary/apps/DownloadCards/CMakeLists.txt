include_directories ("${CMAKE_CURRENT_BINARY_DIR}")
link_directories ("/usr/local/lib")

file (GLOB_RECURSE DC_SOURCES "*.cpp")
file (GLOB_RECURSE DC_UIFILES "*.ui")

qt4_wrap_ui (DC_UIFILES ${DC_UIFILES})

add_executable (download_cards ${DC_SOURCES} ${DC_UIFILES})
target_link_libraries (download_cards ${DEPENDENCIES} qjson)