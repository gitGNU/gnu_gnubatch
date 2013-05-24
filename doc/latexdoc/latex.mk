#
#       Makefile to run from document subdirectory
#
#   Copyright 2013 Free Software Foundation, Inc.
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Build a single LaTeX document within a directory.
#
RM = rm -f
LATEX = TEXMFHOME=$(TEXMFHOME) pdflatex

all:
	$(LATEX) "$(DOCNAME).tex"
	$(LATEX) "$(DOCNAME).tex"
	$(LATEX) "$(DOCNAME).tex"
	$(LATEX) "$(DOCNAME).tex"

clean distclean:
	$(RM) "$(DOCNAME).aux" "$(DOCNAME).log" "$(DOCNAME).out" "$(DOCNAME).toc" "$(DOCNAME).pdf"

