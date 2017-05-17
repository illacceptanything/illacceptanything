COUNTER=0
while [  $COUNTER -lt 5 ]; do
  let COUNTER=COUNTER+1 
  say "main start"
  clear
  echo "task"
  sleep 1500
  clear
  if [ $COUNTER -eq "4" ]; then
    echo "long"
    say "long"
  fi
  say "break start"
  echo "break"
  sleep 300
  say "break done"
  if [ $COUNTER -eq "4" ]; then
    let COUNTER=0
    sleep 900
  fi

done
