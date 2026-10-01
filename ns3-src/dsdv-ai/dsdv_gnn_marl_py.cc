#include "dsdv_gnn_marl_msg.h"
#include <ns3/ai-module.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <iostream>
#include <vector>

namespace py = pybind11;
using namespace ns3;

PYBIND11_MODULE(ns3ai_dsdv_gnn_marl_py, m)
{
    py::class_<GnnObsMsg>(m, "PyGnnObsMsg")
        .def(py::init<>())
        .def_readwrite("numNodes", &GnnObsMsg::numNodes)
        .def_readwrite("numEdges", &GnnObsMsg::numEdges)
        .def_readwrite("numNeighbors", &GnnObsMsg::numNeighbors)
        .def_readwrite("pathDiversity", &GnnObsMsg::pathDiversity)
        .def_property("nodeFeatures",
            [](const GnnObsMsg& msg) {
                return std::vector<float>(msg.nodeFeatures,
                    msg.nodeFeatures + msg.numNodes * MSG_NODE_FEAT);
            },
            [](GnnObsMsg& msg, const std::vector<float>& v) {
                for (size_t i = 0; i < v.size() && i < MAX_NODES * MSG_NODE_FEAT; ++i)
                    msg.nodeFeatures[i] = v[i];
            })
        .def_property("edgeFeatures",
            [](const GnnObsMsg& msg) {
                return std::vector<float>(msg.edgeFeatures,
                    msg.edgeFeatures + msg.numEdges * MSG_EDGE_FEAT);
            },
            [](GnnObsMsg& msg, const std::vector<float>& v) {
                for (size_t i = 0; i < v.size() && i < MAX_EDGES * MSG_EDGE_FEAT; ++i)
                    msg.edgeFeatures[i] = v[i];
            })
        .def_property("edgeSrc",
            [](const GnnObsMsg& msg) {
                return std::vector<uint32_t>(msg.edgeSrc, msg.edgeSrc + msg.numEdges);
            },
            [](GnnObsMsg& msg, const std::vector<uint32_t>& v) {
                for (size_t i = 0; i < v.size() && i < MAX_EDGES; ++i)
                    msg.edgeSrc[i] = v[i];
            })
        .def_property("edgeDst",
            [](const GnnObsMsg& msg) {
                return std::vector<uint32_t>(msg.edgeDst, msg.edgeDst + msg.numEdges);
            },
            [](GnnObsMsg& msg, const std::vector<uint32_t>& v) {
                for (size_t i = 0; i < v.size() && i < MAX_EDGES; ++i)
                    msg.edgeDst[i] = v[i];
            });

    py::class_<GnnActionMsg>(m, "PyGnnActionMsg")
        .def(py::init<>())
        .def_property("probs",
            [](const GnnActionMsg& msg) {
                return std::vector<float>(msg.probs, msg.probs + MAX_NEIGHBORS);
            },
            [](GnnActionMsg& msg, const std::vector<float>& v) {
                for (size_t i = 0; i < v.size() && i < MAX_NEIGHBORS; ++i)
                    msg.probs[i] = v[i];
            });

    // Vector bindings
    using Impl = Ns3AiMsgInterfaceImpl<GnnObsMsg, GnnActionMsg>;
    py::class_<Impl::Cpp2PyMsgVector>(m, "PyObsVector")
        .def("resize", static_cast<void (Impl::Cpp2PyMsgVector::*)(
            Impl::Cpp2PyMsgVector::size_type)>(&Impl::Cpp2PyMsgVector::resize))
        .def("__len__", &Impl::Cpp2PyMsgVector::size)
        .def("__getitem__", [](Impl::Cpp2PyMsgVector& vec, uint32_t i) -> GnnObsMsg& {
            return vec.at(i);
        }, py::return_value_policy::reference);

    py::class_<Impl::Py2CppMsgVector>(m, "PyActVector")
        .def("resize", static_cast<void (Impl::Py2CppMsgVector::*)(
            Impl::Py2CppMsgVector::size_type)>(&Impl::Py2CppMsgVector::resize))
        .def("__len__", &Impl::Py2CppMsgVector::size)
        .def("__getitem__", [](Impl::Py2CppMsgVector& vec, uint32_t i) -> GnnActionMsg& {
            return vec.at(i);
        }, py::return_value_policy::reference);

    py::class_<Impl>(m, "Ns3AiMsgInterfaceImpl")
        .def(py::init<bool, bool, bool, uint32_t, const char*, const char*, const char*, const char*>())
        .def("PyRecvBegin", &Impl::PyRecvBegin)
        .def("PyRecvEnd", &Impl::PyRecvEnd)
        .def("PySendBegin", &Impl::PySendBegin)
        .def("PySendEnd", &Impl::PySendEnd)
        .def("PyGetFinished", &Impl::PyGetFinished)
        .def("GetCpp2PyVector", &Impl::GetCpp2PyVector, py::return_value_policy::reference)
        .def("GetPy2CppVector", &Impl::GetPy2CppVector, py::return_value_policy::reference);
}
