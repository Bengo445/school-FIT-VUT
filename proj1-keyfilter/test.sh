#!/bin/sh

# Test script for IZP 2025/26 proj1.
# Author: Ales Smrcka
# Date: 2025-09-25
export POSIXLY_CORRECT=y

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
run_test()
{
    local stdinred="$1"
    local stdoutref="$2"
    shift 2
    timeout 0.5 ./keyfilter "$@" <$stdinred >test-stdout.tmp 2>&1
    diff -iBw $stdoutref test-stdout.tmp >/dev/null
    result=$?
    rm -f test-stdout.tmp
    return $result
}

# Colored terminal ouput
GREEN=
RED=
RESET=
if [ -t 1 ]; then 
    GREEN="\033[1;32m"
    RED="\033[1;31m"
    RESET="\033[0m"
fi

# $1 test id
# $2 test description
# $3 input content
# $4 expected output
# $5-... SUT arguments
test_case()
{
    local id="$1"
    local desc="$2"
    local inputfile="test-input-${id}.tmp"
    local inputcontent="$3"
    local expectedfile="test-expected-${id}.tmp"
    local expectedcontent="$4"
    shift 4
    printf "$inputcontent" >"$inputfile"
    printf "$expectedcontent" >"$expectedfile"

    run_test "$inputfile" "$expectedfile" "$@"
    local result=$?

    if [ $result -eq 0 ]; then
        printf "${GREEN}✔${RESET} $desc\n"
        rm -f "$inputfile" "$expectedfile"
    else
        printf "${RED}✘${RESET} $desc\n"
        echo "    Input:" $inputfile
        sed 's/^/        /' <$inputfile
        echo "    Execution:"
        echo "        ./keyfilter $@ <$inputfile"
        echo "    Expected:"
        sed 's/^/        /' <$expectedfile
    fi
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
