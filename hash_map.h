/*
    Created by Askar Tsyganov on 11.01.2023.

    This hash table is based on the DySECT hash table from https://arxiv.org/abs/1705.00997
    I modified it to use a list instead of constant array and to use a single hash function instead of a few hash functions.
*/

#ifndef MY_OWN_HASH_TABLE_HASH_MAP_H
#define MY_OWN_HASH_TABLE_HASH_MAP_H

#include <iostream>

#include <array>
#include <exception>
#include <iterator>
#include <list>
#include <memory>
#include <queue>
#include <utility>
#include <vector>

const size_t SUBTABLES_NUMBER = 1 << 8;
const size_t BUCKET_SIZE = 1 << 2;  // TODO: make it a parameter 2

template<class KeyType, class ValueType, class Hash = std::hash<KeyType> >
class HashMap {
public:
    class iterator;

    class const_iterator;

    explicit HashMap(const Hash& hasher = Hash()) : hasher_(hasher) {}

    template<class InputIterator>
    HashMap(InputIterator begin,
            InputIterator end,
            Hash hasher = Hash()) : hasher_(hasher) {
        for (auto it = begin; it != end; it++) {
            insert(*it);
        }
    }

    HashMap(std::initializer_list<std::pair<KeyType, ValueType>> list, const Hash& hasher = Hash()) : hasher_(hasher) {
        for (auto &item : list) {
            insert(item);
        }
    }

    HashMap(const HashMap &other) {
        for (auto &item : other) {
            insert(item);
        }
    }

    HashMap &operator=(const HashMap &other) {
        if (this == &other) {
            return *this;
        }
        clear();
        for (auto &item : other) {
            insert(item);
        }
        return *this;
    }

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
        Subtable_& subtable = subtables_[GetHash(hash, 0)];
        Bucket_& bucket = subtable.buckets_.get()[GetHashBucket(hash, subtable.subtable_size_)];
        Element_ element_(element.first, element.second);
        if (!CheckElementInBucket(bucket,element.first)) {
            InsertElementInBucket(bucket, element_, hash, subtable.power_of_two_);
            ++size_;
        }
        if (bucket.CheckFull(BUCKET_SIZE)) {
            ++subtable.full_buckets_;
            if (subtable.full_buckets_ >= subtable.subtable_size_ * load_factor_) {
                SplitBucketsInSubtable(subtable);
            }
        }
    }

    void erase(KeyType key) {
        size_t hash = hasher_(key);
        auto& subtable = subtables_[GetHash(hash, 0)];
        Bucket_& bucket = subtable.buckets_.get()[GetHashBucket(hash, subtable.subtable_size_)];
        for (auto list_iterator = bucket.data_.begin(); list_iterator != bucket.data_.end(); ++list_iterator) {
            if (list_iterator->val_.first == key && !list_iterator->is_deleted_) {
//                bucket.data_.erase(list_iterator);
                list_iterator->is_deleted_ = true;
                if (((hash >> subtable.power_of_two_) & 1) == 0) {
                    --bucket.size_left_;
                } else {
                    --bucket.size_right_;
                }
                if (bucket.size_right_ == 0 && bucket.size_left_ == 0) {
                    bucket.data_.clear();
                }
                --size_;
                return;
            }
        }
    }

    iterator find(KeyType key) {
        size_t hash = hasher_(key);
        auto& subtable = subtables_[GetHash(hash, 0)];
        Bucket_& bucket = subtable.buckets_.get()[GetHashBucket(hash, subtable.subtable_size_)];
        for (size_t i = 0; i < bucket.data_.size(); ++i) {
            if (bucket.data_[i].val_.first == key && !bucket.data_[i].is_deleted_) {
                return iterator(subtables_, GetHash(hash, 0), GetHashBucket(hash, subtable.subtable_size_), i);
            }
        }
        return end();
    }

    const_iterator find(KeyType key) const {
        size_t hash = hasher_(key);
        auto& subtable = subtables_[GetHash(hash, 0)];
        Bucket_& bucket = subtable.buckets_.get()[GetHashBucket(hash, subtable.subtable_size_)];
        for (size_t i = 0; i < bucket.data_.size(); ++i) {
            if (bucket.data_[i].val_.first == key && !bucket.data_[i].is_deleted_) {
                return const_iterator(subtables_, GetHash(hash, 0), GetHashBucket(hash, subtable.subtable_size_), i);
            }
        }
        return end();
    }

    ValueType &operator[](KeyType key) {  // TODO : too slow
        size_t hash = hasher_(key);
        auto& subtable = subtables_[GetHash(hash, 0)];
        Bucket_& bucket = subtable.buckets_.get()[GetHashBucket(hash, subtable.subtable_size_)];
        if (!CheckElementInBucket(bucket, key)) {
            insert({key, ValueType()});
        }
        return FindElement(key).val_.second;
    }

    const ValueType &at(KeyType key) const {
        auto& element = FindElement(key);
        return element.val_.second;
    }

    iterator begin() {
        for (size_t i = 0; i < SUBTABLES_NUMBER; ++i) {
            for (size_t j = 0; j < subtables_[i].subtable_size_; ++j) {
                if (!subtables_[i].buckets_.get()[j].data_.empty()) {
                    auto& vec = subtables_[i].buckets_.get()[j].data_;
                    for (size_t k = 0; k < vec.size(); ++k) {
                        if (!vec[k].is_deleted_) {
                            return iterator(subtables_, i, j, k);
                        }
                    }
                }
            }
        }
        return iterator(subtables_, SUBTABLES_NUMBER, 0, 0);
    }

    iterator end() {
        return iterator(subtables_, SUBTABLES_NUMBER, 0, 0);
    }

    const_iterator begin() const {
        for (size_t i = 0; i < SUBTABLES_NUMBER; ++i) {
            for (size_t j = 0; j < subtables_[i].subtable_size_; ++j) {
                if (!subtables_[i].buckets_.get()[j].data_.empty()) {
                    auto& vec = subtables_[i].buckets_.get()[j].data_;
                    for (size_t k = 0; k < vec.size(); ++k) {
                        if (!vec[k].is_deleted_) {
                            return const_iterator(subtables_, i, j, k);
                        }
                    }
                }
            }
        }
        return const_iterator(subtables_, SUBTABLES_NUMBER, 0, 0);
    }

    const_iterator end() const {
        return const_iterator(subtables_, SUBTABLES_NUMBER, 0, 0);
    }

    void clear() {
        for (size_t i = 0; i < SUBTABLES_NUMBER; ++i) {
            subtables_[i].buckets_.reset(new Bucket_[1]);
            subtables_[i].subtable_size_ = 1;
            subtables_[i].power_of_two_ = 0;
            subtables_[i].full_buckets_ = 0;
        }
        size_ = 0;
    }

private:
//    using Element_ = std::pair<const KeyType, ValueType>;

    struct Element_ {
        std::pair<const KeyType, ValueType> val_;
        bool is_deleted_;

        Element_(const KeyType& key, const ValueType& value) : val_({key, value}), is_deleted_(false) {}
    };

    struct Bucket_ {
        std::vector<Element_> data_;
        bool is_full_ = false;
        size_t size_left_ = 0;
        size_t size_right_ = 0;


        Bucket_() = default;

        bool CheckFull(size_t capacity) {
            if (!is_full_ && size_left_ >= capacity && size_right_ >= capacity) {  // TODO: make it OR
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

    size_t GetHash8(size_t hash) const {  // TODO: change to 64 bit hash
        return (((hash >> 24) & ((1ll << 8) - 1))) ^ (((hash >> 16) & ((1ll << 8) - 1))) ^
        (((hash >> 8) & ((1 << 8) - 1))) ^ (hash & ((1 << 8) - 1));
    }

    inline size_t GetHash(size_t hash, size_t i) const {
        return (GetHash8(hash >> 32) ^ GetHash8(hash & ((1ll << 32) - 1))) & (SUBTABLES_NUMBER - 1);
    }

    inline size_t GetHashBucket(size_t hash, size_t MOD) const {
        return hash & (MOD - 1);
    }

    Element_& FindElement(KeyType key) const {
        size_t hash = hasher_(key);
        auto& subtable = subtables_[GetHash(hash, 0)];
        auto& bucket = subtable.buckets_.get()[GetHashBucket(hash, subtable.subtable_size_)];
        for (auto& element : bucket.data_) {
            if (element.val_.first == key && !element.is_deleted_) {
                return element;
            }
        }
        throw std::out_of_range("Key not found");
    }

    void SplitBucketsInSubtable(Subtable_& subtable) {
        std::unique_ptr<Bucket_[]> new_buckets = std::make_unique<Bucket_[]>(subtable.subtable_size_ * 2);
        for (size_t i = 0; i < subtable.subtable_size_; ++i) {
            for (auto& element : subtable.buckets_.get()[i].data_) {
                if (element.is_deleted_) {
                    continue;
                }
                size_t hash = hasher_(element.val_.first);
                Bucket_& bucket = new_buckets.get()[GetHashBucket(hash, subtable.subtable_size_ * 2)];
                InsertElementInBucket(bucket, element, hash, subtable.power_of_two_ + 1);
            }
        }
        subtable.buckets_.reset(new_buckets.release());
        subtable.subtable_size_ *= 2;
        ++subtable.power_of_two_;
    }

    void MergeBucketsInSubtableTwice(Subtable_& subtable) {
        if (subtable.low_buckets_ == subtable.subtable_size_ && subtable.subtable_size_ > 1) {
            return;
        }
        std::unique_ptr<Bucket_[]> new_buckets = std::make_unique<Bucket_[]>(subtable.subtable_size_ / 2);
        for (size_t i = 0; i < subtable.subtable_size_; i += 2) {
            for (auto& element : subtable.buckets_.get()[i].data_) {
                size_t hash = hasher_(element.val_.first);
                Bucket_& bucket = new_buckets.get()[GetHashBucket(hash, subtable.subtable_size_ / 2)];
                InsertElementInBucket(bucket, element, hash, subtable.power_of_two_ - 1);
            }
            for (auto& element : subtable.buckets_.get()[i + 1].data_) {
                size_t hash = hasher_(element.val_.first);
                Bucket_& bucket = new_buckets.get()[GetHashBucket(hash, subtable.subtable_size_ / 2)];
                InsertElementInBucket(bucket, element, hash, subtable.power_of_two_ - 1);
            }
        }
        subtable.buckets_.reset(new_buckets.release());
        subtable.subtable_size_ /= 2;
        --subtable.power_of_two_;
    }

    void InsertElementInBucket(Bucket_& bucket, Element_ element, size_t hash, size_t power_of_two_) {
        if (((hash >> power_of_two_) & 1) == 0) {
            ++bucket.size_left_;
        } else {
            ++bucket.size_right_;
        }
//        bucket.data_.push_front(element);
//        for (auto& node : bucket.data_) {
//            if (node.is_deleted_) {
//                node = element;
//                return;
//            }
//        }
        bucket.data_.push_back(element);
    }

    bool CheckElementInBucket(Bucket_& bucket, KeyType key) const {
        for (auto& element : bucket.data_) {
            if (element.val_.first == key && !element.is_deleted_) {
                return true;
            }
        }
        return false;
    }

public:
    class iterator {
    public:
        iterator() = default;

        iterator(std::array<Subtable_, SUBTABLES_NUMBER>& subtables, size_t subtable_number,
        size_t bucket_number, size_t list_number) : subtables_(&subtables), subtable_number_(subtable_number),
        bucket_number_(bucket_number), list_number_(list_number) {}

        iterator& operator++() {
            auto& subtables = (*subtables_);
            auto& vec = subtables.at(subtable_number_).buckets_.get()[bucket_number_].data_;
            for (size_t i = list_number_ + 1; i < vec.size(); ++i) {
                if (!vec[i].is_deleted_) {
                    list_number_ = i;
                    return *this;
                }
            }
            for (size_t i = bucket_number_ + 1; i < subtables.at(subtable_number_).subtable_size_; ++i) {
                auto& bucket = subtables.at(subtable_number_).buckets_.get()[i];
                for (size_t j = 0; j < bucket.data_.size(); ++j) {
                    if (!bucket.data_[j].is_deleted_) {
                        bucket_number_ = i;
                        list_number_ = j;
                        return *this;
                    }
                }
            }
            for (size_t i = subtable_number_ + 1; i < SUBTABLES_NUMBER; ++i) {
                for (size_t j = 0; j < subtables.at(i).subtable_size_; ++j) {
                    auto& bucket = subtables.at(i).buckets_.get()[j];
                    for (size_t k = 0; k < bucket.data_.size(); ++k) {
                        if (!bucket.data_[k].is_deleted_) {
                            subtable_number_ = i;
                            bucket_number_ = j;
                            list_number_ = k;
                            return *this;
                        }
                    }
                }
            }
            subtable_number_ = SUBTABLES_NUMBER;
            bucket_number_ = 0;
            list_number_ = 0;
            return *this;
        }

        iterator operator++(int) {
            iterator tmp = *this;
            ++*this;
            return tmp;
        }

        bool operator==(const iterator& other) const {
            return subtable_number_ == other.subtable_number_ && bucket_number_ == other.bucket_number_ &&
            list_number_ == other.list_number_;
        }

        bool operator!=(const iterator& other) const {
            return !(other == *this);
        }

        std::pair<const KeyType, ValueType>& operator*() const {
            return subtables_[0][subtable_number_].buckets_.get()[bucket_number_].data_[list_number_].val_;
        }

        std::pair<const KeyType, ValueType>* operator->() const {
            return &subtables_[0][subtable_number_].buckets_.get()[bucket_number_].data_[list_number_].val_;
        }

    private:
        std::array<Subtable_, SUBTABLES_NUMBER>* subtables_;
        size_t subtable_number_;
        size_t bucket_number_;
        size_t list_number_;

    };

    class const_iterator {
    public:
        const_iterator() = default;

        const_iterator(const std::array<Subtable_, SUBTABLES_NUMBER>& subtables, size_t subtable_number,
                size_t bucket_number, size_t list_number) : subtables_(&subtables), subtable_number_(subtable_number),
        bucket_number_(bucket_number), list_number_(list_number) {}

        const_iterator& operator++() {
            auto& subtables = (*subtables_);
            auto& vec = subtables.at(subtable_number_).buckets_.get()[bucket_number_].data_;
            for (size_t i = list_number_ + 1; i < vec.size(); ++i) {
                if (!vec[i].is_deleted_) {
                    list_number_ = i;
                    return *this;
                }
            }
            for (size_t i = bucket_number_ + 1; i < subtables.at(subtable_number_).subtable_size_; ++i) {
                auto& bucket = subtables.at(subtable_number_).buckets_.get()[i];
                for (size_t j = 0; j < bucket.data_.size(); ++j) {
                    if (!bucket.data_[j].is_deleted_) {
                        bucket_number_ = i;
                        list_number_ = j;
                        return *this;
                    }
                }
            }
            for (size_t i = subtable_number_ + 1; i < SUBTABLES_NUMBER; ++i) {
                for (size_t j = 0; j < subtables.at(i).subtable_size_; ++j) {
                    auto& bucket = subtables.at(i).buckets_.get()[j];
                    for (size_t k = 0; k < bucket.data_.size(); ++k) {
                        if (!bucket.data_[k].is_deleted_) {
                            subtable_number_ = i;
                            bucket_number_ = j;
                            list_number_ = k;
                            return *this;
                        }
                    }
                }
            }
            subtable_number_ = SUBTABLES_NUMBER;
            bucket_number_ = 0;
            list_number_ = 0;
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator tmp = *this;
            ++*this;
            return tmp;
        }

        bool operator==(const const_iterator& other) const {
            return subtable_number_ == other.subtable_number_ && bucket_number_ == other.bucket_number_ &&
                   list_number_ == other.list_number_;
        }

        bool operator!=(const const_iterator& other) const {
            return !(other == *this);
        }

        const std::pair<const KeyType, ValueType>& operator*() const {
            return subtables_[0][subtable_number_].buckets_.get()[bucket_number_].data_[list_number_].val_;
        }

        const std::pair<const KeyType, ValueType>* operator->() const {
            return &subtables_[0][subtable_number_].buckets_.get()[bucket_number_].data_[list_number_].val_;
        }

    private:
        const std::array<Subtable_, SUBTABLES_NUMBER>* subtables_;
        size_t subtable_number_;
        size_t bucket_number_;
        size_t list_number_;

    };

};

#endif //MY_OWN_HASH_TABLE_HASH_MAP_H
