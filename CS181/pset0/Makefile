all:
	mkdir -p build
	python3 figures.py
	latexmk -output-directory=build -shell-escape -pdf main.tex
	cp build/main.pdf cs181-pset0.pdf
