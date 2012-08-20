#!/bin/sh

	if ! cat /usr/bin/enigma2.sh | grep epg.dat >/dev/null; then
		{
			cp /usr/bin/enigma2.sh /usr/bin/enigma2.sh.xmltvbak
			sed '3i[ -f /media/hdd/epg_new.dat ] && cp /media/hdd/epg_new.dat /media/hdd/epg.dat' /usr/bin/enigma2.sh.xmltvbak > /usr/bin/enigma2.sh
		}
	fi

		echo "=============================================="
		echo " "
    		echo "Downloading EPG file, please wait..."
		echo " "
    		sleep 5
    	if [ -f /etc/epgdat ] ; then
      		epgfile=`more /etc/epgdat`
    	else
      		epgfile="epg_new.dat.gz" 
    	fi
		wget -q http://linux-sat.tv/epg/$epgfile -O /hdd/epg_new.dat.gz 
	if [ $? = 1 ]; then
		echo " "
    		echo "Sorry, the EPG file is not available!"
		echo " "
    		echo "Please try later!"
		echo " "
		echo "=============================================="
		echo " "
		exit 1
	fi
		gzip -df /hdd/epg_new.dat.gz
		cp -f /hdd/epg_new.dat /hdd/epg.dat
		rm -f epg_new.dat.gz
		sleep 5
		echo "EPG file was loaded successfully!!!"
		echo " "
		echo "Enigma2 needs a restart to load the EPG data!"
		echo " "
		echo "=============================================="
		echo " "
		echo "Enjoy -:) "
		sleep 5
		killall -9 enigma2 >/dev/null 2>&1
