#ifndef TECTONIC_STACKEDINDEX_H
#define TECTONIC_STACKEDINDEX_H

#include <vector>
#include <unordered_map>
#include <cstdint>

template<typename InputIndexT, typename OutputIndexT>
class StackedIndex {
public:
    StackedIndex() = default;
    ~StackedIndex() = default;

    OutputIndexT operator()(InputIndexT inputIndex){
        return m_outputStack.at(m_indexMap.at(inputIndex));
    }

private:
    using indexStack_t = std::vector<OutputIndexT>;
    using indexMap_t = std::unordered_map<InputIndexT, typename indexStack_t::const_iterator>;

    indexStack_t m_outputStack;
    indexMap_t m_indexMap;
};


#endif //TECTONIC_STACKEDINDEX_H
