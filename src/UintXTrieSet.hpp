/*
 * Copyright 2020 transmission.aquitaine@yahoo.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef UINTXTRIESET
#define UINTXTRIESET (1)



#include <assert.h>

#include "basic_types.h"



///////////////////////////////////////////////////////////////////////////////



template<typename uintx>
class UintXTrieSet
    {
private:
    struct Node;

public:
    class Iterator
        {
    public:
        Iterator()                  {            } // @suppress("Class members should be properly initialized")
        Iterator(UintXTrieSet *set) { init(set); } // @suppress("Class members should be properly initialized")
        void init(UintXTrieSet *set);
        bool next(uintx *member);
    private:
        UintXTrieSet *set;
        uinta depth;
        Node *trace[8*sizeof(uintx) + 1];
        };

    UintXTrieSet();
    ~UintXTrieSet();
    bool set(const uintx member);
    bool contains(const uintx member);
    bool expunge(const uintx member);
    void clear();
    uinta size() { return count; }

private:
    Node *root;
    uinta count;

    bool seekTrace(const uintx member, Node *trace[], uinta *depth);
    void insert(const uintx member, Node *trace[], uinta depth, bool terminated);
    void prune(Node *trace[], uinta depth);
    void clearTree();
    static Node *clearBit0(Node *n) { return (Node*)((uinta)n & ~((uinta)1)); }
    static Node *setBit0(Node *n)   { return (Node*)((uinta)n | (uinta)1);    }
    static uinta bit0(Node *n)      { return (uinta)n & (uinta)1;             }
    };



typedef UintXTrieSet<uint16> Uint16TrieSet;
typedef UintXTrieSet<uint32> Uint32TrieSet;
typedef UintXTrieSet<uint64> Uint64TrieSet;
typedef UintXTrieSet<uinta>  UintaTrieSet;



///////////////////////////////////////////////////////////////////////////////



template<typename uintx>
struct UintXTrieSet<uintx>::Node
    {
    Node *link[2];
    uintx member;
    uintx mask;

    Node(uintx ownMember, uintx otherMember, Node *otherLink);
    Node(uintx member)                            { link[0] = link[1] = 0; this->member = member; mask = 0;                                        }
    bool frontMatches(uintx keyToMatch)           { uintx frontMask = ~mask ^ (mask - 1); return (keyToMatch & frontMask) == (member & frontMask); }
    Node *decide(uintx keyToMatch)                { return link[(keyToMatch & mask) ? 1 : 0];                                                      }
    void updateLink(Node *oldLink, Node *newLink) { link[oldLink == link[0] ? 0 : 1] = newLink;                                                    }
    };



template<typename uintx>
UintXTrieSet<uintx>::Node::Node(uintx ownMember, uintx otherMember, Node *otherLink)
    {
    member = ownMember;
    uintx x = member ^ otherMember;
    assert(x != 0);
    mask = (~((uintx)0) >> 1) ^ ~((uintx)0);
    while (!(x & mask)) mask >>= 1;
    if (member & mask)
        {
        link[0] = otherLink;
        link[1] = UintXTrieSet<uintx>::setBit0(this);
        }
    else
        {
        link[0] = UintXTrieSet<uintx>::setBit0(this);
        link[1] = otherLink;
        }
    }



///////////////////////////////////////////////////////////////////////////////



template<typename uintx>
void UintXTrieSet<uintx>::Iterator::init(UintXTrieSet<uintx> *set)
    {
    this->set = set;
    depth = 0;
    }

template<typename uintx>
bool UintXTrieSet<uintx>::Iterator::next(uintx *member)
    {
    Node *n = set->root;
    if (!depth)
        {
        if (!n) return NO;
        depth = 1;
        if (UintXTrieSet<uintx>::bit0(n))
            n = trace[0] = UintXTrieSet<uintx>::clearBit0(n);
        else
            {
            trace[0] = n;
            while (!UintXTrieSet<uintx>::bit0(n->link[0])) trace[depth++] = n = n->link[0];
            n = trace[depth++] = UintXTrieSet<uintx>::clearBit0(n->link[0]);
            }
        }
    else
        {
        while (depth >= 2 && UintXTrieSet<uintx>::clearBit0(trace[depth - 2]->link[1]) == trace[depth - 1]) depth--;
        if (depth <= 1) return NO;
        n = trace[depth - 2]->link[1];
        if (UintXTrieSet<uintx>::bit0(n))
            n = trace[depth - 1] = UintXTrieSet<uintx>::clearBit0(n);
        else
            {
            trace[depth - 1] = n;
            while (!UintXTrieSet<uintx>::bit0(n->link[0])) trace[depth++] = n = n->link[0];
            n = trace[depth++] = UintXTrieSet<uintx>::clearBit0(n->link[0]);
            }
        }
    if (member) *member = n->member;
    return YES;
    }



///////////////////////////////////////////////////////////////////////////////



template<typename uintx>
UintXTrieSet<uintx>::UintXTrieSet()
    {
    root = 0;
    count = 0;
    }

template<typename uintx>
UintXTrieSet<uintx>::~UintXTrieSet()
    {
    clear();
    }

template<typename uintx>
bool UintXTrieSet<uintx>::set(const uintx member)
    {
    if (!root)
        {
        root = setBit0(new Node(member));
        count = 1;
        return NO;
        }
    Node *trace[8*sizeof(uintx) + 1];
    uinta depth;
    bool terminated = seekTrace(member, trace, &depth);
    if (terminated)
        {
        Node *n = trace[depth - 1];
        if (n->member == member) return YES;
        }
    insert(member, trace, depth, terminated);
    return NO;
    }

template<typename uintx>
bool UintXTrieSet<uintx>::contains(const uintx member)
    {
    if (!root) return NO;
    if (bit0(root))
        {
        Node *r = clearBit0(root);
        return r->member == member;
        }
    Node *n = root;
    bool terminal = NO;
    for ( ; ; )
        {
        if (terminal) return n->member == member;
        if (!n->frontMatches(member)) return NO;
        n = n->decide(member);
        if (bit0(n))
            {
            terminal = YES;
            n = clearBit0(n);
            }
        else
            terminal = NO;
        }
    }

template<typename uintx>
bool UintXTrieSet<uintx>::expunge(const uintx member)
    {
    if (!root) return NO;
    Node *trace[8*sizeof(uintx) + 1];
    uinta depth;
    if (seekTrace(member, trace, &depth) && trace[depth - 1]->member == member)
         {
         prune(trace, depth);
         return YES;
         }
    else
         return NO;
    }

template<typename uintx>
void UintXTrieSet<uintx>::clear()
    {
    if (!root)
        return;
    else if (bit0(root))
        delete clearBit0(root);
    else
        clearTree();
    root = 0;
    count = 0;
    }

template<typename uintx>
bool UintXTrieSet<uintx>::seekTrace(const uintx member, Node *trace[], uinta *depth)
    {
    if (!root)
        {
        *depth = 0;
        return NO;
        }
    if (bit0(root))
        {
        trace[0] = clearBit0(root);
        *depth = 1;
        return YES;
        }
    Node *n = root;
    uinta i = 0;
    bool terminal = NO;
    for ( ; ; )
        {
        trace[i++] = n;
        if (terminal || !n->frontMatches(member))
            {
            *depth = i;
            return terminal;
            }
        n = n->decide(member);
        if (bit0(n))
            {
            terminal = YES;
            n = clearBit0(n);
            }
        }
    }

template<typename uintx>
void UintXTrieSet<uintx>::insert(const uintx member, Node *trace[], uinta depth, bool terminated)
    {
    Node *other = trace[depth - 1];
    Node *otherLink = terminated ? setBit0(other) : other;
    Node *n = new Node(member, other->member, otherLink);
    if (depth == 1)
        root = n;
    else
        trace[depth - 2]->updateLink(otherLink, n);
    count++;
    }

template<typename uintx>
void UintXTrieSet<uintx>::prune(Node *trace[], uinta depth)
    {
    Node *terminal = trace[depth - 1];
    if (depth == 1)
        root = 0;
    else
        {
        Node *parent = trace[depth - 2];
        if (depth == 2)
            root = parent->link[clearBit0(parent->link[0]) == terminal ? 1 : 0];
        else
            trace[depth - 3]->updateLink(parent, parent->link[clearBit0(parent->link[0]) == terminal ? 1 : 0]);
        if (!terminal->mask)
            parent->mask = 0;
        else if (parent != terminal)
            {
            uinta terminalDepth;
            for (terminalDepth = 0; terminalDepth < depth - 2; terminalDepth++) if (trace[terminalDepth] == terminal) break;
            assert(terminalDepth < depth - 2);
            parent->mask = terminal->mask;
            parent->link[0] = terminal->link[0];
            parent->link[1] = terminal->link[1];
            if (!terminalDepth)
                root = parent;
            else
                trace[terminalDepth - 1]->updateLink(terminal, parent);
            trace[terminalDepth] = parent;
            }
        }
    count--;
    delete terminal;
    }

template<typename uintx>
void UintXTrieSet<uintx>::clearTree()
    {
    Node *trace[8*sizeof(uintx) + 1];
    Node *n = trace[0] = root;
    uinta depth = 1;
    for ( ; ; )
        {
        while (!bit0(n->link[0])) trace[depth++] = n = n->link[0];
        Node *t = clearBit0(n->link[0]);
        if (!t->mask) delete t;
        if (!bit0(n->link[1]))
            trace[depth++] = n = n->link[1];
        else
            {
            while (depth >= 2)
                {
                t = clearBit0(n->link[1]);
                if (!t->mask) delete t;
                while (depth >= 2 && trace[depth - 2]->link[1] == trace[depth - 1]) delete trace[--depth];
                delete trace[--depth];
                if (!depth) return;
                n = trace[depth - 1];
                if (!bit0(n->link[1]))
                    {
                    trace[depth++] = n = n->link[1];
                    break;
                    }
                }
            if (depth == 1)
                {
                t = clearBit0(n->link[1]);
                if (!t->mask) delete t;
                delete n;
                return;
                }
            }
        }
    }



#endif // #ifndef UINTXTRIESET
