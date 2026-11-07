#pragma once
#include <memory>
#include <vector>

#include "forge/viz/recorder.hpp"
#include "forge/viz/scene.hpp"

namespace forge::algo {

struct TreeNode {
    int value = 0;
    std::unique_ptr<TreeNode> left;   // owns the left subtree
    std::unique_ptr<TreeNode> right;  // owns the right subtree
};

// `root` is only used to draw the whole tree while inserting.
void bstInsert(std::unique_ptr<TreeNode>& node, int value, const TreeNode* root, viz::Recorder& rec);
const TreeNode* bstFind(const TreeNode* node, int target);
std::unique_ptr<TreeNode> buildBstFrom(const std::vector<int>& values);

void inorder(const TreeNode* n, std::vector<int>& out);
void preorder(const TreeNode* n, std::vector<int>& out);
std::vector<int> levelOrder(const TreeNode* root);

void heapPush(std::vector<int>& heap, int value);
int heapPop(std::vector<int>& heap);  // heap must not be empty

// Pictures, reused by the English -> Diagram sandbox.
viz::GraphScene drawTree(const TreeNode* root);
viz::GraphScene drawHeap(const std::vector<int>& heap);

}  // namespace forge::algo
