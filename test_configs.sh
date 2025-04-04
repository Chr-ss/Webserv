RED="\033[31m"
GRN="\033[1;32m"
YEL="\033[33m"
BLU="\033[34m"
MAG="\033[35m"
BMAG="\033[1;35m"
CYN="\033[36m"
BCYN="\033[1;36m"
WHT="\033[37m"
RESET="\033[0m"
LINEP="\033[75G"


path=./configs/invalid
exe=./webserv
configs=$(find $path -name "*" | sort)

echo "${BLU}\n\n\tINVALID TESTER${RESET}"
x=0
for config in $configs
do
x=$((x+1))
err_msg=$(timeout --preserve-status 0.1s $exe $config 2>&1 | tr -d '\n')
if [ -z "$err_msg" ];
then
err_msg="DID NOT FAIL"
echo "${RED}${x} : ${config} : \n${err_msg}${RESET}"
else
echo "${GRN}${x} : ${config} : \n${err_msg}${RESET}"
fi

done



path=./configs
exe=./webserv
configs=$(find "$path" -type d -name "invalid" -prune -o -type f ! -name "mime.types" -print)

echo "${BLU}\n\t VALID TESTER${RESET}"
x=0
for config in $configs
do
x=$((x+1))
err_msg=$(timeout --preserve-status 0.1s $exe $config 2>&1 | tr -d '\n')
if [ -z "$err_msg" ];
then
err_msg="DID NOT FAIL"
echo "${GRN}${x} : ${config} : \n${err_msg}${RESET}"
else
echo "${RED}${x} : ${config} : \n${err_msg}${RESET}"
fi

done