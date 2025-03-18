#include "engine/model/terrain/Terrain.h"
#include <glm/gtx/string_cast.hpp>
#include "extern/stb/stb_image.h"

#include "engine/GlobalMemory.h"

Terrain::Terrain() { initSignals(); }

Terrain::~Terrain() { clear(); }

void Terrain::initSignals() { flags.sig_flagChanged.connect(slt_flagChange); }

void Terrain::normalize() {
    if(maxHeight <= minHeight) { return; }

    float minMaxDelta = maxHeight - minHeight;
    float minMaxRange = maxRange - minRange;

    for(auto &vert: vertices) { vert.position.y = ((vert.position.y - minHeight) / minMaxDelta) * minMaxRange + minRange; }

    LOG(LOG_DEBUG, "Normalized height values into range between " << minRange << " and " << maxRange);
    calcMinMax();
}

void Terrain::calcMinMax() {
    minHeight = std::numeric_limits<float>::infinity();
    maxHeight = -std::numeric_limits<float>::infinity();

    for(const auto &vert: vertices) {
        minHeight = fminf(minHeight, vert.position.y);
        maxHeight = fmaxf(maxHeight, vert.position.y);
    }

    LOG(LOG_DEBUG, "Calculated min max height values: " << minHeight << " " << maxHeight);
}

void Terrain::generateFlat(uint32_t X, uint32_t Z, const char *textureFile, const char *normalFile) {
    dimX = X;
    dimY = Z;

    LOG(LOG_INFO, "Generating flat terrain of size " << dimX << "x" << dimY);

    generateFlatPlane();

    calcMinMax();
    normalize();
    calcNormals();

    materials.resize(1);
    createMaterial();
    meshBuffers = VktCore::createPrimitivesDeviceMemory(std::span(indices.data(), indices.size()),
                                                        std::span(vertices.data(), vertices.size()));
}

void Terrain::loadHeightmap(const char *heightmapFile, const char *textureFile) {
    int32_t width, height, channels;
    u_char *hMapData = stbi_load(heightmapFile, &width, &height, &channels, 0);

    dimX = width;
    dimY = height;

    LOG(LOG_INFO, "Loading heightmap terrain of size " << dimX << "x" << dimY);

    generateFlatPlane();

    float yScale = 64.0f / 256.0f;

    for(uint32_t i = 0; i < vertices.size(); i++) {
        u_char *texel = hMapData + (i) * channels;

        float y = *texel;

        hMapAt(i) = y * yScale;
        minHeight = fminf(minHeight, hMapAt(i));
        maxHeight = fmaxf(maxHeight, hMapAt(i));
    }

    calcMinMax();
    normalize();
    calcNormals();

    materials.resize(1);
    createMaterial();
    meshBuffers = VktCore::createPrimitivesDeviceMemory(std::span(indices.data(), indices.size()),
                                                        std::span(vertices.data(), vertices.size()));

}

void Terrain::generateMidpoint(uint32_t size, float roughness, const std::vector<std::string> &textureFiles) {
    clear();

    dimX = size;
    dimY = size;

    LOG(LOG_INFO, "Generating midpoint terrain of size " << size);

    generateFlatPlane();

    uint32_t rectSize = Utils::nextPowerOf(size, 2);
    float currHeight = static_cast<float>(rectSize) / 2.0f;
    float heightReduce = powf(2.0f, -roughness);

    while(rectSize > 0) {
        diamondStep(rectSize, currHeight);
        squareStep(rectSize, currHeight);

        rectSize /= 2;
        currHeight *= heightReduce;
    }

    calcMinMax();
    normalize();
    calcNormals();

    std::vector<std::vector<float>> heights;
    heights.resize(patchesY);
    for(uint32_t patchY = 0; patchY < patchesY; patchY++) {
        heights.at(patchY).resize(patchesX);
        for(uint32_t patchX = 0; patchX < patchesX; patchX++) { heights.at(patchY).at(patchX) = hMapAt(patchX * (patchSize - 1) + (patchSize - 1) / 2, patchY * (patchSize - 1) + (patchSize - 1) / 2); }
    }

    lodManager.loadHeightsPerPatch(heights);

    materials.resize(1);
    createMaterial();
    meshBuffers = VktCore::createPrimitivesDeviceMemory(std::span(indices.data(), indices.size()),
                                                        std::span(vertices.data(), vertices.size()));

}

void Terrain::createMaterial() {
    VktTypes::MaterialPass pass = VktTypes::MaterialPass::OPAQUE;

    VktTypes::GLTFMetallicRoughness::MaterialResources gpuResources;
    gpuResources.colorImage = VktCorePtr->m_whiteImage;
    gpuResources.colorSampler = VktCachePtr->getSampler(VktCache::Sampler::LINEAR);
    gpuResources.metalRoughImage = VktCorePtr->m_whiteImage;
    gpuResources.metalRoughSampler = VktCachePtr->getSampler(VktCache::Sampler::LINEAR);

    std::vector<DescriptorAllocatorDynamic::PoolSizeRatio> sizes = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}
    };

    descriptorPool.initPool(1, sizes);
    materialBuffer = VktBuffers::create(sizeof(VktTypes::GLTFMetallicRoughness::MaterialConstants) * 1,
                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    VktTypes::GLTFMetallicRoughness::MaterialConstants constants;
    constants.colorFactors = {0.156, 0.803, 0.247, 1.0f};
    constants.metalRoughFactors = {1.0f, 0.247f};

    static_cast<VktTypes::GLTFMetallicRoughness::MaterialConstants *>(materialBuffer.info.pMappedData)[0] = constants;

    gpuResources.dataBuffer = materialBuffer.buffer;

    materials[0] = VktCorePtr->writeMaterial(pass, gpuResources, descriptorPool, false);
}

void Terrain::diamondStep(uint32_t rectSize, float currHeight) {
    uint32_t halfRectSize = rectSize / 2;

    std::mt19937 randomGen(randDevice());
    std::uniform_real_distribution<float> distribution(-currHeight, currHeight);

    for(uint32_t x = 0; x < dimX; x += rectSize) {
        for(uint32_t y = 0; y < dimY; y += rectSize) {
            uint32_t nextX = (x + rectSize) % dimX;
            uint32_t nextY = (y + rectSize) % dimY;

            if(nextX < x) { nextX = dimX - 1; }
            if(nextY < y) { nextY = dimY - 1; }

            float topLeft = hMapAt(x, y);
            float topRight = hMapAt(nextX, y);
            float bottomLeft = hMapAt(x, nextY);
            float bottomRight = hMapAt(nextX, nextY);

            uint32_t midX = (x + halfRectSize) % dimX;
            uint32_t midY = (y + halfRectSize) % dimY;

            float randValue = distribution(randomGen);
            float midPoint = (topLeft + topRight + bottomLeft + bottomRight) / 4.0f;

            hMapAt(midX, midY) = midPoint + randValue;
        }
    }
}

void Terrain::squareStep(uint32_t rectSize, float currHeight) {
    uint32_t halfRectSize = rectSize / 2;

    std::mt19937 randomGen(randDevice());
    std::uniform_real_distribution<float> distribution(-currHeight, currHeight);

    for(uint32_t x = 0; x < dimX; x += rectSize) {
        for(uint32_t y = 0; y < dimY; y += rectSize) {
            uint32_t nextX = (x + rectSize) % dimX;
            uint32_t nextY = (y + rectSize) % dimY;

            if(nextX < x) { nextX = dimX - 1; }
            if(nextY < y) { nextY = dimY - 1; }

            uint32_t midX = (x + halfRectSize) % dimX;
            uint32_t midY = (y + halfRectSize) % dimY;

            uint32_t prevMidX = (x - halfRectSize + dimX) % dimX;
            uint32_t prevMidY = (y - halfRectSize + dimY) % dimY;

            float currTopLeft = hMapAt(x, y);
            float currTopRight = hMapAt(nextX, y);
            float currCenter = hMapAt(midX, midY);
            float prevYCenter = hMapAt(midX, prevMidY);
            float currBottomLeft = hMapAt(x, nextY);
            float prevXCenter = hMapAt(prevMidX, midY);

            float currLeftMid = (currTopLeft + currCenter + currBottomLeft + prevXCenter) / 4.0f;
            float currTopMid = (currTopLeft + currCenter + currTopRight + prevYCenter) / 4.0f;

            hMapAt(x, midY) = currLeftMid + distribution(randomGen);
            hMapAt(midX, y) = currTopMid + distribution(randomGen);
        }
    }
}

const glm::vec3 &Terrain::pMapWCoord(int32_t x, int32_t y) { return pMapAt(static_cast<uint32_t>(static_cast<float>(x) / worldScale + static_cast<float>(dimX) / 2), static_cast<uint32_t>(static_cast<float>(y) / worldScale + static_cast<float>(dimY) / 2)); }

float Terrain::hMapWCoord(int32_t x, int32_t y) { return hMapAt(static_cast<uint32_t>(static_cast<float>(x) / worldScale + static_cast<float>(dimX) / 2), static_cast<uint32_t>(static_cast<float>(y) / worldScale + static_cast<float>(dimY) / 2)); }

float Terrain::hMapBaryWCoord(float x, float y) {
    int32_t xF = std::floor(x);
    int32_t yF = std::floor(y);
    uint32_t xLoc = static_cast<uint32_t>(static_cast<float>(xF) / worldScale + static_cast<float>(dimX) / 2);
    uint32_t yLoc = static_cast<uint32_t>(static_cast<float>(yF) / worldScale + static_cast<float>(dimY) / 2);
    glm::vec3 p1;
    glm::vec3 p2;
    glm::vec3 p3;

    float xQuad = x - static_cast<float>(xF);
    float yQuad = y - static_cast<float>(yF);

    if(xLoc % 2 == 0 && yLoc % 2 == 0 && xQuad > yQuad) {
        p1 = pMapWCoord(xF, yF);
        p2 = pMapWCoord(xF + 1, yF + 1);
        p3 = pMapWCoord(xF + 1, yF);
    } else if(xLoc % 2 == 0 && yLoc % 2 == 0 && xQuad <= yQuad) {
        p1 = pMapWCoord(xF, yF);
        p2 = pMapWCoord(xF, yF + 1);
        p3 = pMapWCoord(xF + 1, yF + 1);
    } else if(xLoc % 2 == 1 && yLoc % 2 == 0 && xQuad > -yQuad + 1) {
        p1 = pMapWCoord(xF + 1, yF);
        p2 = pMapWCoord(xF, yF + 1);
        p3 = pMapWCoord(xF + 1, yF + 1);
    } else if(xLoc % 2 == 1 && yLoc % 2 == 0 && xQuad <= -yQuad + 1) {
        p1 = pMapWCoord(xF, yF);
        p2 = pMapWCoord(xF, yF + 1);
        p3 = pMapWCoord(xF + 1, yF);
    } else if(xLoc % 2 == 0 && yLoc % 2 == 1 && xQuad > -yQuad + 1) {
        p1 = pMapWCoord(xF + 1, yF);
        p2 = pMapWCoord(xF, yF + 1);
        p3 = pMapWCoord(xF + 1, yF + 1);
    } else if(xLoc % 2 == 0 && yLoc % 2 == 1 && xQuad <= -yQuad + 1) {
        p1 = pMapWCoord(xF, yF);
        p2 = pMapWCoord(xF, yF + 1);
        p3 = pMapWCoord(xF + 1, yF);
    } else if(xLoc % 2 == 1 && yLoc % 2 == 1 && xQuad > yQuad) {
        p1 = pMapWCoord(xF, yF);
        p2 = pMapWCoord(xF + 1, yF + 1);
        p3 = pMapWCoord(xF + 1, yF);
    } else if(xLoc % 2 == 1 && yLoc % 2 == 1 && xQuad <= yQuad) {
        p1 = pMapWCoord(xF, yF);
        p2 = pMapWCoord(xF, yF + 1);
        p3 = pMapWCoord(xF + 1, yF + 1);
    }

    float h1, h2, h3;
    Utils::barycentric({x, y}, {p1.x, p1.z}, {p2.x, p2.z}, {p3.x, p3.z}, h1, h2, h3);
    return p1.y * h1 + p2.y * h2 + p3.y * h3;
}

void Terrain::generateFlatPlane() {
    LOG(LOG_DEBUG, "Generating flat terrain of size " << dimX << "x" << dimY);

    if((dimX - 1) % (patchSize - 1) != 0) {
        uint32_t nearestSize = ((dimX - 1 + patchSize - 1) / (patchSize - 1) * (patchSize - 1) + 1);
        LOG(LOG_WARNING, "Terrain X dimension size of " << dimX << " on patch size of " << patchSize << " may result in issues. " <<
            "Recommended size is " << nearestSize);

        if(flags[Flags::SET_NEAREST_SIZE]) {
            dimX = nearestSize;
            LOG(LOG_INFO, "Flag SET_NEAREST_SIZE enabled. Setting recommended size as the new size.");
        }
    }

    if((dimY - 1) % (patchSize - 1) != 0) {
        uint32_t nearestSize = ((dimY - 1 + patchSize - 1) / (patchSize - 1) * (patchSize - 1) + 1);
        LOG(LOG_WARNING, "Terrain Y dimension size of " << dimY << " on patch size of " << patchSize << " may result in issues. " <<
            "Recommending size is " << nearestSize);

        if(flags[Flags::SET_NEAREST_SIZE]) {
            dimY = nearestSize;
            LOG(LOG_INFO, "Flag SET_NEAREST_SIZE enabled. Setting recommended size as a new size.");
        }
    }

    patchesX = (dimX - 1) / (patchSize - 1);
    patchesY = (dimY - 1) / (patchSize - 1);

    lodManager.init(maxLOD, patchesX, patchesY, worldScale);

    vertices.resize(dimX * dimY);

    // Generate vertices
    for(int32_t y = 0; y < dimY; y++) {
        for(int32_t x = 0; x < dimX; x++) {
            Vertex_t vertex;
            vertex.position = {(static_cast<float>(x) - static_cast<float>(dimX) / 2) * worldScale,
                               0.0f,
                               (static_cast<float>(y) - static_cast<float>(dimY) / 2) * worldScale};
            vertex.normal = {0.0f, 0.0f, 0.0f};
            vertex.uvX = x;
            vertex.uvY = y;
            vertices.at(xy2i(x, y)) = vertex;
        }
    }

    LOG(LOG_DEBUG, "Generated " << dimX*dimY << " vertices");

    createPatchIndices();
}

float &Terrain::hMapAt(uint32_t x, uint32_t y) { return vertices.at((y * dimX) + x).position.y; }

float &Terrain::hMapAt(uint32_t i) { return vertices.at(i).position.y; }

glm::vec3 &Terrain::pMapAt(uint32_t x, uint32_t y) { return vertices.at((y * dimX) + x).position; }

glm::vec3 &Terrain::pMapAt(uint32_t i) { return vertices.at(i).position; }

std::pair<float, float> Terrain::getMinMaxHeight() { return {minHeight, maxHeight}; }

void Terrain::clear() {
    LOG(LOG_DEBUG, "Clearing terrain resources");

    minHeight = std::numeric_limits<float>::infinity();
    maxHeight = -std::numeric_limits<float>::infinity();

    blendingTexturesCount = 0;

    VktBuffers::destroy(meshBuffers.indexBuffer);
    VktBuffers::destroy(meshBuffers.vertexBuffer);
    VktBuffers::destroy(materialBuffer);
    descriptorPool.destroyPool();
}

void Terrain::addBlendTexture(float height, const VktTypes::Resources::Image &image) {
    if(blendingTexturesCount > blendingTextures.size()) { return; }
    blendingTextures.at(blendingTexturesCount) = {height * worldScale, image};
    LOG(LOG_DEBUG, "Added texture into blending textures at index " << blendingTexturesCount);
    blendingTexturesCount++;
}

void Terrain::calcNormals() {

    assert(indices.size() % 3 == 0);

    for(uint32_t y = 0; y < dimY - 1; y += (patchSize - 1)) {
        for(uint32_t x = 0; x < dimX - 1; x += (patchSize - 1)) {
            uint32_t baseVertex = xy2i(x, y);
            uint32_t numIndices = lodInfo.at(0).info[0][0][0][0].count;
            for(uint32_t i = 0; i < numIndices; i += 3) {
                uint32_t index0 = baseVertex + indices.at(i);
                uint32_t index1 = baseVertex + indices.at(i + 1);
                uint32_t index2 = baseVertex + indices.at(i + 2);

                glm::vec3 v1 = vertices.at(index1).position - vertices.at(index0).position;
                glm::vec3 v2 = vertices.at(index2).position - vertices.at(index0).position;
                glm::vec3 normal = glm::normalize(glm::cross(v1, v2));

                vertices.at(index0).normal += normal;
                vertices.at(index1).normal += normal;
                vertices.at(index2).normal += normal;
            }
        }
    }

    for(auto &vert: vertices) { vert.normal = glm::normalize(vert.normal); }
}

void Terrain::setMaxLOD(uint32_t mLOD) {
    maxLOD = mLOD;
    patchSize = Utils::binPow(static_cast<int32_t>(mLOD + 1)) + 1;

    lodInfo.resize(mLOD + 1);
}

void Terrain::createPatchIndices() {
    for(uint32_t lod = 0; lod <= maxLOD; lod++) {
        for(uint8_t l = 0; l < LEFT; l++) {
            for(uint8_t r = 0; r < RIGHT; r++) {
                for(uint8_t t = 0; t < TOP; t++) {
                    for(uint8_t b = 0; b < BOTTOM; b++) {
                        lodInfo.at(lod).info[l][r][t][b].start = indices.size();
                        createPatchIndicesLOD(lod, lod + l, lod + r, lod + t, lod + b);
                        lodInfo.at(lod).info[l][r][t][b].count = indices.size() - lodInfo.at(lod).info[l][r][t][b].start;

                        LOG(LOG_DEBUG, "Created " << lodInfo.at(lod).info[l][r][t][b].count << " indices for LOD " << lod << " patch variant "
                                        << static_cast<uint32_t>(l) << ' ' << static_cast<uint32_t>(r) << ' ' << static_cast<uint32_t>(t) << ' ' << static_cast<uint32_t>(b));
                    }
                }
            }
        }
    }
}

void Terrain::createPatchIndicesLOD(uint32_t lodCore, uint32_t lodLeft, uint32_t lodRight, uint32_t lodTop, uint32_t lodBottom) {
    uint32_t fanStep = Utils::binPow(lodCore + 1);
    int32_t endPos = patchSize - 1 - fanStep;

    for(uint32_t y = 0; y <= endPos; y += fanStep) {
        for(uint32_t x = 0; x <= endPos; x += fanStep) {
            uint32_t lLeft = x == 0 ? lodLeft : lodCore;
            uint32_t lRight = x == endPos ? lodRight : lodCore;
            uint32_t lBottom = y == 0 ? lodBottom : lodCore;
            uint32_t lTop = y == endPos ? lodTop : lodCore;

            createFan(x, y, lodCore, lLeft, lRight, lTop, lBottom);
        }
    }
}

void Terrain::createFan(uint32_t x, uint32_t y, uint32_t lodCore, uint32_t lodLeft, uint32_t lodRight, uint32_t lodTop, uint32_t lodBottom) {
    uint32_t stepLeft = Utils::binPow(lodLeft);
    uint32_t stepRight = Utils::binPow(lodRight);
    uint32_t stepTop = Utils::binPow(lodTop);
    uint32_t stepBottom = Utils::binPow(lodBottom);
    uint32_t stepCenter = Utils::binPow(lodCore);

    uint32_t iCnt = xy2i(x + stepCenter, y + stepCenter);

    uint32_t indexTemp1 = xy2i(x, y);
    uint32_t indexTemp2 = xy2i(x, y + stepLeft);

    createTriangle(iCnt, indexTemp1, indexTemp2);

    if(lodLeft == lodCore) {
        indexTemp1 = indexTemp2;
        indexTemp2 += stepLeft * dimX;

        createTriangle(iCnt, indexTemp1, indexTemp2);
    }

    indexTemp1 = indexTemp2;
    indexTemp2 += stepTop;

    createTriangle(iCnt, indexTemp1, indexTemp2);

    if(lodTop == lodCore) {
        indexTemp1 = indexTemp2;
        indexTemp2 += stepTop;

        createTriangle(iCnt, indexTemp1, indexTemp2);
    }

    indexTemp1 = indexTemp2;
    indexTemp2 -= stepRight * dimX;

    createTriangle(iCnt, indexTemp1, indexTemp2);

    if(lodRight == lodCore) {
        indexTemp1 = indexTemp2;
        indexTemp2 -= stepRight * dimX;

        createTriangle(iCnt, indexTemp1, indexTemp2);
    }

    indexTemp1 = indexTemp2;
    indexTemp2 -= stepBottom;

    createTriangle(iCnt, indexTemp1, indexTemp2);

    if(lodBottom == lodCore) {
        indexTemp1 = indexTemp2;
        indexTemp2 -= stepBottom;

        createTriangle(iCnt, indexTemp1, indexTemp2);
    }
}

void Terrain::createTriangle(uint32_t i0, uint32_t i1, uint32_t i2) {
    assert(i0 < vertices.size());
    assert(i1 < vertices.size());
    assert(i2 < vertices.size());
    indices.push_back(i0);
    indices.push_back(i1);
    indices.push_back(i2);
    /*
        glm::vec3 pos1 = m_vertices.at(i0).m_position;
        glm::vec3 pos2 = m_vertices.at(i1).m_position;
        glm::vec3 pos3 = m_vertices.at(i2).m_position;

        glm::vec2 tex1 = m_vertices.at(i0).m_texCoord;
        glm::vec2 tex2 = m_vertices.at(i1).m_texCoord;
        glm::vec2 tex3 = m_vertices.at(i2).m_texCoord;

        float x1 = pos2.x - pos1.x;
        float x2 = pos3.x - pos1.x;
        float y1 = pos2.y - pos1.y;
        float y2 = pos3.y - pos1.y;
        float z1 = pos2.z - pos1.z;
        float z2 = pos3.z - pos1.z;

        float s1 = tex2.x - tex1.x;
        float s2 = tex3.x - tex1.x;
        float t1 = tex2.y - tex1.y;
        float t2 = tex3.y - tex1.y;

        float r = 1.0f / (s1 * t2 - s2 * t1);
        glm::vec3 sdir((t2*x1-t1*x2)*r, (t2*y1-t1*y2)*r, (t2*z1-t1*z2)*r);
        glm::vec3 tdir((s1*x2-s2*x1)*r, (s1*y2-s2*y1)*r, (s1*z2-s2*z1)*r);

        m_vertices.at(i0).m_tangent += sdir;
        m_vertices.at(i1).m_tangent += sdir;
        m_vertices.at(i2).m_tangent += sdir;

        m_vertices.at(i0).m_bitangent += tdir;
        m_vertices.at(i1).m_bitangent += tdir;
        m_vertices.at(i2).m_bitangent += tdir;
    */
}

Terrain::renderIterator Terrain::renderIter() const { return Terrain::renderIterator(meshFunc); }

void Terrain::connectCamera(Camera &camera) {
    camera.sig_position.connect(lodManager.slt_cameraPosition);
    camera.sig_VPMatrix.connect(frustumCulling.slt_updateVP);
}

std::pair<uint32_t, uint32_t> Terrain::getCenterCoords() const { return {dimX / 2, dimY / 2}; }

float Terrain::hMapLCoord(uint32_t x, uint32_t y) { return hMapAt(x, y); }

float Terrain::hMapLCoord(std::pair<uint32_t, uint32_t> coords) { return hMapAt(coords.first, coords.second); }

bool Terrain::isPatchInsideFrustum(uint32_t x, uint32_t y) {
    uint32_t x0 = x * (patchSize - 1);
    uint32_t x1 = x0 + patchSize - 1;
    uint32_t y0 = y * (patchSize - 1);
    uint32_t y1 = y0 + patchSize - 1;

    glm::vec3 h00 = pMapAt(x0, y0);
    glm::vec3 h01 = pMapAt(x0, y1);
    glm::vec3 h10 = pMapAt(x1, y0);
    glm::vec3 h11 = pMapAt(x1, y1);

    float minHeight = std::min(h00.y, std::min(h01.y, std::min(h10.y, h11.y)));
    float maxHeight = std::max(h00.y, std::max(h01.y, std::max(h10.y, h11.y)));

    glm::vec3 p00_low(h00.x, minHeight, h00.z);
    glm::vec3 p01_low(h01.x, minHeight, h01.z);
    glm::vec3 p10_low(h10.x, minHeight, h10.z);
    glm::vec3 p11_low(h11.x, minHeight, h11.z);

    glm::vec3 p00_high(h00.x, maxHeight, h00.z);
    glm::vec3 p01_high(h01.x, maxHeight, h01.z);
    glm::vec3 p10_high(h10.x, maxHeight, h10.z);
    glm::vec3 p11_high(h11.x, maxHeight, h11.z);

    return frustumCulling.isPointInside(p00_low) ||
           frustumCulling.isPointInside(p01_low) ||
           frustumCulling.isPointInside(p10_low) ||
           frustumCulling.isPointInside(p11_low) ||
           frustumCulling.isPointInside(p00_high) ||
           frustumCulling.isPointInside(p01_high) ||
           frustumCulling.isPointInside(p10_high) ||
           frustumCulling.isPointInside(p11_high);

    //return m_frustum.IsBoxVisible(p00_low, p11_high);
}