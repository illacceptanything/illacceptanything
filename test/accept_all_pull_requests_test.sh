URL='https://api.github.com/repos/mrkrstphr/illacceptanything/pulls?state=open'
content=$(curl $URL)

if [ $content = '[]' ]; then
    echo "OK"
else
    echo "Fail"
fi
