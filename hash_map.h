//
// Created by Аскар Цыганов on 19.01.2023.
//

#ifndef MY_OWN_HASH_TABLE_HASH_MAP_H
#define MY_OWN_HASH_TABLE_HASH_MAP_H

#include <array>
#include <exception>
#include <iterator>
#include <initializer_list>
#include <list>
#include <memory>
#include <queue>
#include <utility>
#include <vector>

template<class KeyType, class ValueType, class Hash = std::hash<KeyType> >
class HashMap {
public:
    class iterator {
    public:
        iterator() = default;

        iterator(std::vector<std::list<std::pair<const KeyType, ValueType>>>& table, size_t index, typename std::list<std::pair<const KeyType, ValueType>>::iterator it);

        iterator& operator++();

        iterator operator++(int);

        std::pair<const KeyType, ValueType>& operator*();

        std::pair<const KeyType, ValueType>* operator->();

        bool operator==(const iterator& other) const;

        bool operator!=(const iterator& other) const;

    private:
        std::vector<std::list<std::pair<const KeyType, ValueType>>>* table_;
        size_t index_;
        typename std::list<std::pair<const KeyType, ValueType>>::iterator it_;

    };

    class const_iterator {
    public:
        const_iterator() = default;

        const_iterator(const std::vector<std::list<std::pair<const KeyType, ValueType>>>& table, size_t index, typename std::list<std::pair<const KeyType, ValueType>>::const_iterator it);

        const_iterator& operator++();

        const_iterator operator++(int);

        const std::pair<const KeyType, ValueType>& operator*();

        const std::pair<const KeyType, ValueType>* operator->();

        bool operator==(const const_iterator& other) const;

        bool operator!=(const const_iterator& other) const;

    private:
        const std::vector<std::list<std::pair<const KeyType, ValueType>>>* table_;
        size_t index_;
        typename std::list<std::pair<const KeyType, ValueType>>::const_iterator it_;

    };

    explicit HashMap(const Hash& hasher = Hash());

    template<class InputIterator>
    HashMap(InputIterator begin, InputIterator end, Hash hasher = Hash());

    HashMap(std::initializer_list<std::pair<KeyType, ValueType>> list, const Hash& hasher = Hash());

    HashMap(const HashMap &other);

    HashMap &operator=(const HashMap &other);

    size_t size() const;

    bool empty() const;

    Hash hash_function() const;

    void insert(std::pair<KeyType, ValueType> element);

    void erase(KeyType key);

    iterator find(KeyType key);

    const_iterator find(KeyType key) const;

    ValueType &operator[](KeyType key);

    const ValueType &at(KeyType key) const;

    iterator begin();

    iterator end();

    const_iterator begin() const;

    const_iterator end() const;

    void clear();

private:
    Hash hasher_;
    size_t size_;
    size_t capacity_;
    std::vector<std::list<std::pair<const KeyType, ValueType>>> table_;
    double load_factor_ = 0.125;  // TODO : change load factor

    void ReHash();

    void InsertElement(std::pair<KeyType, ValueType> element);

    bool IsExist(KeyType key) const;

};

template<class KeyType, class ValueType, class Hash>
template<class InputIterator>
HashMap<KeyType, ValueType, Hash>::HashMap(InputIterator begin, InputIterator end, Hash hasher) : hasher_(hasher),
                                                                                                  size_(0), capacity_(8), table_(8) {
    while (begin != end) {
        insert(*begin);
        begin++;
    }
}

template<class KeyType, class ValueType, class Hash>
HashMap<KeyType, ValueType, Hash>::HashMap(const HashMap &other) : hasher_(other.hasher_),
                                                                   size_(0), capacity_(other.capacity_), table_(other.capacity_)  {
    hasher_ = other.hasher_;
    for (auto& element : other) {
        insert(element);
    }
}

template<class KeyType, class ValueType, class Hash>
HashMap<KeyType, ValueType, Hash>::HashMap(std::initializer_list<std::pair<KeyType, ValueType>> list,
                                           const Hash& hasher) : hasher_(hasher), size_(0), capacity_(8),
                                                                 table_(8) {
    for (auto &element : list) {
        insert(element);
    }
}

template<class KeyType, class ValueType, class Hash>
HashMap<KeyType, ValueType, Hash>::HashMap(const Hash& hasher) : hasher_(hasher),
                                                                 size_(0), capacity_(8), table_(8) {
}

template<class KeyType, class ValueType, class Hash>
HashMap<KeyType, ValueType, Hash>& HashMap<KeyType, ValueType, Hash>::operator=(const HashMap &other) {
    if (this == &other) {
        return *this;
    }
    clear();
    hasher_ = other.hasher_;
    for (auto& element : other) {
        insert(element);
    }
    return *this;
}

template<class KeyType, class ValueType, class Hash>
size_t HashMap<KeyType, ValueType, Hash>::size() const {
    return size_;
}

template<class KeyType, class ValueType, class Hash>
bool HashMap<KeyType, ValueType, Hash>::empty() const {
    return size_ == 0;
}

template<class KeyType, class ValueType, class Hash>
Hash HashMap<KeyType, ValueType, Hash>::hash_function() const {
    return hasher_;
}

template<class KeyType, class ValueType, class Hash>
void HashMap<KeyType, ValueType, Hash>::insert(std::pair<KeyType, ValueType> element) {
    size_t hash = hasher_(element.first) % capacity_;
    if (!IsExist(element.first)) {
        table_[hash].push_back(element);
        size_++;
        if (size_ * load_factor_ > capacity_ * 1.0) {
            ReHash();
        }
    }
}

template<class KeyType, class ValueType, class Hash>
void HashMap<KeyType, ValueType, Hash>::erase(KeyType key) {
    size_t hash = hasher_(key) % capacity_;
    for (auto it = table_[hash].begin(); it != table_[hash].end(); it++) {
        if (it->first == key) {
            table_[hash].erase(it);
            size_--;
            return;
        }
    }
}

template<class KeyType, class ValueType, class Hash>
typename HashMap<KeyType, ValueType, Hash>::iterator HashMap<KeyType, ValueType, Hash>::find(KeyType key) {
    size_t hash = hasher_(key) % capacity_;
    for (auto it = table_[hash].begin(); it != table_[hash].end(); it++) {
        if (it->first == key) {
            return iterator(table_, hash, it);
        }
    }
    return end();
}

template<class KeyType, class ValueType, class Hash>
typename HashMap<KeyType, ValueType, Hash>::const_iterator HashMap<KeyType, ValueType, Hash>::find(KeyType key) const {
    size_t hash = hasher_(key) % capacity_;
    for (auto it = table_[hash].begin(); it != table_[hash].end(); it++) {
        if (it->first == key) {
            return const_iterator(table_, hash, it);
        }
    }
    return end();
}

template<class KeyType, class ValueType, class Hash>
ValueType &HashMap<KeyType, ValueType, Hash>::operator[](KeyType key) {
    insert({key, ValueType()});
    return find(key)->second;
}

template<class KeyType, class ValueType, class Hash>
const ValueType &HashMap<KeyType, ValueType, Hash>::at(KeyType key) const {
    if (IsExist(key)) {
        return find(key)->second;
    }
    throw std::out_of_range("Key not found");
}

template<class KeyType, class ValueType, class Hash>
typename HashMap<KeyType, ValueType, Hash>::iterator HashMap<KeyType, ValueType, Hash>::begin() {
    for (size_t i = 0; i < capacity_; i++) {
        if (!table_[i].empty()) {
            return iterator(table_, i, table_[i].begin());
        }
    }
    return iterator(table_, capacity_, table_[0].begin());
}

template<class KeyType, class ValueType, class Hash>
typename HashMap<KeyType, ValueType, Hash>::iterator HashMap<KeyType, ValueType, Hash>::end() {
    return iterator(table_, capacity_, table_[0].begin());
}

template<class KeyType, class ValueType, class Hash>
typename HashMap<KeyType, ValueType, Hash>::const_iterator HashMap<KeyType, ValueType, Hash>::begin() const {
    for (size_t i = 0; i < capacity_; i++) {
        if (!table_[i].empty()) {
            return const_iterator(table_, i, table_[i].begin());
        }
    }
    return const_iterator(table_, capacity_, table_[0].begin());
}

template<class KeyType, class ValueType, class Hash>
typename HashMap<KeyType, ValueType, Hash>::const_iterator HashMap<KeyType, ValueType, Hash>::end() const {
    return const_iterator(table_, capacity_, table_[0].begin());
}

template<class KeyType, class ValueType, class Hash>
void HashMap<KeyType, ValueType, Hash>::clear() {
    size_ = 0;
    capacity_ = 8;
    table_.clear();
    table_.resize(8);
}

template<class KeyType, class ValueType, class Hash>
void HashMap<KeyType, ValueType, Hash>::ReHash() {
    auto old_table = table_;
    capacity_ *= 2;
    table_.clear();
    table_.resize(capacity_);
    for (auto &list : old_table) {
        for (auto &element : list) {
            InsertElement(element);
        }
    }
}

template<class KeyType, class ValueType, class Hash>
bool HashMap<KeyType, ValueType, Hash>::IsExist(KeyType key) const {
    size_t hash = hasher_(key) % capacity_;
    for (auto &element : table_[hash]) {
        if (element.first == key) {
            return true;
        }
    }
    return false;
}

template<class KeyType, class ValueType, class Hash>
void HashMap<KeyType, ValueType, Hash>::InsertElement(std::pair<KeyType, ValueType> element) {
    size_t hash = hasher_(element.first) % capacity_;
    table_[hash].push_back(element);
}

template<class KeyType, class ValueType, class Hash>
HashMap<KeyType, ValueType, Hash>::iterator::iterator(
        std::vector<std::list<std::pair<const KeyType, ValueType>>> &table, size_t index,
        typename std::list<std::pair<const KeyType, ValueType>>::iterator it) : table_(&table), index_(index), it_(it) {}

template<class KeyType, class ValueType, class Hash>
typename HashMap<KeyType, ValueType, Hash>::iterator& HashMap<KeyType, ValueType, Hash>::iterator::operator++() {
    if (it_ != --table_->at(index_).end()) {
        it_++;
        return *this;
    }
    for (size_t i = index_ + 1; i < table_->size(); i++) {
        if (!table_->at(i).empty()) {
            index_ = i;
            it_ = table_->at(i).begin();
            return *this;
        }
    }
    index_ = table_->size();
    it_ = table_->at(0).begin();
    return *this;
}

template<class KeyType, class ValueType, class Hash>
typename HashMap<KeyType, ValueType, Hash>::iterator HashMap<KeyType, ValueType, Hash>::iterator::operator++(int) {
    iterator tmp = *this;
    ++(*this);
    return tmp;
}

template<class KeyType, class ValueType, class Hash>
std::pair<const KeyType, ValueType>& HashMap<KeyType, ValueType, Hash>::iterator::operator*() {
    return *it_;
}

template<class KeyType, class ValueType, class Hash>
std::pair<const KeyType, ValueType>* HashMap<KeyType, ValueType, Hash>::iterator::operator->() {
    return it_.operator->();
}

template<class KeyType, class ValueType, class Hash>
bool HashMap<KeyType, ValueType, Hash>::iterator::operator==(const iterator& other) const {
    return table_ == other.table_ && index_ == other.index_ && it_ == other.it_;
}

template<class KeyType, class ValueType, class Hash>
bool HashMap<KeyType, ValueType, Hash>::iterator::operator!=(const iterator& other) const {
    return !(*this == other);
}

template<class KeyType, class ValueType, class Hash>
HashMap<KeyType, ValueType, Hash>::const_iterator::const_iterator(
        const std::vector<std::list<std::pair<const KeyType, ValueType>>>& table, size_t index,
        typename std::list<std::pair<const KeyType, ValueType>>::const_iterator it) : table_(&table), index_(index), it_(it) {}

template<class KeyType, class ValueType, class Hash>
typename HashMap<KeyType, ValueType, Hash>::const_iterator& HashMap<KeyType, ValueType, Hash>::const_iterator::operator++() {
    if (it_ != --table_->at(index_).end()) {
        it_++;
        return *this;
    }
    for (size_t i = index_ + 1; i < table_->size(); i++) {
        if (!table_->at(i).empty()) {
            index_ = i;
            it_ = table_->at(i).begin();
            return *this;
        }
    }
    index_ = table_->size();
    it_ = table_->at(0).begin();
    return *this;
}

template<class KeyType, class ValueType, class Hash>
typename HashMap<KeyType, ValueType, Hash>::const_iterator HashMap<KeyType, ValueType, Hash>::const_iterator::operator++(int) {
    const_iterator tmp = *this;
    ++(*this);
    return tmp;
}

template<class KeyType, class ValueType, class Hash>
const std::pair<const KeyType, ValueType>& HashMap<KeyType, ValueType, Hash>::const_iterator::operator*() {
    return *it_;
}

template<class KeyType, class ValueType, class Hash>
const std::pair<const KeyType, ValueType>* HashMap<KeyType, ValueType, Hash>::const_iterator::operator->() {
    return it_.operator->();
}

template<class KeyType, class ValueType, class Hash>
bool HashMap<KeyType, ValueType, Hash>::const_iterator::operator==(const const_iterator& other) const {
    return table_ == other.table_ && index_ == other.index_ && it_ == other.it_;
}

template<class KeyType, class ValueType, class Hash>
bool HashMap<KeyType, ValueType, Hash>::const_iterator::operator!=(const const_iterator& other) const {
    return !(*this == other);
}

#endif //MY_OWN_HASH_TABLE_HASH_MAP_H
