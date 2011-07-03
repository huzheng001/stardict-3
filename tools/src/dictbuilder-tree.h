/*
 * This file is part of StarDict.
 *
 * StarDict is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * StarDict is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with StarDict.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @breif tree.h
*/

#ifndef __TREE_H__
#define __TREE_H__

#include <algorithm>
#include <list>

template<typename type>
class tree_node
{
public:
    typedef type                    value_type;
    typedef const type              const_type;
    typedef type*                   pointer_type;
    typedef type&                   reference_type;
    typedef std::list<tree_node*>   children_type;
    typedef tree_node<type>         node_type;

    tree_node(const value_type& _value) :
        value(_value)
    { }

    ~tree_node()
    {
        clear();
    }

    void clear()
    {
        for(typename children_type::iterator it = children.begin();
            it != children.end(); it++)
            delete *it;
        children.clear();
    }

    value_type    value;
    children_type children;
    node_type*    parent;
};

template<typename type>
bool children_incream(tree_node<type>*& parent, tree_node<type>*& child)
{
    if (child == NULL)
        return false;

    typename tree_node<type>::children_type::iterator it =
        std::find(parent->children.begin(), parent->children.end(), child);
    if (it == parent->children.end())
        return false;

    child = (++it != parent->children.end())?*it:NULL;
    return true;
}

template<typename type>
bool children_decream(tree_node<type>*& parent, tree_node<type>*& child)
{
    if (parent->children.size() == 0)
        return false;

    if (child == NULL)
    {
        child = parent->children.back();
        return true;
    }

    typename tree_node<type>::children_type::iterator it =
        std::find(parent->children.begin(), parent->children.end(), child);
    if (it == parent->children.end() || it == parent->children.begin())
        return false;

    child = *(--it);
    return true;
}

template<typename type>
class tree_iterator
{
public:
    typedef type            value_type;
    typedef const type      const_type;
    typedef type*           pointer_type;
    typedef type&           reference_type;
    typedef tree_node<type> node_type;
    typedef typename node_type::children_type::size_type
                            size_type;

    tree_iterator()
    {
    }

    tree_iterator(node_type *_prev, node_type *_current,
                  bool (*_incream)(node_type*&, node_type*&),
                  bool (*_decream)(node_type*&, node_type*&)) :
        prev(_prev), current(_current), incream(_incream), decream(_decream)
    {
    }

    tree_iterator(const tree_iterator& right)
    {
        *this = right;
    }

    pointer_type operator->()
    {
        return &(current->value);
    }

    value_type& operator*()
    {
        return current->value;
    }

    tree_iterator& operator++()
    {
        incream(prev, current);
        return *this;
    }

    const tree_iterator operator++(int)
    {
        tree_iterator tmp(*this);
        incream(prev, current);
        return tmp;
    }

    tree_iterator& operator--()
    {
        decream(prev, current);
        return *this;
    }

    const tree_iterator operator--(int)
    {
        tree_iterator tmp(*this);
        decream(prev, current);
        return tmp;
    }

    const tree_iterator& operator=(const tree_iterator& right)
    {
        prev    = right.prev;
        current = right.current;

        incream = right.incream;
        decream = right.decream;
        return *this;
    }

    bool operator==(const tree_iterator& right) const
    {
        return current == right.current;
    }

    bool operator!=(const tree_iterator& right) const
    {
        return !(*this == right);
    }

    tree_iterator append(const value_type& value)
    {
        node_type* node = new node_type(value);
        node->parent = current;
        current->children.push_back(node);
        return tree_iterator(current, node,
                             &children_incream<value_type>,
                             &children_decream<value_type>);
    }

    tree_iterator insert(const tree_iterator& where,
                         const value_type& value)
    {
        typename node_type::children_type::iterator it =
            current->children.find(where->current);
        if (it == current->children.end())
            return append(value);
        node_type* node = new node_type(value);
        node->parent = current;
        current->children.insert(it, node);
        return tree_iterator(current, node,
                             &children_incream<value_type>,
                             &children_decream<value_type>);
    }

    void erase(const tree_iterator& where)
    {
        typename node_type::children_type::iterator it =
            current->children.find(where->current);
        if (it == current->children.end())
            return ;
        delete where->current;
        current->children.erase(it);
    }

    tree_iterator parent()
    {
        node_type grandparent = current->parent?current->parent->parent:NULL;
        return tree_iterator(grandparent, current->parent,
                             &children_incream<value_type>,
                             &children_decream<value_type>);
    }

    tree_iterator begin()
    {
        return tree_iterator(current,
                current->children.size()?current->children.front():NULL,
                &children_incream<value_type>,
                &children_decream<value_type>);
    }

    tree_iterator end()
    {
        return tree_iterator(current, NULL,
                             &children_incream<value_type>,
                             &children_decream<value_type>);
    }

    size_type size() const
    {
        return current->children.size();
    }

    void clear()
    {
        return current->clear();
    }

    node_type *prev, *current;
    bool (*incream)(node_type*&, node_type*&);
    bool (*decream)(node_type*&, node_type*&);
};

template<typename type>
class const_tree_iterator
{
public:
    typedef type            value_type;
    typedef const type      const_type;
    typedef const type*     pointer_type;
    typedef type&           reference_type;
    typedef tree_node<type> node_type;
    typedef typename node_type::children_type::size_type
                            size_type;

    const_tree_iterator()
    {
    }

    const_tree_iterator(node_type *_prev, node_type *_current,
                        bool (*_incream)(node_type*&, node_type*&),
                        bool (*_decream)(node_type*&, node_type*&)) :
        prev(_prev), current(_current), incream(_incream), decream(_decream)
    {

    }

    const_tree_iterator(const tree_iterator<type>& right)
    {
        *this = right;
    }

    const_tree_iterator(const const_tree_iterator& right)
    {
        *this = right;
    }

    pointer_type operator->()
    {
        return &(current->value);
    }

    const_type& operator*()
    {
        return current->value;
    }

    const_tree_iterator& operator++()
    {
        incream(prev, current);
        return *this;
    }

    const const_tree_iterator operator++(int)
    {
        const_tree_iterator tmp(*this);
        incream(prev, current);
        return tmp;
    }

    const_tree_iterator& operator--()
    {
        decream(prev, current);
        return *this;
    }

    const const_tree_iterator operator--(int)
    {
        const_tree_iterator tmp(*this);
        decream(prev, current);
        return tmp;
    }

    const const_tree_iterator& operator=(const tree_iterator<type>& right)
    {
        prev    = right.prev;
        current = right.current;

        incream = right.incream;
        decream = right.decream;
        return *this;
    }

    const const_tree_iterator& operator=(const const_tree_iterator& right)
    {
        prev    = right.prev;
        current = right.current;

        incream = right.incream;
        decream = right.decream;
        return *this;
    }

    bool operator==(const tree_iterator<type>& right) const
    {
        return current == right.current;
    }

    bool operator==(const const_tree_iterator& right) const
    {
        return current == right.current;
    }

    bool operator!=(const tree_iterator<type>& right) const
    {
        return !(*this == right);
    }

    bool operator!=(const const_tree_iterator& right) const
    {
        return !(*this == right);
    }

    const_tree_iterator& parent()
    {
        node_type grandparent = current->parent?current->parent->parent:NULL;
        return const_tree_iterator(grandparent, current->parent,
                                   &children_incream<value_type>,
                                   &children_decream<value_type>);
    }

    const_tree_iterator& begin()
    {
        return const_tree_iterator(current, current->children.front(),
                                   &children_incream<value_type>,
                                   &children_decream<value_type>);
    }

    const_tree_iterator& end()
    {
        return const_tree_iterator(current, NULL,
                                   &children_incream<value_type>,
                                   &children_decream<value_type>);
    }

    size_type size()
    {
        return current->children.size();
    }

    node_type *prev, *current;
    bool (*incream)(node_type*&, node_type*&);
    bool (*decream)(node_type*&, node_type*&);
};

template<typename type>
class tree
{
public:
    typedef type            value_type;
    typedef const type      const_type;
    typedef const type*     pointer_type;
    typedef type&           reference_type;
    typedef tree_node<type> node_type;
    typedef typename node_type::children_type::size_type
                            size_type;

    typedef tree_iterator<value_type>
                            iterator;
    typedef const_tree_iterator<value_type>
                            const_iterator;

    tree() :
        rootnode(value_type())
    {
    }

    tree(const value_type& value) :
        rootnode(value)
    {
    }

    ~tree()
    {
    }

    void clear()
    {

    }

    iterator root()
    {
        return iterator(NULL, &rootnode,
                        &children_incream<value_type>,
                        &children_decream<value_type>);
    }

    const_iterator root() const
    {
        return const_iterator(NULL, const_cast<node_type*>(&rootnode),
                              &children_incream<value_type>,
                              &children_decream<value_type>);
    }

protected:
    node_type rootnode;
};

#endif //__TREE_H__
