#!/bin/bash

#based on : https://lucidbeaming.com/blog/setting-up-a-raspberry-pi-3-to-run-zynaddsubfx-in-a-headless-configuration/
#thanks!

export DBUS_SESSION_BUS_ADDRESS=unix:path=/run/dbus/system_bus_socket

if pgrep zynaddsubfx
 then
 echo Zynaddsubfx is already singing
 exit 0
 else
 env ALSA_DEVICE=hw:1 zynaddsubfx -U -A=0 -o 512 -r 44100 -b 512 -I alsa -O alsa -P 7777 &
 sleep 4

   if pgrep zynaddsubfx
   then
   echo Zyn is singing
   else
   echo Zyn blorked. Epic Fail.
   fi

fi

mini=$(aconnect -o | grep "MINILAB")
 mpk=$(aconnect -o | grep "MPKmini2")
 mio=$(aconnect -o | grep "mio")

if [[ $mini ]]
 then
 aconnect 'Arturia MINILAB':0 'ZynAddSubFX':0
 echo Connected to MINIlab
 elif [[ $mpk ]]
 then
 aconnect 'MPKmini2':0 'ZynAddSubFX':0
 echo Connected to MPKmini
 elif [[ $mio ]]
 then
 aconnect 'mio':0 'ZynAddSubFX':0
 echo Connected to Mio
 else
 echo No known midi devices available. Try aconnect -l
 fi

exit 0
