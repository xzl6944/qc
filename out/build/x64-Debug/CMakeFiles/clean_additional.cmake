# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "src\\app\\App_autogen"
  "src\\app\\CMakeFiles\\App_autogen.dir\\AutogenUsed.txt"
  "src\\app\\CMakeFiles\\App_autogen.dir\\ParseCache.txt"
  "src\\communication\\CMakeFiles\\communication_autogen.dir\\AutogenUsed.txt"
  "src\\communication\\CMakeFiles\\communication_autogen.dir\\ParseCache.txt"
  "src\\communication\\communication_autogen"
  "src\\data\\CMakeFiles\\data_autogen.dir\\AutogenUsed.txt"
  "src\\data\\CMakeFiles\\data_autogen.dir\\ParseCache.txt"
  "src\\data\\data_autogen"
  "src\\logic\\CMakeFiles\\logic_autogen.dir\\AutogenUsed.txt"
  "src\\logic\\CMakeFiles\\logic_autogen.dir\\ParseCache.txt"
  "src\\logic\\logic_autogen"
  "src\\simulator\\CMakeFiles\\simulator_autogen.dir\\AutogenUsed.txt"
  "src\\simulator\\CMakeFiles\\simulator_autogen.dir\\ParseCache.txt"
  "src\\simulator\\simulator_autogen"
  "src\\ui\\CMakeFiles\\ui_autogen.dir\\AutogenUsed.txt"
  "src\\ui\\CMakeFiles\\ui_autogen.dir\\ParseCache.txt"
  "src\\ui\\ui_autogen"
  )
endif()
