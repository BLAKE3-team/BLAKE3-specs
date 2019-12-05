all: blake3.tex blake3.bib
	latexmk -pdf -silent -shell-escape -auxdir=build -outdir=build blake3.tex

preview:
	latexmk -pvc -pdf -silent -shell-escape -auxdir=build -outdir=build blake3.tex
clean:
	rm -rf build/

