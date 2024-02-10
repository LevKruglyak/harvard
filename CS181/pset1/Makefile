all:
	mkdir -p build
	python3 problem2.py
	python3 problem3.py
	python3 problem4.py
	latexmk -output-directory=build -shell-escape -pdf main.tex
	cp build/main.pdf cs181-pset1.pdf
