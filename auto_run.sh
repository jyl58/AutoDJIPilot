#! /bin/sh

screen -dmS "AutoDjiPilot"
screen -x -S "AutoDjiPilot" -p 0 -X stuff "AutoPilotDJI /root/AutoDJIPilot/config/config.lua"
screen -x -S "AutoDjiPilot" -p 0 -X stuff "\n"

exit 0


