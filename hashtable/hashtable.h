#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_

#include <cstdlib>
#include <cstdint>
#include <vector>
#include <atomic>

#include "parlay/sequence.h"

template <typename key_t>
class hashtable {
	uint32_t seed;
	size_t hash(const key_t& key) {
		const int len = sizeof(key_t);
		const uint64_t m = 0xc6a4a7935bd1e995;
		const int r = 47;

		uint64_t h = seed ^ (len * m);

		const uint64_t *data = (const uint64_t*)(&key);
		const uint64_t *end = data + (len / 8);

		while (data != end) {
			uint64_t k = *data++;
			k *= m;
			k ^= k >> r;
			k *= m;
			h ^= k;
			h *= m;
		}

		const unsigned char *data2 = (const unsigned char*)data;
		switch (len & 7) {
			case 7: h ^= (uint64_t)data2[6] << 48;
			case 6: h ^= (uint64_t)data2[5] << 40;
			case 5: h ^= (uint64_t)data2[4] << 32;
			case 4: h ^= (uint64_t)data2[3] << 24;
			case 3: h ^= (uint64_t)data2[2] << 16;
			case 2: h ^= (uint64_t)data2[1] << 8;
			case 1: h ^= (uint64_t)data2[0];
					h *= m;
		};

		h ^= h >> r;
		h *= m;
		h ^= h >> r;

		return h;
	}

	bool cas(size_t index, key_t& exp, key_t& upd) {
		return table[index].compare_exchange_strong(exp, upd);
	}

	parlay::sequence<std::atomic<key_t>> table;

public:
	hashtable(const size_t size) {
		seed = rand();
		table = parlay::sequence<std::atomic<key_t>>(size);
	}

	// Returns true if found, false otherwise
	bool find(key_t key) {
		size_t hash_index = hash(key) % (table.size() - 1);
		size_t index = hash_index;
		while(true){
			key_t val = table[index];
			if(val == key){ 
				// if we found the key we are done
				return true;
			} else if(val == 0 || val < key){
				// if we reach an empty spot or a lesser key we know the key is not present
				return false;
			} else{
				// advance to the next index
				index++;
				if(index > table.size() - 1){
					// wrap around to zero
					index = 0;
				}
				if(index == hash_index){
					// If we got back to the original index without finding the key,
					// then we know the key is not in the table
					return false;
				}
			}

		}
	}

	// Inserts the given key, if it doesn't already exist
	void insert(key_t key) {
		size_t hash_index = hash(key) % (table.size() - 1);
		size_t index = hash_index;
		while(true){
			key_t val = table[index];
			if(val == 0 || val < key){
				//We try to insert
				while(true){
					if(cas(index, val, key) == true){
						if(val != 0){
							insert(val);
						}
						return;
					} else {
						val = table[index];
						if(val > key) {
							break;
						}
					}
				}
			}
			index++;
			if(index > table.size() - 1){
				// wrap around to zero
				index = 0;
			}
			if(index == hash_index){
				// If we got back to the original index, the hash table is full
				return;
			}
		}
	}

	key_t find_replacement(key_t key, size_t start){
		size_t hash_index = hash(key) % (table.size() - 1);
		size_t index = start;
		while(true){
			key_t val = table[index];
			if(val == 0 || hash(val) % (table.size() - 1) < hash_index){
				return val;
			} else {
				index++;
				if(index > table.size() - 1){
					// wrap around to zero
					index = 0;
				}
				if(index == hash_index){
					//hopefully this doesn't happen, that would mean there is no suitable replacement
					return 0;
				}
			}
		}
	}

	// Deletes the given key, if it exists
	void remove(key_t key) {
		size_t hash_index = hash(key) % (table.size() - 1);
		size_t index = hash_index;

		while(true){
			key_t val = table[index];
			if(val == key){
				//key found, delete it
				key_t rep = find_replacement(key, index);
				while(true){
					if(cas(index, key, rep) == true){
						if(rep != 0) {
							remove(rep);	
						}
						return;
					} else {
						step_left_delete(rep, index);
						return;
					}
				}
			} else if(val == 0 || val < key) {
				//key is not present
				return;
			} else {
				//proceed to next index
				index++;
				if(index > table.size() - 1){
					// wrap around to zero
					index = 0;
				}
				if(index == hash_index){
					return;
				}
			}
		}
	}

	void step_left_delete(key_t key, size_t start) {
		size_t index = start;
		if(index == 0) {
			index = table.size() - 1;
		} else {
			index--;
		}
		while(true) {
			key_t val = table[index];
			if(val == key) {
				key_t rep = find_replacement(key, index);
				while(true){
					if(cas(index, key, rep) == true){
						if(rep != 0) {
							remove(rep);	
						}
						return;
					} else {
						step_left_delete(rep, index);
						return;
					}
				}
			} else {
				//proceed to next index
				if(index == 0) {
					index = table.size() - 1;
				} else {
					index--;
				}
				if(index == start){
					return;
				}
			}
		}
	}

};

#endif
