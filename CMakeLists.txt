project(DISPLAY_APP C)

include_directories(fsw/mission_inc)
include_directories(fsw/platform_inc)

# Create the app module
add_cfe_app(display fsw/src/display_app.c fsw/src/display_fb.c)

# depend on IO_LIB
add_cfe_app_dependency(display io_lib)
include_directories(${io_lib_MISSION_DIR}/fsw/public_inc)

# Add table
add_cfe_tables(displayTable fsw/src/display_tbl.c)

# If UT is enabled, then add the tests from the subdirectory
# Note that this is an app, and therefore does not provide
# stub functions, as other entities would not typically make 
# direct function calls into this application.
if (ENABLE_UNIT_TESTS)
  add_subdirectory(unit-test)
endif (ENABLE_UNIT_TESTS)
