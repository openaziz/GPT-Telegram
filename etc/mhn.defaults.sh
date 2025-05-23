#! /bin/sh
#
# mhn.defaults.sh -- create extra profile file for MIME handling
#
# USAGE: mhn.defaults.sh [ search-path [ search-prog ]]

# If a search path is passed to the script, we
# use that, else we use a default search path.
if [ -n "$1" ]; then
    SEARCHPATH=$1
else
    SEARCHPATH="$PATH"
fi

# If a search program is passed to the script, we
# use that, else we use a default search program.
if [ -n "$2" ]; then
    SEARCHPROG=$2
else
    SEARCHPROG="mhn.find.sh"
fi

# put output into a temporary file, so we
# can sort it before output.
TMP=/tmp/nmh_temp.$$
trap "rm -f $TMP" 0 1 2 3 13 15

replfmt=" | sed 's/  *$//; s/^\(.\)/> \1/; s/^$/>/;'"
PGM=`$SEARCHPROG "$SEARCHPATH" par`
if [ -n "$PGM" ]; then
    if par version | grep '1\.52-i18n' >/dev/null; then
        #### Don't use patched par.
        echo "mhn.defaults.sh: $PGM uses a patch that is known to improperly handle " 1>&2
        echo 'some multibyte characters.  If fmt is installed, mhn.defaults will ' 1>&2
        echo 'use it.' 1>&2
        unset PGM
    else
        textfmt=" | $PGM 64"
    fi
fi
if [ -z "$PGM" ]; then
    PGM=`$SEARCHPROG "$SEARCHPATH" fmt`
    if [ -n "$PGM" ]; then
        textfmt=" | $PGM"
    else
        textfmt=
    fi
fi
iconv=`$SEARCHPROG "$SEARCHPATH" iconv`
[ -n "$iconv" ]  &&
    charsetconv=" | $iconv -f ${charset:-us-ascii} -t utf-8${textfmt}"  ||
    charsetconv=

cat >>"$TMP" <<'EOF'
mhstore-store-text: %m%P.txt
mhstore-store-text/calendar: %m%P.ics
mhstore-store-text/html: %m%P.html
mhstore-store-text/richtext: %m%P.rt
mhstore-store-video/mpeg: %m%P.mpg
mhstore-store-application/PostScript: %m%P.ps
EOF

if [ -f "/dev/audioIU" ]; then
    PGM=`$SEARCHPROG "$SEARCHPATH" recorder`
    if [ -n "$PGM" ]; then
	echo "mhstore-store-audio/basic: %m%P.au" >> $TMP
        echo "mhbuild-compose-audio/basic: ${AUDIODIR}recorder %f -au -pause > /dev/tty" >> $TMP
        echo "mhshow-show-audio/basic: %l${AUDIODIR}splayer -au" >> $TMP
    fi
elif [ -f "/dev/audio" ]; then
    PGM=`$SEARCHPROG "$SEARCHPATH" raw2audio`
    if [ -n "$PGM" ]; then
	AUDIODIR="`echo $PGM | awk -F/ '{ for(i=2;i<NF;i++)printf "/%s", $i;}'`"/
	echo "mhstore-store-audio/basic: | ${AUDIODIR}raw2audio -e ulaw -s 8000 -c 1 > %m%P.au" >> $TMP
        echo "mhstore-store-audio/x-next: %m%P.au" >> $TMP
	AUDIOTOOL=`$SEARCHPROG "$SEARCHPATH" audiotool`
	if [ -n "$AUDIOTOOL" ]; then
	    echo "mhbuild-compose-audio/basic: $AUDIOTOOL %f && ${AUDIODIR}raw2audio -F < %f" >> $TMP
	else
	    echo "mhbuild-compose-audio/basic: trap \"exit 0\" 2 && ${AUDIODIR}record | ${AUDIODIR}raw2audio -F" >> $TMP
	fi
	echo "mhshow-show-audio/basic: %l${AUDIODIR}raw2audio 2>/dev/null | ${AUDIODIR}play" >> $TMP

	PGM=`$SEARCHPROG "$SEARCHPATH" adpcm_enc`
	if [ -n "$PGM" ]; then
	    DIR="`echo $PGM | awk -F/ '{ for(i=2;i<NF;i++)printf "/%s", $i;}'`"/
	    if [ -n "$AUDIOTOOL" ]; then
		echo "mhbuild-compose-audio/x-next: $AUDIOTOOL %f && ${DIR}adpcm_enc < %f" >> $TMP
	    else
		echo "mhbuild-compose-audio/x-next: ${AUDIODIR}record | ${DIR}adpcm_enc" >> $TMP
	    fi
	    echo "mhshow-show-audio/x-next: %l${DIR}adpcm_dec | ${AUDIODIR}play" >> $TMP
	else
	    if [ -n "$AUDIOTOOL" ]; then
		echo "mhbuild-compose-audio/x-next: $AUDIOTOOL %f" >> $TMP
	    else
		echo "mhbuild-compose-audio/x-next: ${AUDIODIR}record" >> $TMP
	    fi
	    echo "mhshow-show-audio/x-next: %l${AUDIODIR}play" >> $TMP
	fi
    else
	echo "mhbuild-compose-audio/basic: cat < /dev/audio" >> $TMP
        echo "mhshow-show-audio/basic: %lcat > /dev/audio" >> $TMP
    fi
fi

####
#### mhbuild-disposition-<type>[/<subtype>] entries are used by the
#### WhatNow attach for deciding whether the Content-Disposition
#### should be 'attachment' or 'inline'.  Only those values are
#### supported.  mhbuild-convert-text/html is defined below.
####
cat <<EOF >>${TMP}
mhbuild-convert-text/calendar: mhical -infile %F -contenttype
mhbuild-convert-text: charset=%{charset}; $iconv -f \${charset:-us-ascii} -t utf-8 %F${replfmt}
mhbuild-disposition-text/calendar: inline
mhbuild-disposition-message/rfc822: inline
EOF

PGM=`$SEARCHPROG "$SEARCHPATH" okular`
if [ -n "$PGM" ]; then
    echo "mhshow-show-application/PostScript: %l$PGM %F" >> $TMP
else
    PGM=`$SEARCHPROG "$SEARCHPATH" evince`
    if [ -n "$PGM" ]; then
	echo "mhshow-show-application/PostScript: %l$PGM %F" >> $TMP
    else
	PGM=`$SEARCHPROG "$SEARCHPATH" gv`
	if [ -n "$PGM" ]; then
	    echo "mhshow-show-application/PostScript: %l$PGM %F" >> $TMP
	fi
    fi
fi

PGM=`$SEARCHPROG "$SEARCHPATH" okular`
if [ -n "$PGM" ]; then
    echo "mhshow-show-application/pdf: %l$PGM %F" >> $TMP
else
    PGM=`$SEARCHPROG "$SEARCHPATH" evince`
    if [ -n "$PGM" ]; then
	echo "mhshow-show-application/pdf: %l$PGM %F" >> $TMP
    else
	PGM=`$SEARCHPROG "$SEARCHPATH" xpdf`
	if [ -n "$PGM" ]; then
	    echo "mhshow-show-application/pdf: %l$PGM %F" >> $TMP
	else
	    PGM=`$SEARCHPROG "$SEARCHPATH" gv`
	    if [ -n "$PGM" ]; then
		echo "mhshow-show-application/pdf: %l$PGM %F" >> $TMP
	    else
		PGM=`$SEARCHPROG "$SEARCHPATH" acroread`
		if [ -n "$PGM" ]; then
		    echo "mhshow-show-application/pdf: %l$PGM %F" >> $TMP
		fi
	    fi
	fi
    fi
fi

echo "mhshow-show-text/calendar: mhical -infile %F" >> $TMP
echo "mhshow-show-application/ics: mhical -infile %F" >> $TMP
echo "mhfixmsg-format-text/calendar: mhical -infile %F" >> $TMP
echo "mhfixmsg-format-application/ics: mhical -infile %F" >> $TMP

# The application/vnd.openxmlformats-officedocument.wordprocessingml.document
# through application/onenote associations are from
# http://technet.microsoft.com/en-us/library/cc179224.aspx

cat <<EOF >> ${TMP}
mhshow-suffix-application/msword: .doc
mhshow-suffix-application/ogg: .ogg
mhshow-suffix-application/pdf: .pdf
mhshow-suffix-application/postscript: .ps
mhshow-suffix-application/rtf: .rtf
mhshow-suffix-application/vnd.ms-excel: .xla
mhshow-suffix-application/vnd.ms-excel: .xlc
mhshow-suffix-application/vnd.ms-excel: .xld
mhshow-suffix-application/vnd.ms-excel: .xll
mhshow-suffix-application/vnd.ms-excel: .xlm
mhshow-suffix-application/vnd.ms-excel: .xls
mhshow-suffix-application/vnd.ms-excel: .xlt
mhshow-suffix-application/vnd.ms-excel: .xlw
mhshow-suffix-application/vnd.ms-powerpoint: .pot
mhshow-suffix-application/vnd.ms-powerpoint: .pps
mhshow-suffix-application/vnd.ms-powerpoint: .ppt
mhshow-suffix-application/vnd.ms-powerpoint: .ppz
mhshow-suffix-application/vnd.openxmlformats-officedocument.wordprocessingml.document: .docx
mhshow-suffix-application/vnd.ms-word.document.macroEnabled.12: .docm
mhshow-suffix-application/vnd.openxmlformats-officedocument.wordprocessingml.template: .dotx
mhshow-suffix-application/vnd.ms-word.template.macroEnabled.12: .dotm
mhshow-suffix-application/vnd.openxmlformats-officedocument.spreadsheetml.sheet: .xlsx
mhshow-suffix-application/vnd.ms-excel.sheet.macroEnabled.12: .xlsm
mhshow-suffix-application/vnd.openxmlformats-officedocument.spreadsheetml.template: .xltx
mhshow-suffix-application/vnd.ms-excel.template.macroEnabled.12: .xltm
mhshow-suffix-application/vnd.ms-excel.sheet.binary.macroEnabled.12: .xlsb
mhshow-suffix-application/vnd.ms-excel.addin.macroEnabled.12: .xlam
mhshow-suffix-application/vnd.openxmlformats-officedocument.presentationml.presentation: .pptx
mhshow-suffix-application/vnd.ms-powerpoint.presentation.macroEnabled.12: .pptm
mhshow-suffix-application/vnd.openxmlformats-officedocument.presentationml.slideshow: .ppsx
mhshow-suffix-application/vnd.ms-powerpoint.slideshow.macroEnabled.12: .ppsm
mhshow-suffix-application/vnd.openxmlformats-officedocument.presentationml.template: .potx
mhshow-suffix-application/vnd.ms-powerpoint.template.macroEnabled.12: .potm
mhshow-suffix-application/vnd.ms-powerpoint.addin.macroEnabled.12: .ppam
mhshow-suffix-application/vnd.openxmlformats-officedocument.presentationml.slide: .sldx
mhshow-suffix-application/vnd.ms-powerpoint.slide.macroEnabled.12: .sldm
mhshow-suffix-application/onenote: .onetoc
mhshow-suffix-application/onenote: .onetoc2
mhshow-suffix-application/onenote: .onetmp
mhshow-suffix-application/onenote: .onepkg
mhshow-suffix-application/x-bzip2: .bz2
mhshow-suffix-application/x-cpio: .cpio
mhshow-suffix-application/x-dvi: .dvi
mhshow-suffix-application/x-gzip: .gz
mhshow-suffix-application/x-java-archive: .jar
mhshow-suffix-application/x-javascript: .js
mhshow-suffix-application/x-latex: .latex
mhshow-suffix-application/x-sh: .sh
mhshow-suffix-application/x-tar: .tar
mhshow-suffix-application/x-texinfo: .texinfo
mhshow-suffix-application/x-tex: .tex
mhshow-suffix-application/x-troff-man: .man
mhshow-suffix-application/x-troff-me: .me
mhshow-suffix-application/x-troff-ms: .ms
mhshow-suffix-application/x-troff: .t
mhshow-suffix-application/zip: .zip
mhshow-suffix-audio/basic: .au
mhshow-suffix-audio/midi: .midi
mhshow-suffix-audio/mpeg: .mp3
mhshow-suffix-audio/mpeg: .mpg
mhshow-suffix-audio/x-ms-wma: .wma
mhshow-suffix-audio/x-wav: .wav
mhshow-suffix-image/gif: .gif
mhshow-suffix-image/jpeg: .jpeg
mhshow-suffix-image/jpeg: .jpg
mhshow-suffix-image/png: .png
mhshow-suffix-image/tiff: .tif
mhshow-suffix-image/tiff: .tiff
mhshow-suffix-text: .txt
mhshow-suffix-text/calendar: .ics
mhshow-suffix-text/css: .css
mhshow-suffix-text/html: .html
mhshow-suffix-text/rtf: .rtf
mhshow-suffix-text/sgml: .sgml
mhshow-suffix-text/xml: .xml
mhshow-suffix-video/mpeg: .mpeg
mhshow-suffix-video/mpeg: .mpg
mhshow-suffix-video/mp4: .mp4
mhshow-suffix-video/quicktime: .moov
mhshow-suffix-video/quicktime: .mov
mhshow-suffix-video/quicktime: .qt
mhshow-suffix-video/quicktime: .qtvr
mhshow-suffix-video/x-msvideo: .avi
mhshow-suffix-video/x-ms-wmv: .wmv
EOF

PGM=`$SEARCHPROG "$SEARCHPATH" w3m`
if [ -n "$PGM" ]; then
    echo 'mhshow-show-text/html: charset="%{charset}"; '"\
%l$PGM"' -dump ${charset:+-I} ${charset:+"$charset"} -T text/html %F' >> $TMP
    echo 'mhfixmsg-format-text/html: charset="%{charset}"; '"\
$PGM "'-dump ${charset:+-I} ${charset:+"$charset"} -O utf-8 -T text/html %F' \
         >> $TMP
    echo 'mhbuild-convert-text/html: charset="%{charset}"; '"\
$PGM "'-cols 9999 -dump ${charset:+-I} ${charset:+"$charset"} -O utf-8 -T text/html %F'"\
${replfmt}" >> $TMP
else
    PGM=`$SEARCHPROG "$SEARCHPATH" lynx`
    if [ -n "$PGM" ]; then
	echo 'mhshow-show-text/html: charset="%{charset}"; '"\
%l$PGM"' -child -dump -force-html ${charset:+-assume_charset} ${charset:+"$charset"} %F' >> $TMP
        #### lynx indents with 3 spaces, remove them and any trailing spaces.
        #### Remove last line if it is blank.
        echo 'mhfixmsg-format-text/html: charset="%{charset}"; '"\
$PGM "'-child -dump -force_html ${charset:+-assume_charset} ${charset:+"$charset"} '"\
"'-display_charset utf-8 %F | '"expand | sed -e 's/^   //' -e 's/  *$//' | \
sed '$ {/^$/d;}'" >> $TMP
        echo 'mhbuild-convert-text/html: charset="%{charset}"; '"\
$PGM "'-child -dump -nolist -width=9999 -force_html ${charset:+-assume_charset} ${charset:+"$charset"} '"\
%F | sed -e 's/^   //' -e 's/  *$//' -e 's/^\(.\)/> \1/' -e 's/^$/>/'" >> $TMP
    else
        PGM=`$SEARCHPROG "$SEARCHPATH" elinks`
        if [ -n "$PGM" ]; then
            #### M. Levinson noted that -eval "set document.codepage.assume='$charset'"
            #### can be used with elinks, after setting charset as done above.  However,
            #### quoting becomes a nightmare because the argument must be quoted.  And
            #### with that, bad effects of malicious parameter value quoting, such as shown
            #### in test/mhshow/test-textcharset, seems very difficult to avoid.  It might
            #### be easier if the parameters are put into a temporary config-file.
            echo "mhshow-show-text/html: %l$PGM -dump -force-html \
-eval 'set document.browse.margin_width=0' %F" >> $TMP
            echo "mhfixmsg-format-text/html: $PGM -dump -force-html \
-no-numbering -no-references -eval 'set document.browse.margin_width=0' %F" >> $TMP
            echo "mhbuild-convert-text/html: $PGM -dump -force-html \
-no-numbering -no-references -eval 'set document.browse.margin_width=0' %F${replfmt}" >> $TMP
        else
            echo 'mhbuild-convert-text/html: cat %F' >> $TMP
        fi
    fi
fi

PGM=`$SEARCHPROG "$SEARCHPATH" mpv`
if [ -n "$PGM" ]; then
    echo "mhshow-show-image: %l$PGM --keep-open --really-quiet %F" >> $TMP
    echo "mhshow-show-video: %l$PGM --really-quiet %F" >> $TMP
else
    PGM=`$SEARCHPROG "$SEARCHPATH" xv`
    if [ -n "$PGM" ]; then
	echo "mhshow-show-image: %l$PGM -geometry =-0+0 %F" >> $TMP
    fi
    PGM=`$SEARCHPROG "$SEARCHPATH" mplayer`
    if [ -n "$PGM" ]; then
	echo "mhshow-show-video: %l$PGM %F" >> $TMP
    fi
fi

# libreffice, or possibly staroffice, to read .doc files
PGM=`$SEARCHPROG "$SEARCHPATH" soffice`
if [ -n "$PGM" ]; then
	echo "mhshow-show-application/msword: %l$PGM %F" >> $TMP
fi

# This entry is used to retrieve external-body types that use a "url"
# access-type.
case "`uname`" in
  FreeBSD)
	echo "nmh-access-url: fetch -o -" >> $TMP
	;;
  *)
	PGM=`$SEARCHPROG "$SEARCHPATH" curl`
	if [ -n "$PGM" ]; then
		echo "nmh-access-url: $PGM -L" >> $TMP
	fi
	;;
esac

# Output a sorted version of the file, along with some comments in
# appropriate places.
echo '#: This file was generated by mhn.defaults.sh.'
sort < $TMP | \
    sed -e 's|^\(mhshow-show-application/ics:.*\)|#: might need -notextonly -noinlineonly or -part/-type to show application/ics parts\n\1|'
