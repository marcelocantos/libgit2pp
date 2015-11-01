#ifndef GIT2PP_H
#define GIT2PP_H

/*

git2pp provides convenience wrappers for the libgit2 API.

It uses metaprogramming to avoid replicating the entire API, while still
allowing much more succinct code than raw libgit2.

namespace git2pp:

    Error/check: check() throws Error when a libgit2 result indicates failure.

    Session: wraps git_libgit2_init and git_libgit2_shutdown in RAII.

        operator[](): Wraps libgit2 functions with no input object.

            // libgit2
            git_libgit2_init();
            git_repository * repo = NULL;
            int err = git_repository_open(&repo, ".");
            if (err < 0) {
                ...
            }
            ...
            git_repository_free(repo);
            git_libgit2_shutdown();

            // git2pp
            git2pp::Session git2;
            auto repo = git2[git_repository_open](".");  // Exception on failure.

    UniquePtr<T>: Smart pointer wrapping a libgit2 pointer.

        ctor/operator=(UniquePtr &&): Transfers ownership.

        ctor/operator=(UniquePtr const &): Copies using git_..._dup function,
        where such a function exists.

        operator[](): Wraps libgit2 functions with sigs (T [const] *, ...) or
        (U * *, T [const] *, ...), where U is some other output type, e.g.:

            // libgit2
            git_reference *ref = NULL;
            int err = git_reference_dwim(&ref, "master");
            if (err < 0) {
                ...
            }
            ...
            git_reference_free(ref);

            // git2pp
            auto master = repo[git_reference_dwim]("master");

        as<U>(): Casts one libgit2 object type to another. If this is an
        rvalue reference, transfers ownership to a new UniquePtr<U>, otherwise
        returns a raw U *. WARNING: Will succeed for any pair of types,
        whether it's valid or not.

Both UniquePtr<>::operator[]() and Session::operator[]() return UniquePtr<T>
if the wrapped libgit2 function has a T * * as its first parameter.

If a libgit2 function takes a U * parameter (i.e., in addition the T * that
UniquePtr<T> removes during wrapping) and you have a UniquePtr<U> u, you can
pass it in as &*u. E.g.:

    auto me = repo[git_signature_now]("Me", "me@example.com");
    commit[git_commit_amend](..., &*me, &*me, ...);

Here's a full before-and-after example:

// BEFORE
bool git2ok(int rc) {
    if (rc >= 0) {
        return true;
    }
    const git_error *e = giterr_last();
    fprintf(stderr, "Error %d/%d: %s\n", rc, e->klass, e->message);
    return false;
}

bool show_commit(char const * branch) {
    git_libgit2_init();

    git_repository * repo = NULL;
    git_reference * master = NULL;
    git_object * commit_obj = NULL;
    git_commit * commit = NULL;

    bool ok;

    if (!(ok = git2ok(git_repository_open(&repo, ".")))) {
        goto fail;
    }

    if (!(ok = git2ok(git_reference_dwim(&master, repo, "master")))) {
        goto fail;
    }

    if (!(ok = git2ok(git_reference_peel(&commit_obj, master, GIT_OBJ_COMMIT)))) {
        goto fail;
    }
    commit = (git_commit *)commit_obj;

    std::cout << "master = " << git_oid_tostr_s(git_commit_id(commit)) << "\n";
    std::cout << "author = " << git_commit_author(commit)->name << "\n";
    std::cout << "message = " << git_commit_message(commit) << "\n";

fail:
    git_object_free(commit_obj);
    git_repository_free(repo);
    git_libgit2_shutdown();
    return ok;
}

// AFTER
void show_commit(char const * branch) {
    git2pp::Session git2;

    auto repo = git2[git_repository_open](".");
    auto master = repo[git_reference_dwim](branch);
    auto commit = master[git_reference_peel](GIT_OBJ_COMMIT).as<git_commit>();
    std::cout << "master = " << commit[git_commit_id]() << "\n";
    std::cout << "author = " << commit[git_commit_author]()->name << "\n";
    std::cout << "message = " << commit[git_commit_message]() << "\n";
}

*/

#include <git2.h>

#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>


inline
std::ostream & operator<<(std::ostream & os, git_oid const * oid) {
    return os << git_oid_tostr_s(oid);
}


namespace git2pp {

    class Error : public std::runtime_error {
    public:
        using runtime_error::runtime_error;
    };


    void check(int rc) {
        if (rc < 0) {
            std::ostringstream oss;
            oss << "git2 error " << rc;
            if (auto err = giterr_last()) {
                oss << "/" << err->klass <<  ": " << err->message;
            }
            throw Error{oss.str()};
        }
    }


    namespace detail {

        template <typename T> struct obj_free {};

#define GIT2PP_OBJ_FREE_(type) \
        template <> struct obj_free<git_##type> { \
            void operator()(git_##type * t) const { git_##type##_free(t); } \
        };

        GIT2PP_OBJ_FREE_(annotated_commit);
        GIT2PP_OBJ_FREE_(blame);
        GIT2PP_OBJ_FREE_(blob);
        GIT2PP_OBJ_FREE_(branch_iterator);
        GIT2PP_OBJ_FREE_(buf);
        GIT2PP_OBJ_FREE_(commit);
        GIT2PP_OBJ_FREE_(config);
        GIT2PP_OBJ_FREE_(config_entry);
        GIT2PP_OBJ_FREE_(config_iterator);
        //GIT2PP_OBJ_FREE_(cred);
        GIT2PP_OBJ_FREE_(describe_result);
        GIT2PP_OBJ_FREE_(diff);
        GIT2PP_OBJ_FREE_(diff_stats);
        GIT2PP_OBJ_FREE_(filter_list);
        //GIT2PP_OBJ_FREE_(hashsig);
        GIT2PP_OBJ_FREE_(index);
        GIT2PP_OBJ_FREE_(index_conflict_iterator);
        GIT2PP_OBJ_FREE_(indexer);
        GIT2PP_OBJ_FREE_(merge_file_result);
        GIT2PP_OBJ_FREE_(note);
        GIT2PP_OBJ_FREE_(note_iterator);
        GIT2PP_OBJ_FREE_(object);
        GIT2PP_OBJ_FREE_(odb);
        GIT2PP_OBJ_FREE_(odb_object);
        GIT2PP_OBJ_FREE_(odb_stream);
        GIT2PP_OBJ_FREE_(oid_shorten);
        GIT2PP_OBJ_FREE_(oidarray);
        GIT2PP_OBJ_FREE_(packbuilder);
        GIT2PP_OBJ_FREE_(patch);
        GIT2PP_OBJ_FREE_(pathspec);
        GIT2PP_OBJ_FREE_(pathspec_match_list);
        GIT2PP_OBJ_FREE_(rebase);
        GIT2PP_OBJ_FREE_(refdb);
        GIT2PP_OBJ_FREE_(reference);
        GIT2PP_OBJ_FREE_(reference_iterator);
        GIT2PP_OBJ_FREE_(reflog);
        GIT2PP_OBJ_FREE_(remote);
        GIT2PP_OBJ_FREE_(repository);
        GIT2PP_OBJ_FREE_(revwalk);
        GIT2PP_OBJ_FREE_(signature);
        GIT2PP_OBJ_FREE_(status_list);
        GIT2PP_OBJ_FREE_(strarray);
        GIT2PP_OBJ_FREE_(submodule);
        GIT2PP_OBJ_FREE_(tag);
        GIT2PP_OBJ_FREE_(tree);
        GIT2PP_OBJ_FREE_(tree_entry);
        GIT2PP_OBJ_FREE_(treebuilder);


        template <typename T> struct obj_dup {};

#define GIT2PP_OBJ_DUP_(type) \
        template <> struct obj_dup<git_##type> { \
            static int dup(git_##type * * out, git_##type const * t) { \
                return git_##type##_dup(out, (git_##type *)t); \
            } \
        };

        GIT2PP_OBJ_DUP_(object);
        GIT2PP_OBJ_DUP_(odb_object);
        GIT2PP_OBJ_DUP_(remote);
        GIT2PP_OBJ_DUP_(signature);
        GIT2PP_OBJ_DUP_(tree_entry);

#define GIT2PP_OBJ_OBJECT_DUP_(type) \
        template <> struct obj_dup<git_##type> { \
            static int dup(git_##type * * out, git_##type const * t) { \
                return git_object_dup((git_object * *)out, (git_object *)t); \
            } \
        };

        GIT2PP_OBJ_OBJECT_DUP_(blob);
        GIT2PP_OBJ_OBJECT_DUP_(commit);
        GIT2PP_OBJ_OBJECT_DUP_(tag);
        GIT2PP_OBJ_OBJECT_DUP_(tree);

    }


    template <typename T>
    class UniquePtr;


    namespace detail {

        template <typename T, typename... Params, typename... Args>
        UniquePtr<T> wrap(int (*f)(T * * t, Params... params), Args &&... args);

    }


    template <typename T>
    class UniquePtr {
    public:
        UniquePtr(T * t) : t_{t} { }
        UniquePtr(UniquePtr const & t) : UniquePtr{t[&detail::obj_dup<T>::dup]()} { }
        UniquePtr(UniquePtr &&) = default;

        UniquePtr & operator=(UniquePtr const & t) {
            if (this != &t) {
                t_ = std::move(t[&detail::obj_dup<T>::dup]().t_);
            }
            return *this;
        }
        UniquePtr & operator=(UniquePtr &&) = default;

        T & operator*() const { return *t_; }

        template <typename U, typename... Params>
        auto operator[](int (* method)(U * *, T *, Params...)) const {
            return [this, method](auto &&... args) {
                return detail::wrap(method, &*t_, std::forward<decltype(args)>(args)...);
            };
        }

        template <typename U, typename... Params>
        auto operator[](int (* method)(U * *, T const *, Params...)) const {
            return [this, method](auto &&... args) {
                return detail::wrap(method, &*t_, std::forward<decltype(args)>(args)...);
            };
        }

        template <typename R, typename... Params>
        auto operator[](R (* method)(T *, Params...)) const {
            return [this, method](auto &&... args) {
                return method(&*t_, std::forward<decltype(args)>(args)...);
            };
        }

        template <typename R, typename... Params>
        auto operator[](R (* method)(T const *, Params...)) const {
            return [this, method](auto &&... args) {
                return method(&*t_, std::forward<decltype(args)>(args)...);
            };
        }

        template <typename U>
        UniquePtr<U> as() && {
            return {(U *)t_.release()};
        }

        template <typename U>
        U * as() const {
            return (U *)t_.get();
        }

    private:
        std::unique_ptr<T, detail::obj_free<T>> t_;
    };


    namespace detail {

        template <typename T, typename... Params, typename... Args>
        UniquePtr<T> wrap(int (*f)(T * * t, Params... params), Args &&... args) {
            T * t;
            check(f(&t, std::forward<Args>(args)...));
            return {t};
        }

    }


    class Session {
    public:
        Session() {
            git_libgit2_init();
        }

        ~Session() {
            git_libgit2_shutdown();
        }

        template <typename T, typename... Params>
        auto operator[](int (* method)(T * *, Params...)) {
            return [this, method](auto &&... args) {
                return detail::wrap(method, std::forward<decltype(args)>(args)...);
            };
        }
    };

}

#endif // GIT2PP_H
