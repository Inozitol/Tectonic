#pragma once
#include <cstring>
#include "engine/vulkan/VktTypes.h"

namespace SerialTypes{
    using BinDataVec_t = std::vector<std::byte>;

    /**
     * @brief Custom span that's directly mappable to binary data in BinDataVec_t
     *
     * This can be used to directly map span sequences in form of:
     * [I size][T d1][T d2]...
     *
     * @tparam I Type of size variable
     * @tparam T Type of data
     * @tparam D If true makes the type dynamic with its own memory control and lifetime
     */
    template <typename I, typename T, bool D = false>
    struct Span{

        Span() = default;
        explicit Span(Span<I,T,true>&& other);
        explicit Span(Span<I,T,false>&& other);
        explicit Span(const Span<I,T,true>& other);
        explicit Span(const Span<I,T,false>& other);
        explicit Span(const std::vector<T>& vec) requires(D);
        explicit Span(const std::basic_string<T>& str) requires(D);
        Span(const T* data, std::size_t size) requires(D);
        Span(BinDataVec_t& data, std::size_t& offset);
        ~Span();

        Span<I,T,D>& operator=(Span<I,T,D>&& other) noexcept;
        Span<I,T,D>& operator=(const Span<I,T,true>& other) noexcept;
        Span<I,T,D>& operator=(const Span<I,T,false>& other) noexcept;
        Span<I,T,D>& operator=(const std::basic_string<T>& str) noexcept;
        Span<I,T,D>& operator=(const std::vector<T>& vec) noexcept;

        T& operator[](I i) const;

        bool empty() const;
        void resize(I n);
        I size() const;
        T* data() const;

        struct iter{
        public:
            iter(T* ptr): m_ptr(ptr){}
            iter operator++(){++m_ptr; return *this;}
            bool operator!=(const iter& other) const {return m_ptr != other.m_ptr;}
            const T& operator*() const { return *m_ptr; }
        private:
            T* m_ptr;
        };

        iter begin() const { return iter(m_data);}
        iter end() const { return iter(m_data + (*m_size));}

    private:
        I* m_size = nullptr;
        T* m_data = nullptr;
    };

    template<typename I, typename T, bool D>
    T *Span<I, T, D>::data() const {
        return m_data;
    }

    template<typename I, typename T, bool D>
    I Span<I, T, D>::size() const{
        if(m_size == nullptr) return 0;
        return *m_size;
    }

    template<typename I, typename T, bool D>
    void Span<I, T, D>::resize(I n){
        static_assert(D, "Cannot resize non-owning memory");
        if(m_size != nullptr && n == *m_size) return;
        if(m_data == nullptr){
            m_size = new I;
            m_data = new T[n];
        }else {
            T *tmp = new T[n];
            std::memcpy(tmp, m_data, n * sizeof(T));
            delete m_data;
            m_data = tmp;
        }
        *m_size = n;
    }

    template<typename I, typename T, bool D>
    bool Span<I, T, D>::empty() const{
        return (m_size == nullptr || *m_size == 0);
    }

    template<typename I, typename T, bool D>
    Span<I, T, D>::~Span(){
        if(D){delete(m_size); delete(m_data);}
    }

    template<typename I, typename T, bool D>
    Span<I, T, D>::Span(BinDataVec_t &data, std::size_t& offset):
    m_size(reinterpret_cast<I*>(data.data()+offset)), m_data(reinterpret_cast<T*>(data.data()+offset+sizeof(I))){
        static_assert(!D, "Cannot assign non-owning memory to dynamic Span");
        offset += sizeof(I) + ((*m_size)*sizeof(T));
        if(*m_size == 0) m_data = nullptr;
    }

    template<typename I, typename T, bool D>
    Span<I, T, D>::Span(const T *data, std::size_t size) requires(D):
    m_size(new I), m_data(new T[size]){
        *m_size = size;
        std::memcpy(m_data, data, size*sizeof(T));
    }

    template<typename I, typename T, bool D>
    T& Span<I, T, D>::operator[](I i) const{
        return m_data[i];
    }

    template<typename I, typename T, bool D>
    Span<I, T, D>::Span(const Span<I, T, true> &other):
    m_size(new I), m_data(new T[other.size()]){
        if(other.empty()){
            *this->m_size = 0;
        }else {
            *this->m_size = *other.m_size;
            std::memcpy(this->m_data, other.m_data, (*other.m_size) * sizeof(T));
        }
    }

    template<typename I, typename T, bool D>
    Span<I, T, D>::Span(const Span<I, T, false> &other):
    m_size(other.m_size), m_data(other.m_data){}

    template<typename I, typename T, bool D>
    Span<I, T, D>::Span(Span<I, T, true> &&other):
    m_size(other.m_size), m_data(other.m_data){
        other.m_size = nullptr;
        other.m_data = nullptr;
    }

    template<typename I, typename T, bool D>
    Span<I, T, D>::Span(Span<I, T, false> &&other):
    m_size(other.size()), m_data(other.m_data){}

    template<typename I, typename T, bool D>
    Span<I, T, D>::Span(const std::vector<T> &vec) requires(D):
    m_size(new I), m_data(new T[vec.size()]){
        *this->m_size = vec.size();
        std::memcpy(this->m_data, vec.data(), vec.size()*sizeof(T));
    }

    template<typename I, typename T, bool D>
    Span<I, T, D>::Span(const std::basic_string<T> &str) requires(D):
    m_size(new I), m_data(new T[str.size()+1]){
        *this->m_size = str.size()+1;
        std::memcpy(this->m_data, str.data(), (str.size()+1)*sizeof(T));
    }

    template<typename I, typename T, bool D>
    Span<I, T, D> &Span<I, T, D>::operator=(const std::basic_string<T> &str) noexcept {
        m_size = new I;
        m_data = new T[str.size()+1];
        *m_size = str.size()+1;
        std::memcpy(this->m_data, str.data(), (str.size()+1)*sizeof(T));
        return *this;
    }

    template<typename I, typename T, bool D>
    Span<I, T, D> &Span<I, T, D>::operator=(const std::vector<T> &vec) noexcept {
        this->m_size = new I;
        this->m_data = new T[vec.size()];
        *this->m_size = vec.size();
        std::memcpy(this->m_data, vec.data(), vec.size()*sizeof(T));
        return *this;
    }

    template<typename I, typename T, bool D>
    Span<I,T,D>& Span<I, T, D>::operator=(Span<I,T,D> &&other) noexcept {
        this->m_size = other.m_size;
        this->m_data = other.m_data;
        other.m_size = nullptr;
        other.m_data = nullptr;
        return *this;
    }

    template<typename I, typename T, bool D>
    Span<I, T, D>& Span<I, T, D>::operator=(const Span<I, T, true> &other) noexcept {
        this->m_size = new I;
        this->m_data = new T[other.size()];
        this->m_size = *other.size;
        std::memcpy(this->m_data, other.m_data, (*other.m_size)*sizeof(T));
        return *this;
    }

    template<typename I, typename T, bool D>
    Span<I, T, D>& Span<I, T, D>::operator=(const Span<I, T, false> &other) noexcept {
        this->m_size = other.m_size;
        this->m_data = other.m_data;
        return *this;
    }

    namespace Model{
        constexpr uint32_t NULL_ID = UINT32_MAX;

        using ImageID_t         = uint32_t;
        using ImageSamplerID_t  = uint32_t;
        using MeshID_t          = uint32_t;
        using MaterialID_t      = uint32_t;
        using NodeID_t          = uint32_t;
        using SkinID_t          = uint32_t;
        using AnimationID_t     = uint32_t;
        using AnimSamplerID_t   = uint32_t;
        using AnimChannelID_t   = uint32_t;

        /** Each index is defined by the offset of type size from the previous one */
        constexpr std::size_t VERSION_OFFSET    = 0;
        constexpr std::size_t META_OFFSET       = VERSION_OFFSET    + sizeof(std::byte);
        constexpr std::size_t MESHES_INDEX      = META_OFFSET       + sizeof(std::byte);
        constexpr std::size_t IMAGES_INDEX      = MESHES_INDEX      + sizeof(uint32_t);
        constexpr std::size_t SAMPLERS_INDEX    = IMAGES_INDEX      + sizeof(uint32_t);
        constexpr std::size_t NODES_INDEX       = SAMPLERS_INDEX    + sizeof(uint32_t);
        constexpr std::size_t MATERIALS_INDEX   = NODES_INDEX       + sizeof(uint32_t);
        constexpr std::size_t SKIN_INDEX        = MATERIALS_INDEX   + sizeof(uint32_t);
        constexpr std::size_t ANIMATION_INDEX   = SKIN_INDEX        + sizeof(uint32_t);

        enum class MetaBits : uint8_t{
            SKINNED = 1 << 0
        };

        /** @brief Serializable mesh surface */
        struct MeshSurface{
            uint32_t startIndex;
            uint32_t count;
            MaterialID_t materialIndex;
        };

        /** @brief Serializable mesh */
        template<bool Skinned>
        struct MeshAsset{
            SerialTypes::Span<uint32_t,MeshSurface,true> surfaces;
            SerialTypes::Span<uint32_t,uint32_t,true> indices;
            SerialTypes::Span<uint32_t,VktTypes::Vertex<Skinned>,true> vertices;
        };

        /** @brief Serializable image */
        struct Image{
            SerialTypes::Span<uint32_t,char,true> name;
            VkExtent3D extent {.width = 0, .height = 0, .depth = 0};
            VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
            SerialTypes::Span<uint32_t,std::byte,true> data;
        };

        /** @brief Serializable material resource */
        struct MaterialResources {
            ImageID_t colorImage                = NULL_ID;
            ImageSamplerID_t colorSampler       = NULL_ID;
            ImageID_t metalRoughImage           = NULL_ID;
            ImageSamplerID_t metalRoughSampler  = NULL_ID;
        };

        /** @brief Serializable GLTF material
         * VktTypes::GLTFMetallicRoughness::MaterialConstants and VktTypes::MaterialPass are plain structs so we can copy it directly
         */
        struct GLTFMaterial {
            MaterialResources resources;
            VktTypes::GLTFMetallicRoughness::MaterialConstants constants;
            VktTypes::MaterialPass pass;
        };

        /** @brief Serializable node */
        struct Node {
            SerialTypes::Span<uint32_t,char,true> name;

            NodeID_t parent = NULL_ID;
            SerialTypes::Span<uint32_t,NodeID_t,true> children;

            MeshID_t mesh = NULL_ID;

            glm::mat4 localTransform = glm::identity<glm::mat4>();
            glm::mat4 worldTransform = glm::identity<glm::mat4>();
            glm::mat4 animationTransform = glm::identity<glm::mat4>();

            glm::vec3 translation{0.0f, 0.0f, 0.0f};
            glm::vec3 scale{1.0f};
            glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};

            SkinID_t skin = NULL_ID;
        };

        enum class Interpolation : uint8_t {
            LINEAR,
            STEP,
            CUBICSPLINE
        };

        /** @brief Serializable animation sampler */
        struct AnimationSampler {
            Interpolation interpolation;
            SerialTypes::Span<uint32_t,float,true> inputs;
            SerialTypes::Span<uint32_t,glm::vec4,true> outputsVec4;
        };

        /** @brief Serializable animation channel */
        struct AnimationChannel {
            NodeID_t node = NULL_ID;

            AnimSamplerID_t scaleSampler        = NULL_ID;
            AnimSamplerID_t rotationSampler     = NULL_ID;
            AnimSamplerID_t translationSampler  = NULL_ID;
        };

        /** @brief Serializable animation */
        struct Animation {
            SerialTypes::Span<uint32_t,char,true> name;
            float start = std::numeric_limits<float>::max();
            float end = std::numeric_limits<float>::min();
            std::vector<AnimationSampler> samplers;
            SerialTypes::Span<uint32_t,AnimationChannel,true> channels;
            SerialTypes::Span<uint32_t,std::pair<uint32_t, uint32_t>,true> animatedNodes;
        };

        /** @brief Serializable skin */
        struct Skin {
            SerialTypes::Span<uint32_t,char,true> name;
            NodeID_t skeletonRoot = NULL_ID;
            SerialTypes::Span<uint32_t,NodeID_t,true> skinNodes;
            SerialTypes::Span<uint32_t,glm::mat4,true> inverseBindMatrices;
            SerialTypes::Span<uint32_t,NodeID_t,true> joints;
        };
    }
}