#include "hash_map.h"
#include <iostream>
#include <cstdlib>
#include <functional>
#include <stdexcept>
#include <map>
#include <random>
#include <unordered_map>

void fail(const char *message) {
    std::cerr << "Fail:\n";
    std::cerr << message;
    std::cout << "I want to get WA\n";
    exit(0);
}

struct StrangeInt {
    int x;
    static int counter;
    StrangeInt() {
        ++counter;
    }
    StrangeInt(int x): x(x) {
        ++counter;
    }
    StrangeInt(const StrangeInt& rs): x(rs.x) {
        ++counter;
    }
    bool operator ==(const StrangeInt& rs) const {
        return x == rs.x;
    }

    static void init() {
        counter = 0;
    }

    ~StrangeInt() {
        --counter;
    }

    friend std::ostream& operator <<(std::ostream& out, const StrangeInt& b) {
        out << b.x;
        return out;
    }
};
int StrangeInt::counter;

namespace std {
    template<> struct hash<StrangeInt> {
        size_t operator()(const StrangeInt& x) const {
            return x.x;
        }
    };
}

namespace internal_tests {

/* check that hash_map provides correct interface
 * in terms of constness */
    void const_check() {
        const HashMap<int, int> map{{1, 5}, {3, 4}, {2, 1}};
        std::cerr << "check constness\n";
        if (map.empty())
            fail("incorrect empty method");
        static_assert(std::is_same<
                HashMap<int, int>::const_iterator,
                decltype(map.begin())
        >::value, "'begin' returns not a const iterator");
        auto hash_f = map.hash_function();
        std::cerr << hash_f(0) << "\n";
        for (auto cur : map)
            std::cerr << cur.first << " " << cur.second << "\n";

        HashMap<int, int>::const_iterator it = map.find(3);
        if (it->second != 4)
            fail("not found 3, incorrect find or insert");
        it = map.find(7);
        if (it != map.end())
            fail("found 7? incorrect find or insert");

        static_assert(std::is_same<
                const int,
                std::remove_reference<decltype(map.at(1))>::type
        >::value, "'At' returns non const");
        std::cerr << "ok!\n";
    }

    void exception_check() {
        const HashMap<int, int> map{{2, 3}, {-7, -13}, {0, 8}};
        std::cerr << "check exception...\n";
        try {
            auto cur = map.at(8);
            std::cerr << cur << "\n";
        }
        catch (const std::out_of_range& e) {
            std::cerr << "ok!\n";
            return;
        }
        catch (...) {
            fail("'at' doesn't throw std::out_of_range");
        }
        fail("'at' doesn't throw anything");
    }

    void check_destructor() {
        std::cerr << "check destructor... ";
        StrangeInt::init();
        {
            HashMap<StrangeInt, int> s{
                    {5, 4},
                    {3, 2},
                    {1, 0}
            };
            if (s.size() != 3)
                fail("wrong size");
        }
        if (StrangeInt::counter)
            fail("wrong destructor (or constructors)");
        {
            HashMap<StrangeInt, int> s{
                    {-3, 3},
                    {-2, 2},
                    {-1, 1}
            };
            HashMap<StrangeInt, int> s1(s);
            s1.insert(std::make_pair(0, 0));
            HashMap<StrangeInt, int> s2(s1);
            if (s1.find(0) == s1.end())
                fail("wrong find");
        }
        if (StrangeInt::counter)
            fail("wrong destructor (or constructors)");
        std::cerr << "ok!\n";
    }


    void reference_check() {
        HashMap<int, int> map{{3, 4}, {3, 5}, {4, 7}, {-1, -3}};
        std::cerr << "check references... ";
        map[3] = 7;
        if (map[3] != 7 || map[0] != 0)
            fail("incorrect [ ]");
        auto it = map.find(4);
        if (it == map.end())
            fail("not found 4, incorrect find or insert");
        it->second = 3;
        auto cur = map.find(4);
        if (cur->second != 3)
            fail("can't modificate through iterator");
        std::cerr << "ok!\n";
    }

    size_t stupid_hash(int /*x*/) {
        return 0;
    }

    void hash_check() {
        std::cerr << "check hash functions\n";
        struct Hasher {
            std::hash<std::string> hasher;
            size_t operator()(const std::string& s) const {
                return hasher(s);
            }
        };
        HashMap<std::string, std::string, Hasher> map{
                {"aba", "caba"},
                {"simple", "case"},
                {"test", "test"}
        };
        for (auto cur : map)
            std::cerr << cur.first << " " << cur.second << "\n";
        auto simple_hash = [](unsigned long long x) -> size_t {
            return x % 17239;
        };
        HashMap<int, std::string, decltype(simple_hash)> second_map(simple_hash);
        second_map.insert(std::make_pair(0, "a"));
        second_map.insert(std::make_pair(0, "b"));
        second_map[17239] = "check";
        auto second_hash_fn = second_map.hash_function();
        if (second_hash_fn(17239) != 0)
            fail("wrong hash function in class");
        if (second_map[0] != "a" || second_map[17239] != "check")
            fail("incorrect insert or [ ]");
        for (auto cur : second_map)
            std::cerr << cur.first << " " << cur.second << "\n";
        HashMap<int, int, std::function<size_t(int)>> stupid_map(stupid_hash);
        auto stupid_hash_fn = stupid_map.hash_function();
        for(int i = 0; i < 1000; ++i) {
            stupid_map[i] = i + 1;
            if (stupid_hash_fn(i))
                fail("wrong hash function in class");
        }
        if (stupid_map.size() != 1000)
            fail("Wrong size");
        std::cerr << "ok!\n";
    }

    void check_copy() {
        std::cerr << "check copy correctness...\n";
        HashMap<int, int> First;
        HashMap<int, int> Second(First);
        Second.insert(std::make_pair(1, 1));
        HashMap<int, int> third(Second.begin(), Second.end());
        third[0] = 5;
        if (third.size() != 2)
            fail("Wrong size");
        First = third;
        Second = Second = First;
        if (First.find(0)->second != 5)
            fail("wrong find");
        if (Second[0] != 5)
            fail("wrong [ ]");
        std::cerr << "ok!\n";
    }

    void check_iterators() {
        std::cerr << "check iterators...\n";
        {
            HashMap<int, int> first{{0, 0}};
            HashMap<int, int>::iterator just_iterator;
            HashMap<int, int>::iterator it = first.begin();
            static_assert(std::is_same<
                    const int,
                    std::remove_reference<decltype(it->first)>::type
            >::value, "Iterator's key type isn't const");
            if (it++ != first.begin())
                fail("bad post ++");
            if (!(it == first.end()))
                fail("bad post ++");
            if (++first.begin() != first.end())
                fail("bad pre ++");
            first.erase(0);
            if (first.begin() != first.end())
                fail("bad begin or end");
            just_iterator = first.begin();
        }

        {
            const HashMap<int, int> first{{1, 1}};
            HashMap<int, int>::const_iterator just_iterator;
            HashMap<int, int>::const_iterator it = first.begin();
            if (it++ != first.begin())
                fail("bad post ++");
            if (!(it == first.end()))
                fail("bad post ++");
            if (++first.begin() != first.end())
                fail("bad pre ++");
            just_iterator = it;
        }

        std::cerr << "ok!\n";
    }

    void my_check() {
        std::cerr << "my_check...\n";

        HashMap<int, int> map;

        const int N = 10'000'000;

        std::mt19937 gen(time(0));

        long double start = clock();

        for (int i = 0; i < N; i++) {
            int key = gen() % (N * 10);
            map.insert({key, i});
            auto it = map.begin();
            if (i % (N / 10) == 0) {
                std::cerr << i << std::endl;
            }
        }

        std::cerr << (clock() - start) / CLOCKS_PER_SEC << std::endl;

        if (1) {
            long double start1 = clock();
            std::unordered_map<int, int> st;

            for (int i = 0; i < N; i++) {
                int key = gen() % (N * 10);
                st.insert({key, i});
                if (i % (N / 10) == 0) {
                    std::cerr << i << std::endl;
                }
            }

            std::cerr << "not me " << (clock() - start1) / CLOCKS_PER_SEC << std::endl;
        }

        //std::cerr << "ok!\n";
    }

    void my_check2() {
        std::cerr << "my_check2...\n";

        auto simple_hash = [](unsigned long long x) -> size_t {
            return x % 2;
        };
        //HashMap<int, int, decltype(simple_hash)> map(simple_hash);
        HashMap<int, int> map;

        const int N = 1'00'000;

        std::mt19937 gen(228);

        long double start = clock();

        for (int i = 0; i < N; i++) {
            int key = gen() % (N * 10);
            map[key] = 0;
            map.find(key);
            for (auto j : map) {
                j.second++;
            }
            if (i % (N / 10) == 0) {
                std::cerr << i << std::endl;
                map.clear();
            }
        }

//        for (int i = 0; i < N; i++) {
//            int key = gen() % (N * 10);
//            map.erase(key);
//            if (i % (N / 10) == 0) {
//                std::cerr << i << std::endl;
//            }
//        }

        std::cerr << (clock() - start) / CLOCKS_PER_SEC << std::endl;

        if (1) {
            long double start1 = clock();
            std::unordered_map<int, int> st;

            for (int i = 0; i < N; i++) {
                int key = gen() % (N * 10);
                st[key] = 0;
                st.find(key);
                for (auto j : st) {
                    j.second++;
                }
                if (i % (N / 10) == 0) {
                    std::cerr << i << std::endl;
                    st.clear();
                }
            }

//            for (int i = 0; i < N; i++) {
//                int key = gen() % (N * 10);
//                st.erase(key);
//                if (i % (N / 10) == 0) {
//                    //std::cerr << i << std::endl;
//                }
//            }

            std::cerr << (clock() - start1) / CLOCKS_PER_SEC << std::endl;
        }

        //std::cerr << "ok!\n";
    }

    void run_all() {
        const_check();
        exception_check();
        reference_check();
        hash_check();
        check_destructor();
        check_copy();
        check_iterators();
        for (int t = 0; t < 0; t++) {
            my_check();
        }
        for (int t = 0; t < 1; t++) {
            my_check2();
        }
    }
} // namespace internal_tests

int main() {
    internal_tests::run_all();
    return 0;
}