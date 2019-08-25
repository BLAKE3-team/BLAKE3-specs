all: bao.tex bao.bib
	latexmk -pdf -silent -auxdir=build -outdir=build bao.tex