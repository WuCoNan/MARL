build_lib_example(
    NAME ns3ai_dsdv_gnn_marl
    SOURCE_FILES dsdv_gnn_marl.cc
    LIBRARIES_TO_LINK ${libai} ${libdsdv} ${libwifi} ${libinternet} ${libapplications} ${libmobility} ${libnetwork}
)

pybind11_add_module(ns3ai_dsdv_gnn_marl_py dsdv_gnn_marl_py.cc)
set_target_properties(ns3ai_dsdv_gnn_marl_py PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_dependencies(ns3ai_dsdv_gnn_marl ns3ai_dsdv_gnn_marl_py)
