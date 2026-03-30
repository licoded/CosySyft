// Minimal cuddObj.cc - implements only the methods used by dfa_cudd.
#include <cstdio>
#include <stdexcept>
#include "cudd.h"
#include "cuddObj.hh"

namespace CUDD {

// ---------------------------------------------------------------------------
// Capsule
// ---------------------------------------------------------------------------

class Capsule {
public:
    DdManager *manager;
    PFC errorHandler, timeoutHandler, terminationHandler;
    bool verbose;
    int ref;
    Capsule(unsigned int nv, unsigned int nvz, unsigned int slots,
            unsigned int cache, unsigned long mem, PFC handler)
        : errorHandler(handler), timeoutHandler(handler),
          terminationHandler(handler), verbose(false), ref(1) {
        manager = Cudd_Init(nv, nvz, slots, cache, mem);
        if (!manager) errorHandler("Out of memory");
    }
    ~Capsule() { Cudd_Quit(manager); }
};

// ---------------------------------------------------------------------------
// defaultError
// ---------------------------------------------------------------------------

void defaultError(std::string msg) { throw std::logic_error(msg); }

// ---------------------------------------------------------------------------
// DD
// ---------------------------------------------------------------------------

DD::DD() : p(nullptr), node(nullptr) {}

DD::DD(Capsule *cap, DdNode *n) : p(cap), node(n) {
    if (node) Cudd_Ref(node);
}

DD::DD(Cudd const& mgr, DdNode *n) : p(mgr.p), node(n) {
    if (node) Cudd_Ref(node);
}

DD::DD(const DD& from) : p(from.p), node(from.node) {
    if (node) Cudd_Ref(node);
}

DD::~DD() {}

DdManager* DD::checkSameManager(const DD& other) const {
    DdManager* m = p->manager;
    if (m != other.p->manager) p->errorHandler("Different managers.");
    return m;
}

void DD::checkReturnValue(const void* r) const {
    if (!r) p->errorHandler("CUDD returned null.");
}

void DD::checkReturnValue(int r, int expected) const {
    if (r != expected) p->errorHandler("CUDD returned unexpected value.");
}

DdManager* DD::manager() const { return p->manager; }
DdNode*    DD::getNode() const { return node; }
DdNode*    DD::getRegularNode() const { return Cudd_Regular(node); }
int        DD::nodeCount() const { return Cudd_DagSize(node); }
unsigned int DD::NodeReadIndex() const { return Cudd_NodeReadIndex(node); }

// ---------------------------------------------------------------------------
// ABDD
// ---------------------------------------------------------------------------

ABDD::ABDD() : DD() {}
ABDD::ABDD(Capsule *cap, DdNode *n) : DD(cap, n) {}
ABDD::ABDD(Cudd const& mgr, DdNode *n) : DD(mgr, n) {}
ABDD::ABDD(const ABDD& from) : DD(from) {}

ABDD::~ABDD() {
    if (node) Cudd_RecursiveDeref(p->manager, node);
}

bool ABDD::operator==(const ABDD& o) const { checkSameManager(o); return node == o.node; }
bool ABDD::operator!=(const ABDD& o) const { return !(*this == o); }

// ---------------------------------------------------------------------------
// ADD
// ---------------------------------------------------------------------------

ADD::ADD() : ABDD() {}
ADD::ADD(Capsule *cap, DdNode *n) : ABDD(cap, n) {}
ADD::ADD(Cudd const& mgr, DdNode *n) : ABDD(mgr, n) {}
ADD::ADD(const ADD& from) : ABDD(from) {}

ADD ADD::operator=(const ADD& r) {
    if (this == &r) return *this;
    if (r.node) Cudd_Ref(r.node);
    if (node) Cudd_RecursiveDeref(p->manager, node);
    node = r.node;
    p = r.p;
    return *this;
}

ADD ADD::Ite(const ADD& g, const ADD& h) const {
    DdManager* m = checkSameManager(g); checkSameManager(h);
    DdNode* res = Cudd_addIte(m, node, g.node, h.node);
    checkReturnValue(res);
    return ADD(p, res);
}

// ---------------------------------------------------------------------------
// Cudd manager
// ---------------------------------------------------------------------------

Cudd::Cudd(unsigned int nv, unsigned int nvz, unsigned int slots,
           unsigned int cache, unsigned long mem, PFC handler) {
    p = new Capsule(nv, nvz, slots, cache, mem, handler);
}

Cudd::Cudd(const Cudd& x) : p(x.p) { p->ref++; }

Cudd::~Cudd() {
    if (--p->ref == 0) delete p;
}

Cudd& Cudd::operator=(const Cudd& r) {
    r.p->ref++;
    if (--p->ref == 0) delete p;
    p = r.p;
    return *this;
}

DdManager* Cudd::getManager() const { return p->manager; }

ADD Cudd::addVar(int i) const {
    DdNode* n = Cudd_addIthVar(p->manager, i);
    checkReturnValue(n);
    return ADD(p, n);
}

ADD Cudd::addZero() const {
    DdNode* n = Cudd_ReadZero(p->manager);
    return ADD(p, n);
}

ADD Cudd::constant(CUDD_VALUE_TYPE c) const {
    DdNode* n = Cudd_addConst(p->manager, c);
    checkReturnValue(n);
    return ADD(p, n);
}

void Cudd::checkReturnValue(const void* r) const {
    if (!r) p->errorHandler("CUDD returned null.");
}

void Cudd::checkReturnValue(int r) const {
    if (!r) p->errorHandler("CUDD returned 0.");
}

} // namespace CUDD
