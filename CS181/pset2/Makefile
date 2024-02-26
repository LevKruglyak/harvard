all:
	mkdir -p build
	python3 problem1.py
	python3 problem3.py
	latexmk -output-directory=build -shell-escape -pdf main.tex
	cp build/main.pdf cs181-pset2.pdf
