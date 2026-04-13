We welcome contributions from everyone, whether it’s fixing a typo, submitting a bug report, or adding new features. Please take a moment to read these guidelines before you start.

## Issues
Before opening a new issue, please check if it already exists.
Be clear and descriptive—include steps to reproduce bugs, expected vs actual behavior, and version of the content.

## Coding Guidelines
- Use consistent formatting and naming conventions.
- Keep functions and classes focused—avoid unnecessary complexity.
- Document public methods and APIs.

## Commit & Pull Request Guidelines
Pull request titles and commit names must follow the [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/) format.

The general format is: `<type>[optional scope]: <description>`

Some examples of this are:

- `feat(parser): add support for nested expressions`
- `fix(network): handle packet overflow correctly`
- `docs: Update README with setup instructions`
- `chore: Added CONTRIBUTING.md`

Please make sure to:
- Keep commits **small and focused** (one logical change per commit).
- Use **descriptive PR titles** — they should match the main commit message.
- Reference related issues in the PR body (e.g., `Closes #42`).
- Ensure your branch passes all checks (tests, lint, build) before requesting a review.
- PRs should avoid mixing unrelated changes.