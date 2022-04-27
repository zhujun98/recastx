#define PYBIND11_CPP17
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
namespace py = pybind11;

#include "tomcat_server.hpp"
#include "processing.hpp"

PYBIND11_MODULE(tomcatserver, m) {
    m.doc() = "C++ server for TOMCAT";

    py::class_<tomcat::server>(m, "server")
        .def(py::init<std::string, int, int, int, int, int, int, int>())
        .def("set_callback", &tomcat::server::set_callback)
        .def("serve", &tomcat::server::serve,
             py::call_guard<py::gil_scoped_release>());
}
