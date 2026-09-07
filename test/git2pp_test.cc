// Hermetic test suite for git2pp.h.
//
// The repository under test is built by test/fixture.sh in a temp directory and
// handed to us through GIT2PP_FIXTURE_REPO. Nothing here looks at the ambient
// working directory, so the suite's verdict does not change with the cwd, the
// clone depth, or libgit2pp's own commit history.

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "../git2pp.h"

#include <cstdlib>
#include <string>
#include <vector>

namespace {

    // Pinned by test/fixture.sh. Every input to these hashes (tree, identity,
    // both timestamps, message, branch name) is fixed by that script, so these
    // constants are a frozen reference, not a snapshot of whatever ran last.
    char const * const HEAD_OID = "0e21a9124f2afe3daadc438d949069d493000c78";
    char const * const PARENT_OID = "be78658c2f869f9cc3429a1cc2297906a7acafa2";
    char const * const FIXTURE_BRANCH = "master";
    char const * const FIXTURE_AUTHOR = "Fixture Author";
    char const * const HEAD_MESSAGE = "second commit\n";

    std::string fixture_repo() {
        char const * const path = std::getenv("GIT2PP_FIXTURE_REPO");
        REQUIRE_MESSAGE(path != nullptr,
                        "GIT2PP_FIXTURE_REPO must point at a repo built by test/fixture.sh");
        return path;
    }

    // Bind to exactly the given directory: NO_SEARCH stops libgit2 walking up
    // into whatever repository happens to contain the temp dir.
    auto open_fixture(git2pp::Session & git2, std::string const & path) {
        return git2[git_repository_open_ext](path.c_str(), GIT_REPOSITORY_OPEN_NO_SEARCH, nullptr);
    }

    std::string oid_string(git_oid const * oid) {
        return git_oid_tostr_s(oid);
    }

}

TEST_CASE("walk_parent_oid_matches_fixture") {
    git2pp::Session git2;
    auto repo = open_fixture(git2, fixture_repo());
    auto ref = repo[git_reference_dwim](FIXTURE_BRANCH);
    auto commit = ref[git_reference_peel](GIT_OBJECT_COMMIT).as<git_commit>();

    CHECK(oid_string(commit[git_commit_id]()) == HEAD_OID);
    CHECK(std::string{commit[git_commit_message]()} == HEAD_MESSAGE);
    CHECK(std::string{commit[git_commit_author]()->name} == FIXTURE_AUTHOR);

    auto parent0 = commit[git_commit_parent](0);
    CHECK(oid_string(parent0[git_commit_id]()) == PARENT_OID);
}

TEST_CASE("revwalk_visits_both_fixture_commits_newest_first") {
    git2pp::Session git2;
    auto repo = open_fixture(git2, fixture_repo());
    auto ref = repo[git_reference_dwim](FIXTURE_BRANCH);
    auto commit = ref[git_reference_peel](GIT_OBJECT_COMMIT).as<git_commit>();

    auto revwalk = repo[git_revwalk_new]();
    revwalk[git_revwalk_sorting](GIT_SORT_TIME);
    revwalk[git_revwalk_push](commit[git_commit_id]());

    std::vector<std::string> oids;
    for (auto && oid : revwalk) {
        oids.push_back(oid_string(&oid));
    }

    REQUIRE(oids.size() == 2);
    CHECK(oids[0] == HEAD_OID);
    CHECK(oids[1] == PARENT_OID);
}

TEST_CASE("reference_iterator_lists_only_the_fixture_branch") {
    git2pp::Session git2;
    auto repo = open_fixture(git2, fixture_repo());

    std::vector<std::string> names;
    for (auto && ref : repo[git_reference_iterator_new]()) {
        names.push_back(ref[git_reference_name]());
    }

    REQUIRE(names.size() == 1);
    CHECK(names[0] == std::string{"refs/heads/"} + FIXTURE_BRANCH);
}

TEST_CASE("opening_a_non_repository_throws_error") {
    git2pp::Session git2;
    // The fixture's parent is a bare temp directory, and NO_SEARCH keeps
    // libgit2 from climbing out of it.
    std::string const not_a_repo = fixture_repo() + "/.git/objects";
    CHECK_THROWS_AS(open_fixture(git2, not_a_repo), git2pp::Error);
}

TEST_CASE("peeling_an_unknown_ref_throws_error") {
    git2pp::Session git2;
    auto repo = open_fixture(git2, fixture_repo());
    CHECK_THROWS_AS(repo[git_reference_dwim]("no-such-branch"), git2pp::Error);
}
