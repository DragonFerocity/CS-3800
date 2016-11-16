state=0
while [ $state != 4 ];
do
  #clear;
  ps -acefl > 'psoutput.txt'  #
  echo "What would you like to do?"
  echo "\t1. Print this scripts process ancestry tree"
  echo "\t2. Find which users are online"
  echo "\t3. Figure out someone's processes"
  echo "\t4. Exit"
  read state
  if [ "$state" -eq "1" ]
  then
    PID=$(grep --max-count=1 "$USER.*\-bash" 'psoutput.txt')
    echo $PID | awk '{print $5}' > 'ppid.txt'
    PPID=$(cat ppid.txt)
    echo $PID | awk '{print $4}' > 'pid.txt'
    PID=$(cat pid.txt)
    echo $PID
    while [ $PPID != "0" ];
    do
      echo "  |"
      grep "$PPID" 'psoutput.txt' > 'grep.txt'  #
      while read line;
      do
        echo $line | awk '{print $4}' > 'pid.txt'
        PID=$(cat pid.txt)
        if [ $PID -eq $PPID ]
        then
          echo $line | awk '{print $5}' > 'ppid.txt'
          PPID=$(cat ppid.txt)
          break
        fi
      done < 'grep.txt'
      echo $PID
    done
    echo ""
    rm pid.txt
    rm ppid.txt
    rm grep.txt
  elif [ $state -eq "2" ]
  then
    echo "These users are currently online:"
    who | cut -f1 -d " " | sort | uniq > 'who.txt'  #
    i=1
    while read line;
    do
      printf "\t$i. "
      echo $line
      i=$(expr $i + 1)
    done < 'who.txt'
    echo ""
    rm who.txt
  elif [ $state -eq "3" ]
  then
    echo "These users are currently online:"
    who > 'who.txt'  #
    i=1
    while read line;
    do
      printf "\t$i. "
      echo $line | awk {'print $1'}
      i=$(expr $i + 1)
    done < 'who.txt'
    echo "What user would you like to see the processes of? "
    read userProc
    i=1
    echo "a" > 'temp.txt'
    while read line;
    do
      if [ "$i" -eq "$userProc" ]
      then
        echo $line | cut -f1 -d " " > 'temp.txt'
        userProc=$(cat temp.txt)
        break
      fi
      i=$(expr $i + 1)
    done < 'who.txt'
    grep "$userProc" 'psoutput.txt' | awk '{print $3 "\t" $4 "\t" $5 "\t" $10 "\t" $11 "\t" $12 "\t" $14 " " $15}'
    echo ""
    rm who.txt
    rm temp.txt
  else
    rm psoutput.txt
    echo "Goodbye!"
  fi
done
