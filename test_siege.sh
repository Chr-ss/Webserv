echo "" > test_results.txt
echo -e "TEST: ${YEL} Stresstest with siege -r 1000 -c 1 http://localhost:8080/empty.html: ${RES}"
siege -r 1000 -c 1 http://localhost:8080/http/empty.html
echo ""