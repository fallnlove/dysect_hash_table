/*
    Created by Askar Tsyganov on 11.01.2023.

    This hash table is based on the DySECT hash table from https://arxiv.org/abs/1705.00997
    It will be modified by Iceberg Hashing and other improvements
*/

#ifndef MY_OWN_HASH_TABLE_HASH_MAP_H
#define MY_OWN_HASH_TABLE_HASH_MAP_H

#include <array>
#include <exception>
#include <iterator>
#include <list>
#include <memory>
#include <queue>
#include <utility>
#include <vector>

const size_t SUBTABLES_NUMBER = 1 << 8;
const size_t BUCKET_SIZE = 1 << 3;
const size_t HASH_NUMBER = 3;

template<class KeyType, class ValueType, class Hash = std::hash<KeyType> >
class HashMap {
public:
    class iterator {
    };

    class const_iterator {
    };

    explicit HashMap(Hash hasher = Hash()) {}

    HashMap(std::iterator<std::forward_iterator_tag, std::pair<KeyType, ValueType>> begin,
            std::iterator<std::forward_iterator_tag, std::pair<KeyType, ValueType>> end,
            Hash hasher = Hash()) {}

    HashMap(std::initializer_list<std::pair<KeyType, ValueType>> list, Hash hasher = Hash()) {}

    size_t size() const {
        return size_;
    }

    bool empty() const {
        return size_ == 0;
    }

    Hash hash_function() const {
        return hasher_;
    }

    void insert(std::pair<KeyType, ValueType> element) {
        size_t hash = hasher_(element.first);
        Subtable_& subtable = subtables_[GetHash(hash)];
        Bucket_& bucket = subtable.buckets_.get()[hash & ((1 << subtables_[GetHash(hash)].power_of_two_) - 1)];
        InsertElementInBucket(bucket, {element.first, element.second}, hash, subtable.power_of_two_);
        if (bucket.CheckFull(BUCKET_SIZE)) {
            ++subtable.full_buckets_;
            CheckFullSubtable(subtable);
        }
        ++size_;
    }

    void erase(KeyType key) {}

    const_iterator find(KeyType key) const {}

    iterator find(KeyType key) {}

    ValueType &operator[](KeyType key) {
        auto& element = FindElement(key);
        if (element.is_deleted_ == true) {
            assert(false);
            insert({key, ValueType()});
            return FindElement(key).value_;
        }
        return element.value_;
    }

    const ValueType &at(KeyType key) const {
        auto& element = FindElement(key);
        if (element.is_deleted_ == true) {
            throw std::out_of_range("No such element");
        }
        return element.value_;
    }

    iterator begin() {}

    iterator end() {}

    const_iterator begin() const {}

    const_iterator end() const {}

    void clear() {}

private:
    struct Element_ {
        KeyType key_ = KeyType();
        ValueType value_ = ValueType();
        bool is_deleted_ = true;

        Element_() = default;

        Element_(KeyType key, ValueType value) : key_(key), value_(value), is_deleted_(false) {}

        Element_(KeyType key, ValueType value, bool is_deleted) : key_(key), value_(value), is_deleted_(is_deleted) {}
    };

    struct Bucket_ {
        std::list<Element_> data_;
        bool is_full_ = false;
        size_t size_left_ = 0;
        size_t size_right_ = 0;


        Bucket_() = default;

        bool CheckFull(size_t capacity) {
            if (size_left_ > capacity && size_right_ > capacity && !is_full_) {
                is_full_ = true;
                return true;
            }
            return false;
        }
    };

    struct Subtable_ {
        std::unique_ptr<Bucket_[]> buckets_;
        size_t subtable_size_ = 1;
        size_t full_buckets_ = 0;
        size_t power_of_two_ = 0;

        Subtable_() {
            buckets_ = std::make_unique<Bucket_[]>(1);
        }
    };

    Hash hasher_;
    std::array<Subtable_, SUBTABLES_NUMBER> subtables_;
    size_t size_ = 0;
    Element_ fake_empty_element_ = Element_{ValueType(), KeyType(), true};
    double load_factor_ = 0.8;

    size_t GetHash(size_t hash) const {  // TODO: change to 64 bit hash
        return (((hash >> 32) & ((1 << 8) - 1))) ^ (((hash >> 16) & ((1 << 8) - 1))) ^
        (((hash >> 8) & ((1 << 8) - 1))) ^ (hash & ((1 << 8) - 1));
    }

    Element_& FindElement(KeyType key) {
        size_t hash = hasher_(key);
        for (auto& node : subtables_[GetHash(hash)].buckets_.get()[hash & ((1 << subtables_[GetHash(hash)].power_of_two_) - 1)].data_) {
            if (node.key_ == key && !node.is_deleted_) {
                return node;
            }
        }
        return fake_empty_element_;
    }

    void CheckFullSubtable(Subtable_& subtable) {
        if (subtable.full_buckets_ < subtable.subtable_size_ * load_factor_) {
            return;
        }
        std::unique_ptr<Bucket_[]> new_buckets = std::make_unique<Bucket_[]>(subtable.subtable_size_ * 2);
        for (size_t i = 0; i < subtable.subtable_size_; ++i) {
            for (auto& element : subtable.buckets_.get()[i].data_) {
                if (element.is_deleted_) {
                    continue;
                }
                size_t hash = hasher_(element.key_);
                Bucket_& bucket = new_buckets.get()[hash & ((1 << (subtables_[GetHash(hash)].power_of_two_ + 1)) - 1)];
                InsertElementInBucket(bucket, element, hash, subtable.power_of_two_ + 1);
            }
        }
        subtable.buckets_.reset(new_buckets.release());
        subtable.subtable_size_ *= 2;
        ++subtable.power_of_two_;
    }

    void InsertElementInBucket(Bucket_& bucket, Element_ element, size_t hash, size_t power_of_two_) {
        bucket.data_.push_back(element);
        if (((hash >> power_of_two_) & 1) == 0) {
            ++bucket.size_left_;
        } else {
            ++bucket.size_right_;
        }
    }

};

#endif //MY_OWN_HASH_TABLE_HASH_MAP_H
