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
const size_t BUCKET_SIZE = 1 << 3;  // TODO: make it a parameter 2

template<class KeyType, class ValueType, class Hash = std::hash<KeyType> >
class HashMap {
public:
    class iterator;

    class const_iterator;

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
        InsertElementInBucket(bucket, element, hash, subtable.power_of_two_);
        if (bucket.CheckFull(BUCKET_SIZE)) {  // TODO: make it a parameter
            ++subtable.full_buckets_;
            CheckFullSubtable(subtable);
        }
        ++size_;
    }

    void erase(KeyType key) {
        size_t hash = hasher_(key);
        Subtable_& subtable = subtables_[GetHash(hash)];
        Bucket_& bucket = subtable.buckets_.get()[hash & ((1 << subtables_[GetHash(hash)].power_of_two_) - 1)];
        for (auto iterator = bucket.data_.begin(); iterator != bucket.data_.end(); ++iterator) {
            if (iterator->key_ == key) {
                bucket.data_.erase(iterator);
                --size_;
                return;
            }
        }
    }

    const_iterator find(KeyType key) const {}

    iterator find(KeyType key) {}

    ValueType &operator[](KeyType key) {
        bool is_found = false;
        auto& element = FindElement(key, is_found);
        if (!is_found) {
            insert({key, ValueType()});
            return FindElement(key, is_found).second;
        }
        return element.second;
    }

    const ValueType &at(KeyType key) const {
        bool is_found = false;
        auto& element = FindElement(key, is_found);
        if (!is_found) {
            throw std::out_of_range("Key not found");
        }
        return element.second;
    }

    iterator begin() {}

    iterator end() {}

    const_iterator begin() const {}

    const_iterator end() const {}

    void clear() {
        for (size_t i = 0; i < SUBTABLES_NUMBER; ++i) {
            subtables_[i].buckets_.reset(new Bucket_[1]);
            subtables_[i].power_of_two_ = 0;
            subtables_[i].full_buckets_ = 0;
        }
        size_ = 0;
    }

private:
    using Element_ = std::pair<KeyType, ValueType>;

    struct Bucket_ {
        std::list<Element_> data_;
        bool is_full_ = false;
        size_t size_left_ = 0;
        size_t size_right_ = 0;


        Bucket_() = default;

        bool CheckFull(size_t capacity) {
            if (!is_full_ && size_left_ > capacity && size_right_ > capacity) {  // TODO: make it OR
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

    std::array<Subtable_, SUBTABLES_NUMBER> subtables_;
    Hash hasher_;
    size_t size_ = 0;
    double load_factor_ = 0.8;
    Element_ fake_element_;

    size_t GetHash(size_t hash) const {  // TODO: change to 64 bit hash
        return (((hash >> 32) & ((1 << 8) - 1))) ^ (((hash >> 16) & ((1 << 8) - 1))) ^
        (((hash >> 8) & ((1 << 8) - 1))) ^ (hash & ((1 << 8) - 1));
    }

    Element_& FindElement(KeyType key, bool& is_found) {
        size_t hash = hasher_(key);
        auto& bucket = subtables_[GetHash(hash)].buckets_.get()[hash & ((1 << subtables_[GetHash(hash)].power_of_two_) - 1)];
        for (auto& node : bucket.data_) {
            if (node.first == key) {
                is_found = true;
                return node;
            }
        }
        is_found = false;
        return fake_element_;
    }

    void CheckFullSubtable(Subtable_& subtable) {
        if (subtable.full_buckets_ < subtable.subtable_size_ * load_factor_) {
            return;
        }
        std::unique_ptr<Bucket_[]> new_buckets = std::make_unique<Bucket_[]>(subtable.subtable_size_ * 2);
        for (size_t i = 0; i < subtable.subtable_size_; ++i) {
            for (auto& element : subtable.buckets_.get()[i].data_) {
                size_t hash = hasher_(element.first);
                Bucket_& bucket = new_buckets.get()[hash & ((1 << (subtables_[GetHash(hash)].power_of_two_ + 1)) - 1)];
                InsertElementInBucket(bucket, element, hash, subtable.power_of_two_ + 1);
            }
        }
        subtable.buckets_.reset(new_buckets.release());
        subtable.subtable_size_ *= 2;
        ++subtable.power_of_two_;
    }

    void InsertElementInBucket(Bucket_& bucket, Element_& element, size_t hash, size_t power_of_two_) {
        bucket.data_.push_front(element);
        if (((hash >> power_of_two_) & 1) == 0) {
            ++bucket.size_left_;
        } else {
            ++bucket.size_right_;
        }
    }

    friend class iterator;

public:

    class iterator {
    public:
        using bucket_iterator = std::list<std::pair<const KeyType, ValueType>>::iterator;

        iterator(std::array<Subtable_, SUBTABLES_NUMBER> subtables, size_t subtable_index, size_t bucket_index,
                 bucket_iterator iterator_in_bucket) :
                subtables_(subtables), subtable_index_(subtable_index), bucket_index_(bucket_index),
                iterator_in_bucket_(iterator_in_bucket) {}

        iterator& operator++() {
            if (++iterator_in_bucket_ != subtables_[subtable_index_].buckets_.get()[bucket_index_].data_.end()) {
                return *this;
            }
            for (size_t i = subtable_index_; i < SUBTABLES_NUMBER; ++i) {
                Subtable_& subtable = subtables_[i];
                for (size_t j = bucket_index_; j < BUCKET_SIZE; ++j) {
                    Bucket_& bucket = subtable.buckets_.get()[j];
                    for (auto iterator = bucket.data_.begin(); iterator != bucket.data_.end(); ++iterator) {
                        iterator_in_bucket_ = iterator;
                        return *this;
                    }
                }
            }
        }

        Element_& operator*() {
            return *iterator_in_bucket_;
        }

        Element_* operator->() {
            return *iterator_in_bucket_;
        }

        bool operator==(const iterator& other) const {
            return iterator_in_bucket_ == other.iterator_in_bucket_;
        }

        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }

    private:
        size_t subtable_index_;
        size_t bucket_index_;
        bucket_iterator iterator_in_bucket_;
        std::array<Subtable_, SUBTABLES_NUMBER>* subtables_;
    };

    class const_iterator {
    };

};

#endif //MY_OWN_HASH_TABLE_HASH_MAP_H
