#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <functional>
#include <sys/stat.h>
#include "filesystem.h"

// Helper: check if file exists
bool file_exists(const std::string& name) {
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

// Serialize the BTree recursively: write entries D|path or F|path|content
void serialize_dir(Directory* dir, const std::string& path, std::ofstream& out) {
    BTree* tree = dir->tree;
    std::function<void(BTreeNode*, const std::string&)> recurse;
    recurse = [&](BTreeNode* node, const std::string& curPath) {
        if (!node) return;
        for (int i = 0; i < node->num_keys; ++i) {
            TreeNode* tn = node->keys[i];
            std::string entryPath = curPath + "/" + tn->name;
            if (tn->type == DIRECTORY_TYPE) {
                out << "D|" << entryPath << "\n";
                recurse(tn->data.directory->tree->root, entryPath);
            } else {
                out << "F|" << entryPath << "|" << tn->data.file->content << "\n";
            }
        }
        if (!node->leaf) {
            for (int j = 0; j <= node->num_keys; ++j) {
                recurse(node->children[j], curPath);
            }
        }
    };
    recurse(tree->root, path);
}

int main() {
    Directory* root;
    const std::string persistFile = "fs.img";

    // Load existing state if any
    if (file_exists(persistFile)) {
        root = get_root_directory();
        std::ifstream in(persistFile);
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            char type = line[0];
            std::string rest = line.substr(2);
            if (type == 'D') {
                std::istringstream iss(rest);
                std::string token;
                Directory* cur = root;
                std::vector<std::string> parts;
                while (std::getline(iss, token, '/')) if (!token.empty()) parts.push_back(token);
                for (size_t i = 1; i < parts.size(); ++i) {
                    TreeNode* tn = btree_search(cur->tree, parts[i].c_str());
                    if (!tn) {
                        TreeNode* newd = create_directory(parts[i].c_str());
                        btree_insert(cur->tree, newd);
                        tn = newd;
                    }
                    cur = tn->data.directory;
                }
            } else if (type == 'F') {
                size_t sep = rest.find('|');
                std::string path = rest.substr(0, sep);
                std::string content = rest.substr(sep + 1);
                std::istringstream iss(path);
                std::string token;
                Directory* cur = root;
                std::vector<std::string> parts;
                while (std::getline(iss, token, '/')) if (!token.empty()) parts.push_back(token);
                for (size_t i = 1; i + 1 < parts.size(); ++i) {
                    TreeNode* tn = btree_search(cur->tree, parts[i].c_str());
                    if (tn && tn->type == DIRECTORY_TYPE) cur = tn->data.directory;
                }
                const std::string& fname = parts.back();
                TreeNode* fn = create_txt_file(fname.c_str(), content.c_str());
                btree_insert(cur->tree, fn);
            }
        }
        in.close();
        std::cout << "Estado carregado de " << persistFile << std::endl;
    } else {
        root = get_root_directory();
        std::cout << "Inicializando novo sistema de arquivos." << std::endl;
    }

    std::vector<Directory*> stack;
    stack.push_back(root);

    std::cout << "Comandos: mkdir, rmdir, touch, rm, ls, cd, exit" << std::endl;
    std::string input;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, input)) break;
        std::istringstream iss(input);
        std::vector<std::string> args;
        std::string tok;
        while (iss >> tok) args.push_back(tok);
        if (args.empty()) continue;
        const std::string& cmd = args[0];

        if (cmd == "exit") {
            std::ofstream out(persistFile);
            out << "D|/ROOT\n";
            serialize_dir(root, "/ROOT", out);
            out.close();
            std::cout << "Estado salvo em " << persistFile << std::endl;
            break;
        } else if (cmd == "ls") {
            list_directory_contents(stack.back());
        } else if (cmd == "mkdir" && args.size() >= 2) {
            const char* name = args[1].c_str();
            if (!btree_search(stack.back()->tree, name)) {
                TreeNode* d = create_directory(name);
                btree_insert(stack.back()->tree, d);
            } else std::cout << "Já existe: " << name << std::endl;
        } else if (cmd == "rmdir" && args.size() >= 2) {
            delete_directory(stack.back()->tree, args[1].c_str());
        } else if (cmd == "touch" && args.size() >= 3) {
            const char* name = args[1].c_str();
            size_t pos = input.find(name);
            std::string content = input.substr(pos + strlen(name) + 1);
            if (!btree_search(stack.back()->tree, name) && content.size() <= 1024*1024) {
                TreeNode* f = create_txt_file(name, content.c_str());
                btree_insert(stack.back()->tree, f);
            } else std::cout << "Erro ao criar arquivo: já existe ou conteúdo >1MB" << std::endl;
        } else if (cmd == "rm" && args.size() >= 2) {
            delete_txt_file(stack.back()->tree, args[1].c_str());
        } else if (cmd == "cd" && args.size() >= 2) {
            const std::string& target = args[1];
            if (target == "..") {
                if (stack.size() > 1) stack.pop_back();
                else std::cout << "Já na raiz." << std::endl;
            } else {
                TreeNode* tn = btree_search(stack.back()->tree, target.c_str());
                if (tn && tn->type == DIRECTORY_TYPE) {
                    stack.push_back(tn->data.directory);
                } else {
                    std::cout << "Diretório não encontrado: " << target << std::endl;
                }
            }
        } else {
            std::cout << "Comando inválido." << std::endl;
        }
    }

    return 0;
}
