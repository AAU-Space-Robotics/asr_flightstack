cmake_minimum_required(VERSION 3.5)
project(gunnhild)

find_package(ament_cmake REQUIRED)

# Install launch files
install(DIRECTORY launch/
  DESTINATION share/${PROJECT_NAME}/launch
  FILES_MATCHING PATTERN "*.py"
)

# Install config files
install(DIRECTORY config/
  DESTINATION share/${PROJECT_NAME}/config
  FILES_MATCHING PATTERN "*.yaml"
)

# Install mission scripts
install(DIRECTORY missions/
  DESTINATION lib/${PROJECT_NAME}/missions
  USE_SOURCE_PERMISSIONS
  FILES_MATCHING PATTERN "*.py"
)

ament_package()
