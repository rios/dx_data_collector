# Contributing to dx_data_collector

Thank you for your interest in contributing!

> **Note:** This repository is archived. It is published for transparency and reference only.
> Active development has ceased. Pull requests and issues may not be reviewed or merged.

## If You Fork This Project

If you fork and actively develop this project, here are the contribution guidelines we followed:

### Branch Naming

- `feature/<description>` — new features
- `hotfix/<description>` — urgent bug fixes
- `chore/<description>` — maintenance, dependency updates, refactoring

### Pull Requests

1. Create a branch from `develop` (or `main` if `develop` does not exist).
2. Make focused, well-described commits.
3. Open a pull request with a clear description of what changed and why.
4. Ensure the build passes before requesting review.

### Code Style

- C++17 standard
- Follow the existing naming conventions (snake_case for variables/functions, PascalCase for classes)
- Use Doxygen-style comments for public API headers

### Building and Testing

See the [README](README.md) for build instructions.

To run tests:

```bash
catkin build --catkin-make-args run_tests
```

### Reporting Issues

If you encounter a bug or have a feature request for a fork of this project, please file an issue
in your fork's issue tracker with:

- A clear description of the problem or request
- Steps to reproduce (for bugs)
- Expected vs. actual behavior
- ROS version and OS details

## License

By contributing, you agree that your contributions will be licensed under the Apache-2.0 license.
