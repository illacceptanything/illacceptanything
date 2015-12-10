 letItGo
#!/bin/bash

#Will automatically force their volume to be set to max.  You may want to adjust this so it isn't too loud

amixer -D pulse sset Master unmute&&amixer -D pulse sset Master 100% 

#This will generate a number between 150 and 1,  the value will be stored in the NUMBER variable
NUMBER=$[ ( $RANDOM % 150 )  + 1 ]

#This will open up a video specified to a random time
vlc --start-time $NUMBER https://www.youtube.com/watch?v=L0MK7qz13bU
