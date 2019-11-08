all: blake3.tex blake3.bib
	latexmk -pdf -silent -auxdir=build -outdir=build blake3.tex
