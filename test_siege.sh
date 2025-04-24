echo "" > test_results.txt
echo -e "TEST: ${YEL} Stresstest with siege -r 7000 -c 22 http://localhost:8080/index.html: ${RES}"
siege -r 7000 -c 22 http://localhost:8080/http/empty.html
echo ""
