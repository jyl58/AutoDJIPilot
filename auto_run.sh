#! /bin/sh

screen -dmS "AutoDjiPilotScreen"
screen -x -S "AutoDjiPilotScreen" -p 0 -X stuff "AutoPilotDJI /root/AutoDJIPilot/config/config.lua"
screen -x -S "AutoDjiPilotScreen" -p 0 -X stuff "\n"

exit 0


