---
name: create-pr
description: Create and follow through on a GitHub pull request. Use this skill whenever the user asks to open, create, submit, or prepare a PR, or to push a branch for review. Check the branch and diff, commit only intended files, run relevant validation, open the PR with a useful description, and inspect its quality gate.
---

# Create a pull request

Turn the user's requested changes into a reviewable PR. Follow the current repository instructions and the user's requested order of operations. A request to create a PR authorizes the commits and push needed to open it; it does not authorize merging unless the user also requests a merge. If the user asks only for PR text or a review, deliver that without changing Git or GitHub state.

## Inspect the repository

1. Read `AGENTS.md` and any applicable instructions before changing files. Inspect the current branch, working tree, remotes, default branch, and existing PR for the branch.
2. Use the user's named branch and base when given. Otherwise, reuse the current feature branch. If the work is still on the default branch, create a task branch as required by the repository instructions, unless the user explicitly directed a commit there.
3. Refresh the target refs when network access is available. Compare the complete branch against the intended base using the merge base. Review the commit list, changed paths, and diff so unrelated or private files do not enter the PR.
4. Preserve unrelated working tree changes. Stage explicit paths instead of using `git add .` or `git add -A`. Check staged changes before committing.

## Prepare the branch

1. Finish only work needed for the requested PR. Keep changes within its scope.
2. Read the relevant workflow under `.github/workflows/` and run checks that are practical locally. For this project, the PR quality gate targets `main` and checks shell syntax, native build and tests, Linux packaging, Windows build and tests, and package contents. Run the applicable local checks; report platform checks that require CI.
3. Check whitespace and the final diff. Commit with a concise message that describes the change. Do not amend, rebase, reset, or force push published commits without a specific reason and authorization.
4. Push the branch to its intended remote. If a PR already exists for that branch and base, update it instead of opening a duplicate.

## Open and follow the PR

1. Use GitHub CLI or the available GitHub connector. Choose a draft PR only when the user requested a draft or the work is deliberately incomplete.
2. Write a concrete title and body from the actual diff. Include a short summary, validation performed, and any material limitations or risks. Mention relevant packaging or license effects when the change affects the distributed ZIP. Do not claim that CI passed before it has passed.
3. Open the PR against the intended base and provide its URL. Inspect the quality gate once it starts. If a check fails, read its logs, fix an in-scope defect on the same branch, push the fix, and check the new run. If the failure cannot be resolved within scope, report the specific failure and what remains.
4. Merge only when the user explicitly requested a merge and the required checks have passed. Respect repository branch protection and the user's requested merge method.

## Report

Tell the user the PR URL, source and base branches, what changed, validation results, and any unresolved checks. Keep the report accurate to the latest commit and CI run.
