/*------------------------------------------------------------------------------
 * Copyright (c) 2017-2019
 *     Michael Theall (mtheall)
 *
 * This file is part of tex3ds.
 *
 * tex3ds is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * tex3ds is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with tex3ds.  If not, see <http://www.gnu.org/licenses/>.
 *----------------------------------------------------------------------------*/
/** @file huff.c
 *  @brief Huffman compression routines
 */

#include "compress.h"
#include "future.h"

#include <algorithm>
#include <bitset>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <queue>

namespace
{
/** @brief Bitmap */
typedef std::bitset<512> Bitmap;

size_t find (const Bitmap &bitmap, size_t i)
{
	for (; i < bitmap.size (); ++i)
	{
		if (!bitmap.test (i))
			return i;
	}

	std::abort ();
}

/** @brief Print a Huffman code
 *  @param[in] code    Huffman code
 *  @param[in] codeLen Huffman code length (bits)
 */
void printCode (uint32_t code, size_t codeLen)
{
	for (size_t i = 0; i < codeLen; ++i)
		std::fputc ('0' + ((code >> (codeLen - i - 1)) & 1), stdout);
}

/** @brief Huffman node */
class Node
{
public:
	~Node ()
	{
	}

	Node (uint8_t val, size_t count) : count (count), val (val)
	{
	}

	Node (std::unique_ptr<Node> &&left, std::unique_ptr<Node> &&right)
	    : child{std::move (left), std::move (right)}, count (child[0]->count + child[1]->count)
	{
		// set children's parent to self
		child[0]->parent = this;
		child[1]->parent = this;
	}

	Node ()                  = delete;
	Node (const Node &other) = delete;
	Node (Node &&other)      = delete;
	Node &operator= (const Node &other) = delete;
	Node &operator= (Node &&other) = delete;

	bool operator< (const Node &other) const
	{
		// major key is count
		if (count != other.count)
			return count < other.count;

		// minor key is value
		return val < other.val;
	}

#if 1
	/** @brief Print node
	 *  @param[in] indent Indentation level
	 */
	void print (int indent) const
	{
		if (child[0])
			std::printf ("%*s----: %zu\n", indent, "", count);
		else
		{
			std::printf ("%*s0x%02x: %zu ", indent, "", val, count);
			for (size_t j = 0; j < codeLen; ++j)
				std::fputc ('0' + ((code >> (codeLen - j - 1)) & 1), stdout);
			std::fputc ('\n', stdout);
		}
	}

	/** @brief Print nodes and their children
	 *  @param[in] nodes    Nodes to print
	 *  @param[in] numNodes Number of nodes
	 *  @param[in] indent   Indentation level
	 */
	static void printNodes (std::unique_ptr<Node> nodes[2], size_t numNodes, int indent)
	{
		// print each node
		for (size_t i = 0; i < numNodes; ++i)
		{
			if (nodes[i])
				nodes[i]->print (indent);
		}

		// print the children
		for (size_t i = 0; i < numNodes; ++i)
		{
			if (nodes[i] && nodes[i]->child[0])
				printNodes (nodes[i]->child, 2, indent + 2);
		}
	}
#endif

	/** @brief Print a Huffman tree
	 *  @param[in] code    Huffman code
	 *  @param[in] codeLen Huffman code length (bits)
	 */
	void printTree (uint32_t code, size_t codeLen)
	{
		if (!child[0])
		{
			// this is a data node; print the data
			std::printf ("0x%02x: ", val);
			printCode (code, codeLen);
			std::fputc ('\n', stdout);
		}
		else
		{
			// print the children
			child[0]->printTree ((code << 1) | 0, codeLen + 1);
			child[1]->printTree ((code << 1) | 1, codeLen + 1);
		}
	}

	/** @brief Build Huffman codes
	 *  @param[in] node    Huffman node
	 *  @param[in] code    Huffman code
	 *  @param[in] codeLen Huffman code length (bits)
	 */
	static void buildCodes (std::unique_ptr<Node> &node, uint32_t code, size_t codeLen);

	/** @brief Build lookup table
	 *  @param[in] nodes Table to fill
	 *  @param[in] n     Huffman node
	 */
	static void buildLookup (std::vector<Node *> &nodes, const std::unique_ptr<Node> &node);

	/** @brief Encode Huffman tree
	 *  @param[in] tree   Huffman tree
	 *  @param[in] node   Huffman node
	 *  @param[in] bitmap Tree bitmap
	 */
	static void encodeTree (std::vector<uint8_t> &tree, Node *node, Bitmap &bitmap);

	/** @brief Count number of nodes in subtree
	 *  @returns Number of nodes in subtree
	 */
	size_t numNodes () const
	{
		// sum of children plus self
		if (child[0])
			return child[0]->numNodes () + child[1]->numNodes () + 1;

		// this is a data node, just count self
		return 1;
	}

	/** @brief Count number of leaves in subtree
	 *  @returns Number of leaves in subtree
	 */
	size_t numLeaves () const
	{
		// sum of children
		if (child[0])
			return child[0]->numLeaves () + child[1]->numLeaves ();

		// this is a data node; it is a leaf
		return 1;
	}

	uint32_t getCode () const
	{
		return code;
	}

	uint8_t getCodeLen () const
	{
		return codeLen;
	}

	/** @brief Set tree position
	 *  @param[in] bitmap Tree occupancy
	 *  @param[in] i      Tree position to set
	 */
	void setPos (Bitmap &bitmap, size_t i)
	{
		assert (!bitmap.test (i));
		bitmap.set (i);
	}

private:
	Node *parent;                   ///< Parent node
	std::unique_ptr<Node> child[2]; ///< Children nodes
	size_t count;                   ///< Node weight
	uint32_t code;                  ///< Huffman encoding
	uint16_t pos;                   ///< Huffman tree position
	uint8_t val;                    ///< Huffman tree value
	uint8_t codeLen;                ///< Huffman code length (bits)
};

/** @brief Print a Huffman table (encoded tree)
 *  @param[in] tree    Huffman table
 *  @param[in] pos     Position in table
 *  @param[in] code    Huffman code
 *  @param[in] codeLen Huffman code length (bits)
 */
void printTable (const std::vector<uint8_t> &tree, size_t pos, uint32_t code, size_t codeLen)
{
	size_t offset = tree[pos] & 0x1F;
	size_t child  = (pos & ~1) + offset * 2 + 2;

	// print left subtree
	if (!(tree[pos] & 0x80))
		printTable (tree, child + 0, (code << 1) | 0, codeLen + 1);

	// print right subtree
	if (!(tree[pos] & 0x40))
		printTable (tree, child + 1, (code << 1) | 1, codeLen + 1);

	// print left data node
	if (tree[pos] & 0x80)
	{
		std::printf ("0x%02x: ", tree[child + 0]);
		printCode ((code << 1) | 0, codeLen + 1);
		std::fputc ('\n', stdout);
	}

	// print right data node
	if (tree[pos] & 0x40)
	{
		std::printf ("0x%02x: ", tree[child + 1]);
		printCode ((code << 1) | 1, codeLen + 1);
		std::fputc ('\n', stdout);
	}
}

void Node::buildCodes (std::unique_ptr<Node> &node, uint32_t code, size_t codeLen)
{
	// don't exceept 32-bit codes
	assert (codeLen < 32);

	// assert a full tree; every node has neither or both children
	assert ((node->child[0] && node->child[1]) || (!node->child[0] && !node->child[1]));

	if (node->child[0])
	{
		// build codes for each subtree
		buildCodes (node->child[0], code << 1, codeLen + 1);
		buildCodes (node->child[1], (code << 1) | 1, codeLen + 1);
	}
	else
	{
		// set code for data node
		node->code    = code;
		node->codeLen = codeLen;
	}
}

void Node::buildLookup (std::vector<Node *> &nodes, const std::unique_ptr<Node> &node)
{
	if (node->child[0])
	{
		// build subtree lookups
		buildLookup (nodes, node->child[0]);
		buildLookup (nodes, node->child[1]);
	}
	else
	{
		// set lookup entry
		nodes[node->val] = node.get ();
	}
}

void Node::encodeTree (std::vector<uint8_t> &tree, Node *node, Bitmap &bitmap)
{
	uint8_t mask;
	ssize_t next;

	// make sure this node's position is taken
	assert (bitmap.test (node->pos));

	if (node->numLeaves () <= 64)
	{
		std::queue<Node *> queue;
		queue.push (node);

		do
		{
			node = queue.front ();
			queue.pop ();

			size_t leaf = node->numLeaves ();

			if (leaf == 1)
			{
				// this is a data node; assign its value
				tree[node->pos] = node->val;
				std::printf ("[0x%02x] data=0x%02x\n", node->pos, node->val);
			}
			else
			{
				// this is a branch node
				mask = 0;

				// check if left child is a data node
				if (node->child[0]->numLeaves () == 1)
					mask |= 0x80;

				// check if right child is a data node
				if (node->child[1]->numLeaves () == 1)
					mask |= 0x40;

				// find a free position in the tree
				next = find (bitmap, node->pos);
				assert (next > node->pos);
				assert (next % 2 == 0);
				assert (!bitmap.test (next + 0));
				assert (!bitmap.test (next + 1));
				assert ((next - node->pos - 1) / 2 < 64);

				// encode location/type of children nodes
				tree[node->pos] = ((next - node->pos - 1) / 2) | mask;
				std::printf ("[0x%02x] 0x%02x, left=0x%02zx, right=0x%02zx\n",
				    node->pos,
				    tree[node->pos],
				    next + 0,
				    next + 1);

				// mark the children positions as taken
				bitmap.set (next + 0);
				bitmap.set (next + 1);

				// set the children positions
				node->child[0]->pos = next + 0;
				node->child[1]->pos = next + 1;

				// append children to the node queue (breadth first process)
				queue.push (node->child[0].get ());
				queue.push (node->child[1].get ());
			}
		} while (!queue.empty ());
	}
	else
	{
		// the subtree is large enough that the right child's children's positions
		// might be filled before we can encode them
		mask = 0;

		// get the left and right subtree leaf count
		size_t l_leaf = node->child[0]->numLeaves ();
		size_t r_leaf = node->child[1]->numLeaves ();

		// check if the left child is a data node
		if (l_leaf == 1)
			mask |= 0x80;

		// check if the right child is a data node
		if (r_leaf == 1)
			mask |= 0x40;

		// find a free position in the tree
		next = find (bitmap, node->pos);
		assert (next > node->pos);
		assert (next % 2 == 0);
		assert (!bitmap.test (next + 0));
		assert (!bitmap.test (next + 1));
		assert ((next - node->pos - 1) / 2 < 64);

		// encode location/type of children nodes
		tree[node->pos] = ((next - node->pos - 1) / 2) | mask;
		std::printf ("[0x%02x] 0x%02x, left=0x%02zx, right=0x%02zx\n",
		    node->pos,
		    tree[node->pos],
		    next + 0,
		    next + 1);

		// mark the children positions as taken
		bitmap.set (next + 0);
		bitmap.set (next + 1);

		// set the children positions
		node->child[0]->pos = next + 0;
		node->child[1]->pos = next + 1;

		// clamp position of right child's children
		if (l_leaf > 0x40)
			l_leaf = 0x40;

		// make sure right child's children have a free spot
		assert (!bitmap.test (next + l_leaf * 2 + 0));
		assert (!bitmap.test (next + l_leaf * 2 + 1));

		// reserve right child's children's position
		bitmap.set (next + l_leaf * 2 + 0);
		bitmap.set (next + l_leaf * 2 + 1);

		// encode left subtree
		encodeTree (tree, node->child[0].get (), bitmap);

		// make sure right child's children still have their reservation
		assert (bitmap.test (next + l_leaf * 2 + 0));
		assert (bitmap.test (next + l_leaf * 2 + 1));

		// clear right child's children's reservation
		bitmap.reset (next + l_leaf * 2 + 0);
		bitmap.reset (next + l_leaf * 2 + 1);

		// encode right subtree
		encodeTree (tree, node->child[1].get (), bitmap);
	}
}

/** @brief Build Huffman tree
 *  @param[in] src Source data
 *  @param[in] len Source data length
 *  @returns Root node
 */
static std::unique_ptr<Node> buildTree (const uint8_t *src, size_t len)
{
	// fill in histogram
	std::vector<size_t> histogram (256);
	for (size_t i = 0; i < len; ++i)
		++histogram[src[i]];

	std::vector<std::unique_ptr<Node>> nodes;
	{
		uint8_t val = 0;
		for (const auto &count : histogram)
		{
			if (count > 0)
				nodes.push_back (future::make_unique<Node> (val, count));

			++val;
		}
	}

	// done with histogram
	histogram.clear ();

	// combine nodes
	while (nodes.size () > 1)
	{
		// sort nodes by count; we will combine the two smallest nodes
		std::sort (std::begin (nodes),
		    std::end (nodes),
		    [](const std::unique_ptr<Node> &lhs, const std::unique_ptr<Node> &rhs) -> bool {
			    return *lhs < *rhs;
		    });

		// allocate a parent node
		std::unique_ptr<Node> node =
		    future::make_unique<Node> (std::move (nodes[0]), std::move (nodes[1]));

		// replace first node with self
		nodes[0] = std::move (node);

		// replace second node with last node
		nodes[1] = std::move (nodes.back ());
		nodes.pop_back ();
	}

	// root is the last node left
	std::unique_ptr<Node> root = std::move (nodes[0]);

	// build Huffman codes
	Node::buildCodes (root, 0, 0);

	// return root node
	return root;
}

/** @brief Bitstream */
class Bitstream
{
public:
	Bitstream (std::vector<uint8_t> &buffer) : buffer (buffer), pos (32)
	{
	}

	/** @brief Flush bitstream block, padded to 32 bits */
	void flush ()
	{
		if (pos < 32)
		{
			// this bitstream block has data
			data[0] = code >> 0;
			data[1] = code >> 8;
			data[2] = code >> 16;
			data[3] = code >> 24;

			// reset bitstream block
			pos  = 32;
			code = 0;

			// append bitstream block to output buffer
			buffer.insert (std::end (buffer), std::begin (data), std::end (data));
		}
	}

	/** @brief Push Huffman code onto bitstream
	 *  @param[in] code Huffman code
	 *  @param[in] len  Huffman code length (bits)
	 */
	void push (uint32_t code, size_t len)
	{
		for (size_t i = 1; i <= len; ++i)
		{
			// get next bit position
			--pos;

			// set/reset bit
			if (code & (1U << (len - i)))
				code |= (1U << pos);
			else
				code &= ~(1U << pos);

			if (pos == 0)
			{
				// flush bitstream block
				flush ();
			}
		}
	}

private:
	std::vector<uint8_t> &buffer; ///< Output buffer
	size_t pos;                   ///< Bit position
	uint32_t code;                ///< Bitstream block
	uint8_t data[4];              ///< Bitstream block buffer
};
}

std::vector<uint8_t> huffEncode (const void *source, size_t len)
{
	const uint8_t *src = (const uint8_t *)source;
	size_t count;

	// build Huffman tree
	std::unique_ptr<Node> root = buildTree (src, len);

	// build lookup table
	std::vector<Node *> lookup (256);
	Node::buildLookup (lookup, root);

	// get number of nodes
	count = root->numNodes ();

	// allocate Huffman encoded tree
	std::vector<uint8_t> tree ((count + 2) & ~1);

	// allocate bitmap
	Bitmap bitmap;

	// first slot encodes tree size
	bitmap.set (0);
	tree[0] = count / 2;

	// second slot encodes root node
	root->setPos (bitmap, 1);

	// encode Huffman tree
	Node::encodeTree (tree, root.get (), bitmap);

#if 1
	for (size_t i = 0; i < count + 1; ++i)
	{
		std::printf (" %02x", tree[i]);
		if (i % 8 == 7)
			std::fputc ('\n', stdout);
	}
	std::fputc ('\n', stdout);
#endif

	root->printTree (0, 0);
	std::printf ("==============\n");
	printTable (tree, 1, 0, 0);

#if 1
	return {};
#endif

	// create output buffer
	std::vector<uint8_t> result;

	// append compression header
	compressionHeader (result, 0x28, len);

	// append Huffman encoded tree
	result.insert (std::end (result), std::begin (tree), std::end (tree));

	// we're done with the Huffman encoded tree
	tree.clear ();

	// create bitstream
	Bitstream bitstream (result);

	// encode each input byte
	for (size_t i = 0; i < len; ++i)
	{
		// lookup the byte value's node
		Node *node = lookup[src[i]];

		// add Huffman code to bitstream
		bitstream.push (node->getCode (), node->getCodeLen ());
	}

	// we're done with the Huffman tree and lookup table
	root.release ();
	lookup.clear ();

	// flush the bitstream
	bitstream.flush ();

	// pad the output buffer to 4 bytes
	if (result.size () & 0x3)
		result.resize ((result.size () + 3) & ~0x3);

	// return the output data
	return result;
}

void huffDecode (const void *src, void *dst, size_t size)
{
	const size_t bits   = 8;
	const uint8_t *in   = (const uint8_t *)src;
	uint8_t *out        = (uint8_t *)dst;
	uint32_t treeSize   = ((*in) + 1) * 2; // size of the huffman header
	uint32_t word       = 0;               // 32-bits of input bitstream
	uint32_t mask       = 0;               // which bit we are reading
	uint32_t dataMask   = (1 << bits) - 1; // mask to apply to data
	const uint8_t *tree = in;              // huffman tree
	size_t node;                           // node in the huffman tree
	size_t child;                          // child of a node
	uint32_t offset;                       // offset from node to child

	// point to the root of the huffman tree
	node = 1;

	// move input pointer to beginning of bitstream
	in += treeSize;

	while (size > 0)
	{
		if (mask == 0) // we exhausted 32 bits
		{
			// reset the mask
			mask = 0x80000000;

			// read the next 32 bits
			word = (in[0] << 0) | (in[1] << 8) | (in[2] << 16) | (in[3] << 24);
			in += 4;
		}

		// read the current node's offset value
		offset = tree[node] & 0x1F;

		child = (node & ~1) + offset * 2 + 2;

		if (word & mask) // we read a 1
		{
			// point to the "right" child
			++child;

			if (tree[node] & 0x40) // "right" child is a data node
			{
				// copy the child node into the output buffer and apply mask
				*out++ = tree[child] & dataMask;
				size--;

				// start over at the root node
				node = 1;
			}
			else // traverse to the "right" child
				node = child;
		}
		else // we read a 0
		{
			// pointed to the "left" child

			if (tree[node] & 0x80) // "left" child is a data node
			{
				// copy the child node into the output buffer and apply mask
				*out++ = tree[child] & dataMask;
				size--;

				// start over at the root node
				node = 1;
			}
			else // traverse to the "left" child
				node = child;
		}

		// shift to read next bit (read bit 31 to bit 0)
		mask >>= 1;
	}
}
