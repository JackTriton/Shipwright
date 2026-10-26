# Generate recovery data from the same JSON translators edit. An ASCII byte
# array avoids MSVC's 64 KiB literal limit and source-code encoding differences.
function(soh_add_builtin_ui_catalog target source_root)
    set(catalog "${source_root}/soh/assets/custom/ui-lang/en_US.json")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${catalog}")
    file(READ "${catalog}" catalog_hex HEX)
    string(REPEAT "[0-9a-f]" 64 hex_line)
    string(REGEX REPLACE "(${hex_line})" "\\1\n" catalog_hex "${catalog_hex}")
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " SOH_UI_ENGLISH_BYTES "${catalog_hex}")
    set(generated "${CMAKE_CURRENT_BINARY_DIR}/generated/BuiltinUICatalog.cpp")
    configure_file("${source_root}/soh/soh/Enhancements/Lang/BuiltinUICatalog.cpp.in" "${generated}" @ONLY)
    target_sources(${target} PRIVATE "${generated}")
endfunction()

# A resource-only change must also reach the executable directory, even when
# the executable does not relink (notably Visual Studio's Debug/Release dirs).
function(soh_stage_port_archive target archive)
    add_custom_target(${target}_port_archive
        COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:${target}>"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${archive}" "$<TARGET_FILE_DIR:${target}>/soh.o2r"
        DEPENDS GenerateSohOtr
        COMMENT "Staging current soh.o2r beside ${target}..."
        VERBATIM
    )
    add_dependencies(${target} ${target}_port_archive)
endfunction()

# Preserve every existing game locale when the archive is missing or old.
# All recovery data is generated from the translator-edited JSON files.
function(soh_add_builtin_game_catalog target source_root)
    file(GLOB catalogs CONFIGURE_DEPENDS "${source_root}/soh/assets/custom/game-lang/*.json")
    set(SOH_GAME_CATALOG_DATA "")
    set(SOH_GAME_CATALOG_LOAD "")
    foreach(catalog IN LISTS catalogs)
        get_filename_component(locale "${catalog}" NAME_WE)
        if(NOT locale MATCHES "^[a-z][a-z]_[A-Z][A-Z]$")
            message(FATAL_ERROR "Invalid game catalog locale: ${locale}")
        endif()
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${catalog}")
        file(READ "${catalog}" catalog_hex HEX)
        string(REPEAT "[0-9a-f]" 64 hex_line)
        string(REGEX REPLACE "(${hex_line})" "\\1\n" catalog_hex "${catalog_hex}")
        string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " catalog_bytes "${catalog_hex}")
        string(APPEND SOH_GAME_CATALOG_DATA "    static const unsigned char data_${locale}[] = { ${catalog_bytes} };\n")
        string(APPEND SOH_GAME_CATALOG_LOAD "    result[\"${locale}\"] = nlohmann::json::parse(std::begin(data_${locale}), std::end(data_${locale}));\n")
    endforeach()
    set(generated "${CMAKE_CURRENT_BINARY_DIR}/generated/BuiltinGameCatalog.cpp")
    configure_file("${source_root}/soh/soh/Enhancements/custom-message/BuiltinGameCatalog.cpp.in" "${generated}" @ONLY)
    target_sources(${target} PRIVATE "${generated}")
endfunction()
