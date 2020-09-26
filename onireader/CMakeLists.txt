add_library(_onireader MODULE _onireader)

if (SKBUILD)
  python_extension_module(_onireader)
  install(TARGETS _onireader LIBRARY DESTINATION onireader)
else (SKBUILD)
  set_target_properties(_onireader PROPERTIES PREFIX "${PYTHON_MODULE_PREFIX}"
	SUFFIX "${PYTHON_MODULE_EXTENSION}")
  set_target_properties(_onireader PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/onireader)
endif(SKBUILD)

target_link_libraries(_onireader pybind11::module ${OPENNI2_LIBRARY})
