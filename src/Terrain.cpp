#include <glm/gtx/string_cast.hpp>
#include "model/terrain/Terrain.h"

Logger Terrain::m_logger = Logger("Terrain");

void Terrain::normalize() {
    if(m_maxHeight <= m_minHeight){
        return;
    }

    float minMaxDelta = m_maxHeight - m_minHeight;
    float minMaxRange = m_maxRange - m_minRange;

    for(auto& vert : m_vertices){
        vert.m_position.y = ((vert.m_position.y - m_minHeight)/minMaxDelta) * minMaxRange + m_minRange;
    }

    m_logger(Logger::DEBUG) << "Normalized height values into range between " << m_minRange << " and " << m_maxRange << '\n';
    calcMinMax();
}

void Terrain::calcMinMax() {
    m_minHeight = std::numeric_limits<float>::infinity();
    m_maxHeight = -std::numeric_limits<float>::infinity();

    for(const auto& vert : m_vertices){
        m_minHeight = fminf(m_minHeight,vert.m_position.y);
        m_maxHeight = fmaxf(m_maxHeight,vert.m_position.y);
    }

    m_logger(Logger::DEBUG) << "Calculated min max height values: " << m_minHeight << " " << m_maxHeight << '\n';
}

void Terrain::generateFlat(uint32_t dimX, uint32_t dimZ, const char* textureFile, const char* normalFile) {
    m_dimX = dimX;
    m_dimY = dimZ;

    m_logger(Logger::INFO) << "Generating flat terrain of size " << m_dimX << "x" << m_dimY << '\n';

    generateFlatPlane();

    calcMinMax();
    normalize();
    calcNormals();

    m_materials.resize(1);

    addBlendTexture(0.0f,Texture::createTexture(GL_TEXTURE_2D, textureFile));

    m_materials.at(0).m_diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
    m_materials.at(0).m_ambientColor = glm::vec3(1.0f, 1.0f, 1.0f);

    bufferMeshes();
}

void Terrain::loadHeightmap(const char *heightmapFile, const char *textureFile) {
    int32_t width, height, channels;
    u_char *hMapData = stbi_load(heightmapFile, &width, &height, &channels, 0);

    m_dimX = width;
    m_dimY = height;

    m_logger(Logger::INFO) << "Loading heightmap terrain of size " << m_dimX << "x" << m_dimY << '\n';

    generateFlatPlane();

    float yScale = 64.0f/256.0f;

    for(uint32_t i = 0; i < m_vertices.size(); i++){
        u_char *texel = hMapData + (i) * channels;

        float y = *texel;

        hMapAt(i) = y * yScale;
        m_minHeight = fminf(m_minHeight,hMapAt(i));
        m_maxHeight = fmaxf(m_maxHeight,hMapAt(i));
    }

    calcMinMax();
    normalize();
    calcNormals();

    m_materials.resize(1);

    m_materials.at(0).m_diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
    m_materials.at(0).m_ambientColor = glm::vec3(1.0f, 1.0f, 1.0f);
    m_materials.at(0).m_diffuseTexture = std::make_shared<Texture>(GL_TEXTURE_2D, textureFile);

    bufferMeshes();
}

void Terrain::generateMidpoint(uint32_t size, float roughness, const std::vector<std::string>& textureFiles) {
    clear();

    m_dimX = size;
    m_dimY = size;

    m_logger(Logger::INFO) << "Generating midpoint terrain of size " << size << '\n';

    generateFlatPlane();

    uint32_t rectSize = Utils::nextPowerOf(size,2);
    float currHeight = static_cast<float>(rectSize)/2.0f;
    float heightReduce = powf(2.0f, -roughness);

    while(rectSize > 0){
        diamondStep(rectSize, currHeight);
        squareStep(rectSize, currHeight);

        rectSize /= 2;
        currHeight *= heightReduce;
    }

    calcMinMax();
    normalize();
    calcNormals();

    std::vector<std::vector<float>> heights;
    heights.resize(m_patchesY);
    for(uint32_t patchY = 0; patchY < m_patchesY; patchY++){
        heights.at(patchY).resize(m_patchesX);
        for(uint32_t patchX = 0; patchX < m_patchesX; patchX++){
            heights.at(patchY).at(patchX) = hMapAt(patchX*(m_patchSize-1) + (m_patchSize-1)/2 , patchY*(m_patchSize-1) + (m_patchSize-1)/2);
        }
    }

    m_lodManager.loadHeightsPerPatch(heights);


    m_materials.resize(1);

    m_materials.at(0).m_diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
    m_materials.at(0).m_ambientColor = glm::vec3(1.0f, 1.0f, 1.0f);

    assert(textureFiles.size() <= MAX_TERRAIN_HEIGHT_TEXTURE);

    for(uint32_t i = 0; i < textureFiles.size(); i++){
        addBlendTexture(0.0f,Texture::createTexture(GL_TEXTURE_2D, textureFiles.at(i)));
        m_blendingTextures.at(i).first = abs(m_minRange-m_maxRange)/(textureFiles.size()+1) * static_cast<float>(i+1);
    }

    bufferMeshes();
}

void Terrain::diamondStep(uint32_t rectSize, float currHeight) {
    uint32_t halfRectSize = rectSize / 2;

    std::mt19937 randomGen(m_randDevice());
    std::uniform_real_distribution<float> distribution(-currHeight, currHeight);

    for(uint32_t x = 0; x < m_dimX; x += rectSize){
        for(uint32_t y = 0; y < m_dimY; y += rectSize){
            uint32_t nextX = (x + rectSize) % m_dimX;
            uint32_t nextY = (y + rectSize) % m_dimY;

            if(nextX < x){
                nextX = m_dimX - 1;
            }
            if(nextY < y){
                nextY = m_dimY - 1;
            }

            float topLeft = hMapAt(x, y);
            float topRight = hMapAt(nextX, y);
            float bottomLeft = hMapAt(x, nextY);
            float bottomRight = hMapAt(nextX, nextY);

            uint32_t midX = (x + halfRectSize) % m_dimX;
            uint32_t midY = (y + halfRectSize) % m_dimY;

            float randValue = distribution(randomGen);
            float midPoint = (topLeft + topRight + bottomLeft + bottomRight) / 4.0f;

            hMapAt(midX, midY) = midPoint + randValue;
        }
    }
}

void Terrain::squareStep(uint32_t rectSize, float currHeight) {
    uint32_t halfRectSize = rectSize / 2;

    std::mt19937 randomGen(m_randDevice());
    std::uniform_real_distribution<float> distribution(-currHeight, currHeight);

    for(uint32_t x = 0; x < m_dimX; x += rectSize){
        for(uint32_t y = 0; y < m_dimY; y += rectSize){

            uint32_t nextX = (x + rectSize) % m_dimX;
            uint32_t nextY = (y + rectSize) % m_dimY;

            if(nextX < x){
                nextX = m_dimX - 1;
            }
            if(nextY < y){
                nextY = m_dimY - 1;
            }

            uint32_t midX = (x + halfRectSize) % m_dimX;
            uint32_t midY = (y + halfRectSize) % m_dimY;

            uint32_t prevMidX = (x - halfRectSize + m_dimX) % m_dimX;
            uint32_t prevMidY = (y - halfRectSize + m_dimY) % m_dimY;

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

void Terrain::generateFlatPlane() {
    m_logger(Logger::DEBUG) << "Generating flat terrain of size " << m_dimX << "x" << m_dimY << '\n';

    if((m_dimX-1) % (m_patchSize-1) != 0){
        m_logger(Logger::WARNING) <<
                                  "Terrain X dimension size of " << m_dimX << " on patch size of " << m_patchSize << " may result in issues. " <<
                                  "Recommending size is " << ((m_dimX-1+m_patchSize-1)/(m_patchSize-1)*(m_patchSize-1)+1) << '\n';
    }

    if((m_dimY-1) % (m_patchSize-1) != 0){
        m_logger(Logger::WARNING) <<
                                  "Terrain Y dimension size of " << m_dimY << " on patch size of " << m_patchSize << " may result in issues. " <<
                                  "Recommending size is " << ((m_dimY-1+m_patchSize-1)/(m_patchSize-1)*(m_patchSize-1)+1) << '\n';
    }

    m_patchesX = (m_dimX-1) / (m_patchSize-1);
    m_patchesY = (m_dimY-1) / (m_patchSize-1);

    m_lodManager.init(m_maxLOD, m_patchesX, m_patchesY, m_worldScale);

    m_vertices.resize(m_dimX * m_dimY);

    // Generate vertices
    for(int32_t y = 0; y < m_dimY; y++){
        for(int32_t x = 0; x < m_dimX; x++){
            Vertex vertex;
            vertex.m_position = {(static_cast<float>(x) - static_cast<float>(m_dimX)/2) * m_worldScale,
                                 0.0f,
                                 (static_cast<float>(y) - static_cast<float>(m_dimY)/2) * m_worldScale};
            vertex.m_normal = {0.0f, 0.0f, 0.0f};
            vertex.m_texCoord = {x, y};
            m_vertices.at(xy2i(x,y)) = vertex;
        }
    }

    m_logger(Logger::DEBUG) << "Generated " << m_dimX*m_dimY << " vertices" << '\n';

    createPatchIndices();
}


float& Terrain::hMapAt(uint32_t x, uint32_t y){
    return m_vertices.at((y * m_dimX) + x).m_position.y;
}

float& Terrain::hMapAt(uint32_t i){
    return m_vertices.at(i).m_position.y;
}

std::pair<float, float> Terrain::getMinMaxHeight() {
    return {m_minHeight, m_maxHeight};
}

void Terrain::clear() {
    m_logger(Logger::DEBUG) << "Clearing terrain resources" << '\n';
    Model::clear();

    m_minHeight = std::numeric_limits<float>::infinity();
    m_maxHeight = -std::numeric_limits<float>::infinity();

    m_blendingTexturesCount = 0;
}

void Terrain::addBlendTexture(float height, const std::shared_ptr<Texture>& texture) {
    if(m_blendingTexturesCount > m_blendingTextures.size()){
        return;
    }
    m_blendingTextures.at(m_blendingTexturesCount) = {height * m_worldScale, texture};
    m_logger(Logger::DEBUG) << "Added texture [" << texture->name() << "] into blending textures at index " << m_blendingTexturesCount << '\n';
    m_blendingTexturesCount++;
}

void Terrain::bindBlendingTextures() {
    for(uint32_t i = 0; i < m_blendingTexturesCount; i++){
        m_blendingTextures.at(i).second->bind(COLOR_TEXTURE_UNIT + i);
    }
}

void Terrain::calcNormals() {

    assert(m_indices.size() % 3 == 0);

    for(uint32_t y = 0; y < m_dimY-1; y += (m_patchSize - 1)){
        for(uint32_t x = 0; x < m_dimX-1; x += (m_patchSize - 1)){
            uint32_t baseVertex = xy2i(x,y);
            uint32_t numIndices = m_lodInfo.at(0).info[0][0][0][0].count;
            for(uint32_t i = 0; i < numIndices; i += 3){
                uint32_t index0 = baseVertex + m_indices.at(i);
                uint32_t index1 = baseVertex + m_indices.at(i+1);
                uint32_t index2 = baseVertex + m_indices.at(i+2);

                glm::vec3 v1 = m_vertices.at(index1).m_position - m_vertices.at(index0).m_position;
                glm::vec3 v2 = m_vertices.at(index2).m_position - m_vertices.at(index0).m_position;
                glm::vec3 normal = glm::normalize(glm::cross(v1, v2));

                m_vertices.at(index0).m_normal += normal;
                m_vertices.at(index1).m_normal += normal;
                m_vertices.at(index2).m_normal += normal;
            }
        }
    }

    for(auto & vert : m_vertices){
        vert.m_normal = glm::normalize(vert.m_normal);
    }
}

void Terrain::setMaxLOD(uint32_t maxLOD) {
    m_maxLOD = maxLOD;
    m_patchSize = Utils::binPow(static_cast<int32_t>(maxLOD+1)) + 1;

    m_lodInfo.resize(m_maxLOD+1);
}

void Terrain::setMaxRange(float maxRange) {
    m_maxRange = maxRange;
}

void Terrain::setMinRange(float minRange) {
    m_minRange = minRange;
}

void Terrain::createPatchIndices() {
    for(uint32_t lod = 0; lod <= m_maxLOD; lod++){
        for(uint8_t l = 0; l < LEFT; l++){
            for(uint8_t r = 0; r < RIGHT; r++){
                for(uint8_t t = 0; t < TOP; t++){
                    for(uint8_t b = 0; b < BOTTOM; b++){
                        m_lodInfo.at(lod).info[l][r][t][b].start = m_indices.size();
                        createPatchIndicesLOD(lod, lod + l, lod + r, lod + t, lod + b);
                        m_lodInfo.at(lod).info[l][r][t][b].count = m_indices.size() - m_lodInfo.at(lod).info[l][r][t][b].start;

                        m_logger(Logger::DEBUG) << "Created " << m_lodInfo.at(lod).info[l][r][t][b].count << " indices for LOD " << lod << " patch variant " << l << r << t << b << '\n';
                    }
                }
            }
        }    }
}

void Terrain::createPatchIndicesLOD(uint32_t lodCore, uint32_t lodLeft, uint32_t lodRight, uint32_t lodTop, uint32_t lodBottom) {
    uint32_t fanStep = Utils::binPow(lodCore+1);
    int32_t endPos = m_patchSize - 1 - fanStep;

    for(uint32_t y = 0; y <= endPos; y += fanStep){
        for(uint32_t x = 0; x <= endPos; x += fanStep){
            uint32_t lLeft      = x == 0        ? lodLeft : lodCore;
            uint32_t lRight     = x == endPos   ? lodRight : lodCore;
            uint32_t lBottom    = y == 0        ? lodBottom : lodCore;
            uint32_t lTop       = y == endPos   ? lodTop : lodCore;

            createFan(x,y, lodCore, lLeft, lRight, lTop, lBottom);
        }
    }
}

void Terrain::createFan(uint32_t x, uint32_t y, uint32_t lodCore, uint32_t lodLeft, uint32_t lodRight, uint32_t lodTop, uint32_t lodBottom) {
    uint32_t stepLeft   = Utils::binPow(lodLeft);
    uint32_t stepRight  = Utils::binPow(lodRight);
    uint32_t stepTop    = Utils::binPow(lodTop);
    uint32_t stepBottom = Utils::binPow(lodBottom);
    uint32_t stepCenter = Utils::binPow(lodCore);

    uint32_t iCnt = xy2i(x+stepCenter, y+stepCenter);

    uint32_t indexTemp1 = xy2i(x, y);
    uint32_t indexTemp2 = xy2i(x, y+stepLeft);

    createTriangle(iCnt, indexTemp1, indexTemp2);

    if(lodLeft == lodCore){
        indexTemp1 = indexTemp2;
        indexTemp2 += stepLeft * m_dimX;

        createTriangle(iCnt, indexTemp1, indexTemp2);
    }

    indexTemp1 = indexTemp2;
    indexTemp2 += stepTop;

    createTriangle(iCnt, indexTemp1, indexTemp2);

    if(lodTop == lodCore){
        indexTemp1 = indexTemp2;
        indexTemp2 += stepTop;

        createTriangle(iCnt, indexTemp1, indexTemp2);
    }

    indexTemp1 = indexTemp2;
    indexTemp2 -= stepRight * m_dimX;

    createTriangle(iCnt, indexTemp1, indexTemp2);

    if(lodRight == lodCore){
        indexTemp1 = indexTemp2;
        indexTemp2 -= stepRight * m_dimX;

        createTriangle(iCnt, indexTemp1, indexTemp2);
    }

    indexTemp1 = indexTemp2;
    indexTemp2 -= stepBottom;

    createTriangle(iCnt, indexTemp1, indexTemp2);

    if(lodBottom == lodCore){
        indexTemp1 = indexTemp2;
        indexTemp2 -= stepBottom;

        createTriangle(iCnt, indexTemp1, indexTemp2);
    }
}

void Terrain::createTriangle(uint32_t i0, uint32_t i1, uint32_t i2) {
    assert(i0 < m_vertices.size());
    assert(i1 < m_vertices.size());
    assert(i2 < m_vertices.size());
    m_indices.push_back(i0);
    m_indices.push_back(i1);
    m_indices.push_back(i2);
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

Terrain::meshIterator Terrain::meshIter() {
    std::function<MeshInfo(uint32_t&, uint32_t&)> func = {[this](uint32_t& patchX, uint32_t& patchY){
        patchX = (patchX+1);
        if(patchX == m_patchesX){
            patchY++;
            patchX = 0;
        }
        if(patchY == m_patchesY){
            patchX = 0;
            patchY = 0;
            return MeshInfo();
        }

        const LODManager::patchLOD& pLOD = m_lodManager.getPatchLOD(patchX, patchY);
        uint32_t C = pLOD.core;
        uint32_t L = pLOD.left;
        uint32_t R = pLOD.right;
        uint32_t T = pLOD.top;
        uint32_t B = pLOD.bottom;

        uint32_t baseIndex = m_lodInfo.at(C).info[L][R][T][B].start;

        uint32_t y = patchY * (m_patchSize-1);
        uint32_t x = patchX * (m_patchSize-1);
        uint32_t baseVertex = y * m_dimX + x;

        MeshInfo meshInfo;
        meshInfo.indicesCount = m_lodInfo.at(C).info[L][R][T][B].count;
        meshInfo.verticesOffset = baseVertex;
        meshInfo.indicesOffset = baseIndex;
        meshInfo.matIndex = 0;

        return meshInfo;
    }};

    return Terrain::meshIterator(func);
}

void Terrain::setCamera(Camera &camera) {
    camera.sig_position.connect(m_lodManager.slt_cameraPosition);
}

void Terrain::setScale(float scale) {
    m_worldScale = scale;
}

float Terrain::getScale() {
    return m_worldScale;
}

std::pair<uint32_t, uint32_t> Terrain::getCenterCoords() const {
    return {m_dimX / 2, m_dimY / 2};
}

float Terrain::getHeight(uint32_t x, uint32_t y) {
    return hMapAt(x,y);
}

float Terrain::getHeight(std::pair<uint32_t, uint32_t> coords) {
    return hMapAt(coords.first,coords.second);
}
