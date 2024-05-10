#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>

#include "parlay/primitives.h"
#include "parlay/sequence.h"
#include "parlay/internal/get_time.h"

#include "hashtable.h"

int main(int argc, char **argv) {
	auto usage = "Usage: ./hashtable <n>";
	if (argc != 2) std::cout << usage << std::endl;
	else {
		long n;
		try { n = std::stol(argv[1]); }
		catch (...) { std::cout << usage << std::endl; return 1; }

		parlay::random_generator gen(0);
		std::uniform_int_distribution<long> dis(1, n);

		// generate n random nonzero keys
		auto inserts = parlay::tabulate(n, [&] (long i) {
				auto r = gen[i];
				return dis(r);});

		srand(time(NULL));
		parlay::internal::timer t("Time");
		for (int i = 0; i < 3; i++) {
			hashtable<long> ht(size_t(n * 1.44));
			t.next("construct");
			parlay::for_each(inserts, [&] (auto p) {
					ht.insert(p);});
			t.next("insert");
			parlay::for_each(inserts, [&] (auto p) {
					ht.find(p);});
			t.next("find");
			parlay::for_each(parlay::remove_duplicates(inserts), [&] (auto p) {
					ht.remove(p);});
			t.next("remove");
		}

	}
}
