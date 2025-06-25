#pragma once

#include <pybind11/embed.h>
namespace py = pybind11;

#include <iostream>
#include <vector>
#include <map>

namespace MSIVulkanDemo{


class ScriptManager{

private:
    py::scoped_interpreter interpreter{};

public:
    ScriptManager(){

        

    }

    ~ScriptManager(){}

};



}