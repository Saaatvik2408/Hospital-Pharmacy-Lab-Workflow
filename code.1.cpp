#include <iostream>
using namespace std;

struct Ticket {
    int ticketID;
    int type;
    int time;
    int counterNo;
};

/* ================ Part A: Insertion Sort ================ */

// Comparison function implementing the three-level sort key for tickets
static bool ticketLess(const Ticket& a, const Ticket& b) {
    // A.1: Primary key - type (ascending: 1 -> 3)
    if (a.type != b.type) return a.type < b.type;
    // A.2: Secondary key - time (ascending: smaller time = earlier arrival)
    if (a.time != b.time) return a.time < b.time;
    // A.3: Tie-breaker - ticketID (ascending) when type and time are both equal
    return a.ticketID < b.ticketID;
}

// Insertion Sort: shifts elements right until correct position found for key
// Chosen because it is simple, stable, and efficient for small n (n <= 10)
static void insertionSortTickets(Ticket arr[], int n) {
    for (int i = 1; i < n; i++) {
        Ticket key = arr[i];        // current ticket to be placed
        int j = i - 1;
        // shift tickets that are "greater" than key one position to the right
        while (j >= 0 && ticketLess(key, arr[j])) {
            arr[j+1] = arr[j];
            j--;
        }
        arr[j+1] = key;             // place key in its correct sorted position
    }
}

/* ================ Common Printing Helpers ================ */

// Print stack ticketIDs from top to bottom (reverse of array), space-separated
static void printStackTopToBottom(int* st, int sz) {
    if (sz == 0) { cout << "EMPTY"; return; }
    for (int i = sz-1; i >= 0; i--) {
        if (i != sz-1) cout << ' ';
        cout << st[i];
    }
}

// Print queue ticketIDs from front to rear (index 0 = front), space-separated
static void printQueueFrontToRear(int* qu, int sz) {
    if (sz == 0) { cout << "EMPTY"; return; }
    for (int i = 0; i < sz; i++) {
        if (i) cout << ' ';
        cout << qu[i];
    }
}

// Print array of IDs space-separated with no trailing space
static void printLineIds(int* v, int sz) {
    for (int i = 0; i < sz; i++) {
        if (i) cout << ' ';
        cout << v[i];
    }
}

/* ================ Part B: Stack & Queue Dispatch ================ */

// Uses a Stack (array, back=top) for type-1 emergency tickets (LIFO)
// Uses a Queue (array, front=index 0) for type-2 and type-3 tickets (FIFO)
// Dispatch rule: Stack takes priority over Queue
static int runPartB_DispatchTrace(Ticket sortedTickets[], int n, int dispatchOrder[]) {
    int st[10], stSz = 0;   // Stack: back is top
    int qu[10], quSz = 0;   // Queue: index 0 is front

    for (int i = 0; i < n; i++) {
        if (sortedTickets[i].type == 1)
            st[stSz++] = sortedTickets[i].ticketID;   // B.1: push type==1 to Stack
        else
            qu[quSz++] = sortedTickets[i].ticketID;   // B.2: enqueue type!=1 to Queue
    }

    int dispatchCount = 0;
    cout << "DISPATCH TRACE:\n";

    // Dispatch until both Stack and Queue are empty
    while (stSz > 0 || quSz > 0) {
        int served;
        if (stSz > 0) {
            served = st[--stSz];                        // Stack non-empty: pop from top
        } else {
            served = qu[0];                             // Stack empty: dequeue from front
            for (int i = 0; i < quSz-1; i++) qu[i] = qu[i+1];
            quSz--;
        }
        dispatchOrder[dispatchCount++] = served;
        // Print trace: served ticket, then remaining Stack (top->bottom) and Queue (front->rear)
        cout << "Serve : " << served << " | Stack : ";
        printStackTopToBottom(st, stSz);
        cout << " | Queue : ";
        printQueueFrontToRear(qu, quSz);
        cout << "\n";
    }
    cout << "\n";
    return dispatchCount;
}

/* ================ Part C: Circular Linked List ================ */

// Each node = one service counter; stores ticketIDs assigned to it
struct CounterNode {
    int counterId;
    CounterNode* next;      // next counter; last node -> head (circular)
    int assigned[10];       // ticketIDs assigned to this counter
    int assignedSz;
    CounterNode(int id): counterId(id), next(nullptr), assignedSz(0) {}
};

// Build circular linked list of k counters (1..k), last->next = head
static CounterNode* buildCircularCounters(int k) {
    CounterNode* head = new CounterNode(1);
    CounterNode* cur = head;
    for (int i = 2; i <= k; i++) {
        cur->next = new CounterNode(i);
        cur = cur->next;
    }
    cur->next = head;   // close the circle
    return head;
}

// Free all k counter nodes
static void freeCircularCounters(CounterNode* head, int k) {
    if (!head) return;
    CounterNode* cur = head;
    for (int i = 0; i < k; i++) {
        CounterNode* tmp = cur;
        cur = cur->next;
        delete tmp;
    }
}

// Round-robin assignment: pointer starts at Counter 1, advances after each ticket
static void runPartC_CircularCounterLog(int dispatchOrder[], int dispatchCount, int k) {
    CounterNode* head = buildCircularCounters(k);
    CounterNode* ptr = head;    // pointer starts at Counter 1

    for (int i = 0; i < dispatchCount; i++) {
        // C.1: Append dispatched ticketID to current counter's assigned list
        ptr->assigned[ptr->assignedSz++] = dispatchOrder[i];
        // C.2: Move pointer to next counter node (round-robin)
        ptr = ptr->next;
    }

    // Traverse counters 1..k exactly once and print assignments
    cout << "CIRCULAR COUNTER LOG:\n";
    CounterNode* cur = head;
    for (int i = 0; i < k; i++) {
        cout << "Counter " << cur->counterId << ": ";
        if (cur->assignedSz == 0) cout << "EMPTY";
        else printLineIds(cur->assigned, cur->assignedSz);
        cout << "\n";
        cur = cur->next;
    }
    cout << "\n";

    freeCircularCounters(head, k);
}

/* ================ Part D: Hash Table (Linear Probing) ================ */

// Simple struct to return search results (replaces tuple)
struct SearchResult { bool found; int idx; int probes; };

// Hash table size M=10, linear probing, tombstone deletion
struct HashLP {
    enum { M = 10 };
    int state[M];   // 0=EMPTY, 1=OCCUPIED, 2=DELETED (tombstone)
    int val[M];

    HashLP() { for (int i = 0; i < M; i++) state[i] = 0; }

    // Hash function: h(x) = x mod 10
    int h(int x) const { return x % 10; }

    // Insert using linear probing; can reuse DELETED (tombstone) slots
    void insertKey(int x) {
        int idx = h(x);
        for (int i = 0; i < M; i++) {
            int pos = (idx + i) % M;
            if (state[pos] == 0 || state[pos] == 2) {  // EMPTY or DELETED
                state[pos] = 1; val[pos] = x; return;
            }
        }
    }

    // Search with linear probing
    // D.1: Returns index where key is found
    // D.2: Returns number of probes (first slot = probe 1)
    // Stops at: key found | EMPTY slot | all M slots checked
    SearchResult searchKey(int x) const {
        int idx = h(x);
        for (int i = 0; i < M; i++) {
            int pos = (idx + i) % M;
            if (state[pos] == 0) {                              // EMPTY: stop, not found
                SearchResult r = {false, -1, i+1}; return r;
            }
            if (state[pos] == 1 && val[pos] == x) {             // D.1: found at pos
                SearchResult r = {true, pos, i+1}; return r;    // D.2: i+1 probes used
            }
            // state==2 (tombstone): skip and continue probing
        }
        SearchResult r = {false, -1, M}; return r;             // all slots checked
    }

    // Tombstone deletion: mark slot as DELETED (state=2)
    // If key not present, do nothing
    void deleteKeyTombstone(int x) {
        int idx = h(x);
        for (int i = 0; i < M; i++) {
            int pos = (idx + i) % M;
            if (state[pos] == 0) return;                        // EMPTY: not present
            if (state[pos] == 1 && val[pos] == x) {
                state[pos] = 2; return;                         // mark as tombstone
            }
        }
    }

    // Print all 10 slots: occupied value, EMPTY, or DELETED
    void printTable() const {
        for (int i = 0; i < M; i++) {
            cout << i << ": ";
            if      (state[i] == 0) cout << "EMPTY";
            else if (state[i] == 2) cout << "DELETED";
            else                    cout << val[i];
            cout << "\n";
        }
    }
};

static void runPartD_Hash(int dispatchOrder[], int dispatchCount, int queries[], int q, int kdel) {
    HashLP ht;

    // Insert all dispatched IDs in exact dispatch order from Part B
    for (int i = 0; i < dispatchCount; i++) ht.insertKey(dispatchOrder[i]);

    // Answer q search queries
    cout << "HASH SEARCH RESULTS:\n";
    for (int i = 0; i < q; i++) {
        SearchResult r = ht.searchKey(queries[i]);
        if (r.found)
            cout << "Search " << queries[i] << ": Found at Index " << r.idx << ", Probes: " << r.probes << "\n";
        else
            cout << "Search " << queries[i] << ": Not Found, Probes: " << r.probes << "\n";
    }
    cout << "\n";

    // Delete kdel using tombstone, then print final table state
    ht.deleteKeyTombstone(kdel);
    cout << "FINAL HASH TABLE:\n";
    ht.printTable();
    cout << "\n";
}

/* ================ Part E: BST + Traversals ================ */

struct BSTNode {
    int key;
    BSTNode* left;
    BSTNode* right;
    BSTNode(int k): key(k), left(nullptr), right(nullptr) {}
};

// Standard BST insertion: left < node < right
static BSTNode* bstInsert(BSTNode* root, int x) {
    if (!root) return new BSTNode(x);
    if (x < root->key) root->left  = bstInsert(root->left,  x);
    else if (x > root->key) root->right = bstInsert(root->right, x);
    return root;
}

// Preorder: Root -> Left -> Right
static void preorder(BSTNode* root, int out[], int& sz) {
    if (!root) return;
    out[sz++] = root->key;          // visit root first
    preorder(root->left,  out, sz);
    preorder(root->right, out, sz);
}

// Inorder: Left -> Root -> Right (gives sorted ascending order)
static void inorder(BSTNode* root, int out[], int& sz) {
    if (!root) return;
    inorder(root->left,  out, sz);
    out[sz++] = root->key;
    inorder(root->right, out, sz);
}

// Postorder: Left -> Right -> Root
static void postorder(BSTNode* root, int out[], int& sz) {
    if (!root) return;
    postorder(root->left,  out, sz);
    postorder(root->right, out, sz);
    out[sz++] = root->key;          // visit root last
}

// Free BST using postorder (children freed before parent)
static void freeBST(BSTNode* root) {
    if (!root) return;
    freeBST(root->left);
    freeBST(root->right);
    delete root;
}

static void runPartE_BST(int dispatchOrder[], int dispatchCount) {
    BSTNode* root = nullptr;

    // Insert dispatched ticketIDs in exact dispatch order from Part B
    for (int i = 0; i < dispatchCount; i++) root = bstInsert(root, dispatchOrder[i]);

    int pre[10], in[10], post[10];
    int preSz = 0, inSz = 0, postSz = 0;
    preorder (root, pre,  preSz);
    inorder  (root, in,   inSz);
    postorder(root, post, postSz);

    cout << "BST TRAVERSALS:\n";
    cout << "Preorder: ";  printLineIds(pre,  preSz);  cout << "\n";
    cout << "Inorder: ";   printLineIds(in,   inSz);   cout << "\n";
    cout << "Postorder: "; printLineIds(post, postSz); cout << "\n";

    freeBST(root);
}

/* ================ Main Function ================ */
int main() {
    int n;
    cout << "Enter n (number of tickets):" << endl;
    cin >> n;

    Ticket tickets[10];
    cout << "Enter " << n << " tickets (ticketID type time counterNo):" << endl;
    for (int i = 0; i < n; i++)
        cin >> tickets[i].ticketID >> tickets[i].type >> tickets[i].time >> tickets[i].counterNo;

    int k;
    cout << "Enter k (number of counters):" << endl;
    cin >> k;

    int q;
    cout << "Enter q (number of search queries):" << endl;
    cin >> q;

    int queries[10];
    if (q > 0) {
        cout << "Enter " << q << " query keys:" << endl;
        for (int i = 0; i < q; i++) cin >> queries[i];
    }

    int kdel;
    cout << "Enter kdel (ticket to delete from hash):" << endl;
    cin >> kdel;

    // ========== Part A: Insertion Sort ==========
    insertionSortTickets(tickets, n);
    cout << "SORTED TICKETS:\n";
    for (int i = 0; i < n; i++)
        cout << tickets[i].ticketID << ' ' << tickets[i].type << ' '
             << tickets[i].time    << ' ' << tickets[i].counterNo << "\n";
    cout << "\n";

    // ========== Part B: Stack & Queue Dispatch ==========
    int dispatchOrder[10];
    int dispatchCount = runPartB_DispatchTrace(tickets, n, dispatchOrder);

    // ========== Part C: Circular Counter Log ==========
    runPartC_CircularCounterLog(dispatchOrder, dispatchCount, k);

    // ========== Part D: Hash Table Lookup ==========
    runPartD_Hash(dispatchOrder, dispatchCount, queries, q, kdel);

    // ========== Part E: BST Traversals ==========
    runPartE_BST(dispatchOrder, dispatchCount);

    return 0;
}
