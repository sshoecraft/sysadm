#!/bin/bash
# ---------------------------------------------------------------
# /usr/local/sbin/perlver.sh
# v1.00  2003.10.15  XdG / MIS Center
# ---------------------------------------------------------------

# ----- Global Variables Declaration ----------------------------
PERLMODULE=$1
echo
echo -n "... Perl Module [${PERLMODULE}] Version: "
echo perl -M${PERLMODULE} -e \'print \"\$${PERLMODULE}::VERSION\\n\"\' | sh
echo
