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

#end default cases

test_case 9 "space_one" \
"HELLO WORLD\n" \
"Found: HELLO WORLD" \
HELLO

test_case 10 "space_multiple_output" \
"HELLO YOU\nHELLO ME" \
"Enable:  " \
HELLO

test_case 11 "space_multiple_space_pass" \
"HELLO YOU\nHELLO ME" \
"Enable: MY" \
"HELLO "

test_case 12 "big_line_enable" \
"ABABABABABABABABABACDCDCDCDCDDCDCDCDCDCDEFEFEFFEFEFEFEFEFEFEFEFTHTHTHTHTHTHTHTHTHTHTHAAAAAAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBBBBBPRAHA\nPRAHA\nBRNO\nBREAD" \
"Enable: EN" \
BR


test_case 13 "big_line_found" \
"ABABABABABABABABABACDCDCDCDCDDCDCDCDCDCDEFEFEFFEFEFEFEFEFEFEFEFTHTHTHTHTHTHTHTHTHTHTHAAAAAAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBBBBBPRAHA\nPRAHA\nBRNO" \
"Found: ABABABABABABABABABACDCDCDCDCDDCDCDCDCDCDEFEFEFFEFEFEFEFEFEFEFEFTHTHTHTHTHTHTHTHTHTHTHAAAAAAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBBBBBPRAHA" \
ABABABABABABABABABACDCDCDCDCDDCDCDCDCDCDEFEFEFFEFEFEFEFEFEFEFEFTHTHTHTHTHTHTHTHTHTHTHAAAAAAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBBBBBPRAHA

test_case 14 "european largest" \
"Istanbul\nMoscow\nLondon\nSaint Petersburg\nBerlin\nMadrid\nKyiv\nRome\nBaku\nParis\nVienna\nMinsk\nWarsaw\nHamburg\nBucharest\nBarcelona\nBudapest\nBelgrade\nMunich\nKharkiv\nPrague\nMilan\nKazan\nSofia\nTbilisi\nNizhny Novgorod\nUfa\nBirmingham\nKrasnodar\nSamara\nRostov-on-Don\nYerevan\nVoronezh\nPerm\nCologne\nVolgograd\nOdesa\n" \
"Enable: LR" \
BE

test_case 15 "huge stdin" \
"El Tarter\nSant Juli de Lria\nPas de la Casa\nOrdino\nles Escaldes\nla Massana\nEncamp\nCanillo\nArinsal\nAnys\nAndorra la Vella\nAixirivall\nWarsn\nUmm Suqaym\nUmm Al Quwain City\narf Kalb\nAr Rshidyah\nRas Al Khaimah City\nMuzayri\nMurba\nMaf\nZayed City\nKhawr Fakkn\nKalb\nJumayr\nAl Jazrah al amr\nDubai\nDibba Al-Fujairah\nDibba Al-Hisn\nDayrah\nSharjah\nAsh Sham\nAr Ruways\nAl Manmah\nAl amryah\natt\nAl Fujairah City\nAl Ain City\nAjman City\nAdh Dhayd\nAbu Dhabi\nAb Hayl\nAs Sawah\nNadd al umr\nAl Lusayl\nSuwayn\nAl amdyah\nAl Waheda\nAl Twar First\nAL Twar Second\nAl Qusais Second\nAl Karama\nAl Hudaiba\nKnowledge Village\nThe Palm Jumeirah\nZaabeel\nOud Metha\nBur Dubai\nKhalifah A City\nShakhbout City\nMirdif\nHawr al Anz\nMankhl\nBr Sad\nNyf\nAl Murar al Qadm\nAr Riqqah\nAl Warqaa\nInternational City\nDubai Marina\nDubai Sports City\nDubai Internet City\nAl Sufouh\nAl Safa\nAr Rumaylah\nMushayrif\nAl Jurf\nAl Majaz\nAs Sawah Sharq\nDubai Festival City\nDubai International Financial Centre\nDowntown Dubai\nDubai Investments Park\nJebel Ali\nBani Yas City\nMusaffah\nAl Shamkhah City\nReef Al Fujairah City\nAl Wiqan\nAl Faqaa\nShabiyyat Al Hiyar\nShabiyyat Milhah\nUmm Al Sheif\nAl Badaa\nAl Muteena\nAl Mizhar First\nAl Mizhar Second\nDubai Silicon Oasis\nDubai Motor City\nDAMAC Hills\nAl Furjan\nBusiness Bay\nAl Qusais 1\nAl Twar 3\nAl Khabaisi\nAl Khawaneej 1\nHalwan\nAl Sajaah\nLahbab\nAl Madam\nAl Raafah\nMohammed Bin Zayed City\nMasdar City\nZr K\nWulswl Bihsd\nKuhsn\nLsh\nTukzr\nBati\nMray\nq Kupruk\nZurmat\nZaybk\nZrat-e Shh Maqd\nZindah Jn\nZarghn Shahr\nZaah Sharan\nZaranj\nZamt Klay\nYang Qalah\nBzr-e Yakwlang\nYay Khl\nWshr\nTrmay\nTlak\nTtn\nTr Pul\nTaywarah\nBzr-e Tashkn\nTarinkot\nTaloqan\nTagw-By\nTagb\nMarkaz-e ukmat-e Suln-e Bakwh\nSpn Bldak\nSprah\nSzmah Qalah\nSiyhgird\nShwah\nShnan\nShaykh Amr Klay\nQshql\nShibirghn\nShwk\nShahr-e af\nShahrn\nShahrak\nAlqahdr Shh Jy\nWulswl Sayyid Karam\nMarkaz-e Sayyidbd\nayd\nSidqbd\nSyagaz\nSar-e Tayghn\nSarb\nSar K\nSarfirz Kal\nSar-e Pul\nSar Chakn\nSangn\nSang-e Mshah\nSang-e Chrak\nSang Atesh\nSangar Sary\nAbak\nR-ye Sang\nRdbr\nRustq\nRab-e Sang-ye Pn\nRmak\nQurghn\nQuchangh\nQar\nQarqn\nQarghah\nQarch Gak\nQarwul\nQarah Bgh\nQarah Bgh\nQala i Naw\nQalah-ye Kf\nQalah-ye Kuhnah\nQalah-ye Shahr\nQalt\nQalah-ye Shh\nQdis\nPrn\nPul-e Khumr\nPul-e Alam\nPasnay" \
"Enable: ABCDEHIJKLMNOPQRSTUWYZ" \
""
