build:
	gcc -Wall -Wextra -pedantic -pthread -o mapreduce mapreduce.c example.c

test: build
	./mapreduce test/1.txt test/2.txt test/3.txt test/4.txt test/5.txt | tee result.txt

clean:
	rm mapreduce
	rm result.txt
