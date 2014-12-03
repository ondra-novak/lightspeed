/*
 * avltree.h
 *
 *  Created on: 30.8.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_CONTAINERS_AVLTREE_H_
#define LIGHTSPEED_CONTAINERS_AVLTREE_H_

#include "avltreenode.h"
#include "../memory/sharedPtr.h"
#include "../memory/clusterAlloc.h"
#include "../memory/nodeAlloc.h"

namespace LightSpeed {

	class IRuntimeAlloc;



	template<typename T, typename Cmp, typename Traits = AvlTreeNodeTraits >
	class AVLTree {

	public:


		class Node;
		class DataNode {
		public:
			DataNode(const T &data):data(data) {}
			const T &getData() const {return data;}
			T &getData() {return data;}
		protected:
			friend class Node;
			T data;
		};

		class Node:  public AvlTreeNode<Traits>,public DataNode {
		public:
			Node(const T &data):DataNode(data) {}

		public:
			///Converts T to Node usable for search
			/** AVLTree can only compare nodes. While searching,
			   Node's internal fields are not used for searched node.
			   So to reduce copying and reconstructing whole searched value,
			   you can use this function to convert searched key
			   to searchNode */
			static const Node &getSearchNode(const T &value) {


				natural offs = reinterpret_cast<const byte *>(
						&(reinterpret_cast<const DataNode *>(&value)->data))
						 - reinterpret_cast<const byte *>(&value);
				const DataNode *dn_ptr = reinterpret_cast<const DataNode *>(
						reinterpret_cast<const byte *>(&value) - offs);
				return *static_cast<const Node *>(dn_ptr);
			}			
		};


		typedef Node *PNode;

		class NodeCmp {
		public:
			NodeCmp(const Cmp &cmp):cmp(cmp) {}
			bool operator()(const AvlTreeNode<Traits> *a, const AvlTreeNode<Traits> *b) const {
				return cmp(static_cast<const Node *>(a)->getData(),
						   static_cast<const Node *>(b)->getData());
			}
		protected:
			const Cmp &cmp;
		};

		class Iterator: public IteratorBase<T, Iterator> {
		public:

			Iterator() {}
			Iterator(PNode nd, Direction::Type dir):nditer(nd,dir) {}
			Iterator(const Iterator &other,
						Direction::Type dir = Direction::undefined)
				:nditer(other.nditer,dir) {}
			Iterator(const Cmp &cmp, PNode root, const Node *search,
					Direction::Type dir, bool *found = 0)
				:nditer(NodeCmp(cmp),root,search,dir,found) {}
			Iterator(AvlTreeNode<Traits> **path, natural len, Direction::Type dir)
				:nditer(path,len,dir) {}
			Iterator(const typename AvlTreeNode<Traits>::Iterator &nditer):nditer(nditer) {}

			bool hasItems() const {return nditer.hasItems();}

			const T &getNext()  {
				AvlTreeNode<Traits> *nd = nditer.getNext();
				return static_cast<Node *>(nd)->getData();
			}
			const T &peek() const {
				AvlTreeNode<Traits> *nd = nditer.peek();
				return static_cast<Node *>(nd)->getData();
			}

			bool equalTo(const Iterator &iter) const {
				return nditer.equalTo(iter.nditer);
			}
			bool lessThan(const Iterator &iter) const {
				return nditer.lessThan(iter.nditer);
			}

			bool setDir(Direction::Type e) {
				return nditer.setDir(e);
			}
			Direction::Type getDir() const {
				return nditer.getDir();
			}


			Node *peekNode() const {
				AvlTreeNode<Traits> *nd = nditer.peek();
				return static_cast<Node *>(nd);
			}

			typename AvlTreeNode<Traits>::Iterator &getNodeIterator() {return nditer;}
			const typename AvlTreeNode<Traits>::Iterator &getNodeIterator() const {return nditer;}
		protected:
			typename AvlTreeNode<Traits>::Iterator nditer;
		};

		class WriteIterator: public WriteIteratorBase<T, WriteIterator> {
		public:
			WriteIterator(AVLTree &owner):owner(owner) {}

			void write(const T &val) {owner.insert(val);}

			bool hasItems() const {return true;}
		protected:
			AVLTree &owner;
		};

		AVLTree():tree(0),allocFactory(createDefaultNodeAllocator()),count(0) {}
		AVLTree(const Cmp &cmp, NodeAlloc alloc)
			:tree(0),allocFactory(alloc),cmpOper(cmp),count(0) {}
		AVLTree(const Cmp &cmp)
			:tree(0),allocFactory(createDefaultNodeAllocator()),cmpOper(cmp),count(0) {}

		AVLTree(const AVLTree &other)
			:tree(0),
			 allocFactory(other.allocFactory),
			 cmpOper(other.cmpOper),
			 count(0) {
			insert(other);
		}

		AVLTree &operator=(const AVLTree &other) {
			clear();
			insert(other);
			return *this;
		}

		~AVLTree() {
			clear();
		}

		WriteIterator getWriteIterator() {
			return WriteIterator(*this);
		}

		///Adds value into tree
		/**
		 * @param value new value
		 * @param exist optional argument is set to true, if item is already in
		 *   the tree.
		 * @return iterator, where item has been placed
		 */
		Iterator insert(const T &value, bool *exist = 0) {
			class AutoRemove {
			public:
				AutoRemove(Node *k, bool *ex, IRuntimeAlloc *fact)
					:k(k),ex(ex),fact(fact) {}
				~AutoRemove() {
					if (*ex) {fact->destroyInstance(k);}
				}
			protected:
				Node *k;
				bool *ex;
				IRuntimeAlloc *fact;
			};

			bool ex;
			if (exist == 0) exist = &ex;
			Node *k = allocFactory->createInstanceUsing<Node,T>(value);
			*exist = true;
			AutoRemove z(k,exist,allocFactory);
			return insert(k,exist);
		}


		///Adds node created outside of tree
		/**
		 * @param nd new node, it must be created by factory to allow delete it later. Or
		 * 	 caller must remove the node before tree is destroyed
		 * @param exist optional argument is set to true, if item is already in
		 *   the tree. In this case, new node is not added and you should perform
		 *   correct cleaning (node is not destroyed)
		 * @return iterator, where item has been placed
		 */
		Iterator insert(Node *nd, bool *exist = 0) {
			typename AvlTreeNode<Traits>::Iterator itr;
			bool done = false;
			AvlTreeNode<Traits> *newroot = tree->insert(NodeCmp(cmpOper),nd,done,&itr);
			if (newroot == 0) {
				if (*exist) *exist = true;
			} else {
				tree = static_cast<Node *>(newroot);
				count++;
				if (*exist) *exist = false;
			}
			return Iterator(itr);
		}


		///Merges second tree to current tree
		/**
		 * @param otherTree other tree
		 */
		void insert(const AVLTree &otherTree) {
			Iterator src = otherTree.getFwIter();
			WriteIterator trg = getWriteIterator();
			trg.copy(src);

		}


		///Searches for item
		/**
		 * @param value Item to search.
		 * @return Pointer to found item, or NULL, if not exist
		 */
		const T *find(const T &value) const {
			const Node &srch = Node::getSearchNode(value);
			bool found = false;
			Iterator itr(cmpOper,tree,&srch,Direction::forward,&found);
			if (found) return &itr.getNext();
			else return 0;
		}


		///Searches for node
		/**
		 * @param nd pointer to node to search.
		 * @return found node or null
		 */
		const Node *find(const Node *nd) const {
			bool found = false;
			Iterator itr(cmpOper,tree,const_cast<Node *>(nd),Direction::forward,&found);
			if (found) return itr.peekNode();
			else return 0;
		}

		///seeks for value
		/**
		 * @param value value to seek
		 * @param found optional argument receives true, when item has been found
		 * @param direction for iterator
		 * @return Iterator which points to item
		 */
		Iterator seek(const T &value, bool *found = 0,
				Direction::Type dir = Direction::forward) const {
			const Node &nd = Node::getSearchNode(value);
			return Iterator(cmpOper,tree,&nd,dir,found);
		}

		///seeks for node
		/**
		 * @param node node to seek
		 * @param found optional argument receives true, when item has been found
		 * @param direction for iterator
		 * @return Iterator which points to item
		 */

		Iterator seek(const Node *nd, bool *found = 0,
				Direction::Type dir = Direction::forward) const {
			return Iterator(cmpOper,tree,const_cast<Node *>(nd),dir,found);
		}


		///Erases the value from the tree
		/**
		 * @param value value to erase
		 * @retval true found and erased
		 * @retval false not found
		 */
		bool erase(const T &value) {
			const Node &nd = Node::getSearchNode(value);
			Node *res = remove(&nd);
			if (res) {
				allocFactory->destroyInstance(res);
				return true;
			} else {
				return false;
			}
		}

		///Removes the node
		/**
		 * @param nd node to erase
		 * @return pointer to erased node, or NULL, if not found.
		 *
		 * @note function doesn't release memory for the node. Returned
		 * pointer must be deallocated manually
		 */
		Node *remove(const Node *nd) {
			bool done = false;
			AvlTreeNode<Traits> *removedNode;
			AvlTreeNode<Traits> *nr = tree->remove(NodeCmp(cmpOper),nd,done,removedNode);
			tree = static_cast<Node *>(nr);
			if (removedNode) count --;
			return static_cast<Node *>(removedNode);

		}

		///Removes node referenced by iterator
		/**
		 * @param iter iterator that points to a node
		 * @return node that has been removed, must be deallocated
		 */
		Node *remove(Iterator &iter) {
			bool done = false;
			Node *nd = iter.peekNode();
			typename AvlTreeNode<Traits>::Iterator &nditer = iter.getNodeIterator();
			tree = static_cast<Node *>(tree->remove(nditer,done));
			count--;
			return nd;
		}

		///erases value referned by iterator
		/**
		 * @param iter iterator that refers the item
		 * Iterator is modified after erase to point
		 * next valid item
		 */

		void erase(Iterator &iter) {
			Node *nd = remove(iter);
			allocFactory->destroyInstance(nd);

		}

		///Erases all items in tree
		void clear() {
			clearNode(tree);
			tree = 0;
			count = 0;
		}

		///determines, whether tree is empty
		/**
		 * @retval true tree is empty
		 * @retval fasle tree is not empty
		 */
		bool empty() const {
			return tree == 0;
		}

		///Retrieves count of items in the tree
		natural size() const {
			return count;
		}

		///Retrieves iterator to walk all items in order
		Iterator getFwIter() const {
			return Iterator(tree,Direction::forward);
		}

		///Retrieves iterator to walk all items in backward oorder
		Iterator getBkIter() const {
			return Iterator(tree,Direction::backward);
		}

		template<typename Arch>
		void serialize(Arch &arch) {
			typename Arch::Array arr(arch,size());
			if (arch.storing()) {
				Iterator x = getFwIter();
				while (x.hasItems() && arr.next())
					arch << x.getNext();
			} else {
				clear();
				while (arr.next()) {
					T itm;
					arch >> itm;
					insert(itm);
				}
			}
		}

		NodeAlloc getFactory() const {return allocFactory;}

		void swap(AVLTree &other) {
			std::swap(tree, other.tree);
			std::swap(allocFactory,other.allocFactory);
			std::swap(cmpOper,other.cmpOper);
			std::swap(count,other.count);
		}

	protected:
		Node *tree;
		NodeAlloc allocFactory;
		Cmp cmpOper;
		natural count;

		void clearNode(Node *tree) {
			if (tree == 0) return;
			clearNode(static_cast<Node *>(tree->getLeft()));
			clearNode(static_cast<Node *>(tree->getRight()));
			allocFactory->destroyInstance(tree);
		}


	};
}

#endif /* AVLTREE_H_ */
