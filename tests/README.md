# Tests

The project uses no third-party test framework.

- test_main.cpp contains small unit and integration-style checks for Board,
  Solver, PuzzleGenerator, and Game.
- e2e_cli.sh drives the compiled app with deterministic input and checks the
  startup menus, help, malformed input, EOF, seed validation, and clean exit.
- ../test_runner.sh builds with strict warnings and runs both test levels.

From the repository root:

~~~bash
bash test_runner.sh
~~~

Generator tests use fixed std::mt19937 seeds so failures can be reproduced.
They validate all nine size/difficulty combinations and independently confirm
that each puzzle has exactly one solution.
