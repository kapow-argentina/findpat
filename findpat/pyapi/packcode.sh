#!/bin/sh

DIR="kapow-pyapi"
REV=`../evalrev`
FILE="kapow-pyapi-r$REV.tar.gz"

EXTRAS="Makefile"

if [ -e $DIR ]; then
	rm -rf $DIR
fi

# Arma links
[ '!' -e tools/ktools.py ] && ln -s ../ktools.py tools/ktools.py
[ '!' -e tools/kapow.so ] && ln -s ../kapow.so tools/kapow.so

# Copy files
mkdir $DIR
echo "Copying files"
for f in `python setup.py list` $EXTRAS tools/*py tools/*so; do
	DN=`dirname "$DIR/$f"`
	[ '!' -d "$DN" ] && mkdir -p "$DN"
	cp -r --preserve=all $f $DIR/$f
done

# Pack into .tar.gz
echo "Compresing to $FILE..."
tar -zcf $FILE $DIR --exclude=.svn
