# Debug tools

This directory contains automatization scripts for PostgreSQL development and debugging:

- `setup.sh` - setup environment for development: run `./configure`, create configuration files, etc. It run only once, before development starts
- `build.sh` - compile and build DB with all contribs
- `run.sh` - manage DB instance: create, run, stop, psql
- `test.sh` - run tests
- `clean.sh` - cleaning repository

> Also there is `utils.sh` script - this is an infrastructure script, which is used by other scripts.
> You should not use it directly.

Every script has `-h`/`--help` arguments to display available arguments and usage examples.

The whole `dev` directory should lie in root of PostgreSQL repository root.

Example workflow with these scripts:

1. Run `./dev/setup.sh` when got to work or cleaned repository (for short, repository is not setup)
2. Make changes in code
3. Build DB using `./dev/build.sh`
4. Run tests using `./dev/test.sh --regress`
5. Create and run DB - `./dev/run.sh --init-db --run-db`
6. Run PSQL - `./dev/run.sh --psql`
7. Set breakpoints, run debugger and enter backend PID (shown in psql at startup)
8. Run queries, debug, close session, make changes and again
9. After work, you can clean created files - `./dev/clean.sh --build`

In this repository there are VS Code configuration files including `.vscode/launch.json`. Steps above can be run using VS Code integration:

1. Run task `Setup` to setup environment
2. Make changes in code
3. Build DB using task `Build` or using shortcut `Ctrl + Shift + B`
4. Run tests using task `Test regress` or using shortcut `Ctrl + Shift + T`
5. Create and run DB using task `Run DB` (this task combines both)
6. Run PSQL using task `Run psql`
7. Set breakpoints, run debugger by pressing F5 using configuration `Backend (Attach)`
8. Run queries, debug, close session, make changes and again
9. After work, you can clean created files using task `Clean build` (or `Clean full`)

> To run task you have to run `Tasks: Run task` in command pallette.
> To speed up this, you can bind this to some shortcut. I am using `Alt + T`.
> This allows me to run tasks faster.
