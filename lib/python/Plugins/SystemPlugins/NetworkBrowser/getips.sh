ip=$1
myip=$2
isswap=`free | grep "Swap:            0            0            0"`
rm /tmp/lanips >/dev/null 2>&1
while true
do
 z=$(($z+1))
 ncip="$ip.$z"
  if [ -z "$isswap" ]; then
    (ping -w1 -c1 $ncip >/dev/null 2>&1 && echo $ncip >> /tmp/lanips)& 
  else
    (ping -w1 -c1 $ncip >/dev/null 2>&1 && echo $ncip >> /tmp/lanips)
  fi
 if [ $z -gt 253 ]; then
  break
 fi
done

#sleep 3
lanips=`cat /tmp/lanips`

#erzeuge struktur
#rechnertyp:name:ip:dienst
for ip in $lanips
do
 if [ "$ip" != "$myip" ]; then
  dienst=""
  hn=`/usr/lib/enigma2/python/Plugins/SystemPlugins/NetworkBrowser/getname $ip`
  if [ -z "$hn" ]; then hn=UNKNOWN; fi
  echo host:$hn:$ip:$dienst
 fi
done

rm /tmp/lanips >/dev/null 2>&1
