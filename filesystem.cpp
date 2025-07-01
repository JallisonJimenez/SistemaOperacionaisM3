#include "filesystem.h"
#include <assert.h>
#include <stdbool.h>

void btree_split_child(BTreeNode* parent, int i);
void btree_insert_nonfull(BTreeNode* node, TreeNode* k);


BTree* btree_create() {
    BTree* tree = (BTree*)malloc(sizeof(BTree));
    tree->root = NULL;
    return tree;
}


BTreeNode* btree_create_node(bool leaf) {
    BTreeNode* node = (BTreeNode*)malloc(sizeof(BTreeNode));
    node->leaf = leaf;
    node->num_keys = 0;
    for (int i = 0; i < 2 * BTREE_ORDER; i++) node->children[i] = NULL;
    return node;
}


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

TreeNode* create_txt_file(const char* name, const char* content) {
    
    File* f = (File*)malloc(sizeof(File));
    assert(f);
    f->name = strdup(name);
    f->size = strlen(content);
    f->content = (char*)malloc(f->size + 1);
    assert(f->content);
    strcpy(f->content, content);

  
    TreeNode* node = (TreeNode*)malloc(sizeof(TreeNode));
    assert(node);
    node->name = strdup(name);
    node->type = FILE_TYPE;
    node->data.file = f;
    return node;
}


TreeNode* create_directory(const char* name) {
   
    Directory* d = (Directory*)malloc(sizeof(Directory));
    assert(d);
    d->tree = btree_create();

   
    TreeNode* node = (TreeNode*)malloc(sizeof(TreeNode));
    assert(node);
    node->name = strdup(name);
    node->type = DIRECTORY_TYPE;
    node->data.directory = d;
    return node;
}



void btree_delete_node(BTreeNode* x, const char* k);
TreeNode* btree_get_pred(BTreeNode* x, int idx);
TreeNode* btree_get_succ(BTreeNode* x, int idx);
void btree_fill(BTreeNode* x, int idx);
void btree_borrow_from_prev(BTreeNode* x, int idx);
void btree_borrow_from_next(BTreeNode* x, int idx);
void btree_merge(BTreeNode* x, int idx);

void btree_delete(BTree* tree, const char* k) {
    if (!tree || !tree->root) return;
    btree_delete_node(tree->root, k);

    
    if (tree->root->num_keys == 0) {
        BTreeNode* old = tree->root;
        if (old->leaf) {
            free(old);
            tree->root = NULL;
        } else {
            tree->root = old->children[0];
            free(old);
        }
    }
}

void btree_delete_node(BTreeNode* x, const char* k) {
    int idx = 0;
   
    while (idx < x->num_keys && strcmp(x->keys[idx]->name, k) < 0) idx++;


    if (idx < x->num_keys && strcmp(x->keys[idx]->name, k) == 0) {
        if (x->leaf) {
   
            for (int i = idx+1; i < x->num_keys; ++i)
                x->keys[i-1] = x->keys[i];
            x->num_keys--;
        } else {
   
            if (x->children[idx]->num_keys >= BTREE_ORDER) {
                TreeNode* pred = btree_get_pred(x, idx);
                free(x->keys[idx]);
                x->keys[idx] = pred;
                btree_delete_node(x->children[idx], pred->name);
            }
            else if (x->children[idx+1]->num_keys >= BTREE_ORDER) {
                TreeNode* succ = btree_get_succ(x, idx);
                free(x->keys[idx]);
                x->keys[idx] = succ;
                btree_delete_node(x->children[idx+1], succ->name);
            }
            else {
                btree_merge(x, idx);
                btree_delete_node(x->children[idx], k);
            }
        }
    }
    else {

        if (x->leaf) {

            return;
        }
      
        bool flag = ( (idx == x->num_keys) );
        if (x->children[idx]->num_keys < BTREE_ORDER)
            btree_fill(x, idx);
        if (flag && idx > x->num_keys)
            btree_delete_node(x->children[idx-1], k);
        else
            btree_delete_node(x->children[idx], k);
    }
}

TreeNode* btree_get_pred(BTreeNode* x, int idx) {
    BTreeNode* cur = x->children[idx];
    while (!cur->leaf) cur = cur->children[cur->num_keys];
    return cur->keys[cur->num_keys-1];
}

TreeNode* btree_get_succ(BTreeNode* x, int idx) {
    BTreeNode* cur = x->children[idx+1];
    while (!cur->leaf) cur = cur->children[0];
    return cur->keys[0];
}

void btree_fill(BTreeNode* x, int idx) {
    if (idx > 0 && x->children[idx-1]->num_keys >= BTREE_ORDER)
        btree_borrow_from_prev(x, idx);
    else if (idx < x->num_keys && x->children[idx+1]->num_keys >= BTREE_ORDER)
        btree_borrow_from_next(x, idx);
    else {
        if (idx < x->num_keys)
            btree_merge(x, idx);
        else
            btree_merge(x, idx-1);
    }
}

void btree_borrow_from_prev(BTreeNode* x, int idx) {
    BTreeNode* child = x->children[idx];
    BTreeNode* sib   = x->children[idx-1];

  
    for (int i = child->num_keys-1; i >= 0; --i)
        child->keys[i+1] = child->keys[i];
    if (!child->leaf) {
        for (int i = child->num_keys; i >= 0; --i)
            child->children[i+1] = child->children[i];
    }


    child->keys[0] = x->keys[idx-1];
    if (!x->leaf)
        child->children[0] = sib->children[sib->num_keys];

 
    x->keys[idx-1] = sib->keys[sib->num_keys-1];

    child->num_keys += 1;
    sib->num_keys -= 1;
}

void btree_borrow_from_next(BTreeNode* x, int idx) {
    BTreeNode* child = x->children[idx];
    BTreeNode* sib   = x->children[idx+1];


    child->keys[child->num_keys] = x->keys[idx];
    if (!child->leaf)
        child->children[child->num_keys+1] = sib->children[0];

  
    x->keys[idx] = sib->keys[0];

 
    for (int i = 1; i < sib->num_keys; ++i)
        sib->keys[i-1] = sib->keys[i];
    if (!sib->leaf) {
        for (int i = 1; i <= sib->num_keys; ++i)
            sib->children[i-1] = sib->children[i];
    }
    child->num_keys += 1;
    sib->num_keys -= 1;
}

void btree_merge(BTreeNode* x, int idx) {
    BTreeNode* child = x->children[idx];
    BTreeNode* sib   = x->children[idx+1];

   
    child->keys[BTREE_ORDER-1] = x->keys[idx];

 
    for (int i = 0; i < sib->num_keys; ++i)
        child->keys[i+BTREE_ORDER] = sib->keys[i];
    if (!child->leaf) {
        for (int i = 0; i <= sib->num_keys; ++i)
            child->children[i+BTREE_ORDER] = sib->children[i];
    }

    
    for (int i = idx+1; i < x->num_keys; ++i)
        x->keys[i-1]     = x->keys[i];
    for (int i = idx+2; i <= x->num_keys; ++i)
        x->children[i-1] = x->children[i];

    child->num_keys += sib->num_keys + 1;
    x->num_keys--;

    free(sib);
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
