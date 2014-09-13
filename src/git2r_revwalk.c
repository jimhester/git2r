/*
 *  git2r, R bindings to the libgit2 library.
 *  Copyright (C) 2013-2014 The git2r contributors
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License, version 2,
 *  as published by the Free Software Foundation.
 *
 *  git2r is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <Rdefines.h>
#include "git2.h"

#include "git2r_arg.h"
#include "git2r_commit.h"
#include "git2r_error.h"
#include "git2r_repository.h"

/**
 * Count number of revisions.
 *
 * @param walker The walker to pop the commit from.
 * @return The number of revisions
 */
static size_t git2r_revwalk_count(git_revwalk *walker)
{
    size_t n = 0;
    git_oid oid;

    while (!git_revwalk_next(&oid, walker))
        n++;
    return n;
}

/**
 * List revisions
 *
 * @param repo S4 class git_repository
 * @param topological Sort the commits by topological order; Can be
 * combined with time.
 * @param time Sort the commits by commit time; can be combined with
 * topological.
 * @param reverse Sort the commits in reverse order
 * @return list with S4 class git_commit objects
 */
SEXP git2r_revwalk_list(
    SEXP repo,
    SEXP topological,
    SEXP time,
    SEXP reverse)
{
    int i=0;
    int err = GIT_OK;
    SEXP result = R_NilValue;
    size_t n = 0;
    unsigned int sort_mode = GIT_SORT_NONE;
    git_revwalk *walker = NULL;
    git_repository *repository = NULL;

    if (GIT_OK != git2r_arg_check_logical(topological))
        git2r_error(git2r_err_logical_arg, __func__, "topological");
    if (GIT_OK != git2r_arg_check_logical(time))
        git2r_error(git2r_err_logical_arg, __func__, "time");
    if (GIT_OK != git2r_arg_check_logical(reverse))
        git2r_error(git2r_err_logical_arg, __func__, "reverse");

    repository = git2r_repository_open(repo);
    if (!repository)
        git2r_error(git2r_err_invalid_repository, __func__, NULL);

    if (git_repository_is_empty(repository)) {
        /* No commits, create empty list */
        PROTECT(result = allocVector(VECSXP, 0));
        goto cleanup;
    }

    if (LOGICAL(topological)[0])
        sort_mode |= GIT_SORT_TOPOLOGICAL;
    if (LOGICAL(time)[0])
        sort_mode |= GIT_SORT_TIME;
    if (LOGICAL(reverse)[0])
        sort_mode |= GIT_SORT_REVERSE;

    err = git_revwalk_new(&walker, repository);
    if (GIT_OK != err)
        goto cleanup;

    err = git_revwalk_push_head(walker);
    if (GIT_OK != err)
        goto cleanup;
    git_revwalk_sorting(walker, sort_mode);

    /* Count number of revisions before creating the list */
    n = git2r_revwalk_count(walker);

    /* Create list to store result */
    PROTECT(result = allocVector(VECSXP, n));

    git_revwalk_reset(walker);
    err = git_revwalk_push_head(walker);
    if (GIT_OK != err)
        goto cleanup;
    git_revwalk_sorting(walker, sort_mode);

    for (;;) {
        git_commit *commit;
        SEXP sexp_commit;
        git_oid oid;

        err = git_revwalk_next(&oid, walker);
        if (GIT_OK != err) {
            if (GIT_ITEROVER == err)
                err = GIT_OK;
            goto cleanup;
        }

        err = git_commit_lookup(&commit, repository, &oid);
        if (GIT_OK != err)
            goto cleanup;

        PROTECT(sexp_commit = NEW_OBJECT(MAKE_CLASS("git_commit")));
        git2r_commit_init(commit, repo, sexp_commit);
        SET_VECTOR_ELT(result, i, sexp_commit);
        UNPROTECT(1);
        i++;

        git_commit_free(commit);
    }

cleanup:
    if (walker)
        git_revwalk_free(walker);

    if (repository)
        git_repository_free(repository);

    if (R_NilValue != result)
        UNPROTECT(1);

    if (GIT_OK != err)
        git2r_error(git2r_err_from_libgit2, __func__, giterr_last()->message);

    return result;
}

/**
 * Get list with contributions.
 *
 * @param repo S4 class git_repository
 * @param topological Sort the commits by topological order; Can be
 * combined with time.
 * @param time Sort the commits by commit time; can be combined with
 * topological.
 * @param reverse Sort the commits in reverse order
 * @return list with S4 class git_commit objects
 */
SEXP git2r_revwalk_contributions(
    SEXP repo,
    SEXP topological,
    SEXP time,
    SEXP reverse)
{
    int i=0;
    int err = GIT_OK;
    SEXP result = R_NilValue;
    SEXP names = R_NilValue;
    SEXP when = R_NilValue;
    SEXP author = R_NilValue;
    SEXP email = R_NilValue;
    size_t n = 0;
    unsigned int sort_mode = GIT_SORT_NONE;
    git_revwalk *walker = NULL;
    git_repository *repository = NULL;

    if (GIT_OK != git2r_arg_check_logical(topological))
        git2r_error(git2r_err_logical_arg, __func__, "topological");
    if (GIT_OK != git2r_arg_check_logical(time))
        git2r_error(git2r_err_logical_arg, __func__, "time");
    if (GIT_OK != git2r_arg_check_logical(reverse))
        git2r_error(git2r_err_logical_arg, __func__, "reverse");

    repository = git2r_repository_open(repo);
    if (!repository)
        git2r_error(git2r_err_invalid_repository, __func__, NULL);

    if (git_repository_is_empty(repository))
        goto cleanup;

    if (LOGICAL(topological)[0])
        sort_mode |= GIT_SORT_TOPOLOGICAL;
    if (LOGICAL(time)[0])
        sort_mode |= GIT_SORT_TIME;
    if (LOGICAL(reverse)[0])
        sort_mode |= GIT_SORT_REVERSE;

    err = git_revwalk_new(&walker, repository);
    if (GIT_OK != err)
        goto cleanup;

    err = git_revwalk_push_head(walker);
    if (GIT_OK != err)
        goto cleanup;
    git_revwalk_sorting(walker, sort_mode);

    /* Count number of revisions before creating the list */
    n = git2r_revwalk_count(walker);

    /* Create vectors to store result */
    PROTECT(when = allocVector(REALSXP, n));
    PROTECT(author = allocVector(STRSXP, n));
    PROTECT(email = allocVector(STRSXP, n));
    PROTECT(names = allocVector(STRSXP, 3));
    PROTECT(result = allocVector(VECSXP, 3));
    SET_VECTOR_ELT(result, 0, when);
    SET_STRING_ELT(names, 0, mkChar("when"));
    SET_VECTOR_ELT(result, 1, author);
    SET_STRING_ELT(names, 1, mkChar("author"));
    SET_VECTOR_ELT(result, 2, email);
    SET_STRING_ELT(names, 2, mkChar("email"));
    setAttrib(result, R_NamesSymbol, names);

    git_revwalk_reset(walker);
    err = git_revwalk_push_head(walker);
    if (GIT_OK != err)
        goto cleanup;
    git_revwalk_sorting(walker, sort_mode);

    for (;;) {
        git_commit *commit;
        const git_signature *c_author;
        git_oid oid;

        err = git_revwalk_next(&oid, walker);
        if (GIT_OK != err) {
            if (GIT_ITEROVER == err)
                err = GIT_OK;
            goto cleanup;
        }

        err = git_commit_lookup(&commit, repository, &oid);
        if (GIT_OK != err)
            goto cleanup;

        c_author = git_commit_author(commit);
        REAL(when)[i] =
            (double)(c_author->when.time) +
            60 * (double)(c_author->when.offset);
        SET_STRING_ELT(author, i, mkChar(c_author->name));
        SET_STRING_ELT(author, i, mkChar(c_author->email));
        i++;
        git_commit_free(commit);
    }

cleanup:
    if (walker)
        git_revwalk_free(walker);

    if (repository)
        git_repository_free(repository);

    if (R_NilValue != result)
        UNPROTECT(5);

    if (GIT_OK != err)
        git2r_error(git2r_err_from_libgit2, __func__, giterr_last()->message);

    return result;
}
