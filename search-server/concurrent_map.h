#pragma once
#include <map>
#include <vector>
#include <mutex>

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    struct Access
    {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count)
    {
        dictionaries.resize(bucket_count);

        std::vector<std::mutex> temp(bucket_count);
        mutexes.swap(temp);

        buckets = bucket_count;
    }

    size_t GetBucketIndex(const Key& key)
    {
        return static_cast<uint64_t>(key) % buckets;
    }
    Access operator[](const Key& key)
    {
        size_t bucket = GetBucketIndex(key);
        return { std::lock_guard(mutexes[bucket]), dictionaries[bucket][key] };
    }
    void erase(const Key& key)
    {
        size_t bucket = GetBucketIndex(key);
        std::lock_guard<std::mutex> guard(mutexes[bucket]);
        dictionaries[bucket].erase(key);
    }
    std::map<Key, Value> BuildOrdinaryMap()
    {
        std::map<Key, Value> result;
        for (size_t i = 0; i < buckets; i++)
        {
            std::lock_guard<std::mutex> guard(mutexes[i]);
            result.insert(dictionaries[i].begin(), dictionaries[i].end());
        }

        return result;
    }

private:
    std::vector<std::mutex> mutexes;
    std::vector<std::map<Key, Value>> dictionaries;
    size_t buckets;
};