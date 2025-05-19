#pragma once

#include <vector>

template<typename Type, typename Index = uint32_t> requires std::is_unsigned_v<Index> && std::is_integral_v<Index>
struct IndexableStorage {

    using storeF_t = std::function<void(Type& d, Index i)>;
    storeF_t storeFunc;

    [[nodiscard]]
    Index storeData(const Type &d, bool signal = true) {
        Index dataIndex = nextIndex;
        if(nextIndex < storage.size()) {
            storage[nextIndex++] = Data{.info = DataInfo::STORED, .data = d};
            // Increment nextIndex until we find a free spot or reach the end of storage
            while(storage.at(nextIndex).info & DataInfo::STORED && storage.size() > nextIndex) ++nextIndex;
            return dataIndex;
        }
        storage.emplace_back(Data{.info = DataInfo::STORED, .data = d});
        ++nextIndex;
        if(signal) sig_stored.emit(dataIndex);
        if(storeFunc) storeFunc(storage.at(dataIndex).data, dataIndex);
        return dataIndex;
    }

    [[nodiscard]]
    Index storeData(Type &&d, bool signal = true) {
        Index dataIndex = nextIndex;
        if(nextIndex < storage.size()) {
            storage[nextIndex++] = Data{.info = DataInfo::STORED, .data = d};
            // Increment nextIndex until we find a free spot or reach the end of storage
            while(!(storage.at(nextIndex).info & DataInfo::STORED) && storage.size() > nextIndex) ++nextIndex;
            return dataIndex;
        }
        storage.emplace_back(Data{.info = DataInfo::STORED, .data = d});
        ++nextIndex;
        if(signal) sig_stored.emit(dataIndex);
        if(storeFunc) storeFunc(storage.at(dataIndex).data, dataIndex);
        return dataIndex;
    }

    void storeDataAt(const Type &d, Index i, bool signal = true) {
        if(storage.size() <= i) { storage.resize(i + 1); }
        storage.at(i) = Data{.info = DataInfo::STORED, .data = d};
        if(nextIndex == i) nextIndex = i + 1;
        if(signal) sig_stored.emit(i);
        if(storeFunc) storeFunc(storage.at(i).data, i);
    }

    void storeDataAt(Type &&d, Index i, bool signal = true) {
        if(storage.size() <= i) { storage.resize(i + 1); }
        storage.at(i) = Data{.info = DataInfo::STORED, .data = d};
        if(nextIndex == i) nextIndex = i + 1;
        if(signal) sig_stored.emit(i);
        if(storeFunc) storeFunc(storage.at(i).data, i);
    }

    Type &data(Index i) { return storage.at(i).data; }

    void erase(Index i) {
        storage.at(i).info = DataInfo::NONE;
        if(nextIndex > i) nextIndex = i;
        sig_erased.emit(i);
    }

    enum struct DataInfo : uint8_t {
        NONE = 0,
        STORED = 1 << 0
    };

    INIT_ENUM_FR_OP(DataInfo);

    struct Data {
        DataInfo info = DataInfo::NONE;
        Type data;
    };

    struct iter{
        using iterator_category = std::input_iterator_tag;
        using value_type = std::tuple<Index, Type>;
        using reference = std::tuple<Index, Type&>;

        iter(Index index, std::vector<Data> &storage) : index(index), storageRef(storage) {}

        reference operator*() const { return {index, storageRef.at(index).data}; }

        iter &operator++() {
            ++index;
            if(index >= storageRef.size()) return *this;
            while(!(storageRef.at(index).info & DataInfo::STORED)) ++index;
            return *this;
        }

        iter &operator+=(std::size_t diff){index += diff; return *this;}
        iter &operator-=(std::size_t diff){index -= diff; return *this;}

        iter &operator+(std::size_t diff){index += diff; return *this;}
        iter &operator-(std::size_t diff){index -= diff; return *this;}

        friend bool operator==(iter const &lhs, iter const &rhs) { return lhs.index == rhs.index; }

        friend bool operator!=(iter const &lhs, iter const &rhs) { return lhs.index != rhs.index; }

    private:
        Index index;
        std::vector<Data> &storageRef;
    };

    iter begin() { return iter(indexMin, storage); }
    iter end() { return iter(storage.size(), storage); }
    iter at(Index i) { return iter(i, storage); }

    Index before(Index i) {
        if(i == indexMin) return unknown;
        do { --i; } while(!(storage.at(i).info & DataInfo::STORED) && i > indexMin);
        if(i == indexMax) return unknown;
        return i;
    }

    Index after(Index i) {
        if(i == indexMax || i >= storage.size()) return unknown;
        do { ++i; } while(i < storage.size() && !(storage.at(i).info & DataInfo::STORED) && i < indexMax);
        if(i == indexMax || i >= storage.size()) return unknown;
        return i;
    }

    static constexpr Index indexMin =  std::numeric_limits<Index>::min();
    static constexpr Index indexMax = std::numeric_limits<Index>::max();
    static constexpr Index unknown = indexMax;

    Signal<Index> sig_stored;
    Signal<Index> sig_erased;

private:
    Index nextIndex = indexMin;
    std::vector<Data> storage{};
};