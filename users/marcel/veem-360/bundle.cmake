include(BundleUtilities)

set(resource_path "/Users/thecat/Google Drive/SDN0-Veem/veem-360/")

set(appresources_path "${CMAKE_BINARY_DIR}/Release/veem-360.app/Contents/Resources")
execute_process(COMMAND mkdir "-p" "${appresources_path}")
execute_process(COMMAND rsync "-r" "${resource_path}" "${appresources_path}")

FIXUP_BUNDLE("${CMAKE_BINARY_DIR}/Release/veem-360.app" "" "")
