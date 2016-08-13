#ifndef MESHFACTORY_HPP
#define MESHFACTORY_HPP

#include <memory>
#include <string>

#include "IMesh.hpp"

class SdlManager;

class MeshFactory
{
public:
    enum Types {
        TRIANGLE,
        PLANE,
        CUBE,
//        SPHERE,
//        MONKEY,
    };
public:
    static IMesh::Ptr ProduceMesh(const Types& type);
    static IMesh::Ptr ProduceMesh(const Types& type,
        const SdlManager& sdl,
        const std::string& filename);
};

#endif // MESHFACTORY_HPP
