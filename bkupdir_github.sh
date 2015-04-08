
#!/bin/bash

#---- Jared Henry Oviatt ----#

# uses smtp mail server
# also at https://github.com/underscorejho/backup-to-email 
# using this to practice pull requests

echo 'Preparing to zip contents of this directory...'
echo $PWD

zip -r $PWD/$(date +%Y%m%d).zip $PWD/* -x \*/.git/\* dead.letter 
#zips contents of current directory to (date).zip
# exclude .git directories
# exclude dead.letter files (not sent emails)

echo 'File zipped'
echo 'Preparing to mail to EMAIL_TO_SEND_TO...'

echo '' | mail -t EMAIL_TO_SEND_TO -s "$PWD - $(date +%Y%m%d).zip" -A $PWD/$(date +%Y%m%d).zip

rm $PWD/$(date +%Y%m%d).zip

echo 'done'
