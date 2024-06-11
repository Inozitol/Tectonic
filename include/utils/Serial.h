#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <cstring>
#include <span>

#include "SerialTypes.h"
#include "engine/vulkan/VktTypes.h"

namespace Serial {
    /*
    // TODO Finish this mess
    // Maybe with C++26 when reflection comes out?
    namespace Convert{
        namespace Model {

            using VertexTuple = std::tuple<glm::vec3,
                    float,
                    glm::vec3,
                    float,
                    glm::vec4,
                    glm::uvec4,
                    glm::vec4>;

            using MeshTuple = std::tuple<SerialTypes::Span<uint32_t, SerialTypes::Model::MeshSurface, true>,
                    SerialTypes::Span<uint32_t, uint32_t, true>,
                    SerialTypes::Span<uint32_t, VktTypes::Vertex, true>>;

            using ImageTuple = std::tuple<SerialTypes::Span<uint32_t, char, true>,
                    VkExtent3D,
                    VkFormat,
                    SerialTypes::Span<uint32_t, std::byte, true>>;

            using MaterialResourcesTuple = std::tuple<SerialTypes::Model::ImageID_t,
                    SerialTypes::Model::ImageSamplerID_t,
                    SerialTypes::Model::ImageID_t,
                    SerialTypes::Model::ImageSamplerID_t>;

            using MaterialConstantsTuple = std::tuple<glm::vec4,
                    glm::vec4,
                    std::array<glm::vec4, 14>>;

            using GLTFMaterialTuple = std::tuple<VktTypes::GLTFMetallicRoughness::MaterialConstants,
                    SerialTypes::Model::MaterialResources,
                    VktTypes::MaterialPass>;

            using NodeTuple = std::tuple<SerialTypes::Span<uint32_t, char, true>,
                    SerialTypes::Model::NodeID_t,
                    SerialTypes::Span<uint32_t, SerialTypes::Model::NodeID_t, true>,
                    SerialTypes::Model::MeshID_t,
                    glm::mat4,
                    glm::mat4,
                    glm::mat4,
                    glm::vec3,
                    glm::vec3,
                    glm::quat,
                    SerialTypes::Model::SkinID_t>;

            using AnimSamplerTuple = std::tuple<SerialTypes::Model::Interpolation,
                    SerialTypes::Span<uint32_t, float, true>,
                    SerialTypes::Span<uint32_t, glm::vec4, true>>;

            using AnimChannelTuple = std::tuple<SerialTypes::Model::NodeID_t,
                                                SerialTypes::Model::AnimSamplerID_t,
                                                SerialTypes::Model::AnimSamplerID_t,
                                                SerialTypes::Model::AnimSamplerID_t>;

            using AnimationTuple = std::tuple<SerialTypes::Span<uint32_t, char, true>,
                    float,
                    float,
                    SerialTypes::Span<uint32_t, SerialTypes::Model::AnimationSampler, true>,
                    SerialTypes::Span<uint32_t, SerialTypes::Model::AnimationChannel, true>,
                    SerialTypes::Span<uint32_t, std::pair<uint32_t, uint32_t>, true>>;

            using SkinTuple = std::tuple<SerialTypes::Span<uint32_t, char, true>,
                    SerialTypes::Model::NodeID_t,
                    SerialTypes::Span<uint32_t, SerialTypes::Model::NodeID_t, true>,
                    SerialTypes::Span<uint32_t, glm::mat4, true>,
                    SerialTypes::Span<uint32_t, SerialTypes::Model::NodeID_t, true>>;
        }

        template<typename T>
        struct itemReturn{typedef T type;};
        template<typename T>
        typename itemReturn<T>::type item(){ return T{}; }

        template<>
        struct itemReturn<SerialTypes::Model::MeshAsset>{typedef Model::MeshTuple type;};
        template<>
        typename itemReturn<SerialTypes::Model::MeshAsset>::type item<Model::MeshTuple>(){ return {}; }

        template<>
        struct itemReturn<VktTypes::Vertex>{typedef Model::VertexTuple type;};
        template<>
        itemReturn<VktTypes::Vertex>::type item<Model::VertexTuple>(){ return {}; }

        template<>
        struct itemReturn<SerialTypes::Model::Image>{
            typedef Model::ImageTuple type;
        };
        template<>
        itemReturn<SerialTypes::Model::Image>::type item<Model::ImageTuple>(){ return {}; }

        template<>
        struct itemReturn<SerialTypes::Model::MaterialResources>{
            typedef Model::MaterialResourcesTuple type;
        };
        template<>
        itemReturn<SerialTypes::Model::MaterialResources>::type item<Model::MaterialResourcesTuple>(){ return {}; }


        template<>
        struct itemReturn<VktTypes::GLTFMetallicRoughness::MaterialConstants>{
            typedef Model::MaterialConstantsTuple type;
        };
        template<>
        itemReturn<VktTypes::GLTFMetallicRoughness::MaterialConstants>::type item<Model::MaterialConstantsTuple>(){ return {}; }

        template<>
        struct itemReturn<SerialTypes::Model::GLTFMaterial>{
            typedef Model::GLTFMaterialTuple type;
        };
        template<>
        itemReturn<SerialTypes::Model::GLTFMaterial>::type item<Model::GLTFMaterialTuple>(){ return {}; }

        template<>
        struct itemReturn<SerialTypes::Model::Node>{
            typedef Model::NodeTuple type;
        };
        template<>
        itemReturn<SerialTypes::Model::Node>::type item<Model::NodeTuple>(){ return {}; }

        template<>
        struct itemReturn<SerialTypes::Model::AnimationSampler>{
            typedef Model::AnimSamplerTuple type;
        };
        template<>
        itemReturn<SerialTypes::Model::AnimationSampler>::type item<Model::AnimSamplerTuple>(){ return {}; }

        template<>
        struct itemReturn<SerialTypes::Model::AnimationChannel>{
            typedef Model::AnimChannelTuple type;
        };
        template<>
        itemReturn<SerialTypes::Model::AnimationChannel>::type item<Model::AnimChannelTuple>(){ return {}; }

        template<>
        struct itemReturn<SerialTypes::Model::Animation>{
            typedef Model::AnimationTuple type;
        };
        template<>
        itemReturn<SerialTypes::Model::Animation>::type item<Model::AnimationTuple>(){ return {}; }

        template<>
        struct itemReturn<SerialTypes::Model::Skin>{
            typedef Model::SkinTuple type;
        };
        template<>
        itemReturn<SerialTypes::Model::Skin>::type item<Model::SkinTuple>(){ return {}; }

        namespace Model{
            template<typename T>
            decltype(item<T>()) convert(const T& d){
                return d;
            }

            template<>
            decltype(item<SerialTypes::Model::MeshAsset>()) convert(const SerialTypes::Model::MeshAsset& m){
                return MeshTuple{m.surfaces,m.indices,m.vertices};
            }

            template<>
            decltype(item<VktTypes::Vertex>()) convert(const VktTypes::Vertex& v){
                return VertexTuple{v.position, v.uvX, v.normal, v.uvY, v.color, v.jointIndices, v.jointWeights};
            }

            template<>
            decltype(item<SerialTypes::Model::Image>()) convert(const SerialTypes::Model::Image& i){
                return ImageTuple{i.name, i.extent, i.format, i.data};
            }

            template<>
            decltype(item<SerialTypes::Model::MaterialResources>()) convert(const SerialTypes::Model::MaterialResources& r){
                return MaterialResourcesTuple{r.colorImage, r.colorSampler, r.metalRoughImage, r.metalRoughSampler};
            }

            template<>
            decltype(item<VktTypes::GLTFMetallicRoughness::MaterialConstants>()) convert(const VktTypes::GLTFMetallicRoughness::MaterialConstants& c){
                return MaterialConstantsTuple{c.colorFactors, c.metalRoughFactors, c.extra};
            }

            template<>
            decltype(item<SerialTypes::Model::GLTFMaterial>()) convert(const SerialTypes::Model::GLTFMaterial& m){
                return GLTFMaterialTuple{m.constants,m.resources,m.pass};
            }

            template<>
            decltype(item<SerialTypes::Model::Node>()) convert(const SerialTypes::Model::Node& n){
                return NodeTuple{n.name,n.parent,n.children,n.mesh,n.localTransform,n.worldTransform,
                                 n.animationTransform,n.translation,n.scale,n.rotation,n.skin};
            }

            template<>
            decltype(item<SerialTypes::Model::AnimationSampler>()) convert(const SerialTypes::Model::AnimationSampler& s){
                return AnimSamplerTuple{s.interpolation,s.inputs,s.outputsVec4};
            }

            template<>
            decltype(item<SerialTypes::Model::AnimationChannel>()) convert(const SerialTypes::Model::AnimationChannel& c){
                return AnimChannelTuple{c.node,c.scaleSampler,c.rotationSampler,c.translationSampler};
            }

            template<>
            decltype(item<SerialTypes::Model::Animation>()) convert(const SerialTypes::Model::Animation& a){
                return AnimationTuple{a.name,a.start,a.end,a.samplers,a.channels,a.animatedNodes};
            }
            template<>
            decltype(item<SerialTypes::Model::Skin>()) convert(const SerialTypes::Model::Skin& s){
                return SkinTuple{s.name,s.skeletonRoot,s.skinNodes,s.inverseBindMatrices,s.joints};
            }
        }
    }
*/
    /**
     * Pushes any plain type/struct into data vector
     * @tparam T Type of data
     * @param data Destination
     * @param n Data
     */
    template<typename T>
    void pushData(SerialTypes::BinDataVec_t &data, const T &n) {
        std::byte bytes[sizeof(T)];
        std::copy(static_cast<const std::byte*>(static_cast<const void*>(&n)),
                  static_cast<const std::byte*>(static_cast<const void*>(&n))+sizeof(n),
                  bytes);
        for(std::size_t i = 0; i < sizeof(T); i++){
            data.push_back(bytes[i]);
        }
    }

    /**
     * Pushes any plain type/struct into data vector at a given offset, overwriting existing data
     * @tparam T Type of data
     * @param data Destination
     * @param n Data
     * @param offset Offset
     */
    template<typename T>
    void pushData(SerialTypes::BinDataVec_t &data, const T &n, std::size_t offset) {
        std::byte bytes[sizeof(T)];
        std::copy(static_cast<const std::byte*>(static_cast<const void*>(&n)),
                  static_cast<const std::byte*>(static_cast<const void*>(&n))+sizeof(n),
                  bytes);
        for(std::size_t i = 0; i < sizeof(T); i++){
            data[offset+i] = bytes[i];
        }
    }

    /**
     * Pushes any vector of plain types/structs into data vector
     * @tparam T Type of data inside vector
     * @param data Destination
     * @param vec Vector of data
     */
    template<typename T>
    void pushData(SerialTypes::BinDataVec_t &data, const std::vector<T> &vec){
        pushData<uint32_t>(data, vec.size());
        for(const auto& e : vec){
            pushData<T>(data, e);
        }
    }

    /**
     * Pushes a span of plain types/structs into data vector
     * @tparam T Type of data inside span
     * @param data Destination
     * @param span Span of data
     */
    template<typename T, typename I>
    void pushData(SerialTypes::BinDataVec_t &data, const SerialTypes::Span<I,T,true> &span){
        pushData<I>(data, span.size());
        for(I i = 0; i < span.size(); i++){
            pushData<T>(data, span[i]);
        }
    }


    /**
     * Pushes a basic string into data vector
     * @param data Destination
     * @param str String
     */
    template<typename T>
    void pushData(SerialTypes::BinDataVec_t &data, const std::basic_string<T>& str){
        pushData<uint32_t>(data, str.size());
        for(const auto& c : str){
            pushData<uint8_t>(data,c);
        }
    }

    /**
     * Reads a plain type/struct from data vector at a given offset
     * @tparam T Type of data
     * @param data Source
     * @param offset Offset
     */
    template <typename T>
    T readDataAt(SerialTypes::BinDataVec_t &data, std::size_t offset){
        T n;
        std::memcpy(&n, data.data()+offset, sizeof(T));
        return n;
    }

    /**
     * Reads a plain type/struct from data vector at a given offset and increments offset by size of type
     * @tparam T Type of data
     * @param data Source
     * @param offset Offset
     */
    template <typename T>
    T readDataAtInc(SerialTypes::BinDataVec_t &data, std::size_t& offset){
        T n;
        std::memcpy(&n, data.data()+offset, sizeof(T));
        offset += sizeof(T);
        return n;
    }

    /**
     * Reads a vector of plain types/structs from data vector at a given offset
     * @tparam T Type of data
     * @param data Source
     * @param offset Offset
     * @return Vector of T
     */
    template <typename T>
    std::vector<T> readVectorAt(SerialTypes::BinDataVec_t &data, std::size_t offset){
        uint32_t vecSize = readDataAtInc<uint32_t>(data, offset);
        std::vector<T> vec(vecSize);
        std::memcpy(vec.data(), reinterpret_cast<T*>(data.data()+offset), vecSize*sizeof(T));
        return vec;
    }

    /**
     * Reads a vector of plain types/structs from data vector at a given offset and increments offset by size of vector
     * @tparam T Type of data
     * @param data Source
     * @param offset Offset
     * @return Vector of T
     */
    template <typename T>
    std::vector<T> readVectorAtInc(SerialTypes::BinDataVec_t &data, std::size_t& offset){
        uint32_t vecSize = readDataAtInc<uint32_t>(data, offset);
        std::vector<T> vec(vecSize);
        std::memcpy(vec.data(), reinterpret_cast<T*>(data.data()+offset), vecSize*sizeof(T));
        offset += vecSize*sizeof(T);
        return vec;
    }

    /**
     * Creates a span of plain types/structs from data vector at a given offset and increments offset by size of vector
     * @tparam T Type of data
     * @param data Source
     * @param offset Offset
     * @return Span of T
     */
    template <typename T>
    std::span<T> readSpanAt(SerialTypes::BinDataVec_t &data, std::size_t offset){
        uint32_t spanSize = readDataAtInc<uint32_t>(data, offset);
        std::span<T> span(reinterpret_cast<T*>(data.data()+offset), spanSize);
        return span;
    }

    /**
     * Creates a span of plain types/structs from data vector at a given offset and increments offset by size of vector
     * @tparam T Type of data
     * @param data Source
     * @param offset Offset
     * @return Span of T
     */
    template <typename T>
    std::span<T> readSpanAtInc(SerialTypes::BinDataVec_t &data, std::size_t& offset){
        uint32_t spanSize = readDataAtInc<uint32_t>(data, offset);
        std::span<T> span(reinterpret_cast<T*>(data.data()+offset), spanSize);
        offset += spanSize*sizeof(T);
        return span;
    }

}
