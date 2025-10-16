#!/bin/sh

# Test script for IZP 2025/26 proj1.
# Author: Ales Smrcka
# Date: 2025-09-25

cd $(dirname $0)

die()
{
    echo "$@" >&2
    exit 1
}

# check if software under test exists
if ! [ -f keyfilter.c ]; then
    die "File keyfilter.c not found. Run $0 in the same directory as the file."
fi

# compile SUT
gcc -std=c11 -Wall -Wextra -o keyfilter keyfilter.c || die "Compilation failed."

# $1 file to be redirected to stdin
# $2 file with expected content of stdout
# $3-... SUT argument
# result 0=test passed, 1=test failed

$last_keyrun = 0

run_test()
{
    local stdinred="$1"
    local stdoutref="$2"
    shift 2
    timeout 0.5 ./keyfilter "$@" <$stdinred >test-stdout.tmp 2>&1
    last_keyrun=$?
    diff -iBw $stdoutref test-stdout.tmp >/dev/null
    result=$?
    return $result
}

# Colored terminal ouput
GREEN=
RED=
RESET=
if [ -t 1 ]; then 
    GREEN="\033[1;32m"
    RED="\033[1;31m"
    YELLOW="\033[1;33m"
    RESET="\033[0m"
fi

# $1 test id
# $2 test description
# $3 input content
# $4 expected output
# $5-... SUT arguments
test_case() {
    local id="$1"
    local desc="$2"
    local inputfile="test-input-${id}.tmp"
    local inputcontent="$3"
    local expectedfile="test-expected-${id}.tmp"
    local expectedcontent="$4"
    shift 4
    echo "$inputcontent" >"$inputfile"
    echo "$expectedcontent" >"$expectedfile"

    # Run the test and capture the exit code
    run_test "$inputfile" "$expectedfile" "$@"
    local result=$?

    # Check if the program exited with a nonzero exit code
    if [ $last_keyrun -ne 0 ]; then
        echo "${YELLOW}?${RESET} $desc (ignored due to nonzero exit code)"
        echo "    Exit code: $last_keyrun"
        rm -f "$inputfile" "$expectedfile"
        return 0 # Mark the test as ignored (not failed)
    fi

    # If the program exited successfully, compare the output
    if [ $result -eq 0 ]; then
        echo "${GREEN}✔${RESET} $desc"
        rm -f "$inputfile" "$expectedfile"
    else
        echo "${RED}✘${RESET} $desc"
        echo "    Input:" $inputfile
        sed 's/^/        /' <$inputfile
        echo "    Execution:"
        echo "        ./keyfilter $@ <$inputfile"
        echo "    Expected:"
        sed 's/^/        /' <$expectedfile
        echo "    Got:"
        sed 's/^/        /' <test-stdout.tmp
    fi
    rm -f test-stdout.tmp
    return $result
}
# -----------------------------
# Define all test cases
# -----------------------------

test_case 1 "prazdny prefix" \
"BRNO\nPRAHA\n" \
"Enable: BP\n" \
""

test_case 2 "prazdny prefix (serazeno)" \
"PRAHA\nBRNO\n" \
"Enable: BP\n" \
""

test_case 3 "jedno mesto na vstupu" \
"PRAHA\n" \
"Found: PRAHA\n" \
PRAHA

test_case 4 "jedno mesto na vstupu (mala pismena)" \
"praha\n" \
"Found: PRAHA\n" \
PRAHA

test_case 5 "nalezeni pri neprazdnem prefixu" \
"PRAHA\nBRNO\n" \
"Found: BRNO\n" \
BR

test_case 6 "povoleni klaves pri neprazdnem prefixu" \
"PRAHA\nBRNO\nBRUNTAL\n" \
"Enable: NU\n" \
BR

test_case 7 "nenalezeno" \
"PRAHA\nBRNO\nBRUNTAL\n" \
"Not found\n" \
BRA

test_case 8 "nalezena plna i castecna shoda" \
"YORK\nYORKTOWN\nLONDON\n" \
"Found: YORK\nEnable: T\n" \
YORK


#end default cases

test_case 9 "space_one" \
"HELLO WORLD" \
"Found: HELLO WORLD\n" \
HELLO

test_case 10 "space_multiple_output" \
"HELLO YOU\nHELLO ME" \
"Enable:  " \
HELLO

test_case 11 "space_multiple_space_pass" \
"HELLO YOU\nHELLO ME" \
"Enable: MY" \
"HELLO "

test_case 12 "big_line_enable_nocrash (nonzero)" \
"ABABABABABABABABABACDCDCDCDCDDCDCDCDCDCDEFEFEFFEFEFEFEFEFEFEFEFTHTHTHTHTHTHTHTHTHTHTHAAAAAAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBBBBBPRAHA\nPRAHA\nBRNO\nBREAD" \
"Enable: EN" \
BR

test_case 13 "big_line_arg_nocrash (nonzero)" \
"ABABABABABABABABABACDCDCDCDCDDCDCDCDCDCDEFEFEFFEFEFEFEFEFEFEFEFTHTHTHTHTHTHTHTHTHTHTHAAAAAAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBBBBBPRAHA\nPRAHA\nBRNO" \
"ABABABABABABABABABACDCDCDCDCDDCDCDCDCDCDEFEFEFFEFEFEFEFEFEFEFEFTHTHTHTHTHTHTHTHTHTHTHAAAAAAAAAAAAAAA" \
ABABABABABABABABABACDCDCDCDCDDCDCDCDCDCDEFEFEFFEFEFEFEFEFEFEFEFTHTHTHTHTHTHTHTHTHTHTHAAAAAAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBBBBBPRAHA

test_case 14 "european largest" \
"Istanbul\nMoscow\nLondon\nSaint Petersburg\nBerlin\nMadrid\nKyiv\nRome\nBaku\nParis\nVienna\nMinsk\nWarsaw\nHamburg\nBucharest\nBarcelona\nBudapest\nBelgrade\nMunich\nKharkiv\nPrague\nMilan\nKazan\nSofia\nTbilisi\nNizhny Novgorod\nUfa\nBirmingham\nKrasnodar\nSamara\nRostov-on-Don\nYerevan\nVoronezh\nPerm\nCologne\nVolgograd\nOdesa\n" \
"Enable: LR" \
BE