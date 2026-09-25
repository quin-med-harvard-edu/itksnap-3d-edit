# Run by CPack after staging and before signing. iCloud-synced folders tag
# files with com.apple.FinderInfo, which makes codesign refuse with "resource
# fork, Finder information, or similar detritus not allowed".
#
# Note: this runs inside cpack, where platform variables such as APPLE are
# not defined; the script is only registered on macOS.
foreach(dir IN ITEMS
    "${CPACK_TEMPORARY_INSTALL_DIRECTORY}"
    "${CPACK_TEMPORARY_DIRECTORY}"
    "${CPACK_TOPLEVEL_DIRECTORY}")
  if(dir AND IS_DIRECTORY "${dir}")
    message(STATUS "Stripping extended attributes under ${dir}")
    execute_process(COMMAND xattr -cr "${dir}")
    break()
  endif()
endforeach()
