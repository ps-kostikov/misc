#pragma once

#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>

template<
    typename Key,
    typename Value,
    typename KeyHasher = std::hash<Key>,
    typename KeyComparator = std::equal_to<Key>
>
class LruCache
{
public:
    typedef std::shared_ptr<Value> ValuePtr;
    typedef std::function<Value(const Key&)> ValueGenerator;

    LruCache(size_t maxSize)
        : maxSize_(maxSize)
    {
        cache_.reserve(maxSize);
    }

    //might return empty ValuePtr
    ValuePtr get(const Key& key)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        auto it = cache_.find(key);
        if (it == cache_.end()) {
            return ValuePtr();
        } else {
            promote(it, lock);
            return it->second.valuePtr;
        }
    }

    //never returns empty ValuePtr
    ValuePtr getOrEmplace(const Key& key, const ValueGenerator& generator)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        auto it = cache_.find(key);
        if (it == cache_.end()) {
            lock.unlock();
            //unlocking mutex to avoid deadlocks in user-provided function
            //
            //calling put, since this key might be already inserted by concurrent thread
            return put(key, generator(key));
        } else {
            promote(it, lock);
            return it->second.valuePtr;
        }
    }

    //returns inserted valuePtr
    //(never returns empty ptr)
    ValuePtr put(Key key, Value value)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        auto it = cache_.find(key);
        if (it == cache_.end()) {
            return insertWithEviction(std::move(key), std::move(value), lock);
        } else {
            promote(it, lock);
            if (it->second.valuePtr.unique()) {
                //valuePtr is not referenced by the client and its value
                //can be safely assigned
                *it->second.valuePtr = std::move(value);
            } else {
                //creating new ValuePtr to avoid changing values
                //that is referenced by the client
                it->second.valuePtr.reset(new Value(std::move(value)));
            }
            return it->second.valuePtr;
        }
    }

    /**
     * As standard containers do,
     * returns true if erasion was successful
     */
    bool erase(const Key& key)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        auto it = cache_.find(key);
        if (it == cache_.end()) {
            //key already removed
            return false;
        }
        recencyQueue_.erase(it->second.iter);
        cache_.erase(it);
        return true;
    }

    size_t size() const
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return cache_.size();
    }

private:
    /**
     * According to std::unordered_map::emplace documentation, we can safely
     * store the references to map members
     *
     * If rehashing occurs due to the insertion, all iterators are invalidated.
     * Otherwise iterators are not affected. References are not invalidated.
     */
    typedef std::reference_wrapper<const Key> KeyRef;
    typedef std::list<KeyRef> KeyQueue;
    typedef typename KeyQueue::iterator KeyQueueIterator;

    struct CacheValue
    {
        CacheValue(ValuePtr valuePtr_, KeyQueueIterator iter_)
            : valuePtr(std::move(valuePtr_))
            , iter(iter_)
        {
        }

        ValuePtr valuePtr;
        KeyQueueIterator iter;
    };

    typedef std::unordered_map<Key, CacheValue, KeyHasher, KeyComparator> KeyValueMap;
    typedef typename KeyValueMap::iterator KeyValueMapIterator;

    void promote(
        KeyValueMapIterator cacheIter,
        const std::unique_lock<std::mutex>& /*lock*/
    )
    {
        recencyQueue_.erase(cacheIter->second.iter);
        cacheIter->second.iter = recencyQueue_.emplace(
            recencyQueue_.end(),
            cacheIter->first
        );
    }

    void eraseLeastRecent(
        const std::unique_lock<std::mutex>& /*lock*/
    )
    {
        cache_.erase(recencyQueue_.front().get());
        recencyQueue_.pop_front();
    }

    ValuePtr insert(
        Key key,
        Value value,
        const std::unique_lock<std::mutex>& /*lock*/
    )
    {
        auto valuePtr = std::make_shared<Value>(std::move(value));
        auto it = cache_.emplace(
            std::move(key),
            CacheValue(std::move(valuePtr), recencyQueue_.end())
        ).first;
        it->second.iter = recencyQueue_.emplace(
            recencyQueue_.end(),
            it->first
        );
        return it->second.valuePtr;
    }

    ValuePtr insertWithEviction(
        Key key,
        Value value,
        const std::unique_lock<std::mutex>& lock
    )
    {
        if (cache_.size() >= maxSize_) {
            eraseLeastRecent(lock);
        }
        return insert(std::move(key), std::move(value), lock);
    }

    std::unordered_map<Key, CacheValue, KeyHasher, KeyComparator> cache_;
    //values should be inserted at the end of the list,
    //evicted from the front (promotion can remove values from any place)
    //
    //NOTE: (recencyQueue_.size() == cache_.size()) when mutexes isn't locked
    //(i. e. outside private methods)
    KeyQueue recencyQueue_;
    const size_t maxSize_;
    mutable std::mutex mutex_;
};

