#include <iostream>
#include <vector>
#include <algorithm>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

struct Node
{
    int payload;
    Node* next;
};

void print(const Node* node)
{
    Node* current = const_cast<Node*>(node);
    while (current != nullptr) {
        std::cout << current->payload << " ";
        current = current->next;
    }
    std::cout << std::endl;
}

void destroy(Node* node)
{
    if (!node) {
        return;
    }
    if (node->next) {
        destroy(node->next);
    }
    delete node;
}

Node* reverse(Node* node)
{
    Node* new_next = nullptr;
    auto current = node;
    Node* tmp = nullptr;
    while (current->next != nullptr) {
        tmp = current->next;
        current->next = new_next;
        new_next = current;
        current = tmp;
    }
    current->next = new_next;
    return current;
}

Node* create(size_t size)
{
    if (size == 0) {
        return nullptr;
    }
    auto root = new Node{0, nullptr};
    auto current = root;
    for (int i = 1; i < static_cast<int>(size); ++i) {
        auto next = new Node{i, nullptr};
        current->next = next;
        current = next;
    }
    return root;
}

Node* pairwise_swap(Node* root)
{
    Node* result = nullptr;
    Node* prev = nullptr;
    Node* current = root;
    while (current) {
        if (current->next) {
            Node* next = current->next;
            Node* nextNext = current->next->next;
            if (prev) {
                prev->next = next;
            }
            current->next = nextNext;
            next->next = current;
            if (!result) {
                result = next;
            }

            prev = current;
            current = nextNext;

        } else {
            if (!result) {
                result = current;
            }
            break;
        }
    }
    return result;
}

int main()
{
    for (size_t i: {0, 1, 2, 4, 5, 9, 10}) {
        std::cout << "Test case i = " << i << ":" << std::endl;
        auto root = create(i);
        print(root);
        root = pairwise_swap(root);
        print(root);
        destroy(root);
        std::cout << std::endl;
    }
    return 0;
}
