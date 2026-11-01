#!/usr/bin/env bash
# Automated end-to-end tests: feeds scripted keystrokes to ./evoting and checks the output.
# Usage:  make test      (or)   bash tests/smoke_test.sh
cd "$(dirname "$0")/.." || exit 1
BIN=./evoting
[ -x "$BIN" ] || { echo "Build first: make"; exit 1; }

PASS=0; FAIL=0
check() {  # check <description> <output> <expected text>
  if grep -qF -- "$3" <<<"$2"; then PASS=$((PASS+1)); echo "  PASS  $1"
  else FAIL=$((FAIL+1)); echo "  FAIL  $1   (expected: $3)"; fi
}
check_absent() {  # check_absent <description> <output> <text that must NOT appear>
  if grep -qF -- "$3" <<<"$2"; then FAIL=$((FAIL+1)); echo "  FAIL  $1   (found: $3)"
  else PASS=$((PASS+1)); echo "  PASS  $1"; fi
}
run() { "$BIN" --data "$1" --no-color --ascii; }   # input comes from stdin

# ---- input builders (each sequence of lines mirrors what a person would type) ----
vote()    { printf '1\n%s\npass123\n3\n%s\ny\n\n6\n\n' "$1" "$2"; }      # login, vote, logout
admin()   { printf '3\nadmin\nadmin123\n'; }                              # admin login
alogout() { printf '19\n\n'; }

D=$(mktemp -d); trap 'rm -rf "$D"' EXIT

echo "== Test 1: first run creates demo data and all data files"
OUT=$(printf '5\n' | run "$D/t1")
for f in voters candidates votes admin election audit; do
  [ -f "$D/t1/$f.txt" ] && { PASS=$((PASS+1)); echo "  PASS  $f.txt created"; } || { FAIL=$((FAIL+1)); echo "  FAIL  $f.txt missing"; }
done

echo "== Test 2: voting is blocked before the election starts; results hidden"
OUT=$( { printf '1\nV1001\npass123\n3\n\n6\n\n'; admin; printf '12\n\n'; alogout; printf '5\n'; } | run "$D/t1")
check "voting before start rejected" "$OUT" "Voting has not started yet"
check "results hidden before close"  "$OUT" "Results are available only after the election is CLOSED"

echo "== Test 3: full election (start, votes, stop, results)"
OUT=$( { admin; printf '2\ny\n\n'; alogout;
         vote V1001 101; vote V1002 102; vote V1003 102; vote V1004 103;
         vote V1005 C101; vote V1006 101; vote V1007 102; vote V1008 102;
         printf '1\nV1001\npass123\n3\n\n6\n\n';                      # V1001 tries to vote twice
         printf '1\nV1009\npass123\n3\n999\n0\n\n6\n\n';                # invalid candidate, then cancel
         printf '1\nV1010\npass123\n3\n104\nn\n0\n\n6\n\n';             # decline the confirmation
         admin; printf '4\n1\n\n3\ny\n\n14\n\n12\n\n13\n\n15\n\n'; alogout; printf '5\n'; } | run "$D/t3")
check "election opened"              "$OUT" "The election is now OPEN"
check "receipt issued"               "$OUT" "Receipt No: RCP-"
check "duplicate vote blocked"       "$OUT" "You have already voted"
check "invalid candidate rejected"   "$OUT" "Invalid candidate ID"
check "declined vote not recorded"   "$OUT" "Vote not submitted"
check "election closed"              "$OUT" "The election is now CLOSED"
check "winner (C102 Lakshmi, 4)"     "$OUT" "WINNER: Lakshmi Devi (Green Future Party) with 4 vote(s)"
check "total votes = 8"              "$OUT" "Total votes cast        : 8"
check "votes not cast = 2"           "$OUT" "Votes not cast          : 2"
check "turnout 80.00%"               "$OUT" "Voter turnout           : 80.00%"
check "percentage column"            "$OUT" "50.00%"
check "bar chart printed"            "$OUT" "VOTES BAR CHART"
check "integrity OK"                 "$OUT" "No problems found"
check "audit log shows vote event"   "$OUT" "VOTE_CAST"
check "polymorphic account list"     "$OUT" "Administrator"
check_absent "votes.txt has no voter IDs" "$(cat "$D/t3/votes.txt")" "V100"
[ "$(grep -vc '^#' "$D/t3/votes.txt")" = "8" ] && { PASS=$((PASS+1)); echo "  PASS  votes.txt has 8 ballots"; } || { FAIL=$((FAIL+1)); echo "  FAIL  votes.txt ballot count"; }

echo "== Test 4: data persists after restart; closed election rejects votes"
OUT=$( { printf '4\n\n1\nV1009\npass123\n3\n\n6\n\n'; admin; printf '12\n\n'; alogout; printf '5\n'; } | run "$D/t3")
check "state CLOSED after restart"   "$OUT" "Status             : CLOSED"
check "closed: voting rejected"      "$OUT" "Voting is closed"
check "results still available"      "$OUT" "WINNER: Lakshmi Devi"

echo "== Test 5: invalid input handling"
OUT=$( { printf 'abc\n9\n\n'; printf '2\n123\nJo\nxyz\n17\n25\nabc\n123456\n654321\n123456\n123456\nAB\nV1001\nNEWID1\n\n'; printf '1\nV1001\nwrong\nV1001\nwrong\nV1001\nwrong\n\n'; printf '5\n'; } | run "$D/t5")
check "bad menu text rejected"       "$OUT" "Invalid input. Please enter a whole number between 1 and 5."
check "bad name rejected"            "$OUT" "2-40 characters; letters, spaces, . - ' only."
check "underage rejected"            "$OUT" "between 18 and 120"
check "short password rejected"      "$OUT" "Password must be 6-32 printable characters."
check "password mismatch detected"   "$OUT" "Passwords do not match"
check "duplicate/invalid ID rejected" "$OUT" "not already in use"
check "registration succeeded"       "$OUT" "Registration complete!"
check "login failure message"        "$OUT" "Invalid Voter ID or password."
check "lock-out after 3 attempts"    "$OUT" "Too many failed attempts"

echo "== Test 6: corrupted and missing files are handled gracefully"
mkdir -p "$D/t6"
printf '# voters\nV1001|Aarav Sharma|25|0000000000000000|0\nthis is garbage\nV1002|X|5|zz|9\n' > "$D/t6/voters.txt"
printf 'C101|Arjun Mehta|Progress Alliance|Sun|45|Better roads and power\nbroken|line\n' > "$D/t6/candidates.txt"
printf 'NONSENSE|||\n' > "$D/t6/election.txt"
OUT=$(printf '\n4\n\n5\n' | run "$D/t6")
check "warnings are shown"           "$OUT" "START-UP WARNINGS"
check "corrupt voter skipped"        "$OUT" "voters.txt: skipped a corrupted record"
check "corrupt candidate skipped"    "$OUT" "candidates.txt: skipped a corrupted record"
check "bad election file reset"      "$OUT" "election.txt: corrupted"
check "default admin recreated"      "$OUT" "default admin account created"
check "backup of damaged file kept"  "$(ls "$D/t6")" "voters.txt.corrupt.bak"
check "program still runs"           "$OUT" "Registered voters  : 1"

echo "== Test 7: admin CRUD, search, reset"
OUT=$( { admin;
   printf '8\nTest Person\nTest Party\nStar\n40\nA short test manifesto.\n\n';   # add candidate
   printf '11\nstar\n\n';                                                       # search
   printf '9\n105\n\n\nNew Party\n\n\n\n\n';                                   # edit (keep most values)
   printf '10\n105\ny\n\n';                                                     # remove
   printf '8\nBad|Name\nOk Name\nP|P\nParty X\nSym\nabc\n17\n30\nManifesto text here\n\n'; # validation loops
   printf '5\nAdded By Admin\n40\nsecret1\nsecret1\nADM001\n\n';               # add voter
   printf '4\n4\nadded\n\n';                                                    # search voter
   printf '6\nADM001\ny\n\n';                                                   # remove voter
   printf '17\nno\n\n17\nRESET\n\n';                                           # reset (cancel, then confirm)
   alogout; printf '5\n'; } | run "$D/t7")
check "candidate added"              "$OUT" "Candidate added with ID C105"
check "search finds candidate"       "$OUT" "1 match(es) found"
check "candidate edited"             "$OUT" "Candidate C105 updated"
check "candidate removed"            "$OUT" "Candidate removed"
check "candidate validation loop"    "$OUT" "Letters, spaces, . - ' only (2-40)."
check "admin added voter ADM001"     "$OUT" "Your Voter ID is: ADM001"
check "voter search works"           "$OUT" "Added By Admin"
check "voter removed"                "$OUT" "Voter removed."
check "reset cancelled safely"       "$OUT" "Cancelled. Nothing was changed."
check "reset performed"              "$OUT" "Simulation reset"

echo "== Test 8: rules locked while election is OPEN"
OUT=$( { admin; printf '2\ny\n\n8\nLate Entry\nParty\nSym\n40\nManifesto here\n\n6\nV1010\ny\n\n2\ny\n\n'; alogout; printf '5\n'; } | run "$D/t8")
check "cannot add candidate when OPEN"   "$OUT" "only allowed while the election is NOT STARTED"
check "cannot remove voter when OPEN"    "$OUT" "Removing voters is only allowed"
check "cannot start twice"               "$OUT" "already OPEN"

echo "== Test 9: tie, no-votes, integrity mismatch, password change, end of input"
OUT=$( { admin; printf '2\ny\n\n'; alogout; vote V1001 101; vote V1002 102; admin; printf '3\ny\n\n12\n\n'; alogout; printf '5\n'; } | run "$D/t9")
check "tie is reported"              "$OUT" "WINNER: TIE between Arjun Mehta, Lakshmi Devi - 1 vote(s) each"
OUT=$( { admin; printf '2\ny\n\n3\ny\n\n12\n\n'; alogout; printf '5\n'; } | run "$D/t9b")
check "no votes -> no winner"        "$OUT" "No votes were cast - there is no winner."
check "zero turnout"                 "$OUT" "Voter turnout           : 0.00%"
printf '5\n' | run "$D/t10" >/dev/null
sed -i 's/^\(V1003|[^|]*|[^|]*|[^|]*|\)0$/\11/' "$D/t10/voters.txt"
OUT=$( { admin; printf '14\n\n'; alogout; printf '5\n'; } | run "$D/t10")
check "integrity detects mismatch"   "$OUT" "Voters marked as voted (1) != ballots stored (0)"
OUT=$( { admin; printf '16\nadmin123\nnewpass1\nnewpass1\n\n'; alogout;
         printf '3\nadmin\nadmin123\n3\nadmin\nnewpass1\n'; alogout; printf '5\n'; } | run "$D/t10")
check "password changed"             "$OUT" "Password changed."
check "old password rejected"        "$OUT" "Invalid Admin ID or password."
check "new password accepted"        "$OUT" "Welcome, System Administrator!"
OUT=$(printf '' | run "$D/t10")
check "end of input handled"         "$OUT" "Input closed. Exiting safely"

echo
echo "RESULT: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
