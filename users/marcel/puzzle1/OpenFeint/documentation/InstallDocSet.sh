#!/bin/bash 

mkdir -p ~/Library/Developer/Shared/Documentation/DocSets/

rm -rf ~/Library/Developer/Shared/Documentation/DocSets/com.aurorafeint.OpenFeint.docset/
unzip -qq OpenFeint_DocSet.zip -d ~/Library/Developer/Shared/Documentation/DocSets/
