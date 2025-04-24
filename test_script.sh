#!/bin/bash

RED="\e[31m"
GRE="\e[32m"
YEL="\e[33m"
RES="\e[0m"

PORT=${1:-8080}

echo "" > test_results.txt

echo -e "TEST 1: ${YEL} GET localhost:$PORT: ${RES}"
curl -s --http1.1 -G localhost:$PORT -i -o test_results.txt
head -n 1 test_results.txt | grep -q "200" && echo -e "${GRE} Status is 200 OK${RES}" || echo -e "${RED}Status is not ok${RES}"
echo ""

echo "" > test_results.txt
echo -e "TEST 2: ${YEL} GET localhost:$PORT/index.html: ${RES} "
curl -s --http1.1 -G localhost:$PORT/index.html -i -o test_results.txt
head -n 1 test_results.txt | grep -q "200" && echo -e "${GRE} Status is 200 OK${RES}" || echo -e "${RED}Status is not ok${RES}"
echo ""

echo "" > test_results.txt
echo -e "TEST 3: ${YEL} GET localhost:$PORT/http/contact.html: ${RES} "
curl -s --http1.1 -G localhost:$PORT/http/contact.html -i -o test_results.txt
head -n 1 test_results.txt | grep -q "200" && echo -e "${GRE} Status is 200 OK${RES}" || echo -e "${RED}Status is not ok${RES}"
echo ""

echo "" > test_results.txt
echo -e "TEST 4: ${YEL} GET localhost:$PORT/http/upload.html: ${RES} "
curl -s --http1.1 -G localhost:$PORT/http/upload.html -i -o test_results.txt
head -n 1 test_results.txt | grep -q "200" && echo -e "${GRE} Status is 200 OK${RES}" || echo -e "${RED}Status is not ok${RES}"
echo ""

echo "" > test_results.txt
echo -e "TEST 5: ${YEL} CGI upload txt localhost:$PORT/cgi-bin/nph_CGI_upload.py: ${RES} "
curl -s --http1.1 -F "name=test; filename=text.txt" -H "Content-Type: multipart/form-data" localhost:$PORT/cgi-bin/nph_CGI_upload.py -i -o test_results.txt
head -n 1 test_results.txt | grep -q "302" && echo -e "${GRE} Status is 302 found" || echo -e "${RED}Status is not ok${RES}"
echo ""

echo "" > test_results.txt
echo -e "TEST 6: ${YEL} CGI upload png localhost:$PORT/cgi-bin/nph_CGI_upload.py: ${RES} "
curl -s --http1.1 -F "name=test2; filename=small.png" -H "Content-Type: multipart/form-data" localhost:$PORT/cgi-bin/nph_CGI_upload.py -i -o test_results.txt
head -n 1 test_results.txt | grep -q "302" && echo -e "${GRE} Status is 302 found" || echo -e "${RED}Status is not ok${RES}"
echo ""

echo "" > test_results.txt
echo -e "TEST 7: ${YEL} CGI contact localhost:$PORT/cgi-bin/nph_CGI_submit_contact_info.py: ${RES} "
curl -s --http1.1 -X POST -d "name=Testname Doe&email=test@email.com&message=Hello!" localhost:$PORT/cgi-bin/nph_CGI_submit_contact_info.py -i -o test_results.txt
head -n 1 test_results.txt | grep -q "302" && echo -e "${GRE} Status is 302 found" || echo -e "${RED}Status is not ok${RES}"
echo ""

echo "" > test_results.txt
echo -e "TEST 8: ${YEL} GET localhost:$PORT/cgi-bin/nph_CGI_upload_list.py: ${RES} "
curl -s --http1.1 -G localhost:$PORT/cgi-bin/nph_CGI_upload_list.py -i -o test_results.txt
head -n 1 test_results.txt | grep -q "200" && echo -e "${GRE} Status is 200 OK${RES}" || echo -e "${RED}Status is not ok${RES}"
count=$(grep -o '<li>' test_results.txt | wc -l)
echo " -> Amount of files: $count"
echo ""

echo "" > test_results.txt
echo -e "TEST 9: ${YEL} DELETE localhost:$PORT/text.txt: ${RES} "
curl -s --http1.1 -X DELETE localhost:$PORT/text.txt -i -o test_results.txt
head -n 1 test_results.txt | grep -q "302" && echo -e "${GRE} Status is 302 found" || echo -e "${RED}Status is not ok${RES}"
echo ""

echo "" > test_results.txt
echo -e "TEST 10: ${YEL} try DELETE again localhost:$PORT/text.txt: ${RES} "
curl -s --http1.1 -X DELETE localhost:$PORT/text.txt -i -o test_results.txt
head -n 1 test_results.txt | grep -q "404" && echo -e "${GRE} Status is 404 OK (not found)" || echo -e "${RED}Status is not ok${RES}"
echo ""

echo "" > test_results.txt
echo -e "TEST 11: ${YEL} GET localhost:$PORT/cgi-bin/nph_CGI_upload_list.py after deleting: ${RES} "
curl -s --http1.1 -G localhost:$PORT/cgi-bin/nph_CGI_upload_list.py -i -o test_results.txt
head -n 1 test_results.txt | grep -q "200" && echo -e "${GRE} Status is 200 OK${RES}" || echo -e "${RED}Status is not ok${RES}"
new_count=$(grep -o '<li>' test_results.txt | wc -l)
echo " -> Amount of files after delete: $new_count"
if [[ "$new_count" -lt "$count" ]]; then
	echo -e "${GRE} One item is deleted, OK ${RES}"
else
	echo -e "${RED} No item is deleted, NOT ok ${RES}"
fi
echo ""

echo "" > test_results.txt
echo -e "TEST 12: ${YEL} delete with GET localhost:$PORT?text.txt: ${RES} "
curl -s --http1.1 -X GET localhost:$PORT/cgi-bin/nph_CGI_delete.py?file=small.png -i -o test_results.txt
head -n 1 test_results.txt | grep -q "302" && echo -e "${GRE} Status is 302 found" || echo -e "${RED}Status is not ok${RES}"
echo ""

echo "" > test_results.txt
echo -e "TEST 13: ${YEL} POST upload with query localhost:$PORT/cgi-bin/nph_CGI_upload.py?name=testName&filename=text.txt: ${RES} "
curl -s --http1.1 -X POST \
     -F "file=@data/www/images/text.txt" \
     "localhost:$PORT/cgi-bin/nph_CGI_upload.py?name=testName&filename=text.txt" \
     -i -o test_results.txt
head -n 1 test_results.txt | grep -q "302" && echo -e "${GRE} Status is 302 found" || echo -e "${RED}Status is not ok${RES}"
echo ""

echo "" > test_results.txt
echo -e "TEST 14: ${YEL} GET localhost:$PORT/notexistingfile -> status 404: ${RES}"
curl -s --http1.1 -G localhost:$PORT/notexistingfile -i -o test_results.txt
head -n 1 test_results.txt | grep -q "404" && echo -e "${GRE} Status is 404 OK${RES}" || echo -e "${RED}Status is not ok${RES}"
echo ""

# Content type is not accepted: application/octet-stream, only no multipart/form-data
echo "" > test_results.txt
echo -e "TEST 15: ${YEL} POST request with short body to upload: ${RES}"
curl -s -i localhost:$PORT/cgi-bin/nph_CGI_upload.py -H "Content-Type: text/plain" -o test_results.txt --data "body" 
head -n 1 test_results.txt | grep -q "400" && echo -e "${GRE} Status is 400 OK${RES}" || echo -e "${RED}Status is not ok${RES}"
echo ""

echo "" > test_results.txt
echo -e "TEST 16: ${YEL} POST request with long body to upload: ${RES}"
curl -s -i -X POST localhost:$PORT/cgi-bin/nph_CGI_upload.py -H "Content-Type: text/plain" -o test_results.txt --data @- << EOF
ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss
ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss
ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss
ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss
EOF
head -n 1 test_results.txt | grep -q "400" && echo -e "${GRE} Status is 400 OK${RES}" || echo -e "${RED}Status is not ok${RES}"
echo ""

echo "" > test_results.txt
echo -e "TEST 17: ${YEL} POST request with short body to root: ${RES}"
curl -s -i localhost:$PORT -H "Content-Type: text/plain" -o test_results.txt --data "body" 
head -n 1 test_results.txt | grep -q "200" && echo -e "${GRE} Status is 200 OK${RES}" || echo -e "${RED}Status is not ok${RES}"
echo ""

echo "" > test_results.txt
echo -e "TEST 18: ${YEL} POST request with long body to root: ${RES}"
curl -s -i -X POST localhost:$PORT -H "Content-Type: text/plain" -o test_results.txt --data @- << EOF
ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss
ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss
ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss
ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss
EOF
head -n 1 test_results.txt | grep -q "200" && echo -e "${GRE} Status is 200 OK${RES}" || echo -e "${RED}Status is not ok${RES}"
echo ""

echo "" > test_results.txt
echo -e "TEST 19: ${YEL} GET redirect localhost:$PORT/old-page: ${RES}"
curl -s --http1.1 -G localhost:$PORT/old-page -i -o test_results.txt
head -n 1 test_results.txt | grep -q "301" && echo -e "${GRE} Status is 301 OK${RES}" || echo -e "${RED}Status is not ok${RES}"
echo ""
