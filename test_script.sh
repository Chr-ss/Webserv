#!/bin/bash

RED="\e[31m"
GRE="\e[32m"
YEL="\e[33m"
RES="\e[0m"

echo "" > test_results.txt

echo -e "TEST 1: ${YEL} GET localhost:8080: ${RES}"
curl -s --http1.1 -G localhost:8080 -i -o test_results.txt
head -n 1 test_results.txt | grep -q "200" && echo -e "${GRE} Status is 200 OK${RES}" || echo -e "${RED}Status is not ok${RES}"
echo ""

echo "" > test_results.txt
echo -e "TEST 2: ${YEL} GET localhost:8080/index.html: ${RES} "
curl -s --http1.1 -G localhost:8080/index.html -i -o test_results.txt
head -n 1 test_results.txt | grep -q "200" && echo -e "${GRE} Status is 200 OK${RES}" || echo -e "${RED}Status is not ok${RES}"
echo ""
echo "" > test_results.txt
echo -e "TEST 3: ${YEL} GET localhost:8080/contact.html: ${RES} "
curl -s --http1.1 -G localhost:8080/contact.html -i -o test_results.txt
head -n 1 test_results.txt | grep -q "200" && echo -e "${GRE} Status is 200 OK${RES}" || echo -e "${RED}Status is not ok${RES}"
echo ""

echo "" > test_results.txt
echo -e "TEST 4: ${YEL} GET localhost:8080/upload.html: ${RES} "
curl -s --http1.1 -G localhost:8080/upload.html -i -o test_results.txt
head -n 1 test_results.txt | grep -q "200" && echo -e "${GRE} Status is 200 OK${RES}" || echo -e "${RED}Status is not ok${RES}"
echo ""
echo "" > test_results.txt
echo -e "TEST 5: ${YEL} CGI upload txt localhost:8080/nph_CGI_upload.py: ${RES} "
curl -s --http1.1 -F "name=test; filename=text.txt" -H "Content-Type: multipart/form-data" localhost:8080/nph_CGI_upload.py -i -o test_results.txt
head -n 1 test_results.txt | grep -q "302" && echo -e "${GRE} Status is 302 found" || echo -e "${RED}Status is not ok${RES}"
echo ""

echo "" > test_results.txt
echo -e "TEST 6: ${YEL} CGI upload png localhost:8080/nph_CGI_upload.py: ${RES} "
curl -s --http1.1 -F "name=test2; filename=small.png" -H "Content-Type: multipart/form-data" localhost:8080/nph_CGI_upload.py -i -o test_results.txt
head -n 1 test_results.txt | grep -q "302" && echo -e "${GRE} Status is 302 found" || echo -e "${RED}Status is not ok${RES}"
echo ""

echo "" > test_results.txt
echo -e "TEST 7: ${YEL} CGI contact localhost:8080/nph_CGI_submit_contact_info.py: ${RES} "
curl -s --http1.1 -X POST -d "name=Testname Doe&email=test@email.com&message=Hello!" localhost:8080/nph_CGI_submit_contact_info.py -i -o test_results.txt
head -n 1 test_results.txt | grep -q "302" && echo -e "${GRE} Status is 302 found" || echo -e "${RED}Status is not ok${RES}"
echo ""

echo "" > test_results.txt
echo -e "TEST 8: ${YEL} GET localhost:8080/nph_CGI_upload_list.py: ${RES} "
curl -s --http1.1 -G localhost:8080/nph_CGI_upload_list.py -i -o test_results.txt
head -n 1 test_results.txt | grep -q "200" && echo -e "${GRE} Status is 200 OK${RES}" || echo -e "${RED}Status is not ok${RES}"
count=$(grep -o '<li>' test_results.txt | wc -l)
echo " -> Amount of files: $count"
echo ""

echo "" > test_results.txt
echo -e "TEST 9: ${YEL} DELETE localhost:8080/text.txt: ${RES} "
curl -s --http1.1 -X DELETE localhost:8080/text.txt -i -o test_results.txt
head -n 1 test_results.txt | grep -q "302" && echo -e "${GRE} Status is 302 found" || echo -e "${RED}Status is not ok${RES}"
echo ""
echo "" > test_results.txt
echo -e "TEST 10: ${YEL} try DELETE again localhost:8080/text.txt: ${RES} "
curl -s --http1.1 -X DELETE localhost:8080/text.txt -i -o test_results.txt
head -n 1 test_results.txt | grep -q "404" && echo -e "${GRE} Status is 404 OK (not found)" || echo -e "${RED}Status is not ok${RES}"
echo ""

echo "" > test_results.txt
echo -e "TEST 11: ${YEL} GET localhost:8080/nph_CGI_upload_list.py after deleting: ${RES} "
curl -s --http1.1 -G localhost:8080/nph_CGI_upload_list.py -i -o test_results.txt
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
echo -e "TEST 12: ${YEL} delete with GET localhost:8080?text.txt: ${RES} "
curl -s --http1.1 -X GET localhost:8080//nph_CGI_delete.py?file=small.png -i -o test_results.txt
head -n 1 test_results.txt | grep -q "302" && echo -e "${GRE} Status is 302 found" || echo -e "${RED}Status is not ok${RES}"
echo ""

echo "" > test_results.txt
echo -e "TEST 13: ${YEL} POST upload with query localhost:8080/nph_CGI_upload.py?name=testName&filename=text.txt: ${RES} "
curl -s --http1.1 -X POST \
     -F "file=@data/images/text.txt" \
     "localhost:8080/nph_CGI_upload.py?name=testName&filename=text.txt" \
     -i -o test_results.txt
head -n 1 test_results.txt | grep -q "302" && echo -e "${GRE} Status is 302 found" || echo -e "${RED}Status is not ok${RES}"
echo ""

echo "" > test_results.txt
echo -e "TEST 14: ${YEL} GET localhost:8080/notexistingfile -> status 404: ${RES}"
curl -s --http1.1 -G localhost:8080/notexistingfile -i -o test_results.txt
head -n 1 test_results.txt | grep -q "404" && echo -e "${GRE} Status is 404 OK${RES}" || echo -e "${RED}Status is not ok${RES}"
echo ""
