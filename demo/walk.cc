#include <iostream>

#include "../git2pp.h"

void show_commit(char const * branch) {
    git2pp::Session git2;

    auto repo = git2[git_repository_open_ext](".", 0, nullptr);
    auto master = repo[git_reference_dwim](branch);
    auto commit = master[git_reference_peel](GIT_OBJ_COMMIT).as<git_commit>();

    auto walker = repo[git_revwalk_new]();
    walker[git_revwalk_sorting](GIT_SORT_TIME);
    walker[git_revwalk_push](commit[git_commit_id]());
    git_oid oid;
    while (!git_revwalk_next(&oid, &*walker)) {
        std::cout << &oid << "\n";
    }
    std::cout << "\n";

    auto parent0 = commit[git_commit_parent](0);
    std::cout << "master^ = " << parent0[git_commit_id]() << "\n";
    std::cout << "author = " << parent0[git_commit_author]()->name << "\n";
    std::cout << "message = " << parent0[git_commit_message]() << "\n";

    for (auto ref : repo[git_reference_iterator_new]()) {
        std::cout << ref[git_reference_name]() << "\n";
    }
    std::cout << "\n";

    auto cfg = repo[git_repository_config]();
    for (auto config : cfg[git_config_iterator_new]()) {
        std::cout << config->name << " = " << config->value << "\n";
    }


    auto gitindex = repo[git_repository_index]();
    gitindex[git_index_read](true);
    auto ii = gitindex[git_index_iterator_new]();
    for (auto indexentry : ii) {
        std::cout << "index contains file:" << indexentry->path << std::endl;
    }
}

int main(int argc, char * argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: demo <branch>";
        return 1;
    }
    show_commit(argv[1]);
}
