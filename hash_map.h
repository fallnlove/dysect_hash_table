//
// Created by Аскар Цыганов on 11.01.2023.
//

#ifndef MY_OWN_HASH_TABLE_HASH_MAP_H
#define MY_OWN_HASH_TABLE_HASH_MAP_H

#include <list>
#include <iterator>
#include <utility>

template<class KeyType, class ValueType, class Hash = std::hash<KeyType> >
class HashMap {
public:
    class iterator {
    };

    class const_iterator {
    };

    HashMap() {}

    HashMap(std::iterator<std::forward_iterator_tag, std::pair<KeyType, ValueType>> begin,
            std::iterator<std::forward_iterator_tag, std::pair<KeyType, ValueType>> end,
            Hash hasher = Hash()) {}

    HashMap(std::initializer_list<std::pair<KeyType, ValueType>> list, Hash hasher = Hash()) {}

    size_t size() const {}

    bool empty() const {}

    Hash hash_function() const {}

    void insert(std::pair<KeyType, ValueType> element) {}

    void erase(KeyType key) {}

    const_iterator find(KeyType key) const {}

    iterator find(KeyType key) {}

    ValueType &operator[](KeyType key) {}

    const ValueType &at(KeyType key) const {}

    iterator begin() {}

    iterator end() {}

    const_iterator begin() const {}

    const_iterator end() const {}

    void clear() {}

private:
    Hash hasher_;

};

#endif //MY_OWN_HASH_TABLE_HASH_MAP_H
