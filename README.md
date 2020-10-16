# C-plus-plus-intrusive-container-templates
C++ intrusive container templates.  Abstract node links, no use of
new/delete (AVL tree, singly-linked list, bidirection list, hash table
available currently).

Also look at boost::instrusive, which is STL-compatible.  Links under the
Boost approach are unabstracted pointers.  There is no function to build
a balanced binary tree from a sequence, but you could port mine into the
Boost lib pretty easily.
