#pragma once

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <iostream>
#include <vector>
#include <map>

namespace MSIVulkanDemo{


class JsonSerializerI{

public:
    JsonSerializerI() = default;

    virtual json saveToJson() = 0;

};

class JsonDeserializerI{

public:
    JsonDeserializerI() = default;

    virtual void loadFromJson(json) = 0;

};


class JsonI : public JsonSerializerI, public JsonDeserializerI {};


}