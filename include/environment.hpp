#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <ostream>
#include <string>
#include <unordered_map>
#include "instruction.hpp"

using Table = std::unordered_map<std::string, ValueType>;

class Environment {
private:
    Table table;
    Environment* parent;

public:
    Environment(Table env, Environment* parent);

    void define(std::string name, ValueType value);

    void assign(std::string name, ValueType value);

    ValueType lookup(std::string name);

    Table& resolve(std::string name);

    bool isDefined(std::string name);

    friend std::ostream& operator<<(std::ostream& os, const Environment& env);
};

#endif