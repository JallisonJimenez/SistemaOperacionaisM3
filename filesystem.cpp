#include "filesystem.h"

#include <stdbool.h>

// Cria a estrutura BTree vazia
BTree* btree_create() {
    BTree* tree = (BTree*)malloc(sizeof(BTree));
    tree->root = NULL;
    return tree;
}

// Função auxiliar para criar um novo nó BTreeNode
BTreeNode* btree_create_node(bool leaf) {
    BTreeNode* node = (BTreeNode*)malloc(sizeof(BTreeNode));
    node->leaf = leaf;
    node->num_keys = 0;
    for (int i = 0; i < 2 * BTREE_ORDER; i++) node->children[i] = NULL;
    return node;
}

// Busca em árvore B
TreeNode* btree_search_node(BTreeNode* node, const char* name) {
    int i = 0;
    while (i < node->num_keys && strcmp(name, node->keys[i]->name) > 0) i++;
    if (i < node->num_keys && strcmp(name, node->keys[i]->name) == 0) return node->keys[i];
    if (node->leaf) return NULL;
    return btree_search_node(node->children[i], name);
}

TreeNode* btree_search(BTree* tree, const char* name) {
    if (!tree || !tree->root) return NULL;
    return btree_search_node(tree->root, name);
}

// Insere em nó não cheio
void btree_insert_nonfull(BTreeNode* node, TreeNode* k) {
    int i = node->num_keys - 1;
    if (node->leaf) {
        while (i >= 0 && strcmp(k->name, node->keys[i]->name) < 0) {
            node->keys[i + 1] = node->keys[i];
            i--;
        }
        node->keys[i + 1] = k;
        node->num_keys++;
    } else {
        while (i >= 0 && strcmp(k->name, node->keys[i]->name) < 0) i--;
        i++;
        if (node->children[i]->num_keys == 2 * BTREE_ORDER - 1) {
            btree_split_child(node, i);
            if (strcmp(k->name, node->keys[i]->name) > 0) i++;
        }
        btree_insert_nonfull(node->children[i], k);
    }
}

// Divide um filho cheio
void btree_split_child(BTreeNode* parent, int i) {
    BTreeNode* y = parent->children[i];
    BTreeNode* z = btree_create_node(y->leaf);
    z->num_keys = BTREE_ORDER - 1;

    for (int j = 0; j < BTREE_ORDER - 1; j++)
        z->keys[j] = y->keys[j + BTREE_ORDER];

    if (!y->leaf) {
        for (int j = 0; j < BTREE_ORDER; j++)
            z->children[j] = y->children[j + BTREE_ORDER];
    }

    y->num_keys = BTREE_ORDER - 1;

    for (int j = parent->num_keys; j >= i + 1; j--)
        parent->children[j + 1] = parent->children[j];

    parent->children[i + 1] = z;

    for (int j = parent->num_keys - 1; j >= i; j--)
        parent->keys[j + 1] = parent->keys[j];

    parent->keys[i] = y->keys[BTREE_ORDER - 1];
    parent->num_keys++;
}

void btree_insert(BTree* tree, TreeNode* k) {
    if (!tree->root) {
        tree->root = btree_create_node(true);
        tree->root->keys[0] = k;
        tree->root->num_keys = 1;
    } else {
        if (tree->root->num_keys == 2 * BTREE_ORDER - 1) {
            BTreeNode* s = btree_create_node(false);
            s->children[0] = tree->root;
            btree_split_child(s, 0);
            int i = 0;
            if (strcmp(k->name, s->keys[0]->name) > 0) i++;
            btree_insert_nonfull(s->children[i], k);
            tree->root = s;
        } else {
            btree_insert_nonfull(tree->root, k);
        }
    }
}

void btree_traverse_node(BTreeNode* node) {
    int i;
    for (i = 0; i < node->num_keys; i++) {
        if (!node->leaf) btree_traverse_node(node->children[i]);
        printf("%s (%s)\n", node->keys[i]->name, node->keys[i]->type == FILE_TYPE ? "arquivo" : "diretório");
    }
    if (!node->leaf) btree_traverse_node(node->children[i]);
}

void btree_traverse(BTree* tree) {
    if (tree->root) btree_traverse_node(tree->root);
}

void delete_txt_file(BTree* tree, const char* name) {
    TreeNode* node = btree_search(tree, name);
    if (node && node->type == FILE_TYPE) {
        btree_delete(tree, name);
        printf("Arquivo '%s' deletado.\n", name);
    } else {
        printf("Arquivo '%s' não encontrado ou não é um arquivo.\n", name);
    }
}

void delete_directory(BTree* tree, const char* name) {
    TreeNode* node = btree_search(tree, name);
    if (node && node->type == DIRECTORY_TYPE) {
        Directory* dir = node->data.directory;
        if (dir->tree->root == NULL || dir->tree->root->num_keys == 0)
        {
            btree_delete(tree, name);
            printf("Diretório '%s' removido.\n", name);
        } else {
            printf("Diretório '%s' não está vazio.\n", name);
        }
    } else {
        printf("Diretório '%s' não encontrado ou não é um diretório.\n", name);
    }
}

Directory* get_root_directory() {
    Directory* root = (Directory*)malloc(sizeof(Directory));
    root->tree = btree_create();
    return root;
}

void change_directory(Directory** current, const char* path) {
    TreeNode* node = btree_search((*current)->tree, path);
    if (node && node->type == DIRECTORY_TYPE) {
        *current = node->data.directory;
        printf("Diretório alterado para '%s'.\n", path);
    } else {
        printf("Diretório '%s' não encontrado.\n", path);
    }
}

void list_directory_contents(Directory* dir) {
    printf("Conteúdo do diretório:\n");
    btree_traverse(dir->tree);
}
