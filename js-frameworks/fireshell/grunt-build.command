cd "$(dirname "$0")"
if [ ! -d node_modules ];then
    sudo npm install
fi
grunt build
