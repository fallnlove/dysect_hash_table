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
    class iterator;

    class const_iterator;

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
    struct Bucket_ {
    public:
        std::pair<const KeyType, ValueType> value_;
        bool is_deleted_ = true;
        size_t psl_ = 0;
    };

    Hash hasher_;
    size_t size_;
    size_t capacity_;
    std::vector<std::shared_ptr<Bucket_>> table_;
    double load_factor_ = 0.8;  // TODO : change load factor

    void ReHash();

    void InsertElement(std::pair<KeyType, ValueType> element);

    bool IsExist(KeyType key) const;

    void InitializedElements();

    size_t NextPos(size_t position) const;

    size_t PrevPos(size_t position) const;

public:
    class iterator {
    public:
        iterator() = default;

        iterator(typename std::vector<std::shared_ptr<Bucket_>>::iterator, typename std::vector<std::shared_ptr<Bucket_>>::iterator end);

        iterator& operator++();

        iterator operator++(int);

        std::pair<const KeyType, ValueType>& operator*();

        std::pair<const KeyType, ValueType>* operator->();

        bool operator==(const iterator& other) const;

        bool operator!=(const iterator& other) const;

    private:
        typename std::vector<std::shared_ptr<Bucket_>>::iterator it_;
        typename std::vector<std::shared_ptr<Bucket_>>::iterator end_;

    };

    class const_iterator {
    public:
        const_iterator() = default;

        const_iterator(typename std::vector<std::shared_ptr<Bucket_>>::const_iterator it, typename std::vector<std::shared_ptr<Bucket_>>::const_iterator end);

        const_iterator& operator++();

        const_iterator operator++(int);

        const std::pair<const KeyType, ValueType>& operator*();

        const std::pair<const KeyType, ValueType>* operator->();

        bool operator==(const const_iterator& other) const;

        bool operator!=(const const_iterator& other) const;

    private:
        typename std::vector<std::shared_ptr<Bucket_>>::const_iterator it_;
        typename std::vector<std::shared_ptr<Bucket_>>::const_iterator end_;
    };

};

template<class KeyType, class ValueType, class Hash>
template<class InputIterator>
HashMap<KeyType, ValueType, Hash>::HashMap(InputIterator begin, InputIterator end, Hash hasher) : hasher_(hasher),
                                                                                                  size_(0), capacity_(8), table_(8) {
    InitializedElements();
    while (begin != end) {
        insert(*begin);
        begin++;
    }
}

template<class KeyType, class ValueType, class Hash>
HashMap<KeyType, ValueType, Hash>::HashMap(const HashMap &other) : hasher_(other.hasher_),
                                                                   size_(0), capacity_(other.capacity_), table_(other.capacity_)  {
    InitializedElements();
    hasher_ = other.hasher_;
    for (auto& element : other) {
        insert(element);
    }
}

template<class KeyType, class ValueType, class Hash>
HashMap<KeyType, ValueType, Hash>::HashMap(std::initializer_list<std::pair<KeyType, ValueType>> list,
                                           const Hash& hasher) : hasher_(hasher), size_(0), capacity_(8),
                                                                 table_(8) {
    InitializedElements();
    for (auto &element : list) {
        insert(element);
    }
}

template<class KeyType, class ValueType, class Hash>
HashMap<KeyType, ValueType, Hash>::HashMap(const Hash& hasher) : hasher_(hasher),
                                                                 size_(0), capacity_(8), table_(8) {
    InitializedElements();
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
    if (!IsExist(element.first)) {
        InsertElement(element);
        size_++;
        if ((double)capacity_ * load_factor_ <= (double)size_) {
            ReHash();
        }
    }
}

template<class KeyType, class ValueType, class Hash>
void HashMap<KeyType, ValueType, Hash>::erase(KeyType key) {
    size_t psl = 0;
    size_t position = hasher_(key) % capacity_;
    while (!table_[position].get()->is_deleted_ && table_[position].get()->psl_ >= psl) {
        if (table_[position].get()->value_.first == key) {
            break;
        }
        ++psl;
        position = NextPos(position);
    }
    if (table_[position].get()->is_deleted_ || table_[position].get()->psl_ < psl) {
        return;
    }
    table_[position].get()->is_deleted_ = true;
    size_--;
    position = NextPos(position);
    while (!table_[position].get()->is_deleted_ && table_[position].get()->psl_ > 0) {
        size_t prev_position = PrevPos(position);
        swap(table_[position], table_[prev_position]);
        table_[prev_position].get()->psl_--;
        position = NextPos(position);
    }
}

template<class KeyType, class ValueType, class Hash>
typename HashMap<KeyType, ValueType, Hash>::iterator HashMap<KeyType, ValueType, Hash>::find(KeyType key) {
    size_t psl = 0;
    for (size_t position = hasher_(key) % capacity_; !table_[position].get()->is_deleted_ && table_[position].get()->psl_ >= psl; ++psl, position = NextPos(position)) {
        if (table_[position].get()->value_.first == key) {
            return iterator(table_.begin() + position, table_.end());
        }
    }
    return end();
}

template<class KeyType, class ValueType, class Hash>
typename HashMap<KeyType, ValueType, Hash>::const_iterator HashMap<KeyType, ValueType, Hash>::find(KeyType key) const {
    size_t psl = 0;
    for (size_t position = hasher_(key) % capacity_; !table_[position].get()->is_deleted_ && table_[position].get()->psl_ >= psl; ++psl, position = NextPos(position)) {
        if (table_[position].get()->value_.first == key) {
            return const_iterator(table_.begin() + position, table_.end());
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
        if (!table_[i].get()->is_deleted_) {
            return iterator(table_.begin() + i, table_.end());
        }
    }
    return iterator(table_.end(), table_.end());
}

template<class KeyType, class ValueType, class Hash>
typename HashMap<KeyType, ValueType, Hash>::iterator HashMap<KeyType, ValueType, Hash>::end() {
    return iterator(table_.end(), table_.end());
}

template<class KeyType, class ValueType, class Hash>
typename HashMap<KeyType, ValueType, Hash>::const_iterator HashMap<KeyType, ValueType, Hash>::begin() const {
    for (size_t i = 0; i < capacity_; i++) {
        if (!table_[i].get()->is_deleted_) {
            return const_iterator(table_.begin() + i, table_.end());
        }
    }
    return const_iterator(table_.end(), table_.end());
}

template<class KeyType, class ValueType, class Hash>
typename HashMap<KeyType, ValueType, Hash>::const_iterator HashMap<KeyType, ValueType, Hash>::end() const {
    return const_iterator(table_.end(), table_.end());
}

template<class KeyType, class ValueType, class Hash>
void HashMap<KeyType, ValueType, Hash>::clear() {
    size_ = 0;
    capacity_ = 8;
    table_.clear();
    table_.resize(8);
    InitializedElements();
}

template<class KeyType, class ValueType, class Hash>
void HashMap<KeyType, ValueType, Hash>::ReHash() {
    auto old_table = table_;
    capacity_ *= 2;
    table_.clear();
    table_.resize(capacity_);
    InitializedElements();
    for (auto &element : old_table) {
        if (!element.get()->is_deleted_) {
            InsertElement(element.get()->value_);
        }
    }
}

template<class KeyType, class ValueType, class Hash>
bool HashMap<KeyType, ValueType, Hash>::IsExist(KeyType key) const {
    if (find(key) == end()) {
        return false;
    }
    return true;
}

template<class KeyType, class ValueType, class Hash>
void HashMap<KeyType, ValueType, Hash>::InsertElement(std::pair<KeyType, ValueType> element) {
    size_t start_position = hasher_(element.first) % capacity_;
    size_t psl = 0;
    while(psl <= table_[start_position].get()->psl_ && !table_[start_position].get()->is_deleted_) {
        start_position = NextPos(start_position);
        psl++;
    }
    size_t current_position = start_position;
    while (!table_[start_position].get()->is_deleted_) {
        current_position = NextPos(current_position);
        std::swap(table_[current_position], table_[start_position]);
        table_[current_position].get()->psl_++;
    }
    table_[start_position].reset(new Bucket_({element, false, psl}));
}

template<class KeyType, class ValueType, class Hash>
void HashMap<KeyType, ValueType, Hash>::InitializedElements() {
    for (size_t i = 0; i < capacity_; ++i) {
        table_[i].reset(new Bucket_({{}, true, 0}));
    }
}

template<class KeyType, class ValueType, class Hash>
size_t HashMap<KeyType, ValueType, Hash>::NextPos(size_t position) const {
    ++position;
    if (position == capacity_) {
        position = 0;
    }
    return position;
}

template<class KeyType, class ValueType, class Hash>
size_t HashMap<KeyType, ValueType, Hash>::PrevPos(size_t position) const {
    if (position == 0) {
        position = capacity_;
    }
    --position;
    return position;
}

template<class KeyType, class ValueType, class Hash>
HashMap<KeyType, ValueType, Hash>::iterator::iterator(
        typename std::vector<std::shared_ptr<Bucket_>>::iterator it,
        typename std::vector<std::shared_ptr<Bucket_>>::iterator end) : it_(it), end_(end) {}

template<class KeyType, class ValueType, class Hash>
typename HashMap<KeyType, ValueType, Hash>::iterator& HashMap<KeyType, ValueType, Hash>::iterator::operator++() {
    ++it_;
    while (it_ != end_ && (*it_).get()->is_deleted_) {
        ++it_;
    }
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
    return (*it_).get()->value_;
}

template<class KeyType, class ValueType, class Hash>
std::pair<const KeyType, ValueType>* HashMap<KeyType, ValueType, Hash>::iterator::operator->() {
    return &((*it_).get()->value_);
}

template<class KeyType, class ValueType, class Hash>
bool HashMap<KeyType, ValueType, Hash>::iterator::operator==(const iterator& other) const {
    return it_ == other.it_;
}

template<class KeyType, class ValueType, class Hash>
bool HashMap<KeyType, ValueType, Hash>::iterator::operator!=(const iterator& other) const {
    return !(*this == other);
}

template<class KeyType, class ValueType, class Hash>
HashMap<KeyType, ValueType, Hash>::const_iterator::const_iterator(
        typename std::vector<std::shared_ptr<Bucket_>>::const_iterator it,
        typename std::vector<std::shared_ptr<Bucket_>>::const_iterator end) : it_(it), end_(end) {}

template<class KeyType, class ValueType, class Hash>
typename HashMap<KeyType, ValueType, Hash>::const_iterator& HashMap<KeyType, ValueType, Hash>::const_iterator::operator++() {
    ++it_;
    while (it_ != end_ && (*it_).get()->is_deleted_) {
        ++it_;
    }
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
    return (*it_).get()->value_;
}

template<class KeyType, class ValueType, class Hash>
const std::pair<const KeyType, ValueType>* HashMap<KeyType, ValueType, Hash>::const_iterator::operator->() {
    return &((*it_).get()->value_);
}

template<class KeyType, class ValueType, class Hash>
bool HashMap<KeyType, ValueType, Hash>::const_iterator::operator==(const const_iterator& other) const {
    return it_ == other.it_;
}

template<class KeyType, class ValueType, class Hash>
bool HashMap<KeyType, ValueType, Hash>::const_iterator::operator!=(const const_iterator& other) const {
    return !(*this == other);
}

#endif //MY_OWN_HASH_TABLE_HASH_MAP_H
