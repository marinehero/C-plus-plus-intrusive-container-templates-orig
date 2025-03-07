/*
Copyright (c) 2016 Walter William Karas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// Include once.
#ifndef ABSTRACT_CONTAINER_AVL_TREE_H_
#define ABSTRACT_CONTAINER_AVL_TREE_H_

// Abstract AVL Tree Template.
//
// See avl_tree.html for interface documentation.
//
// Version: 1.6
//
// NOTE: Within the implementation, it's generally more convenient to
// define the depth of the root node to be 0 (0-based depth) rather than
// 1 (1-based depth).

#include "bitset"

namespace abstract_container
{

#ifndef ABSTRACT_CONTAINER_SEARCH_TYPE_
#define ABSTRACT_CONTAINER_SEARCH_TYPE_

enum search_type
  {
    EQUAL = 1,
    equal = 1,
    LESS = 2,
    less = 2,
    GREATER = 4,
    greater = 4,
    LESS_EQUAL = equal | less,
    less_equal = equal | less,
    GREATER_EQUAL = equal | greater,
    greater_equal = equal | greater
  };

#endif

// The base_avl_tree template is the same as the avl_tree template,
// except for one additional template parameter: bset.  Here is the
// reference class for bset.
//
// class bset
//   {
//   public:
//
//     class ANY_bitref
//       {
//       public:
//         operator bool (void);
//         void operator = (bool b);
//       };
//
//     // Does not have to initialize bits.
//     bset(void);
//
//     // Must return a valid value for index when 0 <= index < max_depth
//     ANY_bitref operator [] (unsigned index);
//
//     // Set all bits to 1.
//     void set(void);
//
//     // Set all bits to 0.
//     void reset(void);
//   };
//
template <class abstractor, unsigned max_depth, class bset>
class base_avl_tree
  {
  public:

    typedef typename abstractor::key key;
    typedef typename abstractor::handle handle;
    typedef typename abstractor::size size;

    inline handle insert(handle h);

    inline handle search(key k, search_type st = EQUAL);
    inline handle search_least(void);
    inline handle search_greatest(void);

    inline handle remove(key k);

    inline handle subst(handle new_node);

    void purge(void) { abs.root = null(); }

    bool is_empty(void) { return(abs.root == null()); }

    bool read_error(void) { return(abs.read_error()); }

    base_avl_tree(void) { abs.root = null(); }

    base_avl_tree(const base_avl_tree &) = delete;

    base_avl_tree & operator = (const base_avl_tree &) = delete;

    class iter
      {
      public:

	// NOTE:  GCC allows these member functions to be defined as
	// explicitly inline outside the class, but Visual C++ .NET does
	// not.

	// Initialize depth to invalid value, to indicate iterator is
	// invalid.   (Depth is zero-base.)
	constexpr iter(void) : depth(unsigned(~0)) { }

	void start_iter(base_avl_tree &tree, key k, search_type st = EQUAL)
	  {
	    // Mask of high bit in an int.
	    const int MASK_HIGH_BIT = (int) ~ ((~ (unsigned) 0) >> 1);

	    // Save the tree that we're going to iterate through in a
	    // member variable.
	    tree_ = &tree;

	    int cmp, target_cmp;
	    handle h = tree_->abs.root;
	    unsigned d = 0;

	    depth = unsigned(~0);

	    if (h == null())
	      // Tree is empty.
	      return;

	    if (st & LESS)
	      // Key can be greater than key of starting node.
	      target_cmp = 1;
	    else if (st & GREATER)
	      // Key can be less than key of starting node.
	      target_cmp = -1;
	    else
	      // Key must be same as key of starting node.
	      target_cmp = 0;

	    for ( ; ; )
	      {
		cmp = cmp_k_n(k, h);
		if (cmp == 0)
		  {
		    if (st & EQUAL)
		      {
			// Equal node was sought and found as starting node.
			depth = d;
			break;
		      }
		    cmp = -target_cmp;
		  }
		else if (target_cmp != 0)
		  if (!((cmp ^ target_cmp) & MASK_HIGH_BIT))
		    // cmp and target_cmp are both negative or both positive.
		    depth = d;
		h = cmp < 0 ? get_lt(h) : get_gt(h);
		if (read_error())
		  {
		    depth = unsigned(~0);
		    break;
		  }
		if (h == null())
		  break;
		branch[d] = cmp > 0;
		path_h[d++] = h;
	      }
	  }

	void start_iter_least(base_avl_tree &tree)
	  {
	    tree_ = &tree;

	    handle h = tree_->abs.root;

	    depth = unsigned(~0);

	    branch.reset();

	    while (h != null())
	      {
		if (depth != unsigned(~0))
		  path_h[depth] = h;
		depth++;
		h = get_lt(h);
		if (read_error())
		  {
		    depth = unsigned(~0);
		    break;
		  }
	      }
	  }

	void start_iter_greatest(base_avl_tree &tree)
	  {
	    tree_ = &tree;

	    handle h = tree_->abs.root;

	    depth = unsigned(~0);

	    branch.set();

	    while (h != null())
	      {
		if (depth != unsigned(~0))
		  path_h[depth] = h;
		depth++;
		h = get_gt(h);
		if (read_error())
		  {
		    depth = unsigned(~0);
		    break;
		  }
	      }
	  }

	handle operator * (void)
	  {
	    if (depth == unsigned(~0))
	      return(null());

	    return(depth == 0 ? tree_->abs.root : path_h[depth - 1]);
	  }

	void operator ++ (void)
	  {
	    if (depth != unsigned(~0))
	      {
		handle h = get_gt(**this);
		if (read_error())
		  depth = unsigned(~0);
		else if (h == null())
		  do
		    {
		      if (depth == 0)
			{
			  depth = unsigned(~0);
			  break;
			}
		      depth--;
		    }
		  while (branch[depth]);
		else
		  {
		    branch[depth] = true;
		    path_h[depth++] = h;
		    for ( ; ; )
		      {
			h = get_lt(h);
			if (read_error())
			  {
			    depth = unsigned(~0);
			    break;
			  }
			if (h == null())
			  break;
			branch[depth] = false;
			path_h[depth++] = h;
		      }
		  }
	      }
	  }

	void operator -- (void)
	  {
	    if (depth != unsigned(~0))
	      {
		handle h = get_lt(**this);
		if (read_error())
		  depth = unsigned(~0);
		else if (h == null())
		  do
		    {
		      if (depth == 0)
			{
			  depth = unsigned(~0);
			  break;
			}
		      depth--;
		    }
		  while (!branch[depth]);
		else
		  {
		    branch[depth] = false;
		    path_h[depth++] = h;
		    for ( ; ; )
		      {
			h = get_gt(h);
			if (read_error())
			  {
			    depth = unsigned(~0);
			    break;
			  }
			if (h == null())
			  break;
			branch[depth] = true;
			path_h[depth++] = h;
		      }
		  }
	      }
	  }

	void operator ++ (int) { ++(*this); }

	void operator -- (int) { --(*this); }

	bool read_error(void) { return(tree_->read_error()); }

      protected:

	// Tree being iterated over.
	base_avl_tree *tree_;

	// Records a path into the tree.  If branch[n] is true, indicates
	// take greater branch from the nth node in the path, otherwise
	// take the less branch.  branch[0] gives branch from root, and
	// so on.
	bset branch;

	// Zero-based depth of path into tree.
	unsigned depth;

	// Handles of nodes in path from root to current node (returned by *).
	handle path_h[max_depth - 1];

	int cmp_k_n(key k, handle h)
	  { return(tree_->abs.compare_key_node(k, h)); }
	int cmp_n_n(handle h1, handle h2)
	  { return(tree_->abs.compare_node_node(h1, h2)); }
	handle get_lt(handle h)
	  { return(tree_->abs.get_less(h, true)); }
	handle get_gt(handle h)
	  { return(tree_->abs.get_greater(h, true)); }
	handle null(void) { return(tree_->abs.null()); }

      };

    template<typename fwd_iter>
    bool build(fwd_iter p, size num_nodes)
      {
	// NOTE:  GCC allows me to define this outside the class definition
	// using the following syntax:
	//
	// template <class abstractor, unsigned max_depth, class bset>
	// template<typename fwd_iter>
	// inline void base_avl_tree<abstractor, max_depth, bset>::build(
	//   fwd_iter p, size num_nodes)
	//   {
	//     ...
	//   }
	//
	// but Visual C++ .NET won't accept it.  Is this a GCC extension?

	if (num_nodes == 0)
	  {
	    abs.root = null();
	    return(true);
	  }

	// Gives path to subtree being built.  If branch[N] is false, branch
	// less from the node at depth N, if true branch greater.
	bset branch;

	// If rem[N] is true, then for the current subtree at depth N, it's
	// greater subtree has one more node than it's less subtree.
	bset rem;

        // Depth of root node of current subtree.
	unsigned depth = 0;

        // Number of nodes in current subtree.
	size num_sub = num_nodes;

	// The algorithm relies on a stack of nodes whose less subtree has
	// been built, but whose right subtree has not yet been built.  The
	// stack is implemented as linked list.  The nodes are linked
	// together by having the "greater" handle of a node set to the
	// next node in the list.  "less_parent" is the handle of the first
	// node in the list.
	handle less_parent = null();

	// h is root of current subtree, child is one of its children.
	handle h, child;

	for ( ; ; )
	  {
	    while (num_sub > 2)
	      {
		// Subtract one for root of subtree.
		num_sub--;
		rem[depth] = !!(num_sub & 1);
		branch[depth++] = false;
		num_sub >>= 1;
	      }

	    if (num_sub == 2)
	      {
		// Build a subtree with two nodes, slanting to greater.
		// I arbitrarily chose to always have the extra node in the
		// greater subtree when there is an odd number of nodes to
		// split between the two subtrees.

		h = *p;
		if (read_error())
		  return(false);
		p++;
		child = *p;
		if (read_error())
		  return(false);
		p++;
		set_lt(child, null());
		set_gt(child, null());
		set_bf(child, 0);
		set_gt(h, child);
		set_lt(h, null());
		set_bf(h, 1);
	      }
	    else  // num_sub == 1
	      {
		// Build a subtree with one node.

		h = *p;
		if (read_error())
		  return(false);
		p++;
		set_lt(h, null());
		set_gt(h, null());
		set_bf(h, 0);
	      }

	    while (depth)
	      {
		depth--;
		if (!branch[depth])
		  // We've completed a less subtree.
		  break;

		// We've completed a greater subtree, so attach it to
		// its parent (that is less than it).  We pop the parent
		// off the stack of less parents.
		child = h;
		h = less_parent;
		less_parent = get_gt(h);
		if (read_error())
		  return(false);
		set_gt(h, child);
		// num_sub = 2 * (num_sub - rem[depth]) + rem[depth] + 1
		num_sub <<= 1;
		num_sub += 1 - rem[depth];
		if (num_sub & (num_sub - 1))
		  // num_sub is not a power of 2
		  set_bf(h, 0);
		else
		  // num_sub is a power of 2
		  set_bf(h, 1);
	      }

	    if (num_sub == num_nodes)
	      // We've completed the full tree.
	      break;

	    // The subtree we've completed is the less subtree of the
	    // next node in the sequence.

	    child = h;
	    h = *p;
	    if (read_error())
	      return(false);
	    p++;
	    set_lt(h, child);

	    // Put h into stack of less parents.
	    set_gt(h, less_parent);
	    less_parent = h;

	    // Proceed to creating greater than subtree of h.
	    branch[depth] = true;
	    num_sub += rem[depth++];

	  } // end for ( ; ; )

	abs.root = h;

	return(true);
      }

  protected:

    friend class iter;

    // Create a class whose sole purpose is to take advantage of
    // the "empty member" optimization.
    struct abs_plus_root : public abstractor
      {
	// The handle of the root element in the AVL tree.
	handle root;
      };

    abs_plus_root abs;


    handle get_lt(handle h, bool access = true)
      { return(abs.get_less(h, access)); }
    void set_lt(handle h, handle lh) { abs.set_less(h, lh); }

    handle get_gt(handle h, bool access = true)
      { return(abs.get_greater(h, access)); }
    void set_gt(handle h, handle gh) { abs.set_greater(h, gh); }

    int get_bf(handle h) { return(abs.get_balance_factor(h)); }
    void set_bf(handle h, int bf) { abs.set_balance_factor(h, bf); }

    int cmp_k_n(key k, handle h) { return(abs.compare_key_node(k, h)); }
    int cmp_n_n(handle h1, handle h2)
      { return(abs.compare_node_node(h1, h2)); }

    handle null(void) { return(abs.null()); }

  private:

    // Balances subtree, returns handle of root node of subtree
    // after balancing.
    handle balance(handle bal_h)
      {
	handle deep_h;

	// Either the "greater than" or the "less than" subtree of
	// this node has to be 2 levels deeper (or else it wouldn't
	// need balancing).

	if (get_bf(bal_h) > 0)
	  {
	    // "Greater than" subtree is deeper.

	    deep_h = get_gt(bal_h);
	    if (read_error())
	      return(null());

	    if (get_bf(deep_h) < 0)
	      {
		handle old_h = bal_h;
		bal_h = get_lt(deep_h);
		if (read_error())
		  return(null());
		set_gt(old_h, get_lt(bal_h, false));
		set_lt(deep_h, get_gt(bal_h, false));
		set_lt(bal_h, old_h);
		set_gt(bal_h, deep_h);

		int bf = get_bf(bal_h);
		if (bf != 0)
		  {
		    if (bf > 0)
		      {
			set_bf(old_h, -1);
			set_bf(deep_h, 0);
		      }
		    else
		      {
			set_bf(deep_h, 1);
			set_bf(old_h, 0);
		      }
		    set_bf(bal_h, 0);
		  }
		else
		  {
		    set_bf(old_h, 0);
		    set_bf(deep_h, 0);
		  }
	      }
	    else
	      {
		set_gt(bal_h, get_lt(deep_h, false));
		set_lt(deep_h, bal_h);
		if (get_bf(deep_h) == 0)
		  {
		    set_bf(deep_h, -1);
		    set_bf(bal_h, 1);
		  }
		else
		  {
		    set_bf(deep_h, 0);
		    set_bf(bal_h, 0);
		  }
		bal_h = deep_h;
	      }
	  }
	else
	  {
	    // "Less than" subtree is deeper.

	    deep_h = get_lt(bal_h);
	    if (read_error())
	      return(null());

	    if (get_bf(deep_h) > 0)
	      {
		handle old_h = bal_h;
		bal_h = get_gt(deep_h);
		if (read_error())
		  return(null());
		set_lt(old_h, get_gt(bal_h, false));
		set_gt(deep_h, get_lt(bal_h, false));
		set_gt(bal_h, old_h);
		set_lt(bal_h, deep_h);

		int bf = get_bf(bal_h);
		if (bf != 0)
		  {
		    if (bf < 0)
		      {
			set_bf(old_h, 1);
			set_bf(deep_h, 0);
		      }
		    else
		      {
			set_bf(deep_h, -1);
			set_bf(old_h, 0);
		      }
		    set_bf(bal_h, 0);
		  }
		else
		  {
		    set_bf(old_h, 0);
		    set_bf(deep_h, 0);
		  }
	      }
	    else
	      {
		set_lt(bal_h, get_gt(deep_h, false));
		set_gt(deep_h, bal_h);
		if (get_bf(deep_h) == 0)
		  {
		    set_bf(deep_h, 1);
		    set_bf(bal_h, -1);
		  }
		else
		  {
		    set_bf(deep_h, 0);
		    set_bf(bal_h, 0);
		  }
		bal_h = deep_h;
	      }
	  }

	return(bal_h);
      }

  };

template <class abstractor, unsigned max_depth, class bset>
inline auto base_avl_tree<abstractor, max_depth, bset>::insert(handle h)
  -> handle
  {
    set_lt(h, null());
    set_gt(h, null());
    set_bf(h, 0);

    if (abs.root == null())
      abs.root = h;
    else
      {
	// Last unbalanced node encountered in search for insertion point.
	handle unbal = null();
	// Parent of last unbalanced node.
	handle parent_unbal = null();
	// Balance factor of last unbalanced node.
	int unbal_bf;

	// Zero-based depth in tree.
	unsigned depth = 0, unbal_depth = 0;

	// Records a path into the tree.  If branch[n] is true, indicates
	// take greater branch from the nth node in the path, otherwise
	// take the less branch.  branch[0] gives branch from root, and
	// so on.
	bset branch;

	handle hh = abs.root;
	handle parent = null();
	int cmp;

	do
 	  {
	    if (get_bf(hh) != 0)
	      {
		unbal = hh;
		parent_unbal = parent;
		unbal_depth = depth;
	      }
	    cmp = cmp_n_n(h, hh);
	    if (cmp == 0)
	      // Duplicate key.
	      return(hh);
	    parent = hh;
	    hh = cmp < 0 ? get_lt(hh) : get_gt(hh);
	    if (read_error())
	      return(null());
	    branch[depth++] = cmp > 0;
	  }
	while (hh != null());

	//  Add node to insert as leaf of tree.
	if (cmp < 0)
	  set_lt(parent, h);
	else
	  set_gt(parent, h);

	depth = unbal_depth;

	if (unbal == null())
	  hh = abs.root;
	else
	  {
	    cmp = branch[depth++] ? 1 : -1;
	    unbal_bf = get_bf(unbal);
	    if (cmp < 0)
	      unbal_bf--;
	    else  // cmp > 0
	      unbal_bf++;
	    hh = cmp < 0 ? get_lt(unbal) : get_gt(unbal);
	    if (read_error())
	      return(null());
	    if ((unbal_bf != -2) && (unbal_bf != 2))
	      {
		// No rebalancing of tree is necessary.
		set_bf(unbal, unbal_bf);
		unbal = null();
	      }
	  }

	if (hh != null())
	  while (h != hh)
	    {
	      cmp = branch[depth++] ? 1 : -1;
	      if (cmp < 0)
		{
		  set_bf(hh, -1);
		  hh = get_lt(hh);
		}
	      else // cmp > 0
		{
		  set_bf(hh, 1);
		  hh = get_gt(hh);
		}
	      if (read_error())
		return(null());
	    }

	if (unbal != null())
	  {
	    unbal = balance(unbal);
	    if (read_error())
	      return(null());
	    if (parent_unbal == null())
	      abs.root = unbal;
	    else
	      {
		depth = unbal_depth - 1;
		cmp = branch[depth] ? 1 : -1;
		if (cmp < 0)
		  set_lt(parent_unbal, unbal);
		else  // cmp > 0
		  set_gt(parent_unbal, unbal);
	      }
	  }

      }

    return(h);
  }

template <class abstractor, unsigned max_depth, class bset>
inline auto
  base_avl_tree<abstractor, max_depth, bset>::search(key k, search_type st)
  -> handle
  {
    const int MASK_HIGH_BIT = (int) ~ ((~ (unsigned) 0) >> 1);

    int cmp, target_cmp;
    handle match_h = null();
    handle h = abs.root;

    if (st & LESS)
      target_cmp = 1;
    else if (st & GREATER)
      target_cmp = -1;
    else
      target_cmp = 0;

    while (h != null())
      {
	cmp = cmp_k_n(k, h);
	if (cmp == 0)
	  {
	    if (st & EQUAL)
	      {
		match_h = h;
		break;
	      }
	    cmp = -target_cmp;
	  }
	else if (target_cmp != 0)
	  if (!((cmp ^ target_cmp) & MASK_HIGH_BIT))
	    // cmp and target_cmp are both positive or both negative.
	    match_h = h;
	h = cmp < 0 ? get_lt(h) : get_gt(h);
	if (read_error())
	  {
	    match_h = null();
	    break;
	  }
      }

    return(match_h);
  }

template <class abstractor, unsigned max_depth, class bset>
inline auto
  base_avl_tree<abstractor, max_depth, bset>::search_least(void) -> handle
  {
    handle h = abs.root, parent = null();

    while (h != null())
      {
	parent = h;
	h = get_lt(h);
	if (read_error())
	  {
	    parent = null();
	    break;
	  }
      }

    return(parent);
  }

template <class abstractor, unsigned max_depth, class bset>
inline auto
  base_avl_tree<abstractor, max_depth, bset>::search_greatest(void) -> handle
  {
    handle h = abs.root, parent = null();

    while (h != null())
      {
	parent = h;
	h = get_gt(h);
	if (read_error())
	  {
	    parent = null();
	    break;
	  }
      }

    return(parent);
  }

template <class abstractor, unsigned max_depth, class bset>
inline auto
  base_avl_tree<abstractor, max_depth, bset>::remove(key k) -> handle
  {
    // Zero-based depth in tree.
    unsigned depth = 0, rm_depth;

    // Records a path into the tree.  If branch[n] is true, indicates
    // take greater branch from the nth node in the path, otherwise
    // take the less branch.  branch[0] gives branch from root, and
    // so on.
    bset branch;

    handle h = abs.root;
    handle parent = null(), child;
    int cmp, cmp_shortened_sub_with_path;

    for ( ; ; )
      {
	if (h == null())
	  // No node in tree with given key.
	  return(null());
	cmp = cmp_k_n(k, h);
	if (cmp == 0)
	  // Found node to remove.
	  break;
	parent = h;
	h = cmp < 0 ? get_lt(h) : get_gt(h);
	if (read_error())
	  return(null());
	branch[depth++] = cmp > 0;
	cmp_shortened_sub_with_path = cmp;
      }
    handle rm = h;
    handle parent_rm = parent;
    rm_depth = depth;

    // If the node to remove is not a leaf node, we need to get a
    // leaf node, or a node with a single leaf as its child, to put
    // in the place of the node to remove.  We will get the greatest
    // node in the less subtree (of the node to remove), or the least
    // node in the greater subtree.  We take the leaf node from the
    // deeper subtree, if there is one.

    if (get_bf(h) < 0)
      {
	child = get_lt(h);
	branch[depth] = false;
	cmp = -1;
      }
    else
      {
	child = get_gt(h);
	branch[depth] = true;
	cmp = 1;
      }
    if (read_error())
      return(null());
    depth++;

    if (child != null())
      {
	cmp = -cmp;
	do
	  {
	    parent = h;
	    h = child;
	    if (cmp < 0)
	      {
		child = get_lt(h);
		branch[depth] = false;
	      }
	    else
	      {
		child = get_gt(h);
		branch[depth] = true;
	      }
	    if (read_error())
	      return(null());
	    depth++;
	  }
	while (child != null());

	if (parent == rm)
	  // Only went through do loop once.  Deleted node will be replaced
	  // in the tree structure by one of its immediate children.
	  cmp_shortened_sub_with_path = -cmp;
        else
	  cmp_shortened_sub_with_path = cmp;

	// Get the handle of the opposite child, which may not be null.
	child = cmp > 0 ? get_lt(h, false) : get_gt(h, false);
      }

    if (parent == null())
      // There were only 1 or 2 nodes in this tree.
      abs.root = child;
    else if (cmp_shortened_sub_with_path < 0)
      set_lt(parent, child);
    else
      set_gt(parent, child);

    // "path" is the parent of the subtree being eliminated or reduced
    // from a depth of 2 to 1.  If "path" is the node to be removed, we
    // set path to the node we're about to poke into the position of the
    // node to be removed.
    handle path = parent == rm ? h : parent;

    if (h != rm)
      {
	// Poke in the replacement for the node to be removed.
	set_lt(h, get_lt(rm, false));
	set_gt(h, get_gt(rm, false));
	set_bf(h, get_bf(rm));
	if (parent_rm == null())
	  abs.root = h;
	else
	  {
	    depth = rm_depth - 1;
	    if (branch[depth])
	      set_gt(parent_rm, h);
	    else
	      set_lt(parent_rm, h);
	  }
      }

    if (path != null())
      {
	// Create a temporary linked list from the parent of the path node
	// to the root node.
	h = abs.root;
	parent = null();
	depth = 0;
	while (h != path)
	  {
	    if (branch[depth++])
	      {
	        child = get_gt(h);
		set_gt(h, parent);
	      }
	    else
	      {
	        child = get_lt(h);
		set_lt(h, parent);
	      }
	    if (read_error())
	      return(null());
	    parent = h;
	    h = child;
	  }

	// Climb from the path node to the root node using the linked
	// list, restoring the tree structure and rebalancing as necessary.
	bool reduced_depth = true;
	int bf;
	cmp = cmp_shortened_sub_with_path;
	for ( ; ; )
	  {
	    if (reduced_depth)
	      {
		bf = get_bf(h);
		if (cmp < 0)
		  bf++;
		else  // cmp > 0
		  bf--;
		if ((bf == -2) || (bf == 2))
		  {
		    h = balance(h);
		    if (read_error())
		      return(null());
		    bf = get_bf(h);
		  }
		else
		  set_bf(h, bf);
		reduced_depth = (bf == 0);
	      }
	    if (parent == null())
	      break;
	    child = h;
	    h = parent;
	    cmp = branch[--depth] ? 1 : -1;
	    if (cmp < 0)
	      {
		parent = get_lt(h);
		set_lt(h, child);
	      }
	    else
	      {
		parent = get_gt(h);
		set_gt(h, child);
	      }
	    if (read_error())
	      return(null());
	  }
	abs.root = h;
      }

    return(rm);
  }

template <class abstractor, unsigned max_depth, class bset>
inline auto
  base_avl_tree<abstractor, max_depth, bset>::subst(handle new_node) -> handle
  {
    handle h = abs.root;
    handle parent = null();
    int cmp, last_cmp;

    /* Search for node already in tree with same key. */
    for ( ; ; )
      {
	if (h == null())
	  /* No node in tree with same key as new node. */
	  return(null());
	cmp = cmp_n_n(new_node, h);
	if (cmp == 0)
	  /* Found the node to substitute new one for. */
	  break;
	last_cmp = cmp;
	parent = h;
	h = cmp < 0 ? get_lt(h) : get_gt(h);
	if (read_error())
	  return(null());
      }

    /* Copy tree housekeeping fields from node in tree to new node. */
    set_lt(new_node, get_lt(h, false));
    set_gt(new_node, get_gt(h, false));
    set_bf(new_node, get_bf(h));

    if (parent == null())
      /* New node is also new root. */
      abs.root = new_node;
    else
      {
	/* Make parent point to new node. */
	if (last_cmp < 0)
	  set_lt(parent, new_node);
	else
	  set_gt(parent, new_node);
      }

    return(h);
  }

// I tried to avoid having a separate base_avl_tree template by having
// bitset<max_depth> be the default for the bset template, but Visual
// C++ would not permit this.  It may possibly be desirable to use
// base_avl_tree directly with an optimized version of bset.
//
template <class abstractor, unsigned max_depth = 32>
class avl_tree
  : public base_avl_tree<abstractor, max_depth, std::bitset<max_depth> >
  { };

} // end namespace abstract_container

#endif
