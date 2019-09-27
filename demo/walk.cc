#include <iostream>

#include "../git2pp.h"

void show_commit(char const * shorthand) {
    git2pp::Session git2;

    auto repo = git2[git_repository_open_ext](".", 0, nullptr);
    auto master = repo[git_reference_dwim](shorthand);
    auto commit = master[git_reference_peel](GIT_OBJ_COMMIT).as<git_commit>();

    auto parent0 = commit[git_commit_parent](0);
    std::cout << shorthand << "^ = " << parent0[git_commit_id]() << "\n";
    std::cout << "author = " << parent0[git_commit_author]()->name << "\n";
    std::cout << "message = " << parent0[git_commit_message]() << "\n";

    auto revwalk = repo[git_revwalk_new]();
    revwalk[git_revwalk_sorting](GIT_SORT_TIME);
    revwalk[git_revwalk_push](commit[git_commit_id]());

    std::cout << "revs:\n";
    for (auto && oid : revwalk) {
        std::cout << "  " << &oid << "\n";
    }

    std::cout << "refs:\n";
    for (auto && ref : repo[git_reference_iterator_new]()) {
        std::cout << "  " << ref[git_reference_name]() << "\n";
    }

    std::cout << "branches:\n";
    for (auto && branch : repo[git_branch_iterator_new](GIT_BRANCH_ALL)) {
        std::cout << "  " << branch.ref[git_reference_name]() << (branch.type == GIT_BRANCH_LOCAL ? "" : " (remote)") << "\n";
    }

    std::cout << "config:\n";
    for (auto && config : repo[git_repository_config]()[git_config_iterator_new]()) {
        std::cout << "  " << config->name << " = " << config->value << "\n";
    }

    auto index = repo[git_repository_index]();

#if LIBGIT2PP_HAVE_INDEX_ITERATOR
    std::cout << "index:\n";
    for (auto && entry : index[git_index_iterator_new]()) {
        std::cout << "  " << entry->path << "\n";
    }
#endif

    std::cout << "index conflicts:\n";
    for (auto && conflict : index[git_index_conflict_iterator_new]()) {
        std::cout << "  " << conflict.ancestor << "\n";
    }

    std::cout << "notes:\n";
    for (auto && note : repo[git_note_iterator_new]("refs/notes/commits")) {
        std::cout << "  " << &note.note_id << "\n";
    }

    try {
        std::cout << "rebase:\n";
        for (auto && op : repo[git_rebase_init](nullptr, nullptr, nullptr, nullptr)) {
            std::cout << "  " << &op->id << "\n";
        }
    } catch (std::exception const & e) {
        // Too lazy to test this properly.
        std::cout << "  failure not unexpected: " << e.what() << "\n";
    }
}

int main(int argc, char * argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: demo <branch>";
        return 1;
    }
    show_commit(argv[1]);
}
