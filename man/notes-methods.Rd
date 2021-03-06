% Generated by roxygen2 (4.1.1): do not edit by hand
% Please edit documentation in R/note.r
\docType{methods}
\name{notes}
\alias{notes}
\alias{notes,git_repository-method}
\title{List notes}
\usage{
notes(repo, ref = note_default_ref(repo))

\S4method{notes}{git_repository}(repo, ref = note_default_ref(repo))
}
\arguments{
\item{repo}{The repository}

\item{ref}{Reference to read from. Default is
"refs/notes/commits".}
}
\value{
list with S4 class git_note objects
}
\description{
List all the notes within a specified namespace.
}
\examples{
\dontrun{
## Create and initialize a repository in a temporary directory
path <- tempfile(pattern="git2r-")
dir.create(path)
repo <- init(path)
config(repo, user.name="Alice", user.email="alice@example.org")

## Create a file, add and commit
writeLines("Hello world!", file.path(path, "example.txt"))
add(repo, "example.txt")
commit_1 <- commit(repo, "Commit message 1")

## Create another commit
writeLines(c("Hello world!",
             "HELLO WORLD!"),
           file.path(path, "example.txt"))
add(repo, "example.txt")
commit_2 <- commit(repo, "Commit message 2")

## Create note in default namespace
note_create(commit_1, "Note-1")
note_create(commit_1, "Note-2", force = TRUE)

## Create note in named (review) namespace
note_create(commit_1, "Note-3", ref="refs/notes/review")
note_create(commit_2, "Note-4", ref="review")

## Create note on blob and tree
note_create(tree(commit_1), "Note-5")
note_create(tree(commit_1)["example.txt"], "Note-6")

## List notes in default namespace
notes(repo)

## List notes in 'review' namespace
notes(repo, "review")
}
}
\keyword{methods}

