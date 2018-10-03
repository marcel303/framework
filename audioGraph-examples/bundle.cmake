include(BundleUtilities)

set(resource_path "/Users/thecat/Google Drive/The Grooop - The Tribe/")
set(appresources_path "${CMAKE_BINARY_DIR}/Release/892-landart.app/Contents/Resources")

execute_process(COMMAND mkdir "-p" "${appresources_path}")
execute_process(COMMAND rsync "-r" "${resource_path}" "${appresources_path}")

FIXUP_BUNDLE("${CMAKE_BINARY_DIR}/Release/892-landart.app" "" "")

#

set(resource_path "/Users/thecat/Google Drive/The Grooop - The Tribe/")
set(sdresource_path "/Users/thecat/Sexyshow/media/")
set(appresources_path "${CMAKE_BINARY_DIR}/Release/890-performance.app/Contents/Resources")

execute_process(COMMAND mkdir "-p" "${appresources_path}")
execute_process(COMMAND rsync "-r" "${resource_path}" "${appresources_path}")

execute_process(COMMAND mkdir "-p" "${appresources_path}/media")
execute_process(COMMAND rsync "-r" "${sdresource_path}" "${appresources_path}/media")

#define MEDIA_PATH "/Users/thecat/Sexyshow/media/"

FIXUP_BUNDLE("${CMAKE_BINARY_DIR}/Release/890-performance.app" "" "")

#

set(resource_path "/Users/thecat/framework/audioGraph-examples/data/")
set(appresources_path "${CMAKE_BINARY_DIR}/Release/330-reflections.app/Contents/Resources")

execute_process(COMMAND mkdir "-p" "${appresources_path}")
execute_process(COMMAND rsync "-r" "${resource_path}" "${appresources_path}")

FIXUP_BUNDLE("${CMAKE_BINARY_DIR}/Release/330-reflections.app" "" "")
