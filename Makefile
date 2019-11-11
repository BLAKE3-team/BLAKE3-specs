all: blake3.tex blake3.bib
	latexmk -e '$$pdflatex=q/pdflatex %O -shell-escape %S/' -pdf -silent -auxdir=build -outdir=build blake3.tex
