
cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(MedicalDemo3)

find_package(VTK COMPONENTS 
  CommonColor
  CommonCore
  CommonDataModel
  FiltersCore
  FiltersSources
  FiltersGeometry
  FiltersModeling
  IOImage
  ImagingCore
  InteractionStyle
  RenderingContextOpenGL2
  RenderingCore
  RenderingFreeType
  RenderingGL2PSOpenGL2
  RenderingOpenGL2
)

if (NOT VTK_FOUND)
  message(FATAL_ERROR "MedicalDemo3: Unable to find the VTK build folder.")
endif()

# Prevent a "command line is too long" failure in Windows.
set(CMAKE_NINJA_FORCE_RESPONSE_FILE "ON" CACHE BOOL "Force Ninja to use response files.")
add_executable(MedicalDemo3 MACOSX_BUNDLE MedicalDemo3.cxx )
  target_link_libraries(MedicalDemo3 PRIVATE ${VTK_LIBRARIES}
)
# vtk_module_autoinit is needed
vtk_module_autoinit(
  TARGETS MedicalDemo3
  MODULES ${VTK_LIBRARIES}
)
