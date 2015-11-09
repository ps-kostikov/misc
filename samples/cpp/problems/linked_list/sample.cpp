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

int main()
{
    auto root = new Node{0, nullptr};
    auto current = root;
    for (int i = 1; i <= 10; ++i) {
        auto next = new Node{i, nullptr};
        current->next = next;
        current = next;
    }
    print(root);
    root = reverse(root);
    print(root);
    destroy(root);
    return 0;
}
