## Build Instructions
    $ make

## Unit Tests
Several unit tests are provided in the `unittests` directory. These unit tests must be assembled
before use with the simulator. Ensure that the assembler is compiled (this should be done after completing PA0). 
The unit tests can all be assembled by executing the following command in the `pa2` directory:

    $ ../assembler/assembler unittests/unit_test_file.asm <path>/unit_test_file.out

where `unit_test_file` is any of the unit test files (written in riscv-uconn assembly) in the
`unittests` directory. You may also use the provided `assemble_all.sh' script:

    $ bash assemble_all.sh

## Usage
    $ ./simulator ./assembled_tests/assembled_program_file.out FORWARDING_ENABLED

where `assembled_program_file.out` may be any assembled program file generated by the riscv-uconn
assembler, and `FORWARDING_ENABLED` may be 0 (disabled) or 1 (enabled).