//
//  libgit2pp
//
//  https://github.com/marcelocantos/libgit2pp
//
//  Distributed under the Apache License, Version 2.0. (See accompanying
//  file LICENSE or copy at http://www.apache.org/licenses/LICENSE-2.0)
//

#ifndef GIT2PP_H
#define GIT2PP_H

#include <git2.h>

#if !(LIBGIT2_VER_MAJOR == 0 && LIBGIT2_VER_MINOR < 28)
# define LIBGIT2PP_HAVE_INDEX_ITERATOR 1
#else
# define LIBGIT2PP_HAVE_INDEX_ITERATOR 0
#endif
#if !(LIBGIT2_VER_MAJOR == 0 && LIBGIT2_VER_MINOR < 25)
# define LIBGIT2PP_HAVE_REFERENCE_DUP 1
#else
# define LIBGIT2PP_HAVE_REFERENCE_DUP 0
#endif

#include <functional>
#include <iostream>
#include <memory>
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


    inline
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

        template <typename T> struct obj_no_free {
            void operator()(T * t) const { }
        };

    }


    template <typename T, typename Free = detail::obj_free<T>>
    class UniquePtr;


    namespace detail {

#define GIT2PP_OBJ_FREE_(type) \
        template <> struct obj_free<::git_##type> { \
            void operator()(::git_##type * t) const { ::git_##type##_free(t); } \
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
#if LIBGIT2PP_HAVE_INDEX_ITERATOR
        GIT2PP_OBJ_FREE_(index_iterator);
#endif
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

#define GIT2PP_OBJ_NO_FREE_(type) \
        template <> struct obj_free<type> { \
            void operator()(type * t) const { } \
        };

        GIT2PP_OBJ_NO_FREE_(const git_index_entry);

        template <typename T> struct obj_dup {};

#define GIT2PP_OBJ_DUP_(type) \
        template <> struct obj_dup<git_##type> { \
            static int dup(git_##type * * out, git_##type const * t) { \
                return git_##type##_dup(out, (git_##type *)t); \
            } \
        };

        GIT2PP_OBJ_DUP_(object);
        GIT2PP_OBJ_DUP_(odb_object);
#if LIBGIT2PP_HAVE_REFERENCE_DUP
        GIT2PP_OBJ_DUP_(reference);
#endif
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

    namespace detail {

        template <typename T> class MaybeIterable {
            using iterable = std::nullptr_t;
        };

        template <typename T, typename... Params, typename... Args>
        UniquePtr<T> wrap(int (*f)(T * * t, Params... params), Args &&... args);

        template <typename... Params, typename... Args>
        git_oid wrapOid(int (*f)(git_oid * oid, Params... params), Args &&... args);

    }


    template <typename T, typename Free>
    class UniquePtr : public detail::MaybeIterable<UniquePtr<T, Free>> {
    public:
        using value_type = T;

        UniquePtr(T * t = nullptr) : t_{t} { }

        template <typename U = T>
        UniquePtr(UniquePtr const & t, std::enable_if_t<LIBGIT2PP_HAVE_REFERENCE_DUP || !std::is_same_v<U, git_reference>>* = 0)
        : UniquePtr{t ? t[&detail::obj_dup<T>::dup]() : nullptr}
        { }

        UniquePtr(UniquePtr &&) = default;

        UniquePtr & operator=(UniquePtr const & t) {
            if (this != &t) {
                t_ = std::move(t[&detail::obj_dup<T>::dup]().t_);
            }
            return *this;
        }
        UniquePtr & operator=(UniquePtr &&) = default;

        void reset(T * t = nullptr) {
            t_.reset(t);
        }

        explicit operator bool() const { return bool(t_); }

        T & operator*() const { return *t_; }

        T * operator->() const { return t_.get(); }

        bool operator==(UniquePtr const & that) const { return t_ == that.t_; }
        bool operator!=(UniquePtr const & that) const { return t_ != that.t_; }

        template <typename U, typename... Params, typename = std::enable_if<!std::is_const_v<T>>>
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
        auto operator[](R (* method)(git_oid *, Params...)) const {
            return [this, method](auto &&... args) {
                return detail::wrapOid(method, &*t_, std::forward<decltype(args)>(args)...);
            };
        }

        template <typename R, typename... Params, typename = std::enable_if<!std::is_const_v<T>>>
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
        std::unique_ptr<T, Free> t_;
    };


    namespace detail {

        template <typename T, typename... Params, typename... Args>
        UniquePtr<T> wrap(int (*f)(T * * t, Params... params), Args &&... args) {
            T * t;
            check(f(&t, std::forward<Args>(args)...));
            return {t};
        }

        template <typename... Params, typename... Args>
        git_oid wrapOid(int (*f)(git_oid * oid, Params... params), Args &&... args) {
            git_oid oid;
            check(f(&oid, std::forward<Args>(args)...));
            return oid;
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

    template <typename I, typename NextF, typename Derived>
    class IteratorBase {
    public:
        using git_iterator = I;
        using next_f = NextF;

        IteratorBase(UniquePtr<I> * i, NextF next) : i_{i}, next_{std::move(next)} { }
        IteratorBase() : i_{nullptr} {}
        IteratorBase(IteratorBase const &) = delete;
        IteratorBase(IteratorBase &&) = delete;

        bool operator==(Derived const & that) { return done() && that.done(); }
        bool operator!=(Derived const & that) { return !(*this == that); }

        Derived & operator++() {
            auto self = static_cast<Derived *>(this);
            int rc;
            self->increment(rc);
            if (rc != 0) {
                if (rc != GIT_ITEROVER) {
                    check(rc);
                }
                this->i_ = nullptr;
            }
            return *self;
        }

    protected:
        NextF next_;
        UniquePtr<I> * i_;

    private:
        bool done() const { return !i_; }
    };

    template <typename I, typename NextF, typename T, typename Free = detail::obj_free<T>>
    class Iterator : public IteratorBase<I, NextF, Iterator<I, NextF, T, Free>> {
    private:
        using base = IteratorBase<I, NextF, Iterator>;
    public:
        Iterator(UniquePtr<I> * i, NextF next) : base{i, std::move(next)} { ++*this; }
        Iterator() = default;

        void increment(int & rc) {
            T * t;
            if ((rc = base::next_(&t, &**base::i_)) == 0) {
                t_.reset(t);
            }
        }

        UniquePtr<T, Free> operator*() const { return std::move(t_); }
        UniquePtr<T, Free> & operator->() const { return t_; }

    private:
        mutable UniquePtr<T, Free> t_;
        friend base;
    };

    template <typename I, typename NextF, typename T>
    class StructIterator : public IteratorBase<I, NextF, StructIterator<I, NextF, T>> {
    private:
        using base = IteratorBase<I, NextF, StructIterator>;
    public:
        StructIterator(UniquePtr<I> * i, NextF next) : base{i, std::move(next)} { ++*this; }
        StructIterator() = default;

        void increment(int & rc) {
            T t;
            if ((rc = base::next_(&t, &**base::i_)) == 0) {
                t_ = t;
            }
        }

        T operator*() const { return t_; }
        T & operator->() const { return t_; }

    private:
        mutable T t_;
        friend base;
    };

    template <typename I, typename NextF>
    class BranchIterator : public IteratorBase<I, NextF, BranchIterator<I, NextF>> {
    private:
        using base = IteratorBase<I, NextF, BranchIterator>;
    public:
        struct Entry {
            Entry() = default;
            Entry(Entry const &) = delete;
            Entry& operator=(Entry const &) = delete;
            Entry(Entry &&) = default;
            Entry& operator=(Entry &&) = default;

            UniquePtr<git_reference> ref;
            git_branch_t type;
        };

        BranchIterator(UniquePtr<I> * i, NextF next) : base{i, std::move(next)} { ++*this; }
        BranchIterator() = default;

        void increment(int & rc) {
            git_reference * ref;
            git_branch_t type;
            if ((rc = base::next_(&ref, &type, &**base::i_)) == 0) {
                e_ = {std::move(ref), type};
            }
        }

        Entry const & operator*() const { return e_; }
        Entry const * operator->() const { return &e_; }

    private:
        Entry e_;
        friend base;
    };

    template <typename I, typename NextF>
    class IndexConflictIterator : public IteratorBase<I, NextF, IndexConflictIterator<I, NextF>> {
    private:
        using base = IteratorBase<I, NextF, IndexConflictIterator>;
    public:
        struct Entry {
            git_index_entry const * ancestor;
            git_index_entry const * our;
            git_index_entry const * their;
        };

        IndexConflictIterator(UniquePtr<I> * i, NextF next) : base{i, std::move(next)} { ++*this; }
        IndexConflictIterator() = default;

        void increment(int & rc) {
            Entry e;
            if ((rc = base::next_(&e.ancestor, &e.our, &e.their, &**base::i_)) == 0) {
                e_ = e;
            }
        }

        Entry const & operator*() const { return e_; }
        Entry const * operator->() const { return &e_; }

    private:
        Entry e_;
        friend base;
    };

    template <typename I, typename NextF>
    class NoteIterator : public IteratorBase<I, NextF, NoteIterator<I, NextF>> {
    private:
        using base = IteratorBase<I, NextF, NoteIterator>;
    public:
        struct Entry {
            git_oid note_id;
            git_oid annotated_id;
        };

        NoteIterator(UniquePtr<I> * i, NextF next) : base{i, std::move(next)} { ++*this; }
        NoteIterator() = default;

        void increment(int & rc) {
            Entry e;
            if ((rc = base::next_(&e.note_id, &e.annotated_id, &**base::i_)) == 0) {
                e_ = e;
            }
        }

        Entry const & operator*() const { return e_; }
        Entry const * operator->() const { return &e_; }

    private:
        Entry e_;
        friend base;
    };

    template <typename Iterator>
    class Iterable {
    public:
        using iterator = Iterator;
        using git_iterator = typename iterator::git_iterator;
        using next_f = typename iterator::next_f;

        Iterable(next_f next) : next_(std::move(next)) { }

        Iterator begin() { return {static_cast<UniquePtr<git_iterator> *>(this), next_}; }
        Iterator end() { return {}; }

    private:
        next_f next_;
    };

    namespace detail {

        using ConfigIterable = Iterable<Iterator<git_config_iterator, decltype(&git_config_next), git_config_entry, obj_no_free<git_config_entry>>>;
        template <> class MaybeIterable<UniquePtr<git_config_iterator>> : public ConfigIterable {
        public:
            MaybeIterable() : ConfigIterable(git_config_next) {}
        };

#if LIBGIT2PP_HAVE_INDEX_ITERATOR
        using IndexIteratorIterable = Iterable<Iterator<git_index_iterator, decltype(&git_index_iterator_next), const git_index_entry>>;
        template <> class MaybeIterable<UniquePtr<git_index_iterator>> : public IndexIteratorIterable {
        public:
            MaybeIterable() : IndexIteratorIterable(git_index_iterator_next) {}
        };
#endif

        using ReferenceIterable = Iterable<Iterator<git_reference_iterator, decltype(&git_reference_next), git_reference>>;
        template <> class MaybeIterable<UniquePtr<git_reference_iterator>> : public ReferenceIterable {
        public:
            MaybeIterable() : ReferenceIterable(git_reference_next) {}
        };

        using RebaseIterable = Iterable<StructIterator<git_rebase, decltype(&git_rebase_next), git_rebase_operation *>>;
        template <> class MaybeIterable<UniquePtr<git_rebase>> : public RebaseIterable {
        public:
            MaybeIterable() : RebaseIterable(git_rebase_next) {}
        };

        using RevwalkIterable = Iterable<StructIterator<git_revwalk, decltype(&git_revwalk_next), git_oid>>;
        template <> class MaybeIterable<UniquePtr<git_revwalk>> : public RevwalkIterable {
        public:
            MaybeIterable() : RevwalkIterable(git_revwalk_next) {}
        };

        using BranchIterable = Iterable<BranchIterator<git_branch_iterator, decltype(&git_branch_next)>>;
        template <> class MaybeIterable<UniquePtr<git_branch_iterator>> : public BranchIterable {
        public:
            MaybeIterable() : BranchIterable(git_branch_next) {}
        };

        using IndexConflictIterable = Iterable<IndexConflictIterator<git_index_conflict_iterator, decltype(&git_index_conflict_next)>>;
        template <> class MaybeIterable<UniquePtr<git_index_conflict_iterator>> : public IndexConflictIterable {
        public:
            MaybeIterable() : IndexConflictIterable(git_index_conflict_next) {}
        };

        using NoteIterable = Iterable<NoteIterator<git_note_iterator, decltype(&git_note_next)>>;
        template <> class MaybeIterable<UniquePtr<git_note_iterator>> : public NoteIterable {
        public:
            MaybeIterable() : NoteIterable(git_note_next) {}
        };

    }

}

#endif // GIT2PP_H
