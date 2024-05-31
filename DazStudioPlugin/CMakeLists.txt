set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(DZ_PLUGIN_TGT_NAME dzroblox)
set(DZ_PLUGIN_PROJECT_NAME "DzRoblox")

include_directories(${COMMON_LIB_INCLUDE_DIR})

# if building a plugin and you want the compiled result placed in the Daz Studio ./plugins directory
if(DAZ_STUDIO_EXE_DIR)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DAZ_STUDIO_EXE_DIR}/plugins)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${DAZ_STUDIO_EXE_DIR}/plugins)
endif()

if(WIN32)
	set(OS_SOURCES
		DzRoblox.rc
	)
elseif(APPLE)
	set(OS_SOURCES "")
endif()

set(DPC_IMAGES_CPP "${CMAKE_CURRENT_BINARY_DIR}/DzRobloxIcon.cpp" )
set(DPC_SRC_IMAGES
	"Resources/Images/Daz to Roblox.png"
)

add_custom_command(OUTPUT ${DPC_IMAGES_CPP}
	COMMAND ${DAZ_SDK_DPC_EXE} -mode image -o ${DPC_IMAGES_CPP} ${DPC_SRC_IMAGES}
	DEPENDS ${DPC_SRC_IMAGES}
	WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
set_property(SOURCE ${DPC_IMAGES_CPP} PROPERTY SKIP_AUTOMOC ON)

add_library( ${DZ_PLUGIN_TGT_NAME} SHARED
	DzRobloxAction.cpp
	DzRobloxAction.h
	DzRobloxDialog.cpp
	DzRobloxDialog.h
	pluginmain.cpp
	real_version.h
	version.h
	Resources/resources.qrc
	${DPC_IMAGES_CPP}
	${OS_SOURCES}
	${QA_SRCS}
)

target_include_directories(${DZ_PLUGIN_TGT_NAME}
	PUBLIC
)

target_link_libraries(${DZ_PLUGIN_TGT_NAME}
	PRIVATE
	dzcore
	dzbridge-static
	${DZSDK_QT_CORE_TARGET}
	${DZSDK_QT_GUI_TARGET}
	${DZSDK_QT_SCRIPT_TARGET}
	${DZSDK_QT_NETWORK_TARGET}
	${FBX_IMPORT_LIBS}
	${OPENSUBDIV_LIB}
)

set_target_properties (${DZ_PLUGIN_TGT_NAME}
	PROPERTIES
	FOLDER ""
	PROJECT_LABEL ${DZ_PLUGIN_PROJECT_NAME}
)

if(WIN32)
	target_compile_definitions(${DZ_PLUGIN_TGT_NAME}
		PUBLIC
		$<$<CONFIG:DEBUG>:UNITTEST_DZBRIDGE>
		${DZBRIDGE_LIB_FLAGS}
		__LEGACY_PATHS__
	)
else()
	target_compile_definitions(${DZ_PLUGIN_TGT_NAME}
		PUBLIC
		${DZBRIDGE_LIB_FLAGS}
		__LEGACY_PATHS__
	)
endif()

############################
# PRE-BUILD: Create PLUGINDATA ZIP
############################
# Define your source and destination directories
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/PluginData")
set(ZIP_DEST_DIR "${CMAKE_SOURCE_DIR}/DazStudioPlugin/Resources")
set(ZIP_NAME "plugindata.zip")
# Get all files in the PluginData directory
file(GLOB_RECURSE PLUGINDATA_FILES "${SOURCE_DIR}/*")
# Create the destination directory if it doesn't exist
file(MAKE_DIRECTORY ${ZIP_DEST_DIR})
if(WIN32)
	# PowerShell command to remove the zip file if it exists
	set(PS_REMOVE_ZIP_COMMAND "if (Test-Path -Path ${ZIP_DEST_DIR}/${ZIP_NAME}) { Remove-Item -Path ${ZIP_DEST_DIR}/${ZIP_NAME} -ErrorAction SilentlyContinue }")
	# PowerShell command to zip the folder
	set(PS_ZIP_COMMAND "Compress-Archive -Path ${SOURCE_DIR}/* -DestinationPath ${ZIP_DEST_DIR}/${ZIP_NAME}")
	# Add a custom command that runs before anything else is built
	add_custom_command(OUTPUT ${ZIP_DEST_DIR}/${ZIP_NAME}
					COMMAND echo ${PS_REMOVE_ZIP_COMMAND}
					COMMAND powershell -Command ${PS_REMOVE_ZIP_COMMAND}
					COMMAND echo ${PS_ZIP_COMMAND}
					COMMAND powershell -Command ${PS_ZIP_COMMAND}
					DEPENDS ${PLUGINDATA_FILES}
					WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
					COMMENT "Zipping up the PluginData folder...")
else()
	# Add a custom command that runs before anything else is built
	add_custom_command(OUTPUT ${ZIP_DEST_DIR}/${ZIP_NAME}
                    COMMAND ${CMAKE_COMMAND} -E rm -f ${ZIP_DEST_DIR}/${ZIP_NAME}
                    COMMAND cd ${SOURCE_DIR}
                    COMMAND zip -r ${ZIP_DEST_DIR}/${ZIP_NAME} .
					DEPENDS ${PLUGINDATA_FILES}
					WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
					COMMENT "Zipping up the folder")
endif()
# Create a custom target that depends on the custom command
add_custom_target(plugindata ALL
				DEPENDS ${ZIP_DEST_DIR}/${ZIP_NAME})
# Make sure your main target depends on this custom target
add_dependencies(${DZ_PLUGIN_TGT_NAME} plugindata)
set_target_properties (plugindata
	PROPERTIES
	FOLDER ""
	PROJECT_LABEL "PluginData ZIP"
)
