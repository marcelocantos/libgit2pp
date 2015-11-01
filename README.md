# libgit2pp

C++ convenience wrappers for libgit2. libgit2pp is a single header file, git2pp.h,
to include in lieu of git2.h. Put it wherever suits your project.

To keep the code base small and future-proof, the library uses metaprogramming
techniques to avoid replicating the entire API, while still allowing much more
succinct code than raw libgit2.

## Documentation

* namespace **`git2pp`**

  * **`Error`**/**`check`:** check() throws Error when a libgit2 result indicates failure.

  * **`Session`:** wraps `git_libgit2_init()` and `git_libgit2_shutdown()` in RAII.

    * **`operator[]()`:** Wraps a libgit2 function. If the wrapped function has a `T * *`
      as its first parameter, the returned function removes this parameter and instead
      returns a `UniquePtr<T>` (see below).

      ```cpp
      // libgit2
      git_libgit2_init();
      git_repository * repo = NULL;
      int err = git_repository_open(&repo, ".");
      if (err < 0) {
          …
      }
      …
      git_repository_free(repo);
      git_libgit2_shutdown();
      ```

      ```cpp
      // git2pp
      git2pp::Session git2;
      auto repo = git2[git_repository_open](".");  // Exception on failure.
      ```

  * **`UniquePtr<T>`:** Smart pointer wrapping a libgit2 pointer.

    * **`ctor/operator=(UniquePtr &&)`:** Transfers ownership.

    * **`ctor/operator=(UniquePtr const &)`:** Copies using `git_…_dup` function,
      where such a function exists.

    * **`operator[]()`:** Wraps libgit2 functions with sigs `(T [const] *, …)` or
      `(U * *, T [const] *, …)`, where `U` is some other output type. The resulting
      function removes the `T [const] *` parameter, instead passing `this` behind the
      scenes. It also removes `U * *` and instead returns `UniquePtr<U>`.

      ```cpp
      // libgit2
      git_reference *ref = NULL;
      int err = git_reference_dwim(&ref, "master");
      if (err < 0) {
          …
      }
      …
      git_reference_free(ref);
      ```

      ```cpp
      // git2pp. repo[…] returns a function that takes a char const *, calls
      // git_reference_dwim, and returns a UniquePtr<git_reference>.
      auto master = repo[git_reference_dwim]("master");
      ```

    * **`as<U>()`:** Casts one libgit2 object type to another. If `this` is an
      rvalue reference, transfers ownership to a new `UniquePtr<U>`, otherwise
      returns a raw `U *`. WARNING: Will succeed for any pair of types,
      whether it's valid or not.

  * **`std::ostream & operator<<(std::ostream & os, git_oid const * oid)`:**
    Outputs the hex representation of `oid`.


If a libgit2 function takes additional pointer-to-T parameters and you have
a `UniquePtr<T>`, `t`, you can pass it in as `&*t`, e.g.:

```cpp
auto me = repo[git_signature_now]("Me", "me@example.com");
commit[git_commit_amend](…, &*me, &*me, …);
```

## Example

### Before

```cpp
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

    if (!(ok = git2ok(git_reference_dwim(&master, repo, branch)))) {
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
```

### After

```cpp
void show_commit(char const * branch) {
    git2pp::Session git2;

    auto repo = git2[git_repository_open](".");
    auto master = repo[git_reference_dwim](branch);
    auto commit = master[git_reference_peel](GIT_OBJ_COMMIT).as<git_commit>();
    std::cout << "master = " << commit[git_commit_id]() << "\n";
    std::cout << "author = " << commit[git_commit_author]()->name << "\n";
    std::cout << "message = " << commit[git_commit_message]() << "\n";
}
```
