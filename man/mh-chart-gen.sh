#!/bin/sh
#
# Generates mh-chart.man from other .man files that have a SYNOPSIS
# section.
#
# This code is Copyright (c) 2012, by the authors of nmh.
# See the COPYRIGHT file in the root directory of the nmh
# distribution for complete copyright information.

nmhmandir=$(dirname $0)

# The following ensures the generated date field in the manpage is divorced
# from the local build environment when building distribution packages.
LC_TIME=C; export LC_TIME
unset LANG
datestamp=$(date +%Y-%m-%d)

our_name="mh-chart"
our_desc="chart of all nmh commands and their options"

cat <<__HOOPY_FROOD
.TH MH-CHART %manext7% "${datestamp}" "%nmhversion%"
.
.\" %nmhwarning%
.
.SH NAME
$our_name \- $our_desc
.SH SYNOPSIS
.na
__HOOPY_FROOD

for i in $nmhmandir/*.man; do
  case $i in
    */mh-chart.man) ;;
    *) if grep '^\.SH SYNOPSIS' "$i" >/dev/null; then
         #### Extract lines from just after .SH SYNOPSIS to just before .ad.
         #### Filter out the "typical usage:" section in pick.man.
         awk '/.SH SYNOPSIS/,/^(\.ad|typical usage:)/ {
                if ($0 !~ /^(\.SH SYNOPSIS|\.na|\.ad|typical usage:)/) print
              }' "$i"
         echo
       elif sed 1p "$i" | grep '^\.so man'  >/dev/null
       then  
	 : skip one-line pages that just include others
       else
	 # pages without SYNOPSIS are section 5 and 7
	 see_also="$see_also $i"
       fi ;;
  esac
done

echo '.SH "SEE ALSO"'
for i in $see_also
do
    # extract the section number from the first (.TH) line
    section=$(sed -n '1s/.*manext\([1-7]\).*/\1/p' "$i")

    # get the name/description.  (search for NAME, print the next line)
    name_desc=$(sed -n '/.SH NAME/{n;p;}' "$i")

    # isolate the name(s) (note:  some pages (mh_tailor/mts.conf) have
    # two names, so everything up to the \- is name(s)
    name=$(printf "%s\n" "$name_desc" | sed -n 's/\(.*\) \\-.*/\1/p')
    # escape spaces and hyphens, since this will come after .IR
    name=$(printf "%s\n" "$name" | sed 's/[- ]/\\&/g')

    # everything after the \- is description
    desc=$(printf "%s\n" "$name_desc" | sed -n 's/.*\\- \(.*\)/\1/p')

    # sort first by section, then by name,
    # then remove the leading sort key and break into two lines
    printf "%s\n" "$section.IR $name ($section)XYZZY$desc"
done | sort | sed -e 's/^[1-7]//' -e 's/XYZZY\(.*\)/\
.RS\
\1\
.RE/'

cat <<EOF
.IR $our_name (7)
.RS
$our_desc (this page)
.RE
EOF

exit
