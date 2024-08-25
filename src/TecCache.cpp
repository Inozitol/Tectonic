#include "../include/engine/TecCache.h"

TecCache &TecCache::getInstance() {
    static TecCache instance;
    return instance;
}