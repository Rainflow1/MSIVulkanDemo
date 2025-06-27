#pragma once

#include "../resourceManager.h"
#include "../gameobject_decl.h"
#include "../components/transformComponent.h"
#include "../components/materialComponent.h"
#include "../gameobjectManagerI.h"

#include <pybind11/embed.h>
#include <pybind11/operators.h>
namespace py = pybind11;

#include <iostream>
#include <vector>
#include <type_traits>


namespace PythonBindings{


class Behaviour{

public:
    std::shared_ptr<MSIVulkanDemo::GameObject> gameObject;
    std::vector<py::object> properties;

    Behaviour(){

    }

    virtual void start(){

    }

    virtual void update(float){

    }

    py::object property(py::type T, py::object defaultValue){
        
        py::object newObj = T(defaultValue);

        properties.push_back(newObj);

        return newObj;
    }

    py::object property(py::object obj){

        properties.push_back(obj);

        return obj;
    }

    py::object getComponent(py::type T){
        
        if(T.equal(py::type::of<MSIVulkanDemo::TransformComponent>())){
            return py::cast(gameObject->getComponent<MSIVulkanDemo::TransformComponent>(), py::return_value_policy::reference);
        }

        if(T.equal(py::type::of<MSIVulkanDemo::MaterialComponent>())){
            return py::cast(gameObject->getComponent<MSIVulkanDemo::MaterialComponent>(), py::return_value_policy::reference);
        }
        
        return py::none();
    }

};

class PyBehaviour : public Behaviour{

public:
    using Behaviour::Behaviour;

    void start() override {
        PYBIND11_OVERRIDE_PURE(
            void,
            Behaviour,
            start
        );
    }

    void update(float deltatime) override {
        PYBIND11_OVERRIDE_PURE(
            void,
            Behaviour,
            update,
            deltatime
        );
    }
};


class ObjectRef : Behaviour{
public:
    ObjectRef(){
        std::shared_ptr<MSIVulkanDemo::GameobjectManagerI> tak;
    }

    ObjectRef(py::none){
        
    }

    ObjectRef(std::shared_ptr<MSIVulkanDemo::GameObject> obj){
        gameObject = obj;
    }

    py::object getComponent(py::type T){
        return Behaviour::getComponent(T);
    }

    bool operator==(const py::none) const{
        return gameObject == nullptr;
    }

    void setReference(std::shared_ptr<MSIVulkanDemo::GameObject> obj){
        gameObject = obj;
    }

    std::shared_ptr<MSIVulkanDemo::GameObject> getReference(){
        return gameObject;
    }
    
};

class PyString : public std::string{

};


PYBIND11_EMBEDDED_MODULE(MSIVulkanDemo, m) {
    py::class_<Behaviour, PyBehaviour>(m, "Behaviour")
        .def(py::init<>())
        .def("update", &Behaviour::update)
        .def("start", &Behaviour::start)
        .def("getComponent", &Behaviour::getComponent)
        .def("property", py::overload_cast<py::object>(&Behaviour::property))
        .def("property", py::overload_cast<py::type, py::object>(&Behaviour::property));
    
    py::class_<glm::vec3>(m, "Vector3")
        .def(py::init<>())
        .def(py::init<float, float, float>())
        .def(py::init([](std::tuple<float, float, float> val){
            return std::unique_ptr<glm::vec3>(new glm::vec3(std::get<0>(val), std::get<1>(val), std::get<2>(val)));
        }))
        .def_readwrite("x", &glm::vec3::x)
        .def_readwrite("y", &glm::vec3::y)
        .def_readwrite("z", &glm::vec3::z)
        .def("__str__", [](const glm::vec3 &a){
            return "(" + std::to_string(a.x) + ", " + std::to_string(a.y) + ", " + std::to_string(a.z) + ")";
        }, py::is_operator())
        .def(py::self + py::self)
        .def(py::self - py::self)
        .def(py::self += py::self)
        .def(py::self -= py::self)
        .def(py::self * py::self)
        .def(py::self / py::self);

    py::implicitly_convertible<std::tuple<float, float, float>, glm::vec3>();

    py::class_<MSIVulkanDemo::TransformComponent>(m, "TransformComponent")
        .def(py::init<>())
        .def(py::init<glm::vec3, glm::vec3, glm::vec3>())
        .def("setPosition", &MSIVulkanDemo::TransformComponent::setPosition)
        .def("getPosition", &MSIVulkanDemo::TransformComponent::getPosition)
        .def("setRotation", &MSIVulkanDemo::TransformComponent::setRotation)
        .def("getRotation", &MSIVulkanDemo::TransformComponent::getRotation)
        .def("setScale", &MSIVulkanDemo::TransformComponent::setScale)
        .def("getScale", &MSIVulkanDemo::TransformComponent::getScale);

    py::class_<MSIVulkanDemo::MaterialComponent>(m, "MaterialComponent")
        .def("hasUniform", &MSIVulkanDemo::MaterialComponent::hasUniform)
        .def("hasUniform", [](MSIVulkanDemo::MaterialComponent& mat, const PyString& a){
            return mat.hasUniform(a.c_str());
        })
        .def("setUniform", &MSIVulkanDemo::MaterialComponent::setUniform<glm::vec3>)
        .def("setUniform", [](MSIVulkanDemo::MaterialComponent& mat, const PyString& a, const glm::vec3& b){
            return mat.setUniform(a.c_str(), b);
        });

    py::class_<ObjectRef>(m, "ObjectRef")
        .def(py::init<>())
        .def(py::init<py::none>())
        .def("getComponent", &ObjectRef::getComponent)
        .def("__eq__", [](const ObjectRef& a, py::none n){
            return a==n;
        }, py::is_operator());
    
    py::class_<PyString>(m, "String")
        .def(py::init<>())
        .def(py::init<std::string>())
        .def("__str__", [](const PyString &a){
            return static_cast<std::string>(a);
        }, py::is_operator());


}


}

namespace MSIVulkanDemo{


class Script : public Resource{

private:
    std::string source;
    std::string scriptName;
    std::vector<py::object> behaviours;

    //std::map<std::string, std::pair<PropertyType, std::any>> propierties;
    std::map<std::string, std::any> properties; // FIXME to component

    bool started = false;

public:
    Script(std::string path){

        readFile(path);

        try{
            
            py::object globalScope = py::module_::import("__main__").attr("__dict__");

            py::dict localScope = py::dict(**globalScope);

            py::exec(source, localScope);

            for(auto [key, value] : localScope){

                if(py::isinstance<py::type>(value) && value.attr("__bases__").contains(localScope["Behaviour"])){
                    behaviours.push_back(value());

                    if(scriptName.empty()){
                        scriptName = py::cast<std::string>(key);
                    }
                }
                
            }

        }catch(py::error_already_set e){
            std::cout << "Python error: " << std::endl;
            std::cout << e.what() << std::endl;
        }
        
    }

    ~Script(){}

    void start(std::shared_ptr<GameObject> gameObject){

        started = true;

        for(auto obj : behaviours){
            try{
                obj.cast<PythonBindings::Behaviour*>()->gameObject = gameObject;
                obj.attr("start")();

                for(const auto handle : obj.attr("__dict__")){
                    py::object objCast = obj.attr(handle);
                    for(const auto prop : obj.cast<PythonBindings::Behaviour*>()->properties){
                        if(objCast.is(prop)){
                            //TODO to typelist
                            if(py::isinstance<PythonBindings::ObjectRef>(prop)){ 
                                properties.insert({py::cast<std::string>(handle), std::any(py::cast<PythonBindings::ObjectRef*>(prop))});
                            }
                            else if(py::isinstance<glm::vec3>(prop)){ 
                                properties.insert({py::cast<std::string>(handle), std::any(py::cast<glm::vec3*>(prop))});
                            }
                            else if(py::isinstance<PythonBindings::PyString>(prop)){ 
                                properties.insert({py::cast<std::string>(handle), std::any(static_cast<std::string*>(py::cast<PythonBindings::PyString*>(prop)))});
                            }
                            else{
                                py::print(py::str("Unsupported property type: ") + py::str(py::type::of(prop)));
                            }
                        }
                    }
                }

            }catch(py::error_already_set e){
                std::cout << "Python error: " << std::endl;
                std::cout << e.what() << std::endl;
            }
        }

    }

    void update(float deltatime){

        for(auto obj : behaviours){
            try{
                obj.attr("update")(deltatime);
            }catch(py::error_already_set e){
                std::cout << "Python error: " << std::endl;
                std::cout << e.what() << std::endl;
            }
        }

    }

    bool isStarted(){
        return started;
    }

    std::string getName(){
        return scriptName;
    }

    std::vector<std::pair<std::string, std::any>> getProperties(){
        std::vector<std::pair<std::string, std::any>> props;

        std::copy(properties.begin(), properties.end(), std::back_inserter(props));

        return props;
    }

    bool hasProperty(std::string name){
        return properties.contains(name);
    }

    template<class T>
    void setProperty(std::string name, T val){
        if(hasProperty(name) && properties.at(name).type() == typeid(T)){
            properties.at(name) = std::make_any<T>(val);
        }
    }

private:

    void loadDependency(std::vector<std::any> dependencies){
        return;
    }

    void readFile(std::string path){
        std::ifstream file(path, std::ios::ate);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file: " + path);
        }

        size_t fileSize = (size_t) file.tellg();
        std::string buffer(fileSize, '\0');

        file.seekg(0);
        file.read(&buffer[0], fileSize);

        file.close();

        source = buffer;
    }

};


}