include (OptionDependentOnPackage)
include (PrintOptionStatus)

option (SOLOUD_DYNAMIC "Set to ON to build dynamic SoLoud" OFF)
print_option_status (SOLOUD_DYNAMIC "Build dynamic library")

option (SOLOUD_STATIC "Set to ON to build static SoLoud" ON)
print_option_status (SOLOUD_STATIC "Build static library")

option (SOLOUD_C_API "Set to ON to include the C API" ON)
print_option_status (SOLOUD_C_API "Build C API")

# TODO:
option (SOLOUD_BUILD_DEMOS "Set to ON for building demos" OFF)
print_option_status (SOLOUD_BUILD_DEMOS "Build demos")

option (SOLOUD_GENERATE_GLUE "Set to ON for generating the Glue APIs" OFF)
print_option_status (SOLOUD_GENERATE_GLUE "Generate Glue")
