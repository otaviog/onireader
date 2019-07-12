add_library(_onireader MODULE _onireader)
target_link_libraries(_onireader PRIVATE pybind11::module ${OPENNI2_LIBRARY})
set_target_properties(_onireader PROPERTIES PREFIX "${PYTHON_MODULE_PREFIX}"
  SUFFIX "${PYTHON_MODULE_EXTENSION}")
set_target_properties(_onireader PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/onireader)
