#include <iostream>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include <map>
#include <list>
#include <utility>
#include <shared_mutex>

template <typename Key, typename Value, typename Hash = std::hash<Key>>
class threadsafe_lookup_table
{
private:
    class bucket_type
    {
    private:
        using bucket_value = std::pair<Key, Value>;
        using bucket_data = std::list<bucket_value>;
        using bucket_iterator = typename bucket_data::iterator;
        bucket_data data;
        mutable std::shared_mutex mutex;

        bucket_iterator find_entry_for(Key const &key) const
        {
            return std::find_if(data.begin(), data.end(), [&](bucket_value const &item) {
                return item.first == key;
            });
        }
    
    public:
        Value value_for(Key const &key, Value const &default_value) const
        {
            std::shared_lock<std::shared_mutex> lock(mutex);
            bucket_iterator const res = find_entry_for(key);
            return res == data.end() ? default_value : res->second;
        }

        void add_or_update_mapping(Key const &key, Value const &value)
        {
            std::unique_lock<std::shared_mutex> lock(mutex);
            bucket_iterator const res = find_entry_for(key);
            if (res == data.end())
            {
                data.push_back(bucket_value(key, value));
                return;
            }
            res->second = value;
        }

        void remove_mapping(Key const &key)
        {
            std::unique_lock<std::shared_mutex> lock(mutex);
            bucket_iterator const res = find_entry_for(key);
            if (res != data.end())
            {
                data.erase(res);
            }
        }
    };
    Hash hasher;
    std::vector<std::unique_ptr<bucket_type>> buckets;

    bucket_type &get_bucket(Key const &key) const
    {
        std::size_t const idx = hasher(key) % buckets.size();
        return *(buckets[idx]);
    }

public:
    threadsafe_lookup_table(int nums = 19, Hash const &_hasher = Hash()) : buckets(nums), hasher(_hasher)
    {
        for (int i = 0; i < nums; ++i)
        {
            buckets[i].reset(new bucket_type);
        }
    }

    threadsafe_lookup_table(threadsafe_lookup_table const &other) = delete;

    threadsafe_lookup_table &operator=(threadsafe_lookup_table const &other) = delete;

    Value value_for(Key const &key, Value const &default_value = Value()) const
    {
        return get_bucket(key).value_for(key, default_value);
    }

    void add_or_update_mapping(Key const &key, Value const &value)
    {
        get_bucket(key).add_or_update_mapping(key, value);
    }

    void remove_mapping(Key const &key)
    {
        get_bucket(key).remove_mapping(key);
    }

    std::map<Key, Value> get_map() const
    {
        std::vector<std::unique_lock<std::shared_mutex>> locks;
        for (unsigned i = 0; i < buckets.size(); ++i)
        {
            locks.push_back(std::unique_lock<std::shared_mutex>(buckets[i].mutex));
        }
        std::map<Key, Value> res;
        for (unsigned i = 0; i < buckets.size(); ++i)
        {
            for (auto it = buckets[i].data.begin(); it != buckets[i].data.end(); ++it)
            {
                res.insert(*it);
            }
        }
        return res;
    }
};

int main()
{
    threadsafe_lookup_table<int, int> test_table;

    return 0;
}
