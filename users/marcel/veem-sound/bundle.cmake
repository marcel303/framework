include(BundleUtilities)

set(resource_path "/Users/thecat/Google Drive/SDN0-Veem/veem-sound/")

set(appresources_path "${CMAKE_BINARY_DIR}/Release/veem-sound.app/Contents/Resources")
message("resources: ${resource_path}")
message("app resources: ${appresources_path}")

execute_process(COMMAND mkdir "-p" "${appresources_path}")

execute_process(COMMAND rsync "-r" "${resource_path}" "${appresources_path}")

FIXUP_BUNDLE("${CMAKE_BINARY_DIR}/Release/veem-sound.app" "" "")