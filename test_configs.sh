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

echo -e "${BLU}\tINVALID TESTER${RESET}"
x=0

for config in $configs; do

    err_msg=$("$exe" "$config" 2>&1)

    if [[ "$err_msg" == *"Webserver has started"* ]]; then
        err_msg="DID NOT FAIL"
        echo -e "${RED}${x} : ${config} :\n${err_msg}${RESET}"
    else
        echo -e "${GRN}${x} : ${config} :\n${err_msg}${RESET}"
    fi
    x=$((x + 1))
done


path=./configs
exe=./webserv
configs=$(find "$path" -type d -name "invalid" -prune -o -type f ! -name "mime.types" -print)

echo -e "${BLU}\n\t VALID TESTER${RESET}"
x=0

for config in $configs; do
    x=$((x + 1))

    err_msg=$("$exe" "$config" 2>&1)

    if [[ "$err_msg" != *"Webserver has started"* ]]; then
        err_msg="DID NOT FAIL"
        echo -e "${RED}${x} : ${config} :\n${err_msg}${RESET}"
    else
        echo -e "${GRN}${x} : ${config} :\n${err_msg}${RESET}"
    fi

done