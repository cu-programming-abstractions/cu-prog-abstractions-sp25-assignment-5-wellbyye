/******************************************************************************
 * Robin Hood Hashing
 * Optional bonus: This class implements a Robin Hood hash table.
 * It reduces clustering by stealing spots from entries with shorter probe distances.
 */

#include "RobinHoodHashTable.h"
#include "error.h"
#include <algorithm>
using namespace std;

// A slot to store an entry in the table
struct Slot {
    string value;
    bool isOccupied = false;
    bool isDeleted = false;
};

const int INITIAL_CAPACITY = 17;
const double MAX_LOAD_FACTOR = 0.5;

RobinHoodHashTable::RobinHoodHashTable(HashFunction<string> hashFn)
    : hashFn(hashFn), table(INITIAL_CAPACITY), numElems(0) {
    // Start with an empty table
}

RobinHoodHashTable::~RobinHoodHashTable() {
    // No need to free anything manually
}

int RobinHoodHashTable::size() const {
    return numElems;
}

bool RobinHoodHashTable::isEmpty() const {
    return numElems == 0;
}

// Hash helper
int RobinHoodHashTable::hashIndex(const string& key, int capacity) const {
    return hashFn(key) % capacity;
}

// Calculates how far an element is from where it "should" be
int RobinHoodHashTable::probeDistance(int currentIndex, int idealIndex, int capacity) const {
    return (currentIndex + capacity - idealIndex) % capacity;
}

// Resize and rehash all elements
void RobinHoodHashTable::rehash() {
    Vector<Slot> oldTable = table;
    int newCap = table.size() * 2 + 1;
    table = Vector<Slot>(newCap);
    numElems = 0;

    for (const Slot& slot : oldTable) {
        if (slot.isOccupied && !slot.isDeleted) {
            insert(slot.value); // re-insert in new table
        }
    }
}

bool RobinHoodHashTable::insert(const string& elem) {
    if (contains(elem)) return false;

    if ((double)numElems / table.size() > MAX_LOAD_FACTOR) {
        rehash();
    }

    Slot newSlot{elem, true, false};
    int index = hashIndex(elem, table.size());
    int probeLen = 0;

    while (true) {
        Slot& slot = table[index];

        if (!slot.isOccupied || slot.isDeleted) {
            table[index] = newSlot;
            numElems++;
            return true;
        }

        int ideal = hashIndex(slot.value, table.size());
        int existingProbeLen = probeDistance(index, ideal, table.size());

        if (existingProbeLen < probeLen) {
            // Robin Hood swap: we steal the better spot
            swap(slot, newSlot);
            probeLen = existingProbeLen;
        }

        index = (index + 1) % table.size();
        probeLen++;
    }
}

bool RobinHoodHashTable::contains(const string& elem) const {
    int index = hashIndex(elem, table.size());
    int probeLen = 0;

    while (true) {
        const Slot& slot = table[index];

        if (!slot.isOccupied && !slot.isDeleted) return false;
        if (slot.isOccupied && !slot.isDeleted && slot.value == elem) return true;

        int ideal = hashIndex(slot.value, table.size());
        int existingProbeLen = probeDistance(index, ideal, table.size());
        if (existingProbeLen < probeLen) return false;

        index = (index + 1) % table.size();
        probeLen++;
    }
}

bool RobinHoodHashTable::remove(const string& elem) {
    int index = hashIndex(elem, table.size());
    int probeLen = 0;

    while (true) {
        Slot& slot = table[index];

        if (!slot.isOccupied && !slot.isDeleted) return false;

        if (slot.isOccupied && !slot.isDeleted && slot.value == elem) {
            slot.isOccupied = false;
            slot.isDeleted = true;
            numElems--;
            return true;
        }

        int ideal = hashIndex(slot.value, table.size());
        int existingProbeLen = probeDistance(index, ideal, table.size());
        if (existingProbeLen < probeLen) return false;

        index = (index + 1) % table.size();
        probeLen++;
    }
}
/* * * * * * Test Cases Below This Point * * * * * */
#include "GUI/SimpleTest.h"

// Helper hash function
int testHash(const string& s) {
    int result = 0;
    for (char ch : s) {
        result = (result * 31 + ch);
    }
    return abs(result);
}

STUDENT_TEST("Basic insert and contains test") {
    RobinHoodHashTable table(testHash);

    EXPECT_EQUAL(table.size(), 0);
    EXPECT(table.isEmpty());

    EXPECT(table.insert("apple"));
    EXPECT(table.insert("banana"));
    EXPECT(table.insert("cherry"));

    EXPECT_EQUAL(table.size(), 3);
    EXPECT(!table.isEmpty());

    EXPECT(table.contains("apple"));
    EXPECT(table.contains("banana"));
    EXPECT(!table.contains("durian"));
}

STUDENT_TEST("Remove works correctly") {
    RobinHoodHashTable table(testHash);

    table.insert("x");
    table.insert("y");
    table.insert("z");

    EXPECT(table.contains("y"));
    EXPECT(table.remove("y"));
    EXPECT(!table.contains("y"));
    EXPECT(!table.remove("y")); // can't remove twice
}

STUDENT_TEST("Duplicate insertions don't change size") {
    RobinHoodHashTable table(testHash);

    EXPECT(table.insert("melon"));
    EXPECT(!table.insert("melon")); // duplicate
    EXPECT_EQUAL(table.size(), 1);
}

STUDENT_TEST("Robin Hood hashing handles collisions fairly") {
    RobinHoodHashTable table(testHash);

    for (int i = 0; i < 20; i++) {
        string s = "key" + to_string(i);
        table.insert(s);
    }

    for (int i = 0; i < 20; i++) {
        string s = "key" + to_string(i);
        EXPECT(table.contains(s));
    }
}














