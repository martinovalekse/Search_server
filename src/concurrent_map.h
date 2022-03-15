#pragma once

#include <algorithm>
#include <cstdlib>
#include <future>
#include <map>
#include <mutex>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
	static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

	struct Access {
		Access(Value& value_ref, std::unique_lock<std::mutex>&& value_mutex) : mutex_lock(move(value_mutex)), ref_to_value(value_ref)  {
		}

		std::unique_lock<std::mutex> mutex_lock;
		Value& ref_to_value;
	};

	explicit ConcurrentMap(size_t bucket_count) : undermap_mutex_pair_(bucket_count){
	}

	Access operator[](const Key& key) {
		size_t chosen_position = (static_cast<uint64_t>(key) % static_cast<uint64_t>(undermap_mutex_pair_.size()));
		std::unique_lock<std::mutex> iras(undermap_mutex_pair_[chosen_position].alone_mutex);
		Value* node_to_value;
		node_to_value = &undermap_mutex_pair_[chosen_position].undermap[key];
		return {*node_to_value, move(iras)};
	}

	std::map<Key, Value> BuildOrdinaryMap() {
		std::lock_guard<std::mutex> up_lock(something_);
		std::map<Key, Value> result;
		for (size_t i = 0; i < undermap_mutex_pair_.size(); ++i) {
			std::lock_guard<std::mutex> mutex_lock(undermap_mutex_pair_[i].alone_mutex);
			result.merge(undermap_mutex_pair_[i].undermap);
		}
		return result;
	}

private:
	struct BucketMutex {
		std::mutex alone_mutex;
		std::map<Key, Value> undermap;
	};

	std::vector<BucketMutex> undermap_mutex_pair_;
	std::mutex something_;
};
