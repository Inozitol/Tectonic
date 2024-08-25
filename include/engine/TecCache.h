#pragma once

class TecCache {
public:
    TecCache(TecCache const &) = delete;
    void operator=(TecCache const &) = delete;

    inline static float deltaTime = 0.0;

private:
    static TecCache& getInstance();
    TecCache() = default;
};



