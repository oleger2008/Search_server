#pragma once

#include <cstdlib>
#include <future>
#include <map>
#include <mutex>
#include <string>
#include <type_traits>
#include <vector>

template <typename Key, typename Value>
class ConcurrentMap {
	struct MutexMap {
		std::mutex mutex;
		std::map<Key, Value> map;
	};

public:
	static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

	struct Access {
		std::lock_guard<std::mutex> guard;
		Value& ref_to_value;
	};

	explicit ConcurrentMap(size_t bucket_count)
	: bucket_count_(bucket_count)
	, concurent_map_(bucket_count_)
	{
	}

	Access operator[](const Key& key) {
		auto& local_mutex_map = concurent_map_.at(static_cast<uint64_t>(key) % bucket_count_);
		return {std::lock_guard<std::mutex>(local_mutex_map.mutex), local_mutex_map.map[key]};
	}

	std::map<Key, Value> BuildOrdinaryMap() {
		std::map<Key, Value> ordinary_map;
		for (auto& [mutex, map] : concurent_map_) {
			std::lock_guard<std::mutex> guard(mutex);
			ordinary_map.insert(map.begin(), map.end());
		}
		return ordinary_map;
	}

private:
	size_t bucket_count_ = 1;
	std::vector<MutexMap> concurent_map_;
};
